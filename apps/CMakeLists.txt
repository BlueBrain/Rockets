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

if(NOT ${Libwebsockets_VERSION} VERSION_LESS 2.0.0)
  set(ROCKETS-HTTP-REQUEST_SOURCES httpRequest.cpp)
  set(ROCKETS-HTTP-REQUEST_LINK_LIBRARIES Rockets)
  common_application(rockets-http-request)

  set(ROCKETS-JSONRPC-REQUEST_SOURCES jsonrpcRequest.cpp)
  set(ROCKETS-JSONRPC-REQUEST_LINK_LIBRARIES Rockets)
  common_application(rockets-jsonrpc-request)
endif()

set(ROCKETS-SERVER_SOURCES server.cpp)
set(ROCKETS-SERVER_LINK_LIBRARIES Rockets)
common_application(rockets-server)
