CONFIG_ARCH ?= x86_64
CONFIG_TARGET ?= pc

.PHONY: all
all:
	rm -f polaris.iso
	$(MAKE) polaris.iso

polaris.iso: jinx
	./build-support/makeiso.sh

jinx:
	curl -Lo jinx https://codeberg.org/mintsuki/jinx/raw/commit/4a4316aec6c258a685ebd952c43a52a9c8fe8d5a/jinx
	chmod +x jinx

.PHONY: userspace-full
userspace-full:
	rm -f pkgs/kernel* pkgs/drivers* pkgs/init*
	rm -rf sources/kernel* sources/drivers* sources/init*
	rm -rf builds/kernel builds/drivers builds/init
	./jinx build-all
	./jinx host-build limine

.PHONY: userspace-base
userspace-base:
	rm -f pkgs/kernel* pkgs/drivers* pkgs/init*
	rm -rf sources/kernel* sources/drivers* sources/init*
	rm -rf builds/kernel builds/drivers builds/init
	./jinx build-if-needed base-files kernel drivers init bash coreutils lua nano ncurses readline tzdata xz zlib zstd gcon
	./jinx host-build limine
	
.PHONY: kernel-clean
kernel-clean:
	rm -rf builds/kernel* pkgs/kernel*

.PHONY: init-clean
init-clean:
	rm -rf builds/init* pkgs/init*

.PHONY: drivers-clean
drivers-clean:
	rm -rf builds/drivers* pkgs/drivers*

.PHONY: base-files-clean
base-files-clean:
	rm -rf builds/base-files* pkgs/base-files*

.PHONY: clean
clean: kernel-clean init-clean drivers-clean base-files-clean
	rm -rf iso_root sysroot polaris.iso ramdisk.tar

.PHONY: distclean
distclean: jinx
	./jinx clean
	rm -rf iso_root sysroot polaris.iso ramdisk.tar jinx
	chmod -R 777 .jinx-cache
	rm -rf .jinx-cache

.PHONY: format
format:
	clang-format -i $(shell find . \( -iname *.h -o -iname *.c \) -not -ipath "*./limine*" \
		-not -iname "limine.h" -not -ipath "*./kernel/arch/x86_64-pc/fw/lai/*" \
		-not -ipath "*./sources*" -not -ipath "*./host-pkgs*" \
		-not -ipath "*./pkgs*" -not -ipath "*./builds*" -not -ipath "*./host-builds*" \
		-not -ipath "*./kernel/fb/terminal*" -not -ipath "*./.jinx-cache*" \
		-not -ipath "*./sysroot*" -not -iname "printf.c" -not -iname "printf.h")
