#include "types.h"

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

void puts(char *string) {
	syscall1(0x67, (uint64_t)string);
}

ssize_t read(int fd, void *data, size_t count) {
	return syscall3(0x0, fd, (uint64_t)data, count);
}

ssize_t write(int fd, const void *data, size_t count) {
	return syscall3(0x1, fd, (uint64_t)data, count);
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

void main(void) {
	puts("Hello I am supposed to be the init\n");

	// Change directory to /fun so we can use relative paths
	chdir("/fun");

	// Hard link /fun/hi.txt to /fun/hello.txt
	linkat(AT_FDCWD, "./hi.txt", AT_FDCWD, "./hello.txt", 0);
	int fd = open("./hello.txt", O_RDONLY, 0);

	// Remove /fun/hello.txt hard link
	unlinkat(AT_FDCWD, "./hello.txt", 0);

	// Read the hard link
	char fun[128] = {0};
	puts("Reading /fun/hello.txt:\n");
	read(fd, fun, 128);

	// Close /fun/hi.txt, thus it's deleted since the hard link got removed
	close(fd);
	puts(fun);

	// dirfd = AT_FDCWD (works the same as open with this argument)
	// Open file as read and write, create if neccessary
	fd = openat(AT_FDCWD, "/file_from_user.txt", O_RDWR | O_CREAT, 0);

	// Write to the file
	char data[] = "This file was created from user\n";
	write(fd, data, sizeof(data));

	// Move file offset back to the beginning, since write moved it to the end
	lseek(fd, 0, SEEK_SET);

	// Read what we just wrote and close the file
	read(fd, fun, 128);
	close(fd);
	puts("Reading /file_from_user.txt:\n");
	puts(fun);
}
