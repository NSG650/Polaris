#ifndef TYPES_H
#define TYPES_H

/*
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include <stdint.h>

#define NAME_MAX 256

typedef int64_t ssize_t;
typedef int64_t off_t;

typedef uint64_t dev_t;
typedef uint64_t ino_t;
typedef int32_t mode_t;
typedef int32_t nlink_t;
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

#define O_ACCMODE 0x0007
#define O_EXEC	  1
#define O_RDONLY  2
#define O_RDWR	  3
#define O_SEARCH  4
#define O_WRONLY  5

#define O_APPEND	0x0008
#define O_CREAT		0x0010
#define O_DIRECTORY 0x0020
#define O_EXCL		0x0040
#define O_NOCTTY	0x0080
#define O_NOFOLLOW	0x0100
#define O_TRUNC		0x0200
#define O_NONBLOCK	0x0400
#define O_DSYNC		0x0800
#define O_RSYNC		0x1000
#define O_SYNC		0x2000
#define O_CLOEXEC	0x4000

#define S_IFMT	 0x0F000
#define S_IFBLK	 0x06000
#define S_IFCHR	 0x02000
#define S_IFIFO	 0x01000
#define S_IFREG	 0x08000
#define S_IFDIR	 0x04000
#define S_IFLNK	 0x0A000
#define S_IFSOCK 0x0C000

#define S_ISBLK(m)	(((m)&S_IFMT) == S_IFBLK)
#define S_ISCHR(m)	(((m)&S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m)&S_IFMT) == S_IFIFO)
#define S_ISREG(m)	(((m)&S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m)&S_IFMT) == S_IFDIR)
#define S_ISLNK(m)	(((m)&S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m)&S_IFMT) == S_IFSOCK)

struct stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off_t st_size;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
};

#endif
