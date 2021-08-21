#ifndef RTC_H
#define RTC_H

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

#include <stdint.h>

#define registerB_DataMode (1 << 2)

typedef struct {
	uint32_t hour;
	uint32_t second;
	uint32_t minute;
} time;

typedef struct {
	uint32_t day;
	uint32_t month;
	uint32_t year;
	uint32_t century;
	time time;
} datetime_t;

uint64_t get_unix_timestamp(void);

#endif
