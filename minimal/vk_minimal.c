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

#include "vk_minimal.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "vulkan_dlfcn/vulkan_dlfcn.h"

static VkExtent3D extent_2d_to_3d(VkExtent2D e)
{
	VkExtent3D e3 = {e.width, e.height, 1};
	return e3;
}

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

static void draw_grid(struct vk_minimal_context *actx, VkSubresourceLayout *layout, void *rgba_data)
{
	uint32_t color = 0xffffffff;
	uint32_t x, y;

	for (y = 0; y < actx->extent.height; y++)
	{
		for (x = 0; x < actx->extent.width; x++)
		{
			((uint32_t *)rgba_data)[x] = (x % 100 == 0 || y % 100 == 0) ? color : 0x0;
		}
		rgba_data += layout->rowPitch;
	}
}

void vk_minimal_init(struct vk_minimal_context *actx)
{
	VkResult err;
uint32_t gpu_count;
	err = vkEnumeratePhysicalDevices(actx->instance, &gpu_count, NULL);
	assert(err == VK_SUCCESS && gpu_count > 0);

	VkPhysicalDevice physical_devices[gpu_count];
	err = vkEnumeratePhysicalDevices(actx->instance, &gpu_count, physical_devices);
	assert(err == VK_SUCCESS);
	const VkPhysicalDevice gpu = physical_devices[0];

	uint32_t queue_count;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, NULL);
	assert(queue_count > 0);

	VkQueueFamilyProperties queue_props[queue_count];
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, queue_props);
	assert(queue_props[0].queueFlags & VK_QUEUE_GRAPHICS_BIT);
	if (vkGetPhysicalDeviceSurfaceSupportKHR)
	{
		VkBool32 supported;
		vkGetPhysicalDeviceSurfaceSupportKHR(gpu, 0, actx->surface, &supported);
		assert(supported);
	}

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

	err = vkCreateDevice(gpu, &dci, NULL, &actx->device);
	assert(err == VK_SUCCESS);

	vkGetDeviceQueue(actx->device, 0, 0, &actx->queue);

	VkSurfaceCapabilitiesKHR surf_cap;
	err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, actx->surface, &surf_cap);
	assert(err == VK_SUCCESS);
	actx->extent = surf_cap.currentExtent;

	uint32_t formatCount;
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, actx->surface, &formatCount, NULL);
	assert(err == VK_SUCCESS);
	VkSurfaceFormatKHR surfFormats[formatCount];
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, actx->surface, &formatCount, surfFormats);
	assert(err == VK_SUCCESS);

	uint32_t presentModeCount;
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, actx->surface, &presentModeCount, NULL);
	assert(err == VK_SUCCESS && presentModeCount > 0);
	VkPresentModeKHR presentModes[presentModeCount];
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, actx->surface, &presentModeCount, presentModes);
	assert(err == VK_SUCCESS);

	VkSwapchainCreateInfoKHR sci;
	memset(&sci, 0, sizeof(sci));
	sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sci.pNext = NULL;
	sci.surface = actx->surface;
	sci.minImageCount = 3;
	sci.imageFormat = surfFormats[0].format;
	sci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	sci.imageExtent = actx->extent;
	sci.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	sci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	sci.imageArrayLayers = 1;
	sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	sci.queueFamilyIndexCount = 0;
	sci.pQueueFamilyIndices = NULL;
	sci.presentMode = presentModes[0];
	sci.oldSwapchain = NULL;
	sci.clipped = VK_TRUE;

	err = vkCreateSwapchainKHR(actx->device, &sci, NULL, &actx->swapchain.swapchain);
	assert(err == VK_SUCCESS);

	uint32_t count;
	vkGetSwapchainImagesKHR(actx->device, actx->swapchain.swapchain, &count, NULL);
	actx->swapchain.images = malloc(sizeof(actx->swapchain.images[0])*count);
	vkGetSwapchainImagesKHR(actx->device, actx->swapchain.swapchain, &count, actx->swapchain.images);

	VkImageCreateInfo ici;
	memset(&ici, 0, sizeof(ici));
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.pNext = NULL;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.format = surfFormats[0].format;
	ici.extent = extent_2d_to_3d(actx->extent);
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_LINEAR;
	ici.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	ici.flags = 0;
	ici.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

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
	cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

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

void vk_minimal_imb(struct vk_minimal_context *actx,
                    VkImage image,
                    VkAccessFlags srcAccessMask,
                    VkAccessFlags dstAccessMask,
                    VkImageLayout oldLayout,
                    VkImageLayout newLayout)
{
	VkImageMemoryBarrier imb;
	memset(&imb, 0, sizeof(imb));
	imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imb.pNext = NULL;
	imb.srcAccessMask = srcAccessMask;
	imb.dstAccessMask = dstAccessMask;
	imb.oldLayout = oldLayout;
	imb.newLayout = newLayout;
	imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imb.image = image;
	imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imb.subresourceRange.baseMipLevel = 0;
	imb.subresourceRange.levelCount = 1;
	imb.subresourceRange.baseArrayLayer = 0;
	imb.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(actx->cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imb);
}


