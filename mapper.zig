const std = @import("std");
const expect = @import("std").testing.expect;

const MAX_KEY_SIZE = 2048;
const MAX_VALUE_SIZE = 2048;
const HASH_MAP_SIZE = 101;
const NUMBER_OF_MAPS = 100;

const Value = []u8;
const Key = []u8;

pub const Entry = struct {
    key: Key,
    value: Value,
    next: ?*Entry,
};

var maps: [NUMBER_OF_MAPS]?[]?*Entry = undefined;
var maps_count: usize = 0;
// var alloca = std.heap.loggingAllocator(std.heap.c_allocator);
var allocator = std.heap.c_allocator;

fn hash(key: []const u8) usize {
    var hashVal: usize = 0;
    for (key) |c| {
        hashVal += @intCast(c);
    }
    return @mod(hashVal, HASH_MAP_SIZE);
}

export fn init_map() i32 {
    if (maps_count >= NUMBER_OF_MAPS) {
        std.debug.print("[ZIG_MAPPER_ERROR]Maximum number of maps reached\n", .{});
        return -1;
    }
    const map_id: usize = @intCast(maps_count);
    maps_count += 1;
    const allocating = allocator.alloc(?*Entry, HASH_MAP_SIZE);
    if (allocating) |array| {
        for (array) |*entry| {
            entry.* = null;
        }
        maps[map_id] = array;
    } else |err| {
        std.debug.print("[ZIG_MAPPER_ERROR]Failed to allocate memory: {}\n", .{err});
        return -1;
    }

    std.debug.print("[ZIG_MAPPER_INFO]Creating new map {}\n", .{map_id});
    return @intCast(map_id);
}

export fn set_val(map_id: i32, key: [*c]const u8, key_len: usize, value: [*c]const u8, value_len: usize) i32 {
    if (map_id < 0 or map_id >= maps_count or key_len > MAX_KEY_SIZE or value_len > MAX_VALUE_SIZE) {
        std.debug.print("[ZIG_MAPPER_ERROR]Invalid parameters: map_id={0}, key_len={1}\n", .{ map_id, key_len });
        return -1;
    }

    const map = maps[@intCast(map_id)] orelse return -1;
    const hash_value = hash(key[0..key_len]);

    // Try finding an existing entry with the same key.
    var current_entry_ptr = &map[hash_value];
    while (current_entry_ptr.* != null) {
        const current_entry = current_entry_ptr.*;
        if (current_entry.?.key.len == key_len and std.mem.eql(u8, current_entry.?.key[0..key_len], key[0..key_len])) {
            // Allocate new value buffer and handle allocation failure.
            const newValue = allocator.alloc(u8, value_len) catch {
                std.debug.print("[ZIG_MAPPER_ERROR]Failed to allocate memory for value\n", .{});
                return -1;
            };
            std.mem.copy(u8, newValue[0..value_len], value[0..value_len]);
            allocator.free(current_entry.?.value);
            current_entry.?.value = newValue[0..value_len];
            return 0;
        }
        current_entry_ptr = &current_entry.?.next;
    }

    // Allocate and initialize a new entry if no existing entry matches.
    const new_entry = allocator.create(Entry) catch {
        std.debug.print("[ZIG_MAPPER_ERROR]Failed to allocate memory for new entry\n", .{});
        return -1;
    };
    new_entry.* = Entry{
        .key = allocator.alloc(u8, key_len) catch {
            std.debug.print("[ZIG_MAPPER_ERROR]Failed to allocate memory for key\n", .{});
            allocator.destroy(new_entry);
            return -1;
        },
        .value = allocator.alloc(u8, value_len) catch {
            std.debug.print("[ZIG_MAPPER_ERROR]Failed to allocate memory for value\n", .{});
            allocator.free(new_entry.key);
            allocator.destroy(new_entry);
            return -1;
        },
        .next = null,
    };
    std.mem.copy(u8, new_entry.key[0..key_len], key[0..key_len]);
    std.mem.copy(u8, new_entry.value[0..value_len], value[0..value_len]);

    // Insert the new entry into the map.
    new_entry.next = map[hash_value];
    map[hash_value] = new_entry;

    return 0;
}

