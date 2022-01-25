#!/bin/bash
# Copyright 2020 Amazon.com, Inc. and its affiliates. All Rights Reserved.
# SPDX-License-Identifier: LicenseRef-.amazon.com.-AmznSL-1.0
# Licensed under the Amazon Software License (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
# http://aws.amazon.com/asl/
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.

set -euo pipefail

WITH_CAMERA_SUPPORT="false"

parse_args() {
    while [ "$#" -gt 0 ]; do
        case $1 in
        --with-camera-support)
            WITH_CAMERA_SUPPORT="true"
            ;;
        --help)
            echo "Usage: $0 [OPTION]"
            echo "  --with-camera-support  Installs dependencies required for camera support"
            exit 0
            ;;
        esac
        shift
    done
}

parse_args "$@"

if [ `dpkg --print-architecture` == "arm64" ]; then
    echo "Error: Can't install x86_64 (amd64) packages on an aarch64 (arm64) machine" >&2
    exit -1
fi

cp tools/arm64.list /etc/apt/sources.list.d/
mkdir -p /usr/local/aarch64-linux-gnu/lib/cmake/
cp tools/arm64-toolchain.cmake /usr/local/aarch64-linux-gnu/lib/cmake/

dpkg --add-architecture arm64
sed -i 's/deb http/deb [arch=amd64] http/g' /etc/apt/sources.list
apt update
apt install -y \
    libssl-dev:arm64 \
    libboost-system-dev:arm64 \
    libboost-log-dev:arm64 \
    libboost-thread-dev:arm64 \
    build-essential \
    crossbuild-essential-arm64 \
    cmake \
    unzip \
    git \
    wget \
    curl \
    zlib1g-dev:arm64 \
    libsnappy-dev:arm64

mkdir /tmp/deps && cd /tmp/deps

git clone -b 1.7.4 https://github.com/open-source-parsers/jsoncpp.git
cd jsoncpp
mkdir build && cd build
cmake \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=On \
    -DJSONCPP_WITH_TESTS=Off \
    -DJSONCPP_WITH_POST_BUILD_UNITTEST=Off \
    -DCMAKE_TOOLCHAIN_FILE=/usr/local/aarch64-linux-gnu/lib/cmake/arm64-toolchain.cmake \
    -DCMAKE_INSTALL_PREFIX=/usr/local/aarch64-linux-gnu \
    ..
make install -j`nproc`
cd ../..

wget -q https://github.com/protocolbuffers/protobuf/releases/download/v3.9.2/protobuf-all-3.9.2.tar.gz
tar -zxf protobuf-all-3.9.2.tar.gz
cd protobuf-3.9.2
mkdir build_amd64 && cd build_amd64
../configure
make install -j`nproc`
cd ..
mkdir build_arm64 && cd build_arm64
CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++ \
    ../configure --host=aarch64-linux --prefix=/usr/local/aarch64-linux-gnu
make install -j`nproc`
cd ../..
ldconfig

git clone https://github.com/hartkopp/can-isotp.git
cd can-isotp
git checkout beb4650660179963a8ed5b5cbf2085cc1b34f608
cp include/uapi/linux/can/isotp.h /usr/include/linux/can
cd ..

git clone -b v1.10.9 --recursive https://github.com/aws/aws-iot-device-sdk-cpp-v2.git
cd aws-iot-device-sdk-cpp-v2
mkdir build && cd build
cmake \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_DEPS=ON \
    -DBUILD_TESTING=OFF \
    -DCMAKE_TOOLCHAIN_FILE=/usr/local/aarch64-linux-gnu/lib/cmake/arm64-toolchain.cmake \
    -DCMAKE_INSTALL_PREFIX=/usr/local/aarch64-linux-gnu \
    ..
make install -j`nproc`
cd ../..

# AWS IoT FleetWise Edge camera support requires Fast-DDS and its dependencies:
if [ "${WITH_CAMERA_SUPPORT}" == "true" ]; then
    apt install -y \
        default-jre \
        libasio-dev \
        qemu-user-binfmt

    git clone -b 6.0.0 https://github.com/leethomason/tinyxml2.git
    cd tinyxml2
    mkdir build && cd build
    cmake \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_STATIC_LIBS=ON \
        -DBUILD_TESTS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=On \
        -DCMAKE_TOOLCHAIN_FILE=/usr/local/aarch64-linux-gnu/lib/cmake/arm64-toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/aarch64-linux-gnu \
        ..
    make install -j`nproc`
    cd ../..

    git clone -b v1.1.0 https://github.com/eProsima/foonathan_memory_vendor.git
    cd foonathan_memory_vendor
    mkdir build && cd build
    cmake \
        -DBUILD_SHARED_LIBS=OFF \
        -Dextra_cmake_args="-DCMAKE_CROSSCOMPILING_EMULATOR=qemu-aarch64" \
        -DCMAKE_TOOLCHAIN_FILE=/usr/local/aarch64-linux-gnu/lib/cmake/arm64-toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/aarch64-linux-gnu \
        ..
    make -j`nproc`
    make install
    cd ../..
    git clone -b v1.0.21 https://github.com/eProsima/Fast-CDR.git
    cd Fast-CDR
    mkdir build && cd build
    cmake \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_TOOLCHAIN_FILE=/usr/local/aarch64-linux-gnu/lib/cmake/arm64-toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/aarch64-linux-gnu \
        ..
    make -j`nproc`
    make install
    cd ../..
    git clone -b v2.3.4 https://github.com/eProsima/Fast-DDS.git
    cd Fast-DDS
    mkdir build && cd build
    cmake \
        -DBUILD_SHARED_LIBS=OFF \
        -DCOMPILE_TOOLS=OFF \
        -DCMAKE_CXX_FLAGS="-DUSE_FOONATHAN_NODE_SIZES=1" \
        -DCMAKE_TOOLCHAIN_FILE=/usr/local/aarch64-linux-gnu/lib/cmake/arm64-toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/aarch64-linux-gnu \
        ..
    make -j`nproc`
    make install
    cd ../..
    git clone -b v2.0.1 --recursive https://github.com/eProsima/Fast-DDS-Gen.git
    cd Fast-DDS-Gen
    ./gradlew assemble
    mkdir -p /usr/local/share/fastddsgen/java
    cp share/fastddsgen/java/fastddsgen.jar /usr/local/share/fastddsgen/java
    cp scripts/fastddsgen /usr/local/bin
    cd ../..
fi

rm -rf /tmp/deps
