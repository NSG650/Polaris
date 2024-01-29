<div align="center">

[![GitHub issues](https://img.shields.io/github/issues/nsg650/polaris?label=Issues&style=flat-square)](https://github.com/NSG650/Polaris/issues)
[![GitHub pull requests](https://img.shields.io/github/issues-pr/nsg650/polaris?label=Pull%20Requests&style=flat-square)](https://github.com/NSG650/Polaris/pulls)
[![GitHub](https://img.shields.io/github/license/nsg650/polaris?label=License&style=flat-square)](https://github.com/NSG650/Polaris/blob/master/LICENSE)
[![GitHub commit activity (branch)](https://img.shields.io/github/commit-activity/m/nsg650/polaris/master?label=Commit%20Activity&style=flat-square)](https://github.com/NSG650/Polaris/graphs/commit-activity)
[![GitHub contributors](https://img.shields.io/github/contributors/nsg650/polaris?style=flat-square&label=Contributors)](https://github.com/NSG650/Polaris/graphs/contributors)
[![GitHub Repo stars](https://img.shields.io/github/stars/nsg650/polaris?label=Stargazers&style=flat-square)](https://github.com/NSG650/Polaris/stargazers)

</div>

# Polaris

Polaris is another UNIX-like kernel written in C, which uses [Limine](https://github.com/limine-bootloader/limine) as its default bootloader and boot protocol.
Its goal is to be simple to build and understand, while also acting as a learning experience.

> [!WARNING]
> Polaris is under active development and is not suitable for real usage.

# Demo Video

Here's a demo video of Polaris running DOOM and glx-gears at the same time, while also replying to network pings.

https://user-images.githubusercontent.com/51860844/221930175-4f7fedad-e020-470f-96cf-06a506d6f4a8.mp4

## Building

Building requires a Linux environment, Windows users may use WSL2 without issues.

1. Clone the repository **with submodules**.
2. Install **make** and **xorriso**.
3. Run make on the repository's root.
4. Wait for the build to finish.

## Running

It is recommended to use QEMU for ease of use. An example command to run Polaris would be as follows:

```
qemu-system-x86_64 -M q35 -m 512M -cdrom [ISO path] -serial stdio -boot d -smp [core count]
```

Here are some additional options:

- `-cpu qemu64,+la57`: Tests level 5 paging support.
- `-accel kvm -cpu host`: Enables KVM (requires host support).

## Contributing

We would appreciate issues and pull requests, any help is appreciated!
A detailed contribution guide will be added later into development.

# License
Polaris is licensed under the **Apache License 2.0** which you can read [here](LICENSE).

# Credits
- [Lyre](https://github.com/lyre-os/lyre): Heavy inspiration for VFS and some other bits of code.
```
Copyright 2022 mintsuki and contributors.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
- [Musl](https://musl.libc.org): Various libc functions used in the codebase.
```
Copyright © 2005-2020 Rich Felker, et al.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
