const std = @import("std");
const CShader = @import("cshader/main.zig");
const gpu = @import("gpu");

pub const GPUInterface = gpu.dawn.Interface;

//export fn testImpl() void {
//    std.debug.print("-- LINKED TO ZIG SUCESSFULLY --\n", .{}); // args: anytype)
//}

fn roundUp(numToRound: i32, comptime multiple: i32) i32 { 
    comptime {
        if (!(multiple > 0 and ((multiple & (multiple - 1)) == 0))) {
            @compileError("multiple not a power of 2");
        }
    }
    return (numToRound + multiple - 1) & -multiple;
}

// TODO: Cleanup code
// TODO: Find a neater way of passing around options

export fn doGPUInit() ?*CShader.state {
    var mem = std.heap.c_allocator.create(CShader.state) catch unreachable;
    if (CShader.init(std.heap.c_allocator, .{})) |v| {
        mem.* = v;
    } else |err| {
        std.debug.print("GPU-CPU: {!}", .{err});
        std.heap.c_allocator.free(mem);
        return null;
    }
    return mem;
}

export fn doGPUDeinit(rep: ?*CShader.state) void {
    if (rep) |v| {
        CShader.deinit(v);
    }
}

const options = packed struct(u8) {
    colorReplace: bool,
    dither: bool,
    lumaInvert: bool,
    _padding: u5
};

fn createPass(dev: *gpu.Device, enc: *gpu.CommandEncoder, mod: *gpu.ShaderModule, entry_pt: [*:0]const u8, entries: anytype) *gpu.ComputePassEncoder {
    std.debug.print("Creating shader pass ({s})\n", .{entry_pt});
    const pipeline = dev.createComputePipeline(&.{ .compute = .{ .module = mod, .entry_point = entry_pt } });
    const bindGroup = dev.createBindGroup(&gpu.BindGroup.Descriptor.init(.{ .layout = pipeline.getBindGroupLayout(0), .entries = &entries }));
    const pass = enc.beginComputePass(null);
    pass.setPipeline(pipeline);
    pass.setBindGroup(0, bindGroup, null);
    return pass;
}

export fn doGPUGlut(inst_: ?*CShader.state, palette_ptr: [*]u32, palette_size: u32, level: u32) ?[*]const u8 {
    const inst = inst_ orelse return null;
    const dev = inst.device;
    
    const cubeFace = level * level;

    const palTex = dev.createTexture(&gpu.Texture.Descriptor.init(.{
        .usage = .{
            .texture_binding = true,
            .copy_dst = true
        },
        .size = .{
            .width = palette_size,
            .height = 1
        },
        .format = .rgba8_unorm
    }));
    dev.getQueue().writeTexture(&.{ .texture = palTex }, &.{
        .bytes_per_row = palette_size * 4,
        .rows_per_image = 1
    }, &.{
        .width = palette_size, .height = 1
    }, palette_ptr[0..palette_size]);

    const lutTex = dev.createTexture(&gpu.Texture.Descriptor.init(.{
        .usage = .{
            .storage_binding = true,
            .copy_src = true
        },
        .size = .{
            .width = cubeFace,
            .height = cubeFace,
            .depth_or_array_layers = cubeFace
        },
        .dimension = .dimension_3d,
        .format = .rgba8_unorm
    }));
    
    const copyBufSize = cubeFace * cubeFace * cubeFace * 4;
    const copyBuf = dev.createBuffer(&.{
        .mapped_at_creation = false,
        .usage = .{
            .copy_dst = true,
            .map_read = true
        },
        .size = copyBufSize
    });

    const shd_c = std.fs.cwd().readFileAllocOptions(std.heap.c_allocator, "src/shader.wgsl", 10000, null, @alignOf(u8), 0) catch |err| {
        std.debug.print("Failed to read shader source code: {!}", .{err});
        return null;
    };
    const shd = dev.createShaderModuleWGSL(null, shd_c);
    const enc = dev.createCommandEncoder(null);
    const pass = createPass(dev, enc, shd, "glut", .{
        gpu.BindGroup.Entry.textureView(3, lutTex.createView(null)),
        gpu.BindGroup.Entry.textureView(2, palTex.createView(null))
    });
    pass.dispatchWorkgroups(cubeFace, cubeFace, cubeFace);
    pass.end();

    enc.copyTextureToBuffer(&.{ .texture = lutTex }, &.{ .buffer = copyBuf, .layout = .{
        .offset = 0,
        .bytes_per_row = cubeFace * 4,
        .rows_per_image = cubeFace
    }  }, &.{ .width = cubeFace, .height = cubeFace, .depth_or_array_layers = cubeFace });
    dev.getQueue().submit(&[_]*gpu.CommandBuffer{ enc.finish(null) });
    CShader.waitForBufferMap(inst, copyBuf, copyBufSize);
    return copyBuf.getConstMappedRange(u8, 0, copyBufSize).?.ptr;
}

