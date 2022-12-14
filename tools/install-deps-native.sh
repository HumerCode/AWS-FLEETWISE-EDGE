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

apt update
apt install -y \
    libssl-dev \
    libboost-system-dev \
    libboost-log-dev \
    libboost-thread-dev \
    build-essential \
    cmake \
    unzip \
    git \
    wget \
    curl \
    zlib1g-dev \
    libcurl4-openssl-dev \
    libsnappy-dev \
    doxygen \
    graphviz \
    clang-format-10 \
    clang-tidy-10
update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-10 1000
update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-10 1000

mkdir -p deps-native && cd deps-native

if [ ! -d jsoncpp ]; then
    git clone -b 1.7.4 https://github.com/open-source-parsers/jsoncpp.git
    cd jsoncpp
    mkdir build && cd build
    cmake \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=On \
        -DJSONCPP_WITH_TESTS=Off \
        -DJSONCPP_WITH_POST_BUILD_UNITTEST=Off \
        ..
    cd ../..
fi
make install -j`nproc` -C jsoncpp/build

if [ ! -d protobuf-3.9.2 ]; then
    wget -q https://github.com/protocolbuffers/protobuf/releases/download/v3.9.2/protobuf-all-3.9.2.tar.gz
    tar -zxf protobuf-all-3.9.2.tar.gz
    cd protobuf-3.9.2
    mkdir build && cd build
    ../configure
    cd ../..
fi
make install -j`nproc` -C protobuf-3.9.2/build
ldconfig

if [ ! -d can-isotp ]; then
    git clone https://github.com/hartkopp/can-isotp.git
    cd can-isotp
    git checkout beb4650660179963a8ed5b5cbf2085cc1b34f608
    cd ..
fi
cp can-isotp/include/uapi/linux/can/isotp.h /usr/include/linux/can

if [ ! -d aws-sdk-cpp ]; then
    git clone -b 1.9.253 --recursive https://github.com/aws/aws-sdk-cpp.git
    cd aws-sdk-cpp
    mkdir build && cd build
    cmake \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_ONLY='s3-crt;iot' \
        -DAWS_CUSTOM_MEMORY_MANAGEMENT=ON \
        ..
    cd ../..
fi
make install -j`nproc` -C aws-sdk-cpp/build

if [ ! -d googletest ]; then
    git clone -b release-1.10.0 https://github.com/google/googletest.git
    cd googletest
    mkdir build && cd build
    cmake ..
    cd ../.. 
fi
make install -j`nproc` -C googletest/build

if [ ! -d benchmark ]; then
    git clone -b v1.6.1 https://github.com/google/benchmark.git
    cd benchmark
    mkdir build && cd build
    cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ..
    cd ../.. 
fi
make install -j`nproc` -C benchmark/build

sudo ldconfig

# AWS IoT FleetWise Edge camera support requires Fast-DDS and its dependencies:
if [ "${WITH_CAMERA_SUPPORT}" == "true" ]; then
    apt install -y \
        default-jre \
        libasio-dev

    if [ ! -d tinyxml2 ]; then
        git clone -b 6.0.0 https://github.com/leethomason/tinyxml2.git
        cd tinyxml2
        mkdir build && cd build
        cmake \
            -DBUILD_SHARED_LIBS=OFF \
            -DBUILD_STATIC_LIBS=ON \
            -DBUILD_TESTING=OFF \
            -DCMAKE_POSITION_INDEPENDENT_CODE=On \
            ..
        cd ../..
    fi
    make install -j`nproc` -C tinyxml2/build

    if [ ! -d foonathan_memory_vendor ]; then
        git clone -b v1.1.0 https://github.com/eProsima/foonathan_memory_vendor.git
        cd foonathan_memory_vendor
        mkdir build && cd build
        cmake \
            -DBUILD_SHARED_LIBS=OFF \
            ..
        cd ../..
    fi
    make install -j`nproc` -C foonathan_memory_vendor/build

    if [ ! -d Fast-CDR ]; then
        git clone -b v1.0.21 https://github.com/eProsima/Fast-CDR.git
        cd Fast-CDR
        mkdir build && cd build
        cmake \
            -DBUILD_SHARED_LIBS=OFF \
            ..
        cd ../..
    fi
    make install -j`nproc` -C Fast-CDR/build

    if [ ! -d Fast-DDS ]; then
        git clone -b v2.3.4 https://github.com/eProsima/Fast-DDS.git
        cd Fast-DDS
        mkdir build && cd build
        cmake \
            -DBUILD_SHARED_LIBS=OFF \
            -DCOMPILE_TOOLS=OFF \
            -DCMAKE_CXX_FLAGS="-DUSE_FOONATHAN_NODE_SIZES=1" \
            ..
        cd ../..
    fi
    make install -j`nproc` -C Fast-DDS/build

    if [ ! -d Fast-DDS-Gen ]; then
        git clone -b v2.0.1 --recursive https://github.com/eProsima/Fast-DDS-Gen.git
        cd Fast-DDS-Gen
        ./gradlew assemble
        cd ..
    fi
    mkdir -p /usr/local/share/fastddsgen/java
    cp Fast-DDS-Gen/share/fastddsgen/java/fastddsgen.jar /usr/local/share/fastddsgen/java
    cp Fast-DDS-Gen/scripts/fastddsgen /usr/local/bin
fi
