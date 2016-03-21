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
#include <android_native_app_glue.h>

#include "vulkan_dlfcn/vulkan_dlfcn.h"

#define LOG_TAG "minimal_vulkan"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))

void vk_init(ANativeWindow *window)
{
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
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = NULL;
	inst_info.pApplicationInfo = &app;
	inst_info.enabledLayerCount = 0;
	inst_info.ppEnabledLayerNames = NULL;
	inst_info.enabledExtensionCount = sizeof(iextensions)/sizeof(iextensions[0]);
	inst_info.ppEnabledExtensionNames = iextensions;

	VkInstance inst;
	err = vkCreateInstance(&inst_info, NULL, &inst);
	assert(err == VK_SUCCESS);

	uint32_t gpu_count;
	err = vkEnumeratePhysicalDevices(inst, &gpu_count, NULL);
	assert(err == VK_SUCCESS && gpu_count > 0);

	VkPhysicalDevice physical_devices[gpu_count];
	err = vkEnumeratePhysicalDevices(inst, &gpu_count, physical_devices);
	assert(err == VK_SUCCESS);
	const VkPhysicalDevice gpu = physical_devices[0];

	uint32_t queue_count;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, NULL);
	assert(queue_count > 0);

	VkQueueFamilyProperties queue_props[queue_count];
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, queue_props);
	assert(queue_props[0].queueFlags & VK_QUEUE_GRAPHICS_BIT);

	float queue_priorities[] = {0.0};
	VkDeviceQueueCreateInfo dqci;
	dqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	dqci.pNext = NULL;
	dqci.queueFamilyIndex = 0;
	dqci.queueCount = 1;
	dqci.pQueuePriorities = queue_priorities;

	const char *dextensions[] = {
	  "VK_KHR_swapchain"
	};

	VkDeviceCreateInfo dci;
	dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.pNext = NULL;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &dqci;
	dci.enabledLayerCount = 0;
	dci.ppEnabledLayerNames = NULL;
	dci.enabledExtensionCount = sizeof(dextensions)/sizeof(dextensions[0]);
	dci.ppEnabledExtensionNames = dextensions;
	dci.pEnabledFeatures = NULL;

	VkDevice device;
	err = vkCreateDevice(gpu, &dci, NULL, &device);
	assert(err == VK_SUCCESS);

	VkQueue queue;
	vkGetDeviceQueue(device, 0, 0, &queue);

	VkAndroidSurfaceCreateInfoKHR surf_info;
	surf_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	surf_info.pNext = NULL;
	surf_info.flags = 0;
	surf_info.window = window;

	VkSurfaceKHR surface;
	err = vkCreateAndroidSurfaceKHR(inst, &surf_info, NULL, &surface);
	assert(err == VK_SUCCESS);

    VkSurfaceCapabilitiesKHR surf_cap;
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surf_cap);
	assert(err == VK_SUCCESS);
}

static void handle_cmd(struct android_app* app, int32_t cmd)
{
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (app->window != NULL) {
				vk_init(app->window);
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

    state->onAppCmd = handle_cmd;

    vulkan_dlfcn_init();

    if (state->savedState != NULL) {
    }

    while (1) {
        int ident;
        int events;
        struct android_poll_source* source;

        while ((ident=ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0) {

            if (source != NULL) {
                source->process(state, source);
            }

            if (state->destroyRequested != 0) {
                return;
            }
        }
    }
}
