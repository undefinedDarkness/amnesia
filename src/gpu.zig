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

export fn doGPUWork(inst_: ?*CShader.state, pixels_ptr: [*]u8, width: u32, height: u32, op: options, palette_ptr: [*]u32, palette_size: u32) i32 {
    _ = op;
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

    const writeBuffer = dev.createTexture(&gpu.Texture.Descriptor.init(.{ .usage = .{ .texture_binding = true, .copy_dst = true, .copy_src = true }, .size = .{ .width = width, .height = height }, .format = .rgba8_unorm }));
    defer writeBuffer.release();
    dev.getQueue().writeTexture(&.{ .texture = writeBuffer }, &dataLayout, &.{ .width = width, .height = height }, pixels);

    const paletteBuffer = dev.createTexture(&gpu.Texture.Descriptor.init(.{
        .usage = .{ .texture_binding = true, .copy_dst = true },
        .size = .{ .width = palette_size, .height = 1},
        .format = .rgba8_unorm_srgb,
    }));
    dev.getQueue().writeTexture(&.{ .texture = paletteBuffer }, &.{ .offset = 0, .rows_per_image = 1, .bytes_per_row = palette_size*4 }, &.{ .width = palette_size, .height = 1 }, palette);
    defer paletteBuffer.release();

    const btwBuffer = dev.createTexture(&gpu.Texture.Descriptor.init(.{ .usage = .{ .storage_binding = true, .copy_src = true, .copy_dst = true }, .size = .{ .width = width, .height = height }, .format = .rgba8_unorm }));
    defer btwBuffer.release();

    const readBuffer = dev.createBuffer(&.{ .mapped_at_creation = false, .usage = .{ .map_read = true, .copy_dst = true }, .size = niceBytesPerRow * height });
    defer readBuffer.release();
    const readBufferSize = niceBytesPerRow * height;

    const shaderFile = std.fs.cwd().readFileAllocOptions(std.heap.c_allocator, "src/shader.wgsl", 10000, null, @alignOf(u8), 0) catch |err| {
        std.debug.print("{!}\n", .{err});//args: anytype)
        return -1;
    };
    const computeModule = dev.createShaderModuleWGSL("main.wgsl", shaderFile);
    defer computeModule.release();

    const computePipeline = dev.createComputePipeline(&.{ .compute = .{ .module = computeModule, .entry_point = "main" } });
    defer computePipeline.release();

    const computeBindGroup = dev.createBindGroup(&gpu.BindGroup.Descriptor.init(.{
        .layout = computePipeline.getBindGroupLayout(0),
        .entries = &.{ 
            gpu.BindGroup.Entry.textureView(0, writeBuffer.createView(null)),
            gpu.BindGroup.Entry.textureView(1, btwBuffer.createView(null)),
            gpu.BindGroup.Entry.textureView(2, paletteBuffer.createView(null))
        },
    }));
    defer computeBindGroup.release();

    const encoder = dev.createCommandEncoder(null);
    const computePass = encoder.beginComputePass(null);
    computePass.setPipeline(computePipeline);
    computePass.setBindGroup(0, computeBindGroup, &.{});
    computePass.dispatchWorkgroups(width, height, 1);
    computePass.end();
    // encoder.copyTextureToTexture(&.{.texture=writeBuffer}, &.{.texture=btwBuffer}, &.{.width=width,.height=height});// destination: *const ImageCopyTexture, copy_size: *const Extent3D)
    encoder.copyTextureToBuffer(&.{.texture=btwBuffer}, &.{.buffer=readBuffer,.layout=.{
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
