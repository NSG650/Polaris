# osdev64
An attempt at OSDev and make a Unix-like kernel
# How to build?
Clone recursively the repository (Git `--recursive` flag)

Get yourself a copy of [Limine](https://github.com/limine-bootloader/limine/tree/v2.0-branch-binary)

Build a `x86_64-elf-gcc` compiler then

```sh
make
# Builds the project

make image
# Makes a hard drive image
```

To clean use
```sh
make clean
# Cleans the project
```
# How to run?
The kernel is 64-bit so run it with
```sh
qemu-system-x86_64 -hda d.img -m 512M
```
# What is implemented so far?
- [x] Long mode
- [x] Graphical mode
- [x] Interrupts
- [x] Timer
- [x] Paging
- [x] ACPI
- [x] HPET  
