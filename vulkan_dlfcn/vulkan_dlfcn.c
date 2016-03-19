/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "vulkan_dlfcn.h"
#include <dlfcn.h>

#if __ANDROID__

#include <android/log.h>
#define  LOG_TAG    "vulkan-dlfcn"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#else

#include <stdio.h>
#define  LOGD(...) printf("D:"__VA_ARGS__)
#define  LOGE(...) printf("E:"__VA_ARGS__)
#define  LOGI(...) printf("I:"__VA_ARGS__)

#endif

static void *vulkan_so = NULL;

#define DEF_VK_FCN(x) PFN_##x x = NULL;
#include "vulkan_dlfcn.def"
#undef DEF_VK_FCN


void vulkan_dlfcn_init(void)
{
	vulkan_so = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	if (!vulkan_so)
	{
		LOGE("Vulkan not available: %s\n", dlerror());
	}

#define DEF_VK_FCN(x) \
	if (!(x = (PFN_##x)dlsym(vulkan_so, #x))) { \
		LOGE("Vulkan symbol '%s' is not available: %s\n", #x, dlerror()); \
	}
#include "vulkan_dlfcn.def"
#undef DEF_VK_FCN
}

