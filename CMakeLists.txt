# Copyright (c) 2017, EPFL/Blue Brain Project
#                     Raphael.Dumusc@epfl.ch
#
# This file is part of Rockets <https://github.com/BlueBrain/Rockets>
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License version 3.0 as published
# by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Rockets VERSION 0.1.0)
set(Rockets_VERSION_ABI 1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/common)
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/CMake/common/Common.cmake)
  message(FATAL_ERROR "CMake/common missing, run: git submodule update --init")
endif()

set(ROCKETS_DEB_DEPENDS libboost-test-dev libwebsockets-dev)

include(Common)

set(ROCKETS_DESCRIPTION "Rockets - REST and websockets C++ library")
set(ROCKETS_MAINTAINER "Blue Brain Project <bbp-open-source@googlegroups.com>")
set(ROCKETS_LICENSE LGPL)

common_find_package(Boost SYSTEM COMPONENTS unit_test_framework)
common_find_package(Libwebsockets SYSTEM REQUIRED)
common_find_package(Threads REQUIRED)
common_find_package_post()

set(LCOV_EXCLUDE "rockets/jsoncpp/*")

add_subdirectory(apps)
add_subdirectory(rockets)
add_subdirectory(tests)

set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(ROCKETS_PACKAGE_DEB_DEPENDS libwebsockets-dev)
include(CommonCPack)

set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)
set(DOXYGEN_MAINPAGE_MD README.md)
set(DOXYGEN_EXTRA_INPUT ${PROJECT_SOURCE_DIR}/README.md)
include(DoxygenRule)
