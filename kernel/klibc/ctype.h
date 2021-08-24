#ifndef CTYPE_H
#define CTYPE_H

/*
 * Copyright 2021 NSG650
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

int isxdigit(int c);
int isblank(int c);
int isdigit(int c);
int islower(int c);
int isupper(int c);
int isprint(int c);
int iscntrl(int c);
int isspace(int c);
int isgraph(int c);
int isalpha(int c);
int isalnum(int c);
int ispunct(int c);
int tolower(int c);
int toupper(int c);

#endif
