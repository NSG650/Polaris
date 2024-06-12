#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>

#define NAME_MAX 255

typedef int64_t ssize_t;
typedef int64_t off_t;

typedef uint64_t dev_t;
typedef uint64_t ino_t;
typedef unsigned int mode_t;
typedef unsigned long nlink_t;
typedef int64_t blksize_t;
typedef int64_t blkcnt_t;

typedef int32_t pid_t;
typedef int32_t uid_t;
typedef int32_t gid_t;

typedef int64_t time_t;
typedef int64_t clockid_t;

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

#define O_PATH 010000000

#define O_ACCMODE (03 | O_PATH)
#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR 02

#define O_CREAT 0100
#define O_EXCL 0200
#define O_NOCTTY 0400
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_NONBLOCK 04000
#define O_DSYNC 010000
#define O_ASYNC 020000
#define O_DIRECT 040000
#define O_DIRECTORY 0200000
#define O_NOFOLLOW 0400000
#define O_CLOEXEC 02000000
#define O_SYNC 04010000
#define O_RSYNC 04010000
#define O_LARGEFILE 0100000
#define O_NOATIME 01000000
#define O_TMPFILE 020000000

#define O_EXEC O_PATH
#define O_SEARCH O_PATH

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4

#define F_SETOWN 8
#define F_GETOWN 9
#define F_SETSIG 10
#define F_GETSIG 11

#define F_GETLK 5
#define F_SETLK 6
#define F_SETLKW 7

#define F_SETOWN_EX 15
#define F_GETOWN_EX 16

#define F_GETOWNER_UIDS 17

#define F_DUPFD_CLOEXEC 1030
#define F_ADD_SEALS 1033
#define F_GET_SEALS 1034

#define F_SEAL_SEAL 0x0001
#define F_SEAL_SHRINK 0x0002
#define F_SEAL_GROW 0x0004
#define F_SEAL_WRITE 0x0008

#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2

#define FD_CLOEXEC 1

#define AT_FDCWD -100
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR 0x200
#define AT_SYMLINK_FOLLOW 0x400
#define AT_EACCESS 0x200
#define AT_EMPTY_PATH 0x1000

#define S_IFMT 0x0F000
#define S_IFBLK 0x06000
#define S_IFCHR 0x02000
#define S_IFIFO 0x01000
#define S_IFREG 0x08000
#define S_IFDIR 0x04000
#define S_IFLNK 0x0A000
#define S_IFSOCK 0x0C000

#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 070
#define S_IRGRP 040
#define S_IWGRP 020
#define S_IXGRP 010
#define S_IRWXO 07
#define S_IROTH 04
#define S_IWOTH 02
#define S_IXOTH 01
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000

#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#define S_IEXEC S_IXUSR

#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct stat {
	dev_t st_dev;
	ino_t st_ino;
	nlink_t st_nlink;
	mode_t st_mode;
	uid_t st_uid;
	gid_t st_gid;
	unsigned int __pad0;
	dev_t st_rdev;
	off_t st_size;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	long __unused[3];
};

// taken from linux/include/uapi/linux/fb.h
struct fb_bitfield {
	uint32_t offset;	/* beginning of bitfield	*/
	uint32_t length;	/* length of bitfield		*/
	uint32_t msb_right; /* != 0 : Most significant bit is */
						/* right */
};

struct fb_var_screeninfo {
	uint32_t xres; /* visible resolution		*/
	uint32_t yres;
	uint32_t xres_virtual; /* virtual resolution		*/
	uint32_t yres_virtual;
	uint32_t xoffset; /* offset from virtual to visible */
	uint32_t yoffset; /* resolution			*/

	uint32_t bits_per_pixel; /* guess what			*/
	uint32_t grayscale;		 /* 0 = color, 1 = grayscale,	*/
	/* >1 = FOURCC			*/
	struct fb_bitfield red;	  /* bitfield in fb mem if true color, */
	struct fb_bitfield green; /* else only length is significant */
	struct fb_bitfield blue;
	struct fb_bitfield transp; /* transparency			*/

	uint32_t nonstd; /* != 0 Non standard pixel format */

	uint32_t activate; /* see FB_ACTIVATE_*		*/

	uint32_t height; /* height of picture in mm    */
	uint32_t width;	 /* width of picture in mm     */

	uint32_t accel_flags; /* (OBSOLETE) see fb_info.flags */

	/* Timing: All values in pixclocks, except pixclock (of course) */
	uint32_t pixclock;	   /* pixel clock in ps (pico seconds) */
	uint32_t left_margin;  /* time from sync to picture	*/
	uint32_t right_margin; /* time from picture to sync	*/
	uint32_t upper_margin; /* time from sync to picture	*/
	uint32_t lower_margin;
	uint32_t hsync_len;	  /* length of horizontal sync	*/
	uint32_t vsync_len;	  /* length of vertical sync	*/
	uint32_t sync;		  /* see FB_SYNC_*		*/
	uint32_t vmode;		  /* see FB_VMODE_*		*/
	uint32_t rotate;	  /* angle we rotate counter clockwise */
	uint32_t colorspace;  /* colorspace for FOURCC-based modes */
	uint32_t reserved[4]; /* Reserved for future compatibility */
};

struct fb_fix_screeninfo {
	char id[16];			  /* identification string eg "TT Builtin" */
	unsigned long smem_start; /* Start of frame buffer mem */
	/* (physical address) */
	uint32_t smem_len;		  /* Length of frame buffer mem */
	uint32_t type;			  /* see FB_TYPE_*		*/
	uint32_t type_aux;		  /* Interleave for interleaved Planes */
	uint32_t visual;		  /* see FB_VISUAL_*		*/
	uint16_t xpanstep;		  /* zero if no hardware panning  */
	uint16_t ypanstep;		  /* zero if no hardware panning  */
	uint16_t ywrapstep;		  /* zero if no hardware ywrap    */
	uint32_t line_length;	  /* length of a line in bytes    */
	unsigned long mmio_start; /* Start of Memory Mapped I/O   */
	/* (physical address) */
	uint32_t mmio_len; /* Length of Memory Mapped I/O  */
	uint32_t accel;	   /* Indicate to driver which	*/
	/*  specific chip/card we have	*/
	uint16_t capabilities; /* see FB_CAP_*			*/
	uint16_t reserved[2];  /* Reserved for future compatibility */
};

#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404
#define TCSBRK 0x5409
#define TCXONC 0x540A
#define TIOCSCTTY 0x540E
#define TIOCSTI 0x5412
#define TIOCGWINSZ 0x5413
#define TIOCMGET 0x5415
#define TIOCMSET 0x5418
#define TIOCINQ 0x541B
#define TIOCNOTTY 0x5422

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

#endif
