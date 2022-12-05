obj-m += mbinder.o
mbinder-objs := binder.o binder_alloc.o binderfs.o

KBUILD_CFLAGS += -g
KBUILD_CFLAGS += -D__WSL__

all:
	@make -C ~/projects/linux/linux-kernel-wsl/ M=$(PWD) modules

clean:
	@make -C ~/projects/linux/linux-kernel-wsl/ M=$(PWD) clean


