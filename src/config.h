#pragma once

// core game engine
#define USE_GLFW 1

// core graphic engine
// @warn  GLEW is only usable by enabling GLFW.
#define USE_GLEW 1

// compatibility
#define USE_LR2_FEATURE 1
#define USE_LR2_FONT 1

// game playing
namespace rhythmus
{

constexpr size_t kMaxPlaySession = 4;
constexpr size_t kMaxLaneCount = 16;
constexpr size_t kMaxChannelCount = 2048;

}