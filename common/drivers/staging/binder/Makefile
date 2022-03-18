obj-m += mbinder.o
mbinder-objs := binder.o binder_alloc.o binderfs.o

KBUILD_CFLAGS += -g

all:
	@make -C /root/linux/linux-kernel-wsl/ M=$(PWD) modules

clean:
	@make -C /root/linux/linux-kernel-wsl/ M=$(PWD) clean


