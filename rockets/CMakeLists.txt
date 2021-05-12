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

list(APPEND CPPCHECK_EXTRA_ARGS
  --suppress=*:${CMAKE_CURRENT_SOURCE_DIR}/json.hpp)

# bug while checking json.hpp, --suppress does not remove the error exit code
list(APPEND CPPCHECK_EXTRA_ARGS --error-exitcode=0)

set(ROCKETS_PUBLIC_HEADERS
  helpers.h
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
  jsonrpc/asyncReceiver.h
  jsonrpc/cancellableReceiver.h
  jsonrpc/client.h
  jsonrpc/clientRequest.h
  jsonrpc/errorCodes.h
  jsonrpc/helpers.h
  jsonrpc/http.h
  jsonrpc/notifier.h
  jsonrpc/receiver.h
  jsonrpc/requester.h
  jsonrpc/response.h
  jsonrpc/responseError.h
  jsonrpc/server.h
  jsonrpc/types.h
  jsonrpc/utils.h
  qt/readWriteSocketNotifier.h
  qt/socketProcessor.h
  ws/client.h
  ws/types.h
)
set(ROCKETS_HEADERS
  clientContext.h
  debug.h
  json.hpp
  pollDescriptors.h
  proxyConnectionError.h
  serverContext.h
  serviceThreadPool.h
  unavailablePortError.h
  utils.h
  wrappers.h
  http/channel.h
  http/connection.h
  http/connectionHandler.h
  http/cors.h
  http/registry.h
  http/requestHandler.h
  http/utils.h
  jsonrpc/asyncReceiverImpl.h
  jsonrpc/cancellableReceiverImpl.h
  jsonrpc/receiverImpl.h
  jsonrpc/requestProcessor.h
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
  jsonrpc/asyncReceiver.cpp
  jsonrpc/cancellableReceiver.cpp
  jsonrpc/clientRequest.cpp
  jsonrpc/helpers.cpp
  jsonrpc/http.cpp
  jsonrpc/notifier.cpp
  jsonrpc/receiver.cpp
  jsonrpc/requester.cpp
  jsonrpc/requestProcessor.cpp
  ws/channel.cpp
  ws/connection.cpp
  ws/client.cpp
  ws/messageHandler.cpp
)
# without linking client code with pthread, std::promise::set_value() dies with
# std::system_error what():  Unknown error -1
# https://stackoverflow.com/questions/43928715
# libwebsockets15 comes with cmake package info, which makes websockets the static library
# This results on an error because we are linking a static library with a dynamic one without -fPIC
if(TARGET websockets_shared)
	set(ROCKETS_LINK_LIBRARIES PUBLIC Threads::Threads PRIVATE websockets_shared)
else()
	set(ROCKETS_LINK_LIBRARIES PUBLIC Threads::Threads PRIVATE websockets)
endif()


common_library(Rockets)
