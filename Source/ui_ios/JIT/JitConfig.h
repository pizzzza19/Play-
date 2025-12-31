#ifndef JIT_CONFIG_H
#define JIT_CONFIG_H

#include <stddef.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define PLAY_IOS_JIT 1
#endif
#endif

#ifndef PLAY_IOS_JIT
#define PLAY_IOS_JIT 0
#endif

constexpr size_t PLAY_JIT_REGION_SIZE = 64 * 1024 * 1024;

#endif
