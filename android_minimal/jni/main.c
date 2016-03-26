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

struct app_context {
	VkDevice device;
	VkQueue queue;
	VkCommandBuffer cmd;

	struct {
		VkSwapchainKHR swapchain;
		VkImage *images;
	} swapchain;

	struct {
		VkImage image;
		VkDeviceMemory dm;
		uint32_t size;
	} canvas;
};

static uint32_t get_memory_type_idx(VkPhysicalDevice device, uint32_t typebits, VkFlags properties)
{
	VkPhysicalDeviceMemoryProperties pdmp;
	vkGetPhysicalDeviceMemoryProperties(device, &pdmp);
	uint32_t i;
	for (i = 0; i < pdmp.memoryTypeCount; i++)
	{
		if ((typebits & (1 << i)) && (pdmp.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	assert(0 && "requested memory not found");

	return 0;
}

static void draw_grid(uint32_t *rgba_data)
{
	int x, y;
	for (x = 0; x < 1920; x++)
	{
		for (y = 0; y < 1080; y++)
		{
			rgba_data[y*1920+x] = (x % 100 == 0 || y % 100 == 0) ? 0xffffffff : 0x0;
		}
	}
}

void vk_init(struct app_context *actx, ANativeWindow *window)
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
	err = vkCreateDevice(gpu, &dci, NULL, &actx->device);
	assert(err == VK_SUCCESS);

	vkGetDeviceQueue(actx->device, 0, 0, &actx->queue);

	VkAndroidSurfaceCreateInfoKHR asci;
	asci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	asci.pNext = NULL;
	asci.flags = 0;
	asci.window = window;

	VkSurfaceKHR surface;
	err = vkCreateAndroidSurfaceKHR(inst, &asci, NULL, &surface);
	assert(err == VK_SUCCESS);

	VkSurfaceCapabilitiesKHR surf_cap;
	err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surf_cap);
	assert(err == VK_SUCCESS);

	VkSwapchainCreateInfoKHR sci;
	sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sci.pNext = NULL;
	sci.surface = surface;
	sci.minImageCount = 3;
	sci.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	sci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	sci.imageExtent = surf_cap.currentExtent;
	sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	sci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	sci.imageArrayLayers = 1;
	sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	sci.queueFamilyIndexCount = 0;
	sci.pQueueFamilyIndices = NULL;
	sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	sci.oldSwapchain = NULL;
	sci.clipped = VK_TRUE;

	err = vkCreateSwapchainKHR(actx->device, &sci, NULL, &actx->swapchain.swapchain);
	assert(err == VK_SUCCESS);

	uint32_t count;
	vkGetSwapchainImagesKHR(actx->device, actx->swapchain.swapchain, &count, NULL);
	actx->swapchain.images = malloc(sizeof(actx->swapchain.images[0])*count);
	vkGetSwapchainImagesKHR(actx->device, actx->swapchain.swapchain, &count, actx->swapchain.images);


	VkImageCreateInfo ici;
	const VkExtent3D e3d = {1920, 1080, 1};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.pNext = NULL;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.format = VK_FORMAT_R8G8B8A8_UNORM;
	ici.extent = e3d;
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_LINEAR;
	ici.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	ici.flags = 0;

	err = vkCreateImage(actx->device, &ici, NULL, &actx->canvas.image);
	assert(err == VK_SUCCESS);

	VkMemoryRequirements mr;
	vkGetImageMemoryRequirements(actx->device, actx->canvas.image, &mr);

	VkMemoryAllocateInfo mai;
	mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mai.pNext = NULL;
	mai.allocationSize = actx->canvas.size = mr.size;
	mai.memoryTypeIndex = get_memory_type_idx(gpu, mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	err = vkAllocateMemory(actx->device, &mai, NULL, &actx->canvas.dm);
	assert(err == VK_SUCCESS);

	err = vkBindImageMemory(actx->device, actx->canvas.image, actx->canvas.dm, 0);
	assert(err == VK_SUCCESS);

	VkCommandPoolCreateInfo cpci;
	cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cpci.pNext = NULL;
	cpci.queueFamilyIndex = 0;
	cpci.flags = 0;

	VkCommandPool cmd_pool;
	err = vkCreateCommandPool(actx->device, &cpci, NULL, &cmd_pool);
	assert(err == VK_SUCCESS);

	VkCommandBufferAllocateInfo cbai;
	cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbai.pNext = NULL;
	cbai.commandPool = cmd_pool;
	cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbai.commandBufferCount = 1;

	err = vkAllocateCommandBuffers(actx->device, &cbai, &actx->cmd);
	assert(err == VK_SUCCESS);
}

void vk_draw(struct app_context *actx)
{
	VkResult err;
	void *data;

	err = vkQueueWaitIdle(actx->queue);
	assert(err == VK_SUCCESS);

	err = vkMapMemory(actx->device, actx->canvas.dm, 0, actx->canvas.size, 0, &data);
	assert(err == VK_SUCCESS);

	draw_grid(data);

	vkUnmapMemory(actx->device, actx->canvas.dm);

	err = vkResetCommandBuffer(actx->cmd, 0);
	assert(err == VK_SUCCESS);

	VkCommandBufferInheritanceInfo cbii;
	cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	cbii.pNext = NULL;
	cbii.renderPass = VK_NULL_HANDLE;
	cbii.subpass = 0;
	cbii.framebuffer = VK_NULL_HANDLE;
	cbii.occlusionQueryEnable = VK_FALSE;
	cbii.queryFlags = 0;
	cbii.pipelineStatistics = 0;

	VkCommandBufferBeginInfo cbbi;
	cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cbbi.pNext = NULL;
	cbbi.flags = 0;
	cbbi.pInheritanceInfo = &cbii;

	err = vkBeginCommandBuffer(actx->cmd, &cbbi);
	assert(err == VK_SUCCESS);

	VkImageCopy copy_region = {
	    .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
	    .srcOffset = {0, 0, 0},
	    .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
	    .dstOffset = {0, 0, 0},
	    .extent = {1920, 1080, 1},
	};

	uint32_t idx;

	err = vkAcquireNextImageKHR(actx->device, actx->swapchain.swapchain, UINT64_MAX,
	                            VK_NULL_HANDLE, VK_NULL_HANDLE, &idx);
	assert(err == VK_SUCCESS);

	vkCmdCopyImage(actx->cmd,
	               actx->canvas.image, VK_IMAGE_LAYOUT_GENERAL,
	               actx->swapchain.images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	               1, &copy_region);

	err = vkEndCommandBuffer(actx->cmd);
	assert(err == VK_SUCCESS);

	VkCommandBuffer cmd_bufs[] = {actx->cmd};
	VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	                            .pNext = NULL,
	                            .waitSemaphoreCount = 0,
	                            .pWaitSemaphores = NULL,
	                            .pWaitDstStageMask = NULL,
	                            .commandBufferCount = 1,
	                            .pCommandBuffers = cmd_bufs,
	                            .signalSemaphoreCount = 0,
	                            .pSignalSemaphores = NULL};

	err = vkQueueSubmit(actx->queue, 1, &submit_info, VK_NULL_HANDLE);
	assert(err == VK_SUCCESS);

	err = vkQueueWaitIdle(actx->queue);
	assert(err == VK_SUCCESS);

	VkPresentInfoKHR pi;
	pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pi.pNext = NULL;
	pi.waitSemaphoreCount = 0;
	pi.pWaitSemaphores = NULL;
	pi.swapchainCount = 1;
	pi.pSwapchains = &actx->swapchain.swapchain;
	pi.pImageIndices = &idx;
	pi.pResults = NULL;

	err = vkQueuePresentKHR(actx->queue, &pi);
	assert(err == VK_SUCCESS);
}

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
			vk_init(app->userData, app->window);
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

	struct app_context actx;
	memset(&actx, 0, sizeof(actx));
	state->userData = &actx;

	if (state->savedState != NULL)
	{
	}

	state->onAppCmd = handle_cmd;

	while (1)
	{
		int ident;
		int events;
		struct android_poll_source* source;

		while ((ident=ALooper_pollAll(1000, NULL, &events, (void**)&source)) != ALOOPER_POLL_ERROR)
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
				vk_draw(&actx);
			}
		}
	}
}
