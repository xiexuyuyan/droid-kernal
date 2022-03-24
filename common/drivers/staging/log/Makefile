obj-m += logger.o

KBUILD_CFLAGS += -g
KBUILD_CFLAGS += -D__WSL__

all:
	@make -C /root/linux/linux-kernel-wsl/ M=$(PWD) modules

clean:
	@make -C /root/linux/linux-kernel-wsl/ M=$(PWD) clean


