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
import json

import websockets
from nose.tools import assert_equal
from nose.tools import assert_false
from nose.tools import assert_true
from nose.tools import raises

import rockets


async def hello(websocket, path):
    while True:
        message = await websocket.recv()
        try:
            json_message = json.loads(message)
            method = json_message["method"]
            if method == "NotifyMe":
                notification = rockets.Notification("Hello")
                await websocket.send(str(notification.json))
        except Exception:
            greeting = "Hello {0}!".format(message)
            await websocket.send(greeting)
            if message == "Rockets":
                break


server_url = None


def setup():
    start_server = websockets.serve(hello, "localhost")
    server = asyncio.get_event_loop().run_until_complete(start_server)
    global server_url
    server_url = "localhost:" + str(server.sockets[0].getsockname()[1])


def test_subscribe_async_client():
    client = rockets.AsyncClient(server_url)

    received = asyncio.get_event_loop().create_future()

    async def _do_it():
        await client.connect()

        def _on_message(message):
            received.set_result(message)

        client.ws_observable.subscribe(_on_message)
        await client.send("Rockets")
        await received

    asyncio.get_event_loop().run_until_complete(_do_it())
    assert_equal(received.result(), "Hello Rockets!")


def test_subscribe():
    client = rockets.Client(server_url)
    client.connect()

    def _on_message(message):
        assert_equal(message, "Hello Rockets!")
        asyncio.get_event_loop().stop()

    client.ws_observable.subscribe(_on_message)
    client.send("Rockets")
    asyncio.get_event_loop().run_forever()


def test_notifications():
    client = rockets.Client(server_url)
    client.connect()

    def _on_message(message):
        assert_equal(message.method, "Hello")
        asyncio.get_event_loop().stop()

    client.notifications.subscribe(_on_message)
    client.send("Something")
    client.notify("NotifyMe")
    client.send("Rockets")
    asyncio.get_event_loop().run_forever()


if __name__ == "__main__":
    import nose

    nose.run(defaultTest=__name__)
