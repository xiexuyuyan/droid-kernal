export MAKE_BUILD_LINUX_KERNEL=/mnt/c/Users/CoderWu/Desktop/individual/Projects/linux/linux-kernel-wsl
export CMAKE_BUILD_LINUX_KERNEL=/mnt/c/Users/CoderWu/Desktop/individual/Projects/linux/linux-kernel-wsl

cd /mnt/c/Users/CoderWu/Desktop/individual/Projects/DroidKernel/common/drivers/staging/binder/

make all

mkdir --mode=0755 /dev/binderfs

insmod mbinder.ko

mount -t binder binder /dev/binderfs -o stats=global

ln -s /dev/binderfs/binder /dev/binder

cd -
