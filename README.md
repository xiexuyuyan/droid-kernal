## 编写指南

### 内核模块

1. Linux内核模块的Makefile编写基础规则详细的描述
    - [官方文档](https://github.com/torvalds/linux/blob/master/Documentation/kbuild/modules.rst)
    - ```make -C <path_to_kernel_src> M=$PWD [target]```

## 编译指南

### 典型步骤

1. cmake 指定编译环境类型
    - ```CMAKE_BUILD_TYPE```
    - ```Ninja``` 或者 ```Clion```

2. cmake 指定驱动内核
    - ```CMAKE_BUILD_LINUX_KERNEL```
    - ```/.../linux-kernel-wsl```
    - ```export CMAKE_BUILD_LINUX_KERNEL=/home/yuyan/projects/linux/linux-kernel-wsl```

3. make 指定驱动内核
    - ```MAKE_BUILD_LINUX_KERNEL=/.../linux-kernel-wsl make all```
    - ```export MAKE_BUILD_LINUX_KERNEL=/.../linux-kernel-wsl```
    - ```export MAKE_BUILD_LINUX_KERNEL=/home/yuyan/projects/linux/linux-kernel-wsl```

## 使用的 wsl2 内核

### 微软远程仓库链接

1. 代码仓库 [WSL2-Linux-Kernel](https://github.com/microsoft/WSL2-Linux-Kernel)

2. 代码Tags [Tags](https://github.com/microsoft/WSL2-Linux-Kernel/tags)

3. 当前使用的Tag [linux-msft-wsl-5.15.146.1](https://github.com/microsoft/WSL2-Linux-Kernel/releases/tag/linux-msft-wsl-5.15.146.1)

### 本地使用 Git 管理并关联远程

1. 初始化
    - ```git init .```

2. 设置远程
    - ```git remote add origin https://github.com/xiexuyuyan/linux-kernel-wsl.git```

3. 拉取git 记录
    - ```git fetch```

4. 切换到指定 tag
    - ```git branch droid-kernel linux-msft-wsl-5.15.146.1```

5. 从此 tag 创建分支
    - ```git checkout droid-kernel```

6. 将本地分支与远程分支绑定
    - ```git remote remove github```
    - ```git remote add github git@github.com:xiexuyuyan/linux-kernel-wsl.git```
    - ```git fetch --all```
    - ```git branch --set-upstream-to remotes/github/droid-kernel droid-kernel```

### 编译指南

1. 设置 wsl 镜像的环境变量
    - ```export PRODUCT_OUT_IMAGE=/mnt/c/Users/CoderWu/Desktop/individual/Document```
    - ```export PRODUCT_OUT_IMAGE=/mnt/c/Users/23517/Documents```
