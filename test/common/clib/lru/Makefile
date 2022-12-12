obj-m += LruExample.o
LruExample-y := LruMain.o LruShrinker.o

KBUILD_CFLAGS += -g
KBUILD_CFLAGS += -D__WSL__

# https://github.com/torvalds/linux/blob/master/Documentation/kbuild/modules.rst
#  make -C <path_to_kernel_src> M=$PWD
all:
	@make -C ~/projects/linux/linux-kernel-wsl/ M=$(PWD) modules

clean:
	@make -C ~/projects/linux/linux-kernel-wsl/ M=$(PWD) clean

.PHONY:
	all clean

test:
	make clean
	make all
	sudo dmesg -C
	sudo insmod LruExample.ko
	sudo rmmod LruExample.ko
	sudo dmesg -c

