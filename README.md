# osdev64
osdev thing but now 64 bit
# How to build?
Get yourself a copy of [limine](https://github.com/limine-bootloader/limine/tree/v2.0-branch-binary) and [stivale](https://github.com/stivale/stivale) then

```sh
make
#builds the project

make image # or ./image.sh
#makes a hard drive image

make clean
#cleans the project
```
# How to run?
The kernel is 64 bit so run it with
```sh
qemu-system-x86_64 -hda d.img
```
# What is implemented so far?
- [x] Long mode
- [x] Graphics mode
- [x] Interrupts
- [x] Timer   
