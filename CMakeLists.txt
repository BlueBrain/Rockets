# Copyright (c) 2017-2019, EPFL/Blue Brain Project
#                          Raphael.Dumusc@epfl.ch
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
project(Rockets VERSION 1.0.0)
set(Rockets_VERSION_ABI 1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/common)
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/CMake/common/Common.cmake)
  message(FATAL_ERROR "CMake/common missing, run: git submodule update --init")
endif()

set(ROCKETS_DEB_DEPENDS libboost-test-dev libwebsockets-dev libuv1-dev)
set(ROCKETS_PORT_DEPENDS libwebsockets)

# disable noisy warnings from ChoosePython
set(CHOOSE_PYTHON_DONE ON)
include(FindBoostConfig)

include(Common)

set(ROCKETS_DESCRIPTION "Rockets - REST and websockets C++ library")
set(ROCKETS_MAINTAINER "Blue Brain Project <bbp-open-source@googlegroups.com>")
set(ROCKETS_LICENSE LGPL)

common_find_package(Boost SYSTEM COMPONENTS unit_test_framework)
if(APPLE)
  common_find_package(OpenSSL REQUIRED) # libwebsockets depends on that
endif()
common_find_package(Libwebsockets SYSTEM)
if(NOT Libwebsockets_FOUND) # Ubuntu package only has libwebsockets.pc
  common_find_package_disable(Libwebsockets)
  common_find_package(libwebsockets SYSTEM REQUIRED)
  set(Libwebsockets_VERSION ${libwebsockets_VERSION})
endif()
common_find_package(Threads REQUIRED)
common_find_package_post()

set(ROCKETS_DEPENDENT_LIBRARIES Threads)

option(ROCKETS_BUILD_APPLICATIONS "Build example applications" ON)

if(ROCKETS_BUILD_APPLICATIONS)
  add_subdirectory(apps)
endif()
add_subdirectory(rockets)
add_subdirectory(tests)

set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(ROCKETS_PACKAGE_DEB_DEPENDS libwebsockets-dev)
include(CommonCPack)

set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)
set(DOXYGEN_MAINPAGE_MD README.md)
set(DOXYGEN_EXTRA_INPUT ${PROJECT_SOURCE_DIR}/README.md)
include(DoxygenRule)
