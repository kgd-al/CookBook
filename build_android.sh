#!/bin/bash

set -euo pipefail

wd=$(pwd)
dir=$wd/build/android/
qt=5.15.0
android=27

if ! command -v java > /dev/null
then
    sudo apt install openjdk-21-jdk-headless
fi

if ! command -v ant > /dev/null
then
    sudo apt install ant
fi

android_sdk=$dir/android-sdk-linux
if [ ! -d $android_sdk ]
then
    cd $dir
    target=android-sdk_r24.4.1-linux.tgz
    if [ ! -f $target ]
    then
    # https://developer.android.com/studio/index.html#command-line-tools-only
        wget http://dl.google.com/android/$target
        tar -xf $target
    fi
#     cd android-sdk-linux/tools
#     ./android update sdk --no-ui --filter platform,platform-tools --dry-mode -v
#     rm -v android-sdk*-linux.tgz
    cd $wd
fi

target=android-ndk-r26-linux.zip
android_ndk=$dir/$(basename $target -linux.zip)
if [ ! -d $android_ndk ]
then
    cd $dir
    [ ! -f $target ] && wget https://dl.google.com/android/repository/$target
    [ ! -d $android_ndk ] && unzip -q $target

    cd $wd
fi

export ANDROID_HOME=$android_sdk
export ANDROID_NDK_HOST=linux-x86_64
export ANDROID_NDK_PLATFORM=android-13
export ANDROID_NDK_ROOT=$android_ndk
export ANDROID_NDK_TOOLCHAIN_PREFIX=arm-linux-androideabi
export ANDROID_NDK_TOOLCHAIN_VERSION=4.8
export ANDROID_NDK_TOOLS_PREFIX=arm-linux-androideabi
export ANDROID_SDK_ROOT=$android_sdk
export ANDROID_API_VERSION=android-13

export JAVA_HOME=/Path/To/JavaJDK
export PATH=$PATH:$ANDROID_HOME/tools:/Path/To/ApacheANT/bin:$JAVA_HOME/bin

modules="qtcore qtgui qtwidgets qtbluetooth qtandroidextras"
venv=.venv
if [ ! -d $venv ]
then
    python -m venv $venv
    source $venv/bin/activate
    pip install -U pip
    pip install aqtinstall
    aqt install-qt linux android $qt android -m $modules --autodesktop -O $dir
fi

source $venv/bin/activate

cd $dir
[ ! -f Makefile ] && ./$qt/android/bin/qmake -spec android-clang $wd
make apk
make install
