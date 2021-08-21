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

#include "clock.h"
#include "../cpu/ports.h"

static int is_updating(void) {
	port_byte_out(0x70, 0x0A);
	return port_byte_in(0x71) & 0x80;
}

static unsigned char read(int reg) {
	while (is_updating())
		;
	port_byte_out(0x70, reg);

	return port_byte_in(0x71);
}

static int bcdtobin(int val) {
	return (val & 0xF) + (val >> 4) * 10;
}

static time rtc_get_time(void) {
	time ret_time;
	ret_time.hour = read(0x4);
	ret_time.minute = read(0x2);
	ret_time.second = read(0);
	return ret_time;
}

static datetime_t rtc_get_date_time(void) {
	datetime_t date_time;

	date_time.day = read(0x7);
	date_time.month = read(0x8);
	date_time.year = read(0x9);

	date_time.time = rtc_get_time();

	int registerB = read(0x0B);

	// BCD conversion

	if (~registerB & registerB_DataMode) {
		date_time.time.second = bcdtobin(date_time.time.second);
		date_time.time.minute = bcdtobin(date_time.time.minute);
		date_time.time.hour = bcdtobin(date_time.time.hour);
		date_time.day = bcdtobin(date_time.day);
		date_time.month = bcdtobin(date_time.month);
		date_time.year = bcdtobin(date_time.year);
	}

	// Convert 12 hour clock to 24 hour clock if necessary

	if (!(registerB & 0x02) && (date_time.time.hour & 0x80)) {
		date_time.time.hour = ((date_time.time.hour & 0x7F) + 12) % 24;
	}

	// Calculate the full (4-digit) year

	date_time.year += 2000;
	return date_time;
}

uint64_t get_unix_timestamp(void) {
	datetime_t datetime = rtc_get_date_time();
	uint32_t y, m, d;
	y = datetime.year;
	m = datetime.month;
	d = datetime.day;
	uint64_t t;
	if (m <= 2) {
		m += 12;
		y -= 1;
	}
	// Convert years to days
	t = (365 * y) + (y / 4) - (y / 100) + (y / 400);
	// Convert months to days
	t += (30 * m) + (3 * (m + 1) / 5) + d;
	// Unix time starts on January 1st, 1970
	t -= 719561;
	// Convert days to seconds
	t *= 86400;
	// Add hours, minutes and seconds
	t += (3600 * datetime.time.hour) + (60 * datetime.time.minute) +
		 datetime.time.second;

	return t;
}
