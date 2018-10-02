#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2018, Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
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
# All rights reserved. Do not distribute without further notice.

import asyncio
import websockets
from jsonrpcserver.aio import methods

from nose.tools import assert_true, assert_false, assert_equal
import rockets


got_hello = asyncio.Future()
echo_param = asyncio.Future()

@methods.add
async def hello():
    global got_hello
    got_hello.set_result(True)

@methods.add
async def echo(name):
    global echo_param
    echo_param.set_result(name)

async def server_handle(websocket, path):
    request = await websocket.recv()
    await methods.dispatch(request)

server_url = None
def setup():
    start_server = websockets.serve(server_handle, 'localhost')
    server = asyncio.get_event_loop().run_until_complete(start_server)
    global server_url
    server_url = 'localhost:'+str(server.sockets[0].getsockname()[1])


def test_no_param():
    client = rockets.Client(server_url)
    client.notify('hello')
    asyncio.get_event_loop().run_until_complete(got_hello)
    assert_true(got_hello.result())


def test_param():
    client = rockets.Client(server_url)
    client.notify('echo', {'name':'world'})
    asyncio.get_event_loop().run_until_complete(echo_param)
    assert_equal(echo_param.result(), 'world')


def test_method_not_found():
    client = rockets.Client(server_url)
    client.notify('pong')
    # no effect on the client side

if __name__ == '__main__':
    import nose
    nose.run(defaultTest=__name__)