export fn get_val(map_id: i32, key: [*c]const u8, key_len: i32, value: [*c]u8) i32 {
    if (map_id < 0 or map_id >= maps_count or key_len > MAX_KEY_SIZE) {
        std.debug.print("[ZIG_MAPPER_ERROR]Invalid map id or key size {0},{1}\n", .{ map_id, key_len });
        return -1;
    }
    const map = maps[@intCast(map_id)] orelse return -1;
    const key_size: usize = @intCast(key_len);

    const hash_value = hash(key[0..key_size]);
    var current_entry = map[hash_value];
    while (current_entry) |curr_entry| {
        if (key_len == curr_entry.key.len and std.mem.eql(u8, curr_entry.key[0..key_size], key[0..key_size])) {
            std.mem.copy(u8, value[0..curr_entry.value.len], curr_entry.value[0..curr_entry.value.len]);
            return @intCast(curr_entry.value.len);
        }
        current_entry = curr_entry.next;
    }
    return -1;
}

export fn destroy_map(map_id: i32) void {
    if (map_id < 0 or map_id >= maps_count) {
        std.debug.print("[ZIG_MAPPER_ERROR]Invalid map id or key size: {}\n", .{map_id});
        return;
    }

    var map = maps[@intCast(map_id)] orelse return;

    std.debug.print("[ZIG_MAPPER_INFO]Destroying map {}\n", .{map_id});

    for (0..map.len) |i| {
        var current_entry = map[i];
        while (current_entry) |entry| {
            const next_entry = entry.next;
            allocator.free(entry.key);
            allocator.free(entry.value);
            allocator.destroy(entry);
            current_entry = next_entry;
        }
    }
    allocator.free(map);
    maps[@intCast(map_id)] = null;
}

export fn destroy_all_maps() void {
    for (0..maps_count) |i| {
        destroy_map(@intCast(i));
    }
    maps_count = 0;
}

test "test_init_map" {
    const map_id = init_map();
    try expect(map_id == 0);
    destroy_map(map_id);
}

test "test_set_val" {
    const map_id = init_map();
    const key = "key";
    const value = "value";
    const key_len: i32 = @intCast(key.len);
    const value_len: i32 = @intCast(value.len);
    const result = set_val(map_id, key, key_len, value, value_len);
    try expect(result != -1);
    destroy_map(map_id);
}

test "test_get_val" {
    const map_id = init_map();
    const key = "key";
    const value = "value";
    const key_len: i32 = @intCast(key.len);
    const value_len: i32 = @intCast(value.len);
    var r = set_val(map_id, &key[0], key_len, &value[0], value_len);
    try expect(r != -1);
    var result: [MAX_VALUE_SIZE]u8 = undefined;
    const get_result = get_val(map_id, &key[0], key_len, &result[0]);
    try expect(get_result != -1);
    const r_bytes: usize = @intCast(get_result);
    try expect(std.mem.eql(u8, result[0..r_bytes], value[0..r_bytes]) == true);
    destroy_map(map_id);
}

test "test_shadowing" {
    const map_id = init_map();
    const key = "key";
    const value = "value";
    const key_len: i32 = @intCast(key.len);
    const value_len: i32 = @intCast(value.len);
    var r = set_val(map_id, key, key_len, value, value_len);
    try expect(r != -1);
    const new_value = "new_value";
    const new_value_len: i32 = @intCast(new_value.len);
    r = set_val(map_id, key, key_len, new_value, new_value_len);
    try expect(r != -1);
    var result: [MAX_VALUE_SIZE]u8 = undefined;
    const get_result = get_val(map_id, key, key_len, &result[0]);
    try expect(get_result == new_value_len);
    try expect(std.mem.eql(u8, result[0..new_value_len], new_value[0..new_value_len]) == true);
    destroy_map(map_id);
}
