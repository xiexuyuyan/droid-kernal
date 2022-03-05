obj-m += mbinder.o
mbinder-objs := binder.o binder_alloc.o

KBUILD_CFLAGS += -g

all:
	@make -C /root/linux/linux-5.10.16.3-wsl/ M=$(PWD) modules

clean:
	@make -C /root/linux/linux-5.10.16.3-wsl/ M=$(PWD) clean


