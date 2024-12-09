const std = @import("std");

pub fn add_game(
    b: *std.Build,
    libs: []const *std.Build.Step.Compile,
    options: std.Build.ExecutableOptions,
) *std.Build.Step.Compile {
    const exe = b.addExecutable(options);

    exe.linkLibCpp();
    exe.addIncludePath(b.path("SDLGame/"));
    exe.linkSystemLibrary("SDL2");
    exe.linkSystemLibrary("SDL2_image");
    exe.linkSystemLibrary("SDL2_ttf");

    for (libs) |lib| {
        exe.linkLibrary(lib);
    }

    return exe;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addStaticLibrary(.{
        .name = "sdl-game",
        .target = target,
        .optimize = optimize,
    });

    lib.addCSourceFiles(.{
        .flags = &.{ "--std=c++20", "-Werror" },
        .root = b.path("SDLGame/"),

        .files = &.{
            "app_context.cpp",
            "circle_shape.cpp",
            "countdown.cpp",
            "text_metrics.cpp",
        },
    });

    lib.linkLibCpp();
    lib.linkSystemLibrary("SDL2");
    lib.linkSystemLibrary("SDL2_image");
    lib.linkSystemLibrary("SDL2_ttf");
    b.installArtifact(lib);

    const forester_game = add_game(b, &.{lib}, .{
        .name = "forester",
        .target = target,
        .optimize = optimize,
    });

    forester_game.addCSourceFiles(.{
        .flags = &.{ "--std=c++20", "-Werror" },
        .root = b.path("Forester/"),

        .files = &.{
            "main.cpp",
            "world_chunk.cpp",
            "world_coordinate.cpp",
            "world_position.cpp",
            "world_space.cpp",
        },
    });

    b.installArtifact(forester_game);

    const chess_game = add_game(b, &.{lib}, .{
        .name = "chess",
        .target = target,
        .optimize = optimize,
    });

    chess_game.addCSourceFiles(.{
        .flags = &.{ "--std=c++20", "-Werror" },
        .root = b.path("Chess/"),

        .files = &.{
            "ChessMoveManager.cpp",
            "ChessPlayer.cpp",
            "ChessPlayerAI.cpp",
            "GameScreen_Chess.cpp",
            "main.cpp",
        },
    });

    b.installArtifact(chess_game);

    const laser_wars_game = add_game(b, &.{lib}, .{
        .name = "zombies",
        .target = target,
        .optimize = optimize,
    });

    laser_wars_game.addCSourceFiles(.{
        .flags = &.{ "--std=c++20", "-Werror" },
        .root = b.path("Zombies/"),

        .files = &.{
            "main.cpp",
            "entity_location.cpp",
            "level_state.cpp",
        },
    });

    b.installArtifact(laser_wars_game);
}
