#include <fs/devtmpfs.h>
#include <fs/vfs.h>
#include <klibc/random.h>
#include <sys/timer.h>

// Mersenne Twister
// Based on mt19937ar.c found here:
// http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/MT2002/CODES/mt19937ar.c

#define W 64
#define N 312
#define M 156
#define R 31

#define A 0xB5026F5AA96619E9ULL
#define U 29
#define D 0x5555555555555555ULL
#define S 17
#define B 0x71D67FFFEDA60000ULL
#define T 37
#define C 0xFFF7EEE000000000ULL
#define L 43
#define F 6364136223846793005

#define MASK_LOW ((1ULL << R) - 1)
#define MASK_UPP (~MASK_LOW)

static uint64_t state[N] = {0};
static uint32_t index = N + 1;

// :god:
static uint64_t _random_get_seed(void) {
	int *k = kmalloc(sizeof(int));
	int dk = *k;
	kfree(k);
	return (timer_count() ^ dk);
}

uint64_t (*random_get_seed)(void) = _random_get_seed;

static ssize_t randdev_read(struct resource *this,
							struct f_description *description, void *buf,
							off_t offset, size_t count) {
	(void)this;
	(void)description;
	(void)offset;

	uint64_t *buffer = buf;

	for (size_t i = 0; i < count / sizeof(uint64_t); i++) {
		buffer[i] = random();
	}

	return count;
}

static ssize_t randdev_write(struct resource *this,
							 struct f_description *description, const void *buf,
							 off_t offset, size_t count) {
	(void)this;
	(void)description;
	(void)buf;
	(void)offset;
	return count;
}

static void twist(void) {
	for (uint32_t i = 0; i < N; ++i) {
		uint64_t x = (state[i] & MASK_UPP) + (state[(i + 1) % N] & MASK_LOW);
		uint64_t xa = x >> 1;
		if (x & 1) {
			xa ^= A;
		}
		state[i] = state[(i + M) % N] ^ xa;
	}
	index = 0;
}

void randdev_init(void) {
	struct resource *randdev = resource_create(sizeof(struct resource));
	randdev->read = randdev_read;
	randdev->write = randdev_write;
	randdev->stat.st_size = 0;
	randdev->stat.st_blocks = 0;
	randdev->stat.st_blksize = 4096;
	randdev->stat.st_rdev = resource_create_dev_id();
	randdev->stat.st_mode = 0666 | S_IFCHR;
	devtmpfs_add_device(randdev, "urandom");
	vfs_symlink(vfs_root, "/dev/urandom", "/dev/random");
}

uint64_t random(void) {
	if (index >= N) {
		if (index > N) {
			random_set_seed((*random_get_seed)());
		}
		twist();
	}

	uint64_t y = state[index];
	y ^= (y >> U) & D;
	y ^= (y << S) & B;
	y ^= (y << T) & C;
	y ^= y >> L;

	++index;
	return y;
}

void random_set_seed(uint64_t seed) {
	state[0] = seed;
	index = N;
	for (uint32_t i = 1; i < N; ++i) {
		state[i] = F * (state[i - 1] ^ (state[i - 1] >> (W - 2))) + i;
	}
}
