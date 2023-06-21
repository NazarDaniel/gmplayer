#pragma once

#include <SDL_audio.h>

inline constexpr int NUM_FRAMES         = 2048;
inline constexpr int NUM_CHANNELS       = 2;
inline constexpr int NUM_VOICES         = 8;
inline constexpr int FRAME_SIZE         = NUM_VOICES * NUM_CHANNELS;
inline constexpr int MAX_VOLUME_VALUE   = SDL_MIX_MAXVOLUME;