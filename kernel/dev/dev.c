/*
 * Copyright 2021, 2022 NSG650
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

#include "dev.h"
#include "../fs/devtmpfs.h"
#include "../klibc/lock.h"
#include <stddef.h>
#include <stdint.h>

static dev_t device_id_counter = 1;

dev_t dev_new_id(void) {
	static lock_t lock = 0;
	LOCK(lock);
	dev_t new_id = device_id_counter++;
	UNLOCK(lock);
	return new_id;
}

bool dev_add_new(struct resource *device, const char *dev_name) {
	device->st.st_rdev = dev_new_id();
	devtmpfs_add_device(device, dev_name);
	return true;
}