export fn doGPUSlut(inst_: ?*CShader.state, pixels_ptr: [*]u8, px_w: u32, px_h: u32, lut_ptr: [*]u32, lut_imsize: u32) i32 {
    const inst = inst_ orelse return -1;
    const dev = inst.device;
    
    const lutLevel = @floatToInt(u32, std.math.cbrt(@intToFloat(f32, lut_imsize)));
    const lutSize = lutLevel * lutLevel;
    const lutTex = dev.createTexture(&gpu.Texture.Descriptor.init(.{
        .format = .rgba8_unorm,
        .dimension = .dimension_3d,
        .usage = .{
            .texture_binding = true,
            .copy_dst = true
        },
        .size = .{
            .width = lutSize,
            .height = lutSize,
            .depth_or_array_layers = lutSize
        }
    }));
    dev.getQueue().writeTexture(&.{.texture = lutTex}, &.{.bytes_per_row = lutSize * 4, .rows_per_image = lutSize, .offset = 0}, &.{
        .width = lutSize, .height = lutSize, .depth_or_array_layers = lutSize
    }, lut_ptr[0 .. lutSize * lutSize * lutSize * 4]);

    const lutSampler = dev.createSampler(&.{
        .mag_filter = .linear,
        .min_filter = .linear,
        .mipmap_filter = .linear
    });

    const srcTexDesc = gpu.Texture.Descriptor.init(.{
        .format = .rgba8_unorm,
        .usage = .{
            .texture_binding = true,
            .storage_binding = true,
            .copy_src = true
        },
        .size = .{
            .width = px_w,
            .height = px_h
        }
    });
    const srcTex = dev.createTexture(&srcTexDesc);
    dev.getQueue().writeTexture(&.{.texture=srcTex}, &.{.bytes_per_row = px_w * 4, .rows_per_image = px_h}, &.{.width = px_w,.height=px_h}, pixels_ptr[0..px_w*px_h*4]);
    // const destTex = dev.createTexture(&srcTexDesc);
    const niceBytesPerRow = @intCast(u32, roundUp(@intCast(i32, 4*px_w), 256));
    const remainder = niceBytesPerRow - px_w*4;
    const destBuf = dev.createBuffer(&.{
        .mapped_at_creation = false,
        .usage = .{
            .copy_dst = true,
            .map_read = true
        },
        .size = niceBytesPerRow * px_h
    });

    const shd_c = std.fs.cwd().readFileAllocOptions(std.heap.c_allocator, "src/shader.wgsl", 10000, null, @alignOf(u8), 0) catch |err| {
        std.debug.print("Failed to read shader source code: {!}", .{err});
        return -1;
    };
    const shd = dev.createShaderModuleWGSL(null, shd_c);
    const enc = dev.createCommandEncoder(null);
    const pass = createPass(dev, enc, shd, "slut", .{
        gpu.BindGroup.Entry.textureView(0, srcTex.createView(null)),
        gpu.BindGroup.Entry.textureView(1, srcTex.createView(null)),
        gpu.BindGroup.Entry.sampler(5, lutSampler)
        // gpu.BindGroup.Entry.sampler(4, )
        //gpu.BindGroup.Entry.textureView(4, lutTex.createView(null))
    });
    pass.dispatchWorkgroups(px_w, px_h, 1);
    pass.end();
    
    enc.copyTextureToBuffer(&.{.texture = srcTex }, &.{ .buffer = destBuf, .layout = .{
        .bytes_per_row = niceBytesPerRow,
        .rows_per_image = px_h
    }  }, &.{
        .width = px_w,
        .height = px_h
    });
    CShader.waitForBufferMap(inst, destBuf, px_w*px_h*4);
    const bufArr = destBuf.getConstMappedRange(u8, 0, px_w*px_h*4) orelse return -1;
    var h_w: u32 = 0;
    var start: u32 = 0;
    while (h_w < px_h) : ({ h_w += 1; start += px_w*4; }) {
        @memcpy(pixels_ptr[start .. start + px_w*4], bufArr[start+remainder .. start+remainder+px_w]);
    }
    return 0;
}

