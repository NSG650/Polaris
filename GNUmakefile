CONFIG_ARCH ?= x86_64
CONFIG_TARGET ?= pc

.PHONY: all
all:
	rm -f builds/kernel.* builds/drivers.* builds/init.*
	./jinx build-all
	./build-support/makeiso.sh

.PHONY: clean
clean:
	rm -rf build *.iso *.img *.tar *.tar.gz
	$(MAKE) -C kernel/arch/$(CONFIG_ARCH)-$(CONFIG_TARGET) clean

.PHONY: distclean
distclean: clean
	$(MAKE) -C kernel/arch/$(CONFIG_ARCH)-$(CONFIG_TARGET) distclean

.PHONY: format
format:
	clang-format -i $(shell find . \( -iname *.h -o -iname *.c \) -not -ipath "*./limine*" \
		-not -iname "limine.h" -not -ipath "*./kernel/arch/x86_64-pc/fw/lai/*" \
		-not -ipath "*./kernel/arch/x86_64-pc/include/debug/zydis/*" \
		-not -ipath "*./kernel/arch/x86_64-pc/include/debug/Zycore/*" \
		-not -ipath "*./sources*" -not -ipath "*./host-pkgs*" \
		-not -ipath "*./pkgs*" -not -ipath "*./builds*" -not -ipath "*./host-builds*" \
		-not -ipath "*./kernel/fb/terminal*" -not -ipath "*./.jinx-cache*")
