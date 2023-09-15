#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H

/*
 * Copyright 2021 - 2023 NSG650
 * Copyright 2021 - 2023 Neptune
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

#define CPUID_INVARIANT_TSC (1 << 8)
#define CPUID_TSC_DEADLINE (1 << 24)
#define CPUID_SMEP (1 << 7)
#define CPUID_SMAP (1 << 20)
#define CPUID_UMIP (1 << 2)
#define CPUID_X2APIC (1 << 21)
#define CPUID_GBPAGE (1 << 26)

#endif
