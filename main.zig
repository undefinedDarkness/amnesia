const clap = @import("libs/clap/clap.zig");
const std = @import("std");
const gpu = @import("src/gpu.zig");
const builtin = @import("builtin");
const m_gpu = @import("gpu");
pub const GPUInterface = m_gpu.dawn.Interface;

var colours: [64]u32 = undefined;
fn loadPalette(path: []const u8) ![]const u32 {
    var fp = try std.fs.cwd().openFile(path, .{ .mode = .read_only });
    var fp_r = fp.reader();
    var buf: [100]u8 = undefined;
    var col_buf: [4]u8 = undefined;
    var col_idx: usize = 0;

    while (fp_r.readUntilDelimiter(&buf, '\n')) |_line| {
        const line = std.mem.trim(u8, _line, "\r");
        _ = try std.fmt.hexToBytes(&col_buf, line);
        var col = @bitCast(u32, col_buf);
        colours[col_idx] = col;
        col_idx += 1;
        std.debug.print("\x1b[38;2;{d};{d};{d}m██████\x1b[0m", .{ col_buf[0], col_buf[1], col_buf[2] });
    } else |err| switch (err) {
        error.EndOfStream => {},
        else => return err,
    }
    std.debug.print("\n", .{}); //args: anytype)
    std.log.info("Loaded {d} colours from {s}", .{ col_idx + 1, path });
    return colours[0..col_idx];
}

extern fn loadImageRGBA([*:0]const u8, *u32, *u32) ?[*]u8;
extern fn unloadImageRGBA([*]u8) void;
extern fn writeImageRGBA([*:0]const u8, [*]const u8, u32, u32, u32) i32;
extern fn atkinsonDither(u32, u32, u32, [*]u8) void;

pub fn main() !void {
    if (builtin.os.tag == .windows) {
        const win = @cImport({
            @cDefine("WIN32_LEAN_AND_MEAN", "1");
            @cInclude("windows.h");
        });
        _ = win.SetConsoleOutputCP(win.CP_UTF8);
    }

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    var alloc = gpa.allocator();
    defer _ = gpa.deinit();
    const help_msg = (
        \\-h, --help                      Display this help & exit.
        \\-d, --dither_method <str>       Specify dithering method to use (bayer,atkinson,bluenoise)
        \\-g, --generate_lut <u32>        Generate a lookup table of the specified level
        \\-m, --grayscale                 Make image grayscale
        \\-u, --use_lut <str>             Use a lut file & apply to image
        \\-p, --palette <str>             Use a palette hex file to read in colours and replace with nearest colours in image
        \\-l, --luma_invert               Perform luma inversion
        \\-o, --output <str>              Specify output file location (WILL WRITE PNG)
        \\<str>
    );
    const params = comptime clap.parseParamsComptime(help_msg);
    var diag = clap.Diagnostic{};
    const res = clap.parse(clap.Help, &params, clap.parsers.default, .{ .diagnostic = &diag }) catch |err| {
        diag.report(std.io.getStdErr().writer(), err) catch {};
        return err;
    };
    defer res.deinit();
    // var loaded_with_stb = false;

    if (res.args.help != 0) {
        const preface = (
            \\--- amnesia ---
            \\use blue noise dithering with grayscale or it won't make sense
        );
        std.debug.print("\n{s}\n{s}", .{preface, help_msg});
        return;
    }

    const gpu_state = gpu.doGPUInit() orelse blk: {
        std.log.err("Cannot initialize GPU", .{});
        break :blk null;
    };
    defer gpu.doGPUDeinit(gpu_state);
    var gpu_options = gpu.options{};

    var palette: ?[]const u32 = null;
    if (res.args.palette) |v| {
        palette = try loadPalette(v);
        gpu_options.colorReplace = true;
    }

    if (res.args.grayscale != 0) {
        gpu_options.grayscale = true;
    }

    var output_path: [:0]const u8 = "output.png";
    if (res.args.output) |v| {
        output_path = try std.mem.concatWithSentinel(alloc, u8, &[_][]const u8{v}, 0);
    }
    defer {
        if (res.args.output) |v| {
            _ = v;
            alloc.free(output_path);
        }
    }

    var image_width: u32 = undefined;
    var image_height: u32 = undefined;
    if (res.args.generate_lut) |level| {
        if (palette != null) {
            // TODO: Fix issues when level*level*4 isnt a multiple of 256 
            var stride: u32 = undefined;
            var image = gpu.doGPUGlut(gpu_state, palette.?.ptr, @intCast(u32, palette.?.len), level, &stride) orelse {
                std.log.err("Failed to generate LUT", .{});
                return;
            };
            image_width = level * level * level;
            image_height = level * level * level;

            if (-1 != writeImageRGBA(output_path, image, image_width, image_height, stride)) {
                std.log.info("Wrote to {s}", .{output_path});
            } else {
                std.log.err("Failed to write to {s}", .{output_path});
            }
            return;
        }
    }

    var lut: ?[*]u8 = null;
    var lut_width: u32 = undefined;
    if (res.args.use_lut) |lut_path| {
        const lut_path_z = try std.mem.concatWithSentinel(alloc, u8, &[_][]const u8{ lut_path }, 0);
        defer alloc.free(lut_path_z);
        var lut_height: u32 = undefined;
        lut = loadImageRGBA(lut_path_z, &lut_width, &lut_height) orelse {
            std.log.err("Failed to load LUT image {s}", .{ lut_path });
            return;
        };
        if (lut_width != lut_height) {
            std.log.err("LUT image ({s}) not a square, likely invalid.", .{ lut_path });
            return;
        }
    }

    if (res.positionals.len == 0) {
        return;
    }

    const image_path = try std.mem.concatWithSentinel(alloc, u8, &[_][]const u8{res.positionals[0]}, 0);
    defer alloc.free(image_path);
    var image = loadImageRGBA(image_path.ptr, &image_width, &image_height) orelse {
        std.log.err("Failed to load image {s}", .{res.positionals[0]});
        return;
    };
    // defer unloadImageRGBA(image);

    if (res.args.dither_method) |v| {
        if (std.mem.eql(u8, v, "bayer")) {
            gpu_options.Bayer_dither = true;
        } else if (std.mem.eql(u8, v, "atkinson")) {
            atkinsonDither(0, image_height, image_width, image);
        } else if (std.mem.eql(u8, v, "bluenoise")) {
            gpu_options.BlueNoise_dither = true;
        }
    }

    if (res.args.luma_invert != 0) {
        gpu_options.lumaInvert = true;
    }

    if (-1 == gpu.doGPUWork(gpu_state, image, image_width, image_height, gpu_options, if (gpu_options.colorReplace) palette.?.ptr else null, if (gpu_options.colorReplace) @intCast(u32, palette.?.len) else 0)) {
        std.log.err("Failed to do GPU work", .{});
        return;
    } else {
        std.log.info("Finished GPU work", .{});
    }

    if (lut) |_lut| {
        if (-1 == gpu.doGPUSlut(gpu_state, image, image_width, image_height, _lut, lut_width)) {
            std.log.err("Failed to apply LUT to image", .{});
            return;
        } else {
            std.log.info("Finished applying LUT to image", .{});
        }
    }

    if (-1 != writeImageRGBA(output_path, image, image_width, image_height, image_width * 4)) {
        std.log.info("Wrote to {s}", .{output_path});
    } else {
        std.log.err("Failed to write to {s}", .{output_path});
    }
}
