#!/bin/bash

CUR_DIR=`pwd`

if [ "$1" == "gk7205v200" ]; then
    export PATH="/home/smb/GK7205V200/Software/GKIPCLinuxV100R001C00SPC020/tools/toolchains/arm-gcc6.3-linux-uclibceabi/bin/:$PATH"
elif [ "$1" == "rv1126" ]; then
    export PATH="/home/smb/RV1126_RV1109_LINUX_SDK_V2.2.5.1_20230530/prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/:$PATH"
    HOST=arm-linux-gnueabihf
    SIP_LIB_SUFFIX=arm-unknown-linux-gnueabihf
elif [ "$1" == "x86_64" ]; then
    HOST=
    SIP_LIB_SUFFIX=x86_64-unknown-linux-gnu
else
    echo "$1 not support, only support gk7205v200, rv1126, x86_64"
    exit 0
fi

CHIP_TYPE=$1
INSTALL_DIR=$CUR_DIR/_install

build_jthread()
{
    SOURCE_DIR=jthread-1.3.3
    SOURCE_INSTALL_DIR=$INSTALL_DIR/jthread

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvzf $SOURCE_DIR.tar.gz
    cd $SOURCE_DIR
    mkdir -p build && cd build
    cmake .. -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_INSTALL_PREFIX=$SOURCE_INSTALL_DIR -DCMAKE_C_COMPILER=$HOST-gcc -DCMAKE_CXX_COMPILER=$HOST-g++ \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
        -DJTHREAD_COMPILE_STATIC_ONLY=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    make && make install
    
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

build_jrtplib()
{
    SOURCE_DIR=jrtplib-3.11.2
    SOURCE_INSTALL_DIR=$INSTALL_DIR/jrtplib

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvzf $SOURCE_DIR.tar.gz
    cd $SOURCE_DIR
    mkdir -p build && cd build
    cmake .. -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_INSTALL_PREFIX=$SOURCE_INSTALL_DIR -DCMAKE_C_COMPILER=$HOST-gcc -DCMAKE_CXX_COMPILER=$HOST-g++ \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -DJRTPLIB_USE_BIGENDIAN=OFF\
        -DJTHREAD_FOUND=1 -DJTHREAD_INCLUDE_DIRS="$INSTALL_DIR/jthread/include" -DJTHREAD_LIBRARIES="-L$INSTALL_DIR/jthread/lib -ljthread -lpthread"
    make && make install
    
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR

    rm -r $SOURCE_INSTALL_DIR/lib/libjrtp.so*
}

build_openssl()
{
    SOURCE_DIR=openssl-1.1.1h
    SOURCE_INSTALL_DIR=$INSTALL_DIR/openssl

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvzf $SOURCE_DIR.tar.gz
    cd $SOURCE_DIR
    ./Configure no-asm no-shared no-async linux-generic32 --prefix=$SOURCE_INSTALL_DIR --cross-compile-prefix=$HOST-
    make && make install
    
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

build_live555()
{
    SOURCE_DIR=live
    SOURCE_INSTALL_DIR=$INSTALL_DIR/live

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvzf ${SOURCE_DIR}555-latest.tar.gz
    cd $SOURCE_DIR
    cp $CUR_DIR/config.$CHIP_TYPE ./ -raf
    export PREFIX_DIR="$INSTALL_DIR"
    ./genMakefiles $CHIP_TYPE
    make && make install
    
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

build_osip2()
{
    SOURCE_DIR=libosip2-5.3.0
    SOURCE_INSTALL_DIR=$INSTALL_DIR/osip2

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

build_exosip()
{
    SOURCE_DIR=exosip-5.3.0
    SOURCE_INSTALL_DIR=$INSTALL_DIR/exosip

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvzf $SOURCE_DIR.tar.gz
    cd $SOURCE_DIR
    ./autogen.sh
    ./configure --prefix=$SOURCE_INSTALL_DIR --host=$HOST --enable-shared=no --enable-static=yes --disable-openssl \
        OSIP_CFLAGS="-I$INSTALL_DIR/osip2/include" OSIP_LIBS="-L$INSTALL_DIR/osip2/lib -losip2 -losipparser2"
    make && make install
    
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

build_x264()
{
    SOURCE_DIR=libx264-git
    SOURCE_INSTALL_DIR=$INSTALL_DIR/x264

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvf $SOURCE_DIR.tar.xz
    cd $SOURCE_DIR
    ./configure --prefix=$SOURCE_INSTALL_DIR --cross-prefix=$HOST- --host=$HOST --disable-asm --disable-opencl --enable-static 
    make && make install
    
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

build_ffmpeg()
{
    SOURCE_DIR=ffmpeg-7.0
    SOURCE_INSTALL_DIR=$INSTALL_DIR/ffmpeg

    cd $CUR_DIR/third_lib
    [ -d $SOURCE_INSTALL_DIR ] && [ "$1" != "rebuild" ] && exit
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
    [ -d $SOURCE_INSTALL_DIR ] && rm -r $SOURCE_INSTALL_DIR

    tar -xvzf $SOURCE_DIR.tar.gz
    cd $SOURCE_DIR
    export PKG_CONFIG_PATH="$INSTALL_DIR/x264/lib/pkgconfig:$PKG_CONFIG_PATH"
    ./configure --cross-prefix=$HOST- --enable-cross-compile --target-os=linux --cc=$HOST-gcc --arch=arm --prefix=$SOURCE_INSTALL_DIR \
        --enable-static --enable-gpl --enable-nonfree --enable-swscale --enable-ffmpeg --disable-armv5te --disable-yasm --enable-libx264 --enable-protocol=rtmp \
        --extra-cflags=-I$INSTALL_DIR/x264/include --extra-ldflags=-L$INSTALL_DIR/x264/lib --extra-libs="-lpthread" --pkg-config="pkg-config --static"
    make -j4 && make install
    
    cd $CUR_DIR/third_lib
    [ -d $SOURCE_DIR ] && rm -r $SOURCE_DIR
}

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
    ./configure --prefix=$SOURCE_INSTALL_DIR --host=$HOST --disable-ssl --disable-libyuv --disable-libwebrtc --disable-v4l2 --with-ffmpeg=$INSTALL_DIR/ffmpeg \
        CFLAGS="-DPJMEDIA_HAS_VIDEO=1" \
        LDFLAGS="-L$INSTALL_DIR/x264/lib -L$INSTALL_DIR/ffmpeg/lib" \
        CPPFLAGS="-I$INSTALL_DIR/x264/include -I$INSTALL_DIR/ffmpeg/include"
    make -j4 && make install

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
    cmake .. -DCMAKE_C_COMPILER=${HOST}-gcc -DCMAKE_CXX_COMPILER=${HOST}-g++ -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}/gb28181 -DCHIP_TYPE=$CHIP_TYPE
    make && make install

    cp libgb28181.so /home/smb/work/ -raf
    cp gb28181_test /home/smb/work/ -raf
}

[ "$2" == "jthread" ] && build_jthread rebuild && exit
[ "$2" == "jrtplib" ] && build_jrtplib rebuild && exit
[ "$2" == "openssl" ] && build_openssl rebuild && exit
[ "$2" == "live555" ] && build_live555 rebuild && exit
[ "$2" == "osip2" ] && build_osip2 rebuild && exit
[ "$2" == "exosip" ] && build_exosip rebuild && exit
[ "$2" == "x264" ] && build_x264 rebuild && exit
[ "$2" == "ffmpeg" ] && build_ffmpeg rebuild && exit
[ "$2" == "pjsip" ] && build_pjsip rebuild && exit
[ "$2" == "libuuid" ] && build_libuuid rebuild && exit
[ "$2" == "alsa" ] && build_alsa rebuild && exit
[ "$2" == "mxml" ] && build_mxml rebuild && exit
[ "$2" == "main" ] && build_main && exit

if [ "$2" == "" ]; then
    # build_x264
    # build_ffmpeg
    # build_pjsip
    # build_libuuid
    # build_alsa
    
    # build_openssl
    # build_live555

    build_jthread
    build_jrtplib
    build_osip2
    build_exosip
    build_mxml
    
    build_main
fi
