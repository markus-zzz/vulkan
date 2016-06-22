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
#include <dlfcn.h>
#include <stdio.h>

#include <android_native_app_glue.h>

typedef void (*fptr_t)(struct android_app*);

void android_main(struct android_app* state)
{
	// Make sure glue isn't stripped.
	app_dummy();

	const char *path = state->activity->internalDataPath;
	char dst_path[512];
	snprintf(dst_path, sizeof(dst_path), "%s/libloadme.so", path);

	FILE *src;
	src = fopen("/sdcard/Download/libloadme.so", "r");
	FILE *dst;
	dst = fopen(dst_path, "w");
	while (!feof(src))
	{
		char byte;
		fread(&byte, 1, 1, src);
		fwrite(&byte, 1, 1, dst);
	}

	fclose(src);
	fclose(dst);

	void *handle = dlopen(dst_path, RTLD_NOW | RTLD_GLOBAL);
	if (!handle)
	{
		const char *str = dlerror();
		assert(str);
		return;
	}

	fptr_t am = dlsym(handle, "android_main");
	if (!am)
	{
		const char *str = dlerror();
		assert(str);
		return;
	}

	am(state);
}
