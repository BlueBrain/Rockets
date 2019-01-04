#!/bin/bash
set -e

export PKG_CONFIG_PATH=$(brew --prefix openssl)/lib/pkgconfig

mkdir $BUILD_TYPE
cd $BUILD_TYPE

cmake -GNinja \
    -DCMAKE_INSTALL_PREFIX=$PWD/install \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    ..

ninja all && \
    ninja $PROJECT_NAME-tests && \
    ninja install
