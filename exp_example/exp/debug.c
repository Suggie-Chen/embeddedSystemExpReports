#include <stdio.h>
#include <stdarg.h>
#include "stm32f4xx_hal.h"
extern int board_putc(unsigned char c);

// ----------------------------------------------------------------------------
static int trace_write(const char *s, int n) {
	for (int i = 0; i < n; ++i) {
		board_putc(s[i]);
	}
	return n;
}

__weak int trace_printf(const char *format, ...) {
	int ret;
	va_list ap;

	va_start(ap, format);

	// TODO: rewrite it to no longer use newlib, it is way too heavy

	static char buf[256];

	// Print to the local buffer
	ret = vsnprintf(buf, sizeof(buf), format, ap);
	if (ret > 0) {
		// Transfer the buffer to the device
		ret = trace_write(buf, (size_t) ret);
	}

	va_end(ap);
	return ret;
}