export fn doGPUWork(inst_: ?*CShader.state, pixels_ptr: [*]u8, width: u32, height: u32, op: options, palette_ptr: [*]u32, palette_size: u32) i32 {
    const inst = inst_ orelse return -1;
    const len = width * height * 4;
    const pixels = pixels_ptr[0..len];
    const paletteSizeInBytes = 4 * palette_size;
    _ = paletteSizeInBytes;
    const palette = palette_ptr[0..palette_size];

    const dev = inst.device;

    const niceBytesPerRow = @intCast(u32, roundUp(@intCast(i32, 4*width), 256));
    std.debug.print("Using {d} bytes per row instead of {d}\n", .{ niceBytesPerRow, 4*width});//args: anytype)
    const dataLayout = gpu.Texture.DataLayout{
        .offset = 0,
        .rows_per_image = height,
        .bytes_per_row = width*4
    };

    // for (palette) |c| {
        // c.* = @bitReverse(c.*);
    //     std.debug.print("{x} ", .{c});
    // }
    std.debug.print("\n", .{});//args: anytype)

    const writeBuffer = dev.createTexture(&gpu.Texture.Descriptor.init(.{ .usage = .{ .texture_binding = true, .storage_binding = true, .copy_dst = true, .copy_src = true }, .size = .{ .width = width, .height = height }, .format = .rgba8_unorm }));
    defer writeBuffer.release();
    dev.getQueue().writeTexture(&.{ .texture = writeBuffer }, &dataLayout, &.{ .width = width, .height = height }, pixels);

    const paletteBuffer = dev.createTexture(&gpu.Texture.Descriptor.init(.{
        .usage = .{ .texture_binding = true, .copy_dst = true },
        .size = .{ .width = palette_size, .height = 1},
        .format = .rgba8_unorm_srgb,
    }));
    dev.getQueue().writeTexture(&.{ .texture = paletteBuffer }, &.{ .offset = 0, .rows_per_image = 1, .bytes_per_row = palette_size*4 }, &.{ .width = palette_size, .height = 1 }, palette);
    defer paletteBuffer.release();

    const btwBuffer = dev.createTexture(&gpu.Texture.Descriptor.init(.{ .usage = .{ .texture_binding = true, .storage_binding = true, .copy_src = true, .copy_dst = true }, .size = .{ .width = width, .height = height }, .format = .rgba8_unorm }));
    defer btwBuffer.release();

    const readBuffer = dev.createBuffer(&.{ .mapped_at_creation = false, .usage = .{ .map_read = true, .copy_dst = true }, .size = niceBytesPerRow * height });
    defer readBuffer.release();
    const readBufferSize = niceBytesPerRow * height;

    const shaderFile = std.fs.cwd().readFileAllocOptions(std.heap.c_allocator, "src/shader.wgsl", 10000, null, @alignOf(u8), 0) catch |err| {
        std.debug.print("{!}\n", .{err});//args: anytype)
        return -1;
    };
    defer std.heap.c_allocator.free(shaderFile);
    const shaderModule = dev.createShaderModuleWGSL(null, shaderFile);
   
    const encoder = dev.createCommandEncoder(null);
    

    if (op.dither) {
        const pass = createPass(dev, encoder, shaderModule, "ditherPass", .{
            gpu.BindGroup.Entry.textureView(0, writeBuffer.createView(null)),
            gpu.BindGroup.Entry.textureView(1, btwBuffer.createView(null))
        });
        pass.dispatchWorkgroups(width, height, 1);
        pass.end();
    }

    const pass = createPass(dev, encoder, shaderModule, "colourPass", .{
        gpu.BindGroup.Entry.textureView(0, if (op.dither) btwBuffer.createView(null) else writeBuffer.createView(null)),
        gpu.BindGroup.Entry.textureView(1, if (op.dither) writeBuffer.createView(null) else btwBuffer.createView(null))
    });
    pass.dispatchWorkgroups(width, height, 1);
    pass.end();
   
    encoder.copyTextureToBuffer(&.{.texture= if (op.dither) writeBuffer else btwBuffer}, &.{.buffer=readBuffer,.layout=.{
        .offset = 0,
        .rows_per_image = height,
        .bytes_per_row = niceBytesPerRow //niceBytesPerRow
    }}, &.{.width=width,.height=height});
    //encoder.copyBufferToBuffer(btwBuffer, 0, readBuffer, 0, len);

    const commands = encoder.finish(null);
    dev.getQueue().submit(&[_]*gpu.CommandBuffer{commands});

    CShader.waitForBufferMap(inst, readBuffer, readBufferSize);

    if (readBuffer.getConstMappedRange(u8, 0, readBufferSize)) |v| {
        var copiedBytesA: u32 = 0;
        var copiedBytesB : u32= 0;
        const rowEndOffset = niceBytesPerRow - 4*width;
        const rowSize = 4*width;
        while (copiedBytesA < len) : ({ copiedBytesA += rowSize; copiedBytesB += rowSize + rowEndOffset; }) {
            @memcpy(pixels[copiedBytesA .. copiedBytesA+rowSize], v[copiedBytesB .. copiedBytesB+rowSize]);
        }
        // const c = std.time.milliTimestamp();
        //std.debug.print("ALL GPU WORK FINISHED in {d}ms\n", .{ c-b });// args: anytype)
    } else {
        std.debug.print("Got null value back from readBuffer\n", .{});// args: anytype)
        return -1;
    }
    return 0;
}
