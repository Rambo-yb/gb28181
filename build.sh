#!/bin/bash

CUR_DIR=`pwd`

if [ "$1" == "gk7205v200" ]; then
    export PATH="/home/smb/GK7205V200/Software/GKIPCLinuxV100R001C00SPC020/tools/toolchains/arm-gcc6.3-linux-uclibceabi/bin/:$PATH"
    HOST=arm-gcc6.3-linux-uclibceabi
elif [ "$1" == "rv1126" ]; then
    export PATH="/home/smb/RV1126_RV1109_LINUX_SDK_V2.2.5.1_20230530/buildroot/output/rockchip_rv1126_rv1109/host/bin/:$PATH"
    HOST=arm-linux-gnueabihf
    SIP_LIB_SUFFIX=arm-unknown-linux-gnueabihf
elif [ "$1" == "x86_64" ]; then
    HOST=
    SIP_LIB_SUFFIX=x86_64-unknown-linux-gnu
else
    echo "$1 not support, only support gk7205v200, rv1126, x86_64"
    exit 0
fi

INSTALL_DIR=$CUR_DIR/_install

build_pjsip()
{
    SOURCE_DIR=pjproject-2.14.1
    SOURCE_INSTALL_DIR=$INSTALL_DIR/pjsip

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvzf $SOURCE_DIR.tar.gz
    cd $SOURCE_DIR
    ./configure --prefix=$SOURCE_INSTALL_DIR --host=$HOST --disable-ssl --disable-libyuv --disable-libwebrtc 
    make && make install

    cd $SOURCE_INSTALL_DIR/lib
    for file in lib*-$SIP_LIB_SUFFIX.a; do
        base=$(basename "$file" "-$SIP_LIB_SUFFIX.a")
        mv "$file" "${base}.a"
    done

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

build_libuuid()
{
    SOURCE_DIR=libuuid-1.0.3
    SOURCE_INSTALL_DIR=$INSTALL_DIR/libuuid

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvzf $SOURCE_DIR.tar.gz
    cd $SOURCE_DIR
    ./configure --prefix=$SOURCE_INSTALL_DIR --host=$HOST --enable-shared=no --enable-static=yes
    make && make install
    
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

build_alsa()
{
    SOURCE_DIR=alsa-lib-1.2.9
    SOURCE_INSTALL_DIR=$INSTALL_DIR/alsa

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvf $SOURCE_DIR.tar.bz2
    cd $SOURCE_DIR
    ./configure --prefix=$SOURCE_INSTALL_DIR --host=$HOST --enable-shared=yes --enable-static=no
    make && make install
        
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

build_mxml()
{
    SOURCE_DIR=mxml-4.0.3
    SOURCE_INSTALL_DIR=$INSTALL_DIR/mxml

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    unzip $SOURCE_DIR.zip
    cd $SOURCE_DIR
    ./configure --prefix=$SOURCE_INSTALL_DIR --host=$HOST --disable-shared
    make && make install
        
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

build_main()
{
    cd $CUR_DIR
    if [ -d $CUR_DIR/build ]; then
        rm -r $CUR_DIR/build
    fi 
    if [ -d $INSTALL_DIR/gb28181 ]; then
        rm -r $INSTALL_DIR/gb28181
    fi 

    mkdir build
    cd $CUR_DIR/build
    cmake .. -DCMAKE_C_COMPILER=${HOST}-gcc -DCMAKE_CXX_COMPILER=${HOST}-g++ -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}/gb28181
    make && make install
}

[ "$2" == "pjsip" ] && build_pjsip rebuild && exit
[ "$2" == "libuuid" ] && build_libuuid rebuild && exit
[ "$2" == "alsa" ] && build_alsa rebuild && exit
[ "$2" == "mxml" ] && build_mxml rebuild && exit
[ "$2" == "main" ] && build_main && exit

if [ "$2" == "" ]; then
    build_pjsip
    build_libuuid
    build_alsa
    build_mxml
    
    build_main
fi
