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
#include <string.h>
#include <xcb/xcb.h>

#include "vulkan_dlfcn/vulkan_dlfcn.h"
#include "vk_minimal.h"

#define LOGI(...) ((void)printf(__VA_ARGS__))
#define LOGW(...) ((void)printf(__VA_ARGS__))

int main(int argc, char **argv)
{
	// Load libvulkan.so
	vulkan_dlfcn_init();

	struct vk_minimal_context actx;
	memset(&actx, 0, sizeof(actx));

	xcb_connection_t *connection;
	xcb_screen_t *screen;
	xcb_window_t window;

	const xcb_setup_t *setup;
	xcb_screen_iterator_t iter;
	int scr;
	uint32_t value_mask, value_list[32];

	connection = xcb_connect(NULL, &scr);
	assert(connection);

	setup = xcb_get_setup(connection);
	iter = xcb_setup_roots_iterator(setup);
	while (scr-- > 0)
		xcb_screen_next(&iter);

	screen = iter.data;

	window = xcb_generate_id(connection);

	value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value_list[0] = screen->black_pixel;
	value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	xcb_create_window(connection, XCB_COPY_FROM_PARENT, window,
	                  screen->root, 0, 0, 1920, 1080, 0,
	                  XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
	                  value_mask, value_list);

	xcb_map_window(connection, window);

	const uint32_t coords[] = {100, 100};
	xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);

	xcb_flush(connection);

	VkResult err;
	VkApplicationInfo app;
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pNext = NULL;
	app.pApplicationName = NULL;
	app.applicationVersion = 0;
	app.pEngineName = NULL;
	app.engineVersion = 0;
	app.apiVersion = VK_API_VERSION_1_0;

	const char *iextensions[] = {
	  "VK_KHR_surface",
	  "VK_KHR_xcb_surface"
	};

	VkInstanceCreateInfo inst_info;
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = NULL;
	inst_info.pApplicationInfo = &app;
	inst_info.enabledLayerCount = 0;
	inst_info.ppEnabledLayerNames = NULL;
	inst_info.enabledExtensionCount = sizeof(iextensions)/sizeof(iextensions[0]);
	inst_info.ppEnabledExtensionNames = iextensions;

	err = vkCreateInstance(&inst_info, NULL, &actx.instance);
	assert(err == VK_SUCCESS);

	VkXcbSurfaceCreateInfoKHR asci;
	asci.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	asci.pNext = NULL;
	asci.flags = 0;
	asci.window = window;
	asci.connection = connection;

	err = vkCreateXcbSurfaceKHR(actx.instance, &asci, NULL, &actx.surface);
	assert(err == VK_SUCCESS);

	vk_minimal_init(&actx);

	while (1)
	{
		vk_minimal_draw(&actx);
	}

	return 0;
}

