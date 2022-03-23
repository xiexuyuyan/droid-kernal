obj-m += logger.o

KBUILD_CFLAGS += -g
KBUILD_CFLAGS += -DWSL

all:
	@make -C /root/linux/linux-kernel-wsl/ M=$(PWD) modules

clean:
	@make -C /root/linux/linux-kernel-wsl/ M=$(PWD) clean


