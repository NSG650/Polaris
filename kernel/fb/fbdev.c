#include "debug/debug.h"
#include <errno.h>
#include <fb/fb.h>
#include <fs/vfs.h>
#include <klibc/mem.h>
#include <mm/mmap.h>

#define FB_ACTIVATE_NOW 0	  /* set values immediately (or vbl)*/
#define FB_ACTIVATE_NXTOPEN 1 /* activate on next open	*/
#define FB_ACTIVATE_TEST 2	  /* don't set, round up impossible */
#define FB_ACTIVATE_MASK 15

#define FB_VMODE_NONINTERLACED 0 /* non interlaced */
#define FB_VMODE_INTERLACED 1	 /* interlaced	*/
#define FB_VMODE_DOUBLE 2		 /* double scan */
#define FB_VMODE_ODD_FLD_FIRST 4 /* interlaced: top line first */
#define FB_VMODE_MASK 255

#define FB_TYPE_PACKED_PIXELS 0		 /* Packed Pixels	*/
#define FB_TYPE_PLANES 1			 /* Non interleaved planes */
#define FB_TYPE_INTERLEAVED_PLANES 2 /* Interleaved planes	*/
#define FB_TYPE_TEXT 3				 /* Text/attributes	*/
#define FB_TYPE_VGA_PLANES 4		 /* EGA/VGA planes	*/
#define FB_TYPE_FOURCC 5			 /* Type identified by a V4L2 FOURCC */

#define FB_VISUAL_MONO01 0			   /* Monochr. 1=Black 0=White */
#define FB_VISUAL_MONO10 1			   /* Monochr. 1=White 0=Black */
#define FB_VISUAL_TRUECOLOR 2		   /* True color	*/
#define FB_VISUAL_PSEUDOCOLOR 3		   /* Pseudo color (like atari) */
#define FB_VISUAL_DIRECTCOLOR 4		   /* Direct color */
#define FB_VISUAL_STATIC_PSEUDOCOLOR 5 /* Pseudo color readonly */
#define FB_VISUAL_FOURCC 6			   /* Visual identified by a V4L2 FOURCC */

#define FBIOGET_VSCREENINFO 0x4600
#define FBIOPUT_VSCREENINFO 0x4601
#define FBIOGET_FSCREENINFO 0x4602

#define FBIOBLANK 0x4611

static struct fb_var_screeninfo framebuffer_var_info = {0};
static struct fb_fix_screeninfo framebuffer_fix_info = {0};
static struct resource *framebuff_res;

static ssize_t fbdev_write(struct resource *this,
						   struct f_description *description, const void *buf,
						   off_t offset, size_t count) {
	(void)description;
	spinlock_acquire_or_wait(&this->lock);
	memcpy((void *)(framebuff.address + offset), buf, count);
	spinlock_drop(&this->lock);
	return 0;
}

static void *fbdev_mmap(struct resource *this, size_t file_page, int flags) {
	(void)flags;
	spinlock_acquire_or_wait(&this->lock);
	size_t offset = file_page * PAGE_SIZE;

	if (offset >= (framebuff.height * framebuff.pitch)) {
		return NULL;
	}

	spinlock_drop(&this->lock);

	return (void *)(((uint64_t)(framebuff.address) - MEM_PHYS_OFFSET) + offset);
}

static int fbdev_ioctl(struct resource *this, struct f_description *description,
					   uint64_t request, uint64_t arg) {
	switch (request) {
		case FBIOGET_VSCREENINFO:
			*(struct fb_var_screeninfo *)arg = framebuffer_var_info;
			return 0;
		case FBIOGET_FSCREENINFO:
			*(struct fb_fix_screeninfo *)arg = framebuffer_fix_info;
			return 0;
		case FBIOPUT_VSCREENINFO:
			spinlock_acquire_or_wait(&this->lock);
			framebuffer_var_info = *(struct fb_var_screeninfo *)arg;
			spinlock_drop(&this->lock);
			return 0;
		case FBIOBLANK:
			return 0;
	}

	return resource_default_ioctl(this, description, request, arg);
}

void fbdev_init(void) {
	extern uint8_t framebuffer_initialised;
	if (!framebuffer_initialised)
		return;

	framebuffer_fix_info.smem_len = framebuff.pitch * framebuff.height;
	framebuffer_fix_info.mmio_len = framebuff.pitch * framebuff.height;
	framebuffer_fix_info.line_length = framebuff.pitch;
	framebuffer_fix_info.type = FB_TYPE_PACKED_PIXELS;
	framebuffer_fix_info.visual = FB_VISUAL_TRUECOLOR;

	framebuffer_var_info.xres = framebuff.width;
	framebuffer_var_info.yres = framebuff.height;
	framebuffer_var_info.xres_virtual = framebuff.width;
	framebuffer_var_info.yres_virtual = framebuff.height;
	framebuffer_var_info.bits_per_pixel = framebuff.bpp;
	framebuffer_var_info.red = framebuff.color_masks[0];
	framebuffer_var_info.green = framebuff.color_masks[1];
	framebuffer_var_info.blue = framebuff.color_masks[2];
	framebuffer_var_info.activate = FB_ACTIVATE_NOW;
	framebuffer_var_info.vmode = FB_VMODE_NONINTERLACED;
	framebuffer_var_info.width = -1;
	framebuffer_var_info.height = -1;

	framebuff_res = resource_create(sizeof(struct resource));

	framebuff_res->ioctl = fbdev_ioctl;
	framebuff_res->write = fbdev_write;
	framebuff_res->mmap = fbdev_mmap;
	framebuff_res->can_mmap = true;

	framebuff_res->stat.st_size = 0;
	framebuff_res->stat.st_blocks = 0;
	framebuff_res->stat.st_blksize = 4096;
	framebuff_res->stat.st_rdev = resource_create_dev_id();
	framebuff_res->stat.st_mode = 0666 | S_IFCHR;

	devtmpfs_add_device(framebuff_res, "fbdev");
	vfs_symlink(vfs_root, "/dev/fbdev", "/dev/fb0");
}
