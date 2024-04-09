obj-m += logger.o

KBUILD_CFLAGS += -g
KBUILD_CFLAGS += -D__WSL__

all:
	@make -C $(MAKE_BUILD_LINUX_KERNEL)/ M=$(PWD) modules

clean:
	@make -C $(MAKE_BUILD_LINUX_KERNEL)/ M=$(PWD) clean


