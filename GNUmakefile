CONFIG_ARCH ?= x86_64
CONFIG_TARGET ?= pc

.PHONY: all
all:
	rm -f polaris.iso
	$(MAKE) polaris.iso

polaris.iso: jinx
	rm -f builds/kernel.* builds/drivers.* builds/init.*
	./jinx build-all
	./build-support/makeiso.sh

jinx:
	curl -Lo jinx https://raw.githubusercontent.com/mintsuki/jinx/28efd9f86ffbaeccb0f4552a4f1ad85f42fccad0/jinx
	chmod +x jinx

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
		-not -ipath "*./kernel/arch/x86_64-pc/include/debug/zydis/*" \
		-not -ipath "*./kernel/arch/x86_64-pc/include/debug/Zycore/*" \
		-not -ipath "*./sources*" -not -ipath "*./host-pkgs*" \
		-not -ipath "*./pkgs*" -not -ipath "*./builds*" -not -ipath "*./host-builds*" \
		-not -ipath "*./kernel/fb/terminal*" -not -ipath "*./.jinx-cache*")
