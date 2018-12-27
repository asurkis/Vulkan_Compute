#pragma once
#include "SharedConst.h"

#define WINDOW_CLASS_NAME _T("WindowClass1")
#define SWAPCHAIN_IMAGE_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define MAX_FRAMES_IN_FLIGHT 1
#define FRAME_TIME_COUNT 128

#ifndef NDEBUG
const char *const layers[] = {
    "VK_LAYER_LUNARG_standard_validation",
};
#endif

