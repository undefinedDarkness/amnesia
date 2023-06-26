const std = @import("std");
const CShader = @import("cshader/main.zig");
const gpu = @import("gpu");

pub const GPUInterface = gpu.dawn.Interface;
// const pixel = struct { r: u8, g: u8, b: u8, a: u8 };

export fn testImpl() void {
    std.debug.print("-- LINKED TO ZIG SUCESSFULLY --\n", .{}); // args: anytype)
}

fn roundUp(numToRound: i32, comptime multiple: i32) i32 { 
    comptime {
        if (!(multiple > 0 and ((multiple & (multiple - 1)) == 0))) {
            @compileError("multiple not a power of 2");
        }
    }
    return (numToRound + multiple - 1) & -multiple;
}

export fn doGPUDither(pixels_ptr: [*]u8, len: u32, width: u32, height: u32) i32 {
    const pixels = pixels_ptr[0..len];
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    var alloc = gpa.allocator();
    const a = std.time.milliTimestamp();
    var inst = CShader.init(alloc, .{}) catch return;
    const b = std.time.milliTimestamp();
    std.debug.print("Initialized GPU in {d}ms\n", .{ b - a });//args: anytype)
    defer inst.deinit();

    const dev = inst.getDevice();

    const niceBytesPerRow = @intCast(u32, roundUp(@intCast(i32, 4*width), 256));
    std.debug.print("Using {d} bytes per row instead of {d}\n", .{ niceBytesPerRow, 4*width});//args: anytype)
    const dataLayout = gpu.Texture.DataLayout{
        .offset = 0,
        .rows_per_image = height,
        .bytes_per_row = width*4
    };


    const writeBuffer = dev.createTexture(&gpu.Texture.Descriptor.init(.{ .usage = .{ .texture_binding = true, .copy_dst = true, .copy_src = true }, .size = .{ .width = width, .height = height }, .format = .rgba8_unorm_srgb }));
    defer writeBuffer.release();
    dev.getQueue().writeTexture(&.{ .texture = writeBuffer }, &dataLayout, &.{ .width = width, .height = height }, pixels);

    const btwBuffer = dev.createTexture(&gpu.Texture.Descriptor.init(.{ .usage = .{ .storage_binding = true, .copy_src = true, .copy_dst = true }, .size = .{ .width = width, .height = height }, .format = .rgba8_unorm }));
    defer btwBuffer.release();


    const readBuffer = dev.createBuffer(&.{ .mapped_at_creation = false, .usage = .{ .map_read = true, .copy_dst = true }, .size = niceBytesPerRow * height });
    defer readBuffer.release();
    const readBufferSize = niceBytesPerRow * height;

    // const textureSampler = dev.createSampler(null);
    // defer textureSampler.release();    

    const computeModule = dev.createShaderModuleWGSL("main.wgsl", @embedFile("shader.wgsl"));
    defer computeModule.release();

    const computePipeline = dev.createComputePipeline(&.{ .compute = .{ .module = computeModule, .entry_point = "main" } });
    defer computePipeline.release();

    const computeBindGroup = dev.createBindGroup(&gpu.BindGroup.Descriptor.init(.{
        .layout = computePipeline.getBindGroupLayout(0),
        .entries = &.{ 
            gpu.BindGroup.Entry.textureView(0, writeBuffer.createView(null)),
            gpu.BindGroup.Entry.textureView(1, btwBuffer.createView(null)),
            // gpu.BindGroup.Entry.sampler(2, textureSampler)
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

    inst.waitForBufferMap(readBuffer, readBufferSize);

    if (readBuffer.getConstMappedRange(u8, 0, readBufferSize)) |v| {
        var copiedBytesA: u32 = 0;
        var copiedBytesB : u32= 0;
        const rowEndOffset = niceBytesPerRow - 4*width;
        const rowSize = 4*width;
        while (copiedBytesA < len) {
            @memcpy(pixels[copiedBytesA .. copiedBytesA+rowSize], v[copiedBytesB .. copiedBytesB+rowSize]);
            copiedBytesA += rowSize;
            copiedBytesB += rowSize + rowEndOffset;
        }
        const c = std.time.milliTimestamp();
        std.debug.print("ALL GPU WORK FINISHED in {d}ms\n", .{ c-b });// args: anytype)
    } else {
        std.debug.print("Got null value back from readBuffer\n", .{});// args: anytype)
        return -1;
    }
    return 0;
}
