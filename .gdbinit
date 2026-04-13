set substitute-path /base_dir/sources/kernel kernel
file builds/kernel/arch/x86_64-pc/polaris.elf -o 0xffffffff80000000
target remote :1234
