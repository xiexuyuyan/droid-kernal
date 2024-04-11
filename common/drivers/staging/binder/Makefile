obj-m += mbinder.o
mbinder-objs := binder.o binder_alloc.o binderfs.o

EXTRA_CFLAGS := -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter

KBUILD_CFLAGS += -g
KBUILD_CFLAGS += -D__WSL__

all:
	@make -C $(MAKE_BUILD_LINUX_KERNEL)/ M=$(PWD) modules

clean:
	@make -C $(MAKE_BUILD_LINUX_KERNEL)/ M=$(PWD) clean


