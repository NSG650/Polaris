#ifndef PANIC_H
#define PANIC_H

/*
 * Copyright 2021, 2022 NSG650
 * Copyright 2021, 2022 Sebastian
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

#include <stdbool.h>
#include <stddef.h>
#include <stdnoreturn.h>

noreturn void panic(const char *message, char *file, bool assert, size_t line);

#define PANIC(b) (panic(b, __FILE__, false, __LINE__))
#define ASSERT(b) ((b) ? (void)0 : panic(#b, __FILE__, true, __LINE__))

#endif
