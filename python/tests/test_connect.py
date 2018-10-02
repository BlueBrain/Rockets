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

from nose.tools import assert_true, assert_false, assert_equal
import rockets


async def server_handle(websocket, path):
    await websocket.recv()

server_url = None
def setup():
    start_server = websockets.serve(server_handle, 'localhost')
    server = asyncio.get_event_loop().run_until_complete(start_server)
    global server_url
    server_url = 'localhost:'+str(server.sockets[0].getsockname()[1])


def test_connect_ws():
    client = rockets.Client('ws://'+server_url)
    assert_equal(client.url(), 'ws://'+server_url)
    assert_false(client.connected())


def test_connect_wss():
    client = rockets.Client('wss://'+server_url)
    assert_equal(client.url(), 'wss://'+server_url)
    assert_false(client.connected())


def test_connect_http():
    client = rockets.Client('http://'+server_url)
    assert_equal(client.url(), 'ws://'+server_url)
    assert_false(client.connected())


def test_connect_https():
    client = rockets.Client('https://'+server_url)
    assert_equal(client.url(), 'wss://'+server_url)
    assert_false(client.connected())


def test_connect():
    client = rockets.Client(server_url)
    assert_equal(client.url(), 'ws://'+server_url)
    assert_false(client.connected())


def test_reconnect():
    client = rockets.Client(server_url)
    client.connect()
    assert_true(client.connected())
    client.disconnect()
    assert_false(client.connected())
    client.connect()
    assert_true(client.connected())


def test_double_disconnect():
    client = rockets.Client(server_url)
    client.connect()
    assert_true(client.connected())
    client.disconnect()
    assert_false(client.connected())
    client.disconnect()
    assert_false(client.connected())


if __name__ == '__main__':
    import nose
    nose.run(defaultTest=__name__)
