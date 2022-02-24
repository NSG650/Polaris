#ifndef PORTS_H
#define PORTS_H

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

#include <stddef.h>
#include <stdint.h>

void outportb(uint16_t port, uint8_t val);
uint8_t inportb(uint16_t port);
void outportw(uint16_t port, uint16_t val);
uint16_t inportw(uint16_t port);
void outportdw(uint16_t port, uint32_t val);
uint32_t inportdw(uint16_t port);

#endif
