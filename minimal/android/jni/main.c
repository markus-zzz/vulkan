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

#include <assert.h>

#include <android/log.h>
#include <android/window.h>
#include <android_native_app_glue.h>

#include "vulkan_dlfcn/vulkan_dlfcn.h"
#include "vk_minimal.h"

#define LOG_TAG "minimal_vulkan"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))


static void handle_cmd(struct android_app* app, int32_t cmd)
{
	switch (cmd)
	{
		case APP_CMD_SAVE_STATE:
		break;
		case APP_CMD_INIT_WINDOW:
		// The window is being shown, get it ready.
		if (app->window != NULL)
		{
			VkResult err;
			struct vk_minimal_context *actx = app->userData;
			VkAndroidSurfaceCreateInfoKHR asci;
			asci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
			asci.pNext = NULL;
			asci.flags = 0;
			asci.window = app->window;

			VkSurfaceKHR surface;
			err = vkCreateAndroidSurfaceKHR(actx->instance, &asci, NULL, &actx->surface);
			assert(err == VK_SUCCESS);

			vk_minimal_init(actx);
		}
		break;
		case APP_CMD_TERM_WINDOW:
		// The window is being hidden or closed, clean it up.
		break;
		case APP_CMD_GAINED_FOCUS:
		// Start animation
		break;
		case APP_CMD_LOST_FOCUS:
		// Stop animation
		break;
	}
}

void android_main(struct android_app* state)
{
	// Make sure glue isn't stripped.
	app_dummy();

	// Load libvulkan.so
	vulkan_dlfcn_init();

	// Turns out that this is quite essential to make the swapchain work properly
	ANativeActivity_setWindowFlags(state->activity, AWINDOW_FLAG_FULLSCREEN, 0);

	struct vk_minimal_context actx;
	memset(&actx, 0, sizeof(actx));

	state->userData = &actx;

	VkResult err;
	VkApplicationInfo app;
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pNext = NULL;
	app.pApplicationName = NULL;
	app.applicationVersion = 0;
	app.pEngineName = NULL;
	app.engineVersion = 0;
	app.apiVersion = VK_MAKE_VERSION (1,0,2);

	const char *iextensions[] = {
	  "VK_KHR_surface",
	  "VK_KHR_android_surface"
	};

	VkInstanceCreateInfo inst_info;
	memset(&inst_info, 0, sizeof(inst_info));
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = NULL;
	inst_info.pApplicationInfo = &app;
	inst_info.enabledLayerCount = 0;
	inst_info.ppEnabledLayerNames = NULL;
	inst_info.enabledExtensionCount = sizeof(iextensions)/sizeof(iextensions[0]);
	inst_info.ppEnabledExtensionNames = iextensions;

	err = vkCreateInstance(&inst_info, NULL, &actx.instance);
	assert(err == VK_SUCCESS);

	if (state->savedState != NULL)
	{
	}

	state->onAppCmd = handle_cmd;

	while (1)
	{
		int ident;
		int events;
		struct android_poll_source* source;

		while ((ident=ALooper_pollAll(0, NULL, &events, (void**)&source)) != ALOOPER_POLL_ERROR)
		{
			if (source != NULL)
			{
				source->process(state, source);
			}

			if (state->destroyRequested != 0)
			{
				return;
			}

			if (actx.device != VK_NULL_HANDLE)
			{
				vk_minimal_draw(&actx);
			}
		}
	}
}
