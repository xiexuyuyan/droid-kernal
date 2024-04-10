## 编写指南

### 内核模块

1. Linux内核模块的Makefile编写基础规则详细的描述
    - [官方文档](https://github.com/torvalds/linux/blob/master/Documentation/kbuild/modules.rst)
    - ```make -C <path_to_kernel_src> M=$PWD [target]```

## 使用 cmake 编译项目

### 典型步骤

#### cmake 指定编译 linux 内核目录

1. 环境变量
    - ```export CMAKE_BUILD_LINUX_KERNEL=/.../linux-kernel-wsl```

2. 命令行
    - ```cmake -B build -G Ninja -D CMAKE_BUILD_LINUX_KERNEL=/.../linux-kernel-wsl```

3. Clion
    - 要对 IDE 进行设置
    > .idea/cmake.xml

    ```xml
    <?xml version="1.0" encoding="UTF-8"?>
    <project version="4">
      <component name="CMakeSharedSettings">
        <configurations>
          <configuration PROFILE_NAME="Debug-WSL" ENABLED="true" GENERATION_DIR="build" CONFIG_NAME="Debug" TOOLCHAIN_NAME="WSL" GENERATION_OPTIONS="-G Ninja">
            <ADDITIONAL_GENERATION_ENVIRONMENT>
              <envs>
                <env name="CMAKE_BUILD_LINUX_KERNEL" value="/mnt/c/Users/CoderWu/Desktop/individual/Projects/linux/linux-kernel-wsl" />
              </envs>
            </ADDITIONAL_GENERATION_ENVIRONMENT>
          </configuration>
        </configurations>
      </component>
    </project>
    ```

    - 设置好编译器后则会自动生成环境变量

    ```bash
    C:\WINDOWS\system32\wsl.exe
        --distribution Ubuntu-20.04
        --exec /bin/bash -c "
            export CMAKE_COLOR_DIAGNOSTICS=ON
            && export CLION_IDE=TRUE
            && export CMAKE_BUILD_LINUX_KERNEL=/mnt/c/Users/CoderWu/Desktop/individual/Projects/linux/linux-kernel-wsl
            && export JETBRAINS_IDE=TRUE
            && cd /mnt/c/Users/CoderWu/Desktop/individual/Projects/DroidKernel/build
            && /usr/bin/cmake
                -DCMAKE_BUILD_TYPE=Debug
                -G Ninja
                -S /mnt/c/Users/CoderWu/Desktop/individual/Projects/DroidKernel
                -B /mnt/c/Users/CoderWu/Desktop/individual/Projects/DroidKernel/build
        "
    ```

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

2. 编译内核模块设置环境变量
    - ```export MAKE_BUILD_LINUX_KERNEL=/.../linux-kernel-wsl```
