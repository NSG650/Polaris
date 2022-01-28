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

#include "clock.h"
#include "../cpu/ports.h"

static uint8_t is_updating(void) {
	port_byte_out(0x70, 0xA);
	return port_byte_in(0x71) & 0x80;
}

static uint8_t read(uint8_t reg) {
	while (is_updating())
		;
	port_byte_out(0x70, reg);

	return port_byte_in(0x71);
}

static int bcdtobin(int val) {
	return (val & 0xF) + (val >> 4) * 10;
}

static struct time rtc_get_time(void) {
	struct time ret_time;
	ret_time.second = read(0);
	ret_time.minute = read(2);
	ret_time.hour = read(4);
	return ret_time;
}

static struct datetime rtc_get_date_time(void) {
	struct datetime date_time;

	date_time.time = rtc_get_time();

	date_time.day = read(7);
	date_time.month = read(8);
	date_time.year = read(9);

	const uint8_t registerB = read(0xB);

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
	if (!(registerB & 2) && (date_time.time.hour & 0x80)) {
		date_time.time.hour = ((date_time.time.hour & 0x7F) + 12) % 24;
	}

	// Calculate the full (4-digit) year
	date_time.year += 2000; // NOTE: Only works for the 21nd century
	return date_time;
}

uint64_t get_unix_timestamp(void) {
	const struct datetime datetime = rtc_get_date_time();
	const uint8_t d = datetime.day;
	uint8_t m = datetime.month;
	uint16_t y = datetime.year;
	if (m <= 2) {
		m += 12;
		y -= 1;
	}
	// Convert years to days
	uint64_t t = (365 * y) + (y / 4) - (y / 100) + (y / 400);
	// Convert months to days
	t += (30 * m) + (3 * (m + 1) / 5) + d;
	// UNIX time starts on January 1st, 1970
	t -= 719561;
	// Convert days to seconds
	t *= 86400;
	// Add hours, minutes and seconds
	t += (3600 * datetime.time.hour) + (60 * datetime.time.minute) +
		 datetime.time.second;

	return t;
}
