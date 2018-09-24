#!/bin/bash
set -e

export CPATH=$(brew --prefix openssl)/include

mkdir $BUILD_TYPE
cd $BUILD_TYPE

cmake -GNinja \
    -DCLONE_SUBPROJECTS=ON \
    -DCMAKE_INSTALL_PREFIX=$PWD/install \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    ..

ninja all && \
    ninja $PROJECT_NAME-tests && \
    ninja install
