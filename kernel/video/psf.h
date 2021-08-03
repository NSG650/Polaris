#ifndef PSF_H
#define PSF_H

#include <stdint.h>

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t headersize;
	uint32_t flags;

	uint32_t numglyph;
	uint32_t glyph_size;
	uint32_t height;
	uint32_t width;

	uint8_t data[];
} PSF;

#endif
