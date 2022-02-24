#ifndef MMIO_H
#define MMIO_H

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

void outmmb(void *addr, uint8_t value);
void outmmw(void *addr, uint16_t value);
void outmmdw(void *addr, uint32_t value);
void outmmqw(void *addr, uint64_t value);

uint8_t inmmb(void *addr);
uint16_t inmmw(void *addr);
uint32_t inmmdw(void *addr);
uint64_t inmmqw(void *addr);

#endif
