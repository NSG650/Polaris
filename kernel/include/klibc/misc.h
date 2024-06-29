#ifndef MISC_H
#define MISC_H

#define MIN(A, B)                      \
	({                                 \
		__auto_type MIN_a = A;         \
		__auto_type MIN_b = B;         \
		MIN_a < MIN_b ? MIN_a : MIN_b; \
	})

#define MAX(A, B)                      \
	({                                 \
		__auto_type MAX_a = A;         \
		__auto_type MAX_b = B;         \
		MAX_a > MAX_b ? MAX_a : MAX_b; \
	})

#define DIV_ROUNDUP(A, B)        \
	({                           \
		__auto_type _a_ = A;     \
		__auto_type _b_ = B;     \
		(_a_ + (_b_ - 1)) / _b_; \
	})

#define ALIGN_UP(A, B)                  \
	({                                  \
		__auto_type _a__ = A;           \
		__auto_type _b__ = B;           \
		DIV_ROUNDUP(_a__, _b__) * _b__; \
	})

#define ALIGN_DOWN(A, B)     \
	({                       \
		__auto_type _a_ = A; \
		__auto_type _b_ = B; \
		(_a_ / _b_) * _b_;   \
	})

#define SIZEOF_ARRAY(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

#endif
