obj-m += logger.o
OUTPUT_DIR = build

KBUILD_CFLAGS += -g

all:
	@make -C /root/linux/linux-5.10.16.3-wsl/ M=$(PWD) modules

clean:
	@make -C /root/linux/linux-5.10.16.3-wsl/ M=$(PWD) clean


