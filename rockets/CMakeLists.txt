# Copyright (c) 2017, EPFL/Blue Brain Project
#                     Raphael.Dumusc@epfl.ch
#                     Daniel.Nachbaur@epfl.ch
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

set(ROCKETS_PUBLIC_HEADERS
  server.h
  socketBasedInterface.h
  socketListener.h
  types.h
  http/client.h
  http/filter.h
  http/helpers.h
  http/request.h
  http/response.h
  http/types.h
  qt/readWriteSocketNotifier.h
  qt/socketProcessor.h
  ws/client.h
  ws/types.h
)
set(ROCKETS_HEADERS
  clientContext.h
  pollDescriptors.h
  proxyConnectionError.h
  serverContext.h
  serviceThreadPool.h
  utils.h
  http/channel.h
  http/connection.h
  http/connectionHandler.h
  http/cors.h
  http/registry.h
  http/requestHandler.h
  http/utils.h
  jsoncpp/json/json.h
  jsoncpp/json/json-forwards.h
  ws/channel.h
  ws/connection.h
  ws/messageHandler.h
)
set(ROCKETS_SOURCES
  log.cpp
  clientContext.cpp
  pollDescriptors.cpp
  serverContext.cpp
  server.cpp
  serviceThreadPool.cpp
  utils.cpp
  http/channel.cpp
  http/connection.cpp
  http/client.cpp
  http/connectionHandler.cpp
  http/registry.cpp
  http/requestHandler.cpp
  http/utils.cpp
  jsoncpp/jsoncpp.cpp
  ws/channel.cpp
  ws/connection.cpp
  ws/client.cpp
  ws/messageHandler.cpp
)
# without linking client code with pthread, std::promise::set_value() dies with
# std::system_error what():  Unknown error -1
# https://stackoverflow.com/questions/43928715
set(ROCKETS_LINK_LIBRARIES PUBLIC Threads::Threads PRIVATE websockets)

if(MSVC)
  list(APPEND ROCKETS_LINK_LIBRARIES Ws2_32)
else()
  set_source_files_properties(jsoncpp/jsoncpp.cpp PROPERTIES COMPILE_FLAGS
    -Wno-shadow)
endif()

common_library(Rockets)