void vk_minimal_draw(struct vk_minimal_context *actx)
{
	VkResult err;
	void *data;

	err = vkQueueWaitIdle(actx->queue);
	assert(err == VK_SUCCESS);

	err = vkMapMemory(actx->device, actx->canvas.dm, 0, actx->canvas.size, 0, &data);
	assert(err == VK_SUCCESS);

	VkImageSubresource is;
	is.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	is.mipLevel = 0;
	is.arrayLayer = 0;

	VkSubresourceLayout layout;
	vkGetImageSubresourceLayout(actx->device, actx->canvas.image, &is, &layout);

	draw_grid(actx, &layout, data);

	vkUnmapMemory(actx->device, actx->canvas.dm);

	err = vkResetCommandBuffer(actx->cmd, 0);
	assert(err == VK_SUCCESS);

	VkCommandBufferInheritanceInfo cbii;
	memset(&cbii, 0, sizeof(cbii));
	cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	cbii.pNext = NULL;
	cbii.renderPass = VK_NULL_HANDLE;
	cbii.subpass = 0;
	cbii.framebuffer = VK_NULL_HANDLE;
	cbii.occlusionQueryEnable = VK_FALSE;
	cbii.queryFlags = 0;
	cbii.pipelineStatistics = 0;

	VkCommandBufferBeginInfo cbbi;
	memset(&cbbi, 0, sizeof(cbbi));
	cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cbbi.pNext = NULL;
	cbbi.flags = 0;
	cbbi.pInheritanceInfo = &cbii;

	err = vkBeginCommandBuffer(actx->cmd, &cbbi);
	assert(err == VK_SUCCESS);

	vk_minimal_imb(actx, actx->canvas.image, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_GENERAL);

	uint32_t idx = 0;

	VkSemaphore acquire_sem;
	VkSemaphore copy_sem;
	VkSemaphoreCreateInfo csi;
	csi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	csi.pNext = NULL;
	csi.flags = 0;

	err = vkCreateSemaphore(actx->device, &csi, NULL, &acquire_sem);
	assert(err == VK_SUCCESS);
	err = vkCreateSemaphore(actx->device, &csi, NULL, &copy_sem);
	assert(err == VK_SUCCESS);

	err = vkAcquireNextImageKHR(actx->device, actx->swapchain.swapchain, 0, acquire_sem, VK_NULL_HANDLE, &idx);
	assert(err == VK_SUCCESS);

	vk_minimal_imb(actx, actx->swapchain.images[idx], VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	VkImageCopy ic;
	memset(&ic, 0, sizeof(ic));
	ic.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ic.srcSubresource.mipLevel = 0;
	ic.srcSubresource.baseArrayLayer = 0;
	ic.srcSubresource.layerCount = 1;
	ic.srcOffset.x = 0;
	ic.srcOffset.y = 0;
	ic.srcOffset.z = 0;
	ic.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ic.dstSubresource.mipLevel = 0;
	ic.dstSubresource.baseArrayLayer = 0;
	ic.dstSubresource.layerCount = 1;
	ic.dstOffset.x = 0;
	ic.dstOffset.y = 0;
	ic.dstOffset.z = 0;
	ic.extent = extent_2d_to_3d(actx->extent);

	vkCmdCopyImage(actx->cmd, actx->canvas.image, VK_IMAGE_LAYOUT_GENERAL, actx->swapchain.images[idx], VK_IMAGE_LAYOUT_GENERAL, 1, &ic);

	vk_minimal_imb(actx, actx->swapchain.images[idx], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	err = vkEndCommandBuffer(actx->cmd);
	assert(err == VK_SUCCESS);

	VkSubmitInfo si;
	memset(&si, 0, sizeof(si));
	si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	si.pNext = NULL;
	si.waitSemaphoreCount = 1;
	si.pWaitSemaphores = &acquire_sem;
	si.pWaitDstStageMask = NULL;
	si.commandBufferCount = 1;
	si.pCommandBuffers = &actx->cmd;
	si.signalSemaphoreCount = 1;
	si.pSignalSemaphores = &copy_sem;

	err = vkQueueSubmit(actx->queue, 1, &si, VK_NULL_HANDLE);
	assert(err == VK_SUCCESS);

	VkPresentInfoKHR pi;
	pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pi.pNext = NULL;
	pi.waitSemaphoreCount = 1;
	pi.pWaitSemaphores = &copy_sem;
	pi.swapchainCount = 1;
	pi.pSwapchains = &actx->swapchain.swapchain;
	pi.pImageIndices = &idx;
	pi.pResults = NULL;

	err = vkQueuePresentKHR(actx->queue, &pi);
	assert(err == VK_SUCCESS);

	err = vkQueueWaitIdle(actx->queue);
	assert(err == VK_SUCCESS);

	vkDestroySemaphore(actx->device, acquire_sem, NULL);
	vkDestroySemaphore(actx->device, copy_sem, NULL);
}

