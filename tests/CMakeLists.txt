# Copyright (c) 2017-2018, EPFL/Blue Brain Project
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

if(NOT BOOST_FOUND)
  return()
endif()

if(${Libwebsockets_VERSION} VERSION_LESS 2.1.0)
  set(EXCLUDE_FROM_TESTS jsonRpcHttp.cpp)
endif()

set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} Rockets)

list(APPEND LCOV_EXCLUDE '${PROJECT_SOURCE_DIR}/rockets/json.hpp')
include(CommonCTest)
