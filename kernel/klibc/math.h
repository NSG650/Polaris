#ifndef MATH_H
#define MATH_H

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

#define DIV_ROUNDUP(A, B)        \
	({                           \
		typeof(A) _a_ = A;       \
		typeof(B) _b_ = B;       \
		(_a_ + (_b_ - 1)) / _b_; \
	})

#define ALIGN_UP(A, B)                  \
	({                                  \
		typeof(A) _a__ = A;           \
		typeof(B) _b__ = B;           \
		DIV_ROUNDUP(_a__, _b__) * _b__; \
	})

#define ALIGN_DOWN(A, B)   \
	({                     \
		typeof(A) _a_ = A; \
		typeof(B) _b_ = B; \
		(_a_ / _b_) * _b_; \
	})

#endif
