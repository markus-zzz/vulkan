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

#ifndef VK_MINIMAL_H
#define VK_MINIMAL_H

#include "vulkan_dlfcn/vulkan_dlfcn.h"

struct vk_minimal_context {
	VkInstance instance;
	VkDevice device;
	VkSurfaceKHR surface;
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

	uint32_t cntr;
	VkExtent2D extent;
};

void vk_minimal_init(struct vk_minimal_context *actx);
void vk_minimal_draw(struct vk_minimal_context *actx);

#endif
