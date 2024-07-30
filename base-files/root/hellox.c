#include <X11/Xlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#if defined(__x86_64__)
#include <cpuid.h>
#endif

int main(void) {
	Display *d;
	Window w;
	XEvent e;
	int s;
	struct utsname uname_buffer;

	char buffer[1024] = {0};
	uname(&uname_buffer);

#if defined(__x86_64__)
	char cpu_brand_string[64] = {0};
	__get_cpuid(0x80000002, (uint32_t *)(cpu_brand_string + 0),
				(uint32_t *)(cpu_brand_string + 4),
				(uint32_t *)(cpu_brand_string + 8),
				(uint32_t *)(cpu_brand_string + 12));
	__get_cpuid(0x80000003, (uint32_t *)(cpu_brand_string + 16),
				(uint32_t *)(cpu_brand_string + 20),
				(uint32_t *)(cpu_brand_string + 24),
				(uint32_t *)(cpu_brand_string + 28));
	__get_cpuid(0x80000004, (uint32_t *)(cpu_brand_string + 32),
				(uint32_t *)(cpu_brand_string + 36),
				(uint32_t *)(cpu_brand_string + 40),
				(uint32_t *)(cpu_brand_string + 44));
	const char *p = cpu_brand_string;
	while (*p == ' ') {
		++p;
	}
#endif

	snprintf(buffer, 1024, "%s running on %s!", uname_buffer.sysname, p);

	d = XOpenDisplay(NULL);
	if (d == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	s = DefaultScreen(d);
	w = XCreateSimpleWindow(d, RootWindow(d, s), 10, 10, 500, 100, 1,
							BlackPixel(d, s), WhitePixel(d, s));
	XSelectInput(d, w, ExposureMask | KeyPressMask);
	XMapWindow(d, w);

	while (1) {
		XNextEvent(d, &e);
		if (e.type == Expose) {
			XDrawString(d, w, DefaultGC(d, s), 10, 50, buffer, strlen(buffer));
		}
	}

	XCloseDisplay(d);
	return 0;
}
