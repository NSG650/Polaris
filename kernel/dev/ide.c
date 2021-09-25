/*
 * Copyright 2021 Misha
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

#include "ide.h"
#include "../sys/pci.h"
#include "../klibc/printf.h"

void ide_init() {
	struct pci_device *ideDrive;
	for (size_t i = 0; i < 100; i++) {
		struct pci_device *dev = PCIDevicesArray[i];
		if (dev != NULL) {
			if (dev->classCode == 0x1 && dev->subclass == 0x01) {
				ideDrive = dev;
				break;
			}
		}
	}
	if (ideDrive == NULL) {
		printf("IDE: NO Ide controller found\n");
		return;
	}
	else{
		printf("Found IDE controller\n");
	}
}
