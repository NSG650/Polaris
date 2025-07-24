#include "types.h"
#include <stdint.h>

int stdin = 0;
int stdout = 1;

struct fbdev_info {
	size_t pitch, bpp;
	uint16_t width, height;
};

struct sysinfo {
	long uptime;			 /* Seconds since boot */
	unsigned long loads[3];	 /* 1, 5, and 15 minute load averages */
	unsigned long totalram;	 /* Total usable main memory size */
	unsigned long freeram;	 /* Available memory size */
	unsigned long sharedram; /* Amount of shared memory */
	unsigned long bufferram; /* Memory used by buffers */
	unsigned long totalswap; /* Total swap space size */
	unsigned long freeswap;	 /* swap space still available */
	unsigned short procs;	 /* Number of current processes */
	unsigned long totalhigh; /* Total high memory size */
	unsigned long freehigh;	 /* Available high memory size */
	unsigned int mem_unit;	 /* Memory unit size in bytes */
	char _f[20 - 2 * sizeof(long) - sizeof(int)]; /* Padding to 64 bytes */
};

#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04

#define MAP_FAILED ((void *)(-1))
#define MAP_FILE 0x00
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED 0x10
#define MAP_ANON 0x20
#define MAP_ANONYMOUS 0x20
#define MAP_NORESERVE 0x4000

#define MS_ASYNC 0x01
#define MS_INVALIDATE 0x02
#define MS_SYNC 0x04

#define MCL_CURRENT 0x01
#define MCL_FUTURE 0x02

#define POSIX_MADV_NORMAL 0
#define POSIX_MADV_RANDOM 1
#define POSIX_MADV_SEQUENTIAL 2
#define POSIX_MADV_WILLNEED 3
#define POSIX_MADV_DONTNEED 4

#define MADV_NORMAL 0
#define MADV_RANDOM 1
#define MADV_SEQUENTIAL 2
#define MADV_WILLNEED 3
#define MADV_DONTNEED 4
#define MADV_FREE 8

#define MREMAP_MAYMOVE 1
#define MREMAP_FIXED 2

#define MFD_CLOEXEC 1U
#define MFD_ALLOW_SEALING 2U

extern uint64_t syscall0(uint64_t syscall_number);
extern uint64_t syscall1(uint64_t syscall_number, uint64_t args0);
extern uint64_t syscall2(uint64_t syscall_number, uint64_t args0,
						 uint64_t args1);
extern uint64_t syscall3(uint64_t syscall_number, uint64_t args0,
						 uint64_t args1, uint64_t args2);
extern uint64_t syscall4(uint64_t syscall_number, uint64_t args0,
						 uint64_t args1, uint64_t args2, uint64_t args3);
extern uint64_t syscall5(uint64_t syscall_number, uint64_t args0,
						 uint64_t args1, uint64_t args2, uint64_t args3,
						 uint64_t args4);
extern uint64_t syscall6(uint64_t syscall_number, uint64_t args0,
						 uint64_t args1, uint64_t args2, uint64_t args3,
						 uint64_t args4, uint64_t args5);

extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);

size_t strlen(char *string) {
	size_t count = 0;
	while (*string++)
		count++;
	return count;
}

char *ltoa(int64_t value, char *str, int base) {
	char *rc;
	char *ptr;
	char *low;
	// Check for supported base.
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	rc = ptr = str;
	// Set '-' for negative decimals.
	if (value < 0 && base == 10) {
		*ptr++ = '-';
	}
	// Remember where the numbers start.
	low = ptr;
	// The actual conversion.
	do {
		// Modulo is negative for negative value. This trick makes abs()
		// unnecessary.
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnop"
				 "qrstuvwxyz"[35 + value % base];
		value /= base;
	} while (value);
	// Terminating the string.
	*ptr-- = '\0';
	// Invert the numbers.
	while (low < ptr) {
		char tmp = *low;
		*low++ = *ptr;
		*ptr-- = tmp;
	}
	return rc;
}

void puts(char *string) {
	syscall1(0x67, (uint64_t)string);
}

ssize_t read(int fd, void *data, size_t count) {
	return syscall3(0x0, fd, (uint64_t)data, count);
}

ssize_t write(int fd, const void *data, size_t count) {
	return syscall3(0x1, fd, (uint64_t)data, count);
}

int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags) {
	return syscall4(0x106, dirfd, (uint64_t)pathname, (uint64_t)buf, flags);
}

int ioctl(int fd, unsigned long request, void *arg) {
	return syscall3(0x10, fd, request, (uint64_t)arg);
}

int open(const char *path, int flags, mode_t mode) {
	return syscall3(0x2, (uint64_t)path, flags, mode);
}

off_t lseek(int fd, off_t offset, int whence) {
	return syscall3(0x8, fd, offset, whence);
}

int openat(int dirfd, const char *path, int flags, mode_t mode) {
	return syscall4(0x101, dirfd, (uint64_t)path, flags, mode);
}

int close(int fd) {
	return syscall1(0x3, fd);
}

int chdir(const char *path) {
	return syscall1(0x50, (uint64_t)path);
}

int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath,
		   int flags) {
	return syscall5(0x109, olddirfd, (uint64_t)oldpath, newdirfd,
					(uint64_t)newpath, flags);
}

int unlinkat(int dirfd, const char *pathname, int flags) {
	return syscall3(0x107, dirfd, (uint64_t)pathname, flags);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
		   off_t offset) {
	return (void *)syscall6(0x9, (uint64_t)addr, length, prot, flags, fd,
							offset);
}

int munmap(void *addr, size_t length) {
	return syscall2(0xb, (uint64_t)addr, length);
}

int fork(void) {
	return syscall0(0x39);
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
	return syscall2(0x23, (uint64_t)req, (uint64_t)rem);
}

int execve(char *path, char **argv, char **envp) {
	return syscall3(0x3b, (uint64_t)path, (uint64_t)argv, (uint64_t)envp);
}

int waitpid(int pid_to_wait_on, int *status, int mode) {
	return syscall3(0x72, pid_to_wait_on, (uint64_t)status, mode);
}

int pipe(int *pipe_fds, int flags) {
	return syscall2(0x125, (uint64_t)pipe_fds, flags);
}

int dup3(int old_fd, int new_fd, int flags) {
	return syscall3(0x124, old_fd, new_fd, flags);
}

void puts_to_console(char *string) {
	write(stdout, string, strlen(string));
}

void puts_to_console_with_length(char *string, size_t length) {
	write(stdout, string, length);
}

int sysinfo(struct sysinfo *info) {
	return syscall1(0x63, (uint64_t)info);
}

int sethostname(char *name, int length) {
	return syscall2(0xaa, (uint64_t)name, (uint64_t)length);
}

int msleep(long msec) {
	struct timespec ts;
	int res;

	if (msec <= 0) {
		return -1;
	}

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;

	nanosleep(&ts, &ts);

	return res;
}

void *memset(void *d, int c, size_t n);

void main(void) {
	for (;;) {
		int fork_ret = fork();
		if (fork_ret == 0) {
			chdir("/root");
			char *argv[] = {"/usr/bin/gcon", NULL};
			char *envp[] = {"USER=root",
							"HOME=/root",
							"PATH=/usr/bin:/usr/local/bin:/bin",
							"SHELL=/usr/bin/bash",
							"DISPLAY=:0",
							NULL};

			if (execve(argv[0], argv, envp) < 0)
				puts_to_console("Failed to execve :(\n");

			syscall1(0x3c, 1);
		}
		int status = 0;
		waitpid(fork_ret, &status, 0);
	}
}
