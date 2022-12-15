#include "types.h"

int stdin = 0;
int stdout = 1;

struct fbdev_info {
	size_t pitch, bpp;
	uint16_t width, height;
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

void puts(char *string) {
	syscall1(0x67, (uint64_t)string);
}

ssize_t read(int fd, void *data, size_t count) {
	return syscall3(0x0, fd, (uint64_t)data, count);
}

ssize_t write(int fd, const void *data, size_t count) {
	return syscall3(0x1, fd, (uint64_t)data, count);
}

int fstatat(int dirfd, const char *pathname, struct stat *buf,
			int flags) {
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

int execve(char *path, char **argv, char **envp) {
	return syscall3(0x3b, (uint64_t)path, (uint64_t)argv, (uint64_t)envp);
}

void puts_to_console(char *string) {
	write(stdout, string, strlen(string));
}

void *memset(void *d, int c, size_t n);

void main(void) {
	if (!fork()) {
		puts_to_console("Hello I am the forked process!\n");
		char *argv[] = {
			"/bin/hello",
			NULL
		};
		char *envp[] = {
			"USER=root",
			NULL
		};
		if(execve(argv[0], argv, envp) == -1) {
			puts_to_console("Failed to execve :((\n");
		}
		for (;;)
			;
	}
	puts_to_console("Hello from init!\n");
}
