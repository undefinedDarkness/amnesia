const std = @import("std");
const sample_utils = @import("sample_utils.zig");
const glfw = @import("glfw");
const gpu = @import("gpu");

pub const GPUInterface = gpu.dawn.Interface;

const InitOptions = struct { power: gpu.PowerPreference = .low_power, backend: ?gpu.BackendType = .opengl };
const Self = @This();

state: sample_utils.Setup = undefined,

pub fn init(allocator: std.mem.Allocator, options: InitOptions) !Self {
    gpu.Impl.init();
    return Self{ .state = try sample_utils.setup(allocator, options.power, options.backend orelse sample_utils.detectBackendType(allocator) catch .opengl) };
}

pub fn getDevice(d: *Self) *gpu.Device {
    return d.state.device;
}

pub fn waitForBufferMap(d: *Self, buf: *gpu.Buffer, bufSize: usize) void {
    var ref: gpu.Buffer.MapAsyncStatus = undefined;
    const cb = (struct {
        pub inline fn wait(ctx: *gpu.Buffer.MapAsyncStatus, resp: gpu.Buffer.MapAsyncStatus) void {
            ctx.* = resp;
        }
    }).wait;
    buf.mapAsync(.{ .read = true }, 0, bufSize, &ref, cb); // context: anytype, comptime callback: fn(ctx:@TypeOf(context), status:MapAsyncStatus)callconv(.Inline)void)
    while (true) {
        if (ref == .success) {
            break;
        } else {
            d.getDevice().tick();
        }
    }
}

pub fn deinit(d: *Self) void {
    d.state.adapter.release();
    d.state.device.release();
    d.state.instance.release();
    d.state.window.destroy();
}

// pub fn main() !void {
//     std.debug.print("\n=== Tests are run here since there are issues running them in the test env, basically if it doesnt crash, its good !!! ===\n\n", .{}); // args: anytype)

//     std.debug.print("Simple init/deinit test\n", .{});
//     var gpa = std.heap.GeneralPurposeAllocator(.{}){};
//     defer _ = gpa.deinit();
//     var alloc = gpa.allocator();
//     var inst = try Self.init(alloc, .{});
//     inst.deinit();

//     std.debug.print("Simple gpu compute test\n", .{});
//     var inst2 = try Self.init(alloc, .{});
//     var dev = inst2.getDevice();

//     var writeBuffer = dev.createBuffer(&.{ .mapped_at_creation = true, .size = 4 * @sizeOf(f32), .usage = .{ .map_write = true, .copy_src = true } });
//     var writeBufferArray = writeBuffer.getMappedRange(f32, 0, 4).?; // len: usize)
//     for (0..4) |i| {
//         writeBufferArray[i] = @intToFloat(f32, i);
//     }
//     writeBuffer.unmap();

//     var readBuffer = dev.createBuffer(&.{ .mapped_at_creation = false, .size = 4 * @sizeOf(f32), .usage = .{ .map_read = true, .copy_dst = true } });

//     var commandEncoder = dev.createCommandEncoder(null);
//     commandEncoder.copyBufferToBuffer(writeBuffer, 0, readBuffer, 0, 4 * @sizeOf(f32));

//     var copyCommands = commandEncoder.finish(null);
//     dev.getQueue().submit(&[_]*const gpu.CommandBuffer{copyCommands});

//     var response: gpu.Buffer.MapAsyncStatus = undefined;
//     const callback = (struct {
//         pub inline fn callback(ctx: *gpu.Buffer.MapAsyncStatus, status: gpu.Buffer.MapAsyncStatus) void {
//             ctx.* = status;
//         }
//     }).callback;

//     readBuffer.mapAsync(.{ .read = true }, 0, 4 * @sizeOf(f32), &response, callback); // comptime callback: fn(ctx:@TypeOf(context), status:MapAsyncStatus)callconv(.Inline)void)

//     while (true) {
//         if (response == .success) {
//             break;
//         } else {
//             dev.tick();
//         }
//     }

//     const readBufferArray = readBuffer.getConstMappedRange(f32, 0, 4).?;
//     for (0..4) |i| {
//         std.debug.print("{d}\n", .{readBufferArray[i]}); // args: anytype)
//     }

//     inst2.deinit();
// }
