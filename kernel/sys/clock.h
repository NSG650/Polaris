#ifndef RTC_H
#define RTC_H

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

#include <stdint.h>

#define registerB_DataMode (1 << 2)

struct time {
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
};

struct datetime {
	struct time time;
	uint8_t day;
	uint8_t month;
	uint16_t year;
};

uint64_t get_unix_timestamp(void);

#endif
