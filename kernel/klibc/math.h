#ifndef MATH_H
#define MATH_H

#define DIV_ROUNDUP(A, B)        \
	({                           \
		typeof(A) _a_ = A;       \
		typeof(B) _b_ = B;       \
		(_a_ + (_b_ - 1)) / _b_; \
	})

#define ALIGN_UP(A, B)               \
	({                               \
		typeof(A) _a_ = A;           \
		typeof(B) _b_ = B;           \
		DIV_ROUNDUP(_a_, _b_) * _b_; \
	})

#define ALIGN_DOWN(A, B)   \
	({                     \
		typeof(A) _a_ = A; \
		typeof(B) _b_ = B; \
		(_a_ / _b_) * _b_; \
	})

#endif
