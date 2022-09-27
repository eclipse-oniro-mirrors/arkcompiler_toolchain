# 获取源码及安装依赖工具
## 推荐操作系统Ubuntu18及以上
## 安装repo工具

安装码云repo工具，由于权限可切换到root用户下安装，安装后再切换个人用户目录操作

```
curl https://gitee.com/oschina/repo/raw/fork_flow/repo-py3 > /usr/local/bin/repo
chmod a+x /usr/local/bin/repo
pip3 install -i https://pypi.tuna.tsinghua.edu.cn/simple requests
```

## 下载源码
```
repo init -u https://gitee.com/lordwithcc/manifest.git -b Ark_Standalone_Build 
repo sync -c
repo forall -c 'git lfs pull'
```

## 编译环境准备

安装依赖工具
```
sudo apt-get update && sudo apt-get install binutils git git-lfs gnupg flex bison gperf build-essential zip curl zlib1g-dev gcc-multilib g++-multilib libc6-dev-i386 lib32ncurses5-dev x11proto-core-dev libx11-dev genext2fs liblz4-tool libssl-dev libtinfo5 lib32z1-dev ccache libgl1-mesa-dev libxml2-utils xsltproc unzip m4 bc gnutls-bin python3.8 python3-pip ruby default-jdk u-boot-tools mtools mtd-utils scons gcc-arm-none-eabi gcc-arm-linux-gnueabi
```

安装编译器及二进制工具
```
cd arkcompiler
./toolchain/build/compile_script/gen.sh ark
./toolchain/build/prebuilts_download/prebuilts_download.sh
```
