#pragma once

#if !defined(_DEBUG) && !defined(NDEBUG)
#define NDEBUG
#endif

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define WIN32_LEAN_AND_MEAN
#include <tchar.h>
#include <Windows.h>

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
