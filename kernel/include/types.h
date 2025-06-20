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

typedef uint32_t socklen_t;

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
#define TIOCGSID 0x5429

// Start ioctl stuff
#define _IOC_NRBITS 8
#define _IOC_TYPEBITS 8

#ifndef _IOC_SIZEBITS
#define _IOC_SIZEBITS 14
#endif

#ifndef _IOC_DIRBITS
#define _IOC_DIRBITS 2
#endif

#define _IOC_NRMASK ((1 << _IOC_NRBITS) - 1)
#define _IOC_TYPEMASK ((1 << _IOC_TYPEBITS) - 1)
#define _IOC_SIZEMASK ((1 << _IOC_SIZEBITS) - 1)
#define _IOC_DIRMASK ((1 << _IOC_DIRBITS) - 1)

#define _IOC_NRSHIFT 0
#define _IOC_TYPESHIFT (_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_SIZESHIFT (_IOC_TYPESHIFT + _IOC_TYPEBITS)
#define _IOC_DIRSHIFT (_IOC_SIZESHIFT + _IOC_SIZEBITS)

#ifndef _IOC_NONE
#define _IOC_NONE 0U
#endif

#ifndef _IOC_WRITE
#define _IOC_WRITE 1U
#endif

#ifndef _IOC_READ
#define _IOC_READ 2U
#endif

#define _IOC(dir, type, nr, size)                            \
	(((dir) << _IOC_DIRSHIFT) | ((type) << _IOC_TYPESHIFT) | \
	 ((nr) << _IOC_NRSHIFT) | ((size) << _IOC_SIZESHIFT))

#define _IOC_TYPECHECK(t) (sizeof(t))

#define _IO(type, nr) _IOC(_IOC_NONE, (type), (nr), 0)
#define _IOR(type, nr, argtype) \
	_IOC(_IOC_READ, (type), (nr), (_IOC_TYPECHECK(argtype)))
#define _IOW(type, nr, argtype) \
	_IOC(_IOC_WRITE, (type), (nr), (_IOC_TYPECHECK(argtype)))

#define TIOCGPTN \
	_IOR('T', 0x30, unsigned int)		/* Get Pty Number (of pty-mux device) */
#define TIOCSPTLCK _IOW('T', 0x31, int) /* Lock/unlock Pty */
#define TIOCGDEV                                                             \
	_IOR('T', 0x32, unsigned int) /* Get primary device node of /dev/console \
								   */
#define TCGETX 0x5432			  /* SYS5 TCGETX compatibility */
#define TCSETX 0x5433
#define TCSETXF 0x5434
#define TCSETXW 0x5435
#define TIOCSIG _IOW('T', 0x36, int) /* pty: generate signal */
#define TIOCVHANGUP 0x5437
#define TIOCGPKT _IOR('T', 0x38, int)	/* Get packet mode state */
#define TIOCGPTLCK _IOR('T', 0x39, int) /* Get Pty lock state */
#define TIOCGEXCL _IOR('T', 0x40, int)	/* Get exclusive mode state */
#define TIOCGPTPEER _IO('T', 0x41)		/* Safely open the slave */

#define POLLIN 0x0001
#define POLLPRI 0x0002
#define POLLOUT 0x0004
#define POLLERR 0x0008
#define POLLHUP 0x0010
#define POLLNVAL 0x0020
#define POLLRDNORM 0x0040
#define POLLRDBAND 0x0080
#define POLLWRNORM 0x0100
#define POLLWRBAND 0x0200
#define POLLRDHUP 0x2000

#define SOCK_CLOEXEC O_CLOEXEC
#define SOCK_NONBLOCK O_NONBLOCK

#endif
