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

from threading import Thread, Event
from nose.tools import assert_true, assert_false, assert_equal, raises
import rockets

got_message = None

@methods.add
async def hello():
    pass

@methods.add
async def ping():
    return 'pong'

@methods.add
async def double(value):
    return value*2

async def server_handle(websocket, path):
    request = await websocket.recv()
    response = await methods.dispatch(request)
    if not response.is_notification:
        await websocket.send(str(response))
    global got_message
    got_message.set_result(True)


class TestClass():
    def _run_server(self):
        asyncio.set_event_loop(self.server_loop)
        start_server = websockets.serve(server_handle, 'localhost')
        server = self.server_loop.run_until_complete(start_server)

        self.server_url = 'localhost:'+str(server.sockets[0].getsockname()[1])
        self.server_ready.set()

        self.got_message = self.server_loop.create_future()
        global got_message
        got_message = self.got_message
        self.server_loop.run_until_complete(self.got_message)

    def setup(self):
        self.server_ready = Event()
        self.server_loop = asyncio.new_event_loop()
        self.server_thread = Thread(target=self._run_server)
        self.server_thread.start()

    def teardown(self):
        self.server_thread.join()

    def test_notify(self):
        async def run_notebook_cell():
            self.server_ready.wait()
            client = rockets.Client('ws://'+self.server_url)
            client.notify('hello')

        asyncio.get_event_loop().run_until_complete(run_notebook_cell())

    def test_request(self):
        async def run_notebook_cell():
            self.server_ready.wait()
            client = rockets.Client('ws://'+self.server_url)
            assert_equal(client.request('ping'), 'pong')

        asyncio.get_event_loop().run_until_complete(run_notebook_cell())

    def test_invalid_environment(self):
        self.server_ready.wait()
        client = rockets.Client('ws://'+self.server_url)

        async def run_notebook_cell(client):
            try:
                client.request('ping')
                return False
            except RuntimeError:
                return True

        called = asyncio.get_event_loop().run_until_complete(run_notebook_cell(client))
        assert_true(called)

        # unblock server thread
        client.notify('hello')

    def test_async_request(self):
        called = asyncio.get_event_loop().create_future()
        def _on_done(the_task):
            called.set_result(the_task.result())

        async def run_notebook_cell():
            self.server_ready.wait()
            client = rockets.Client('ws://'+self.server_url)
            task = client.async_request('ping')
            task.add_done_callback(_on_done)
            await task

        asyncio.get_event_loop().run_until_complete(run_notebook_cell())
        assert_equal(asyncio.get_event_loop().run_until_complete(called), 'pong')

    def test_async_batch(self):
        called = asyncio.get_event_loop().create_future()
        def _on_done(the_task):
            called.set_result(the_task.result())

        async def run_notebook_cell():
            self.server_ready.wait()
            client = rockets.Client('ws://'+self.server_url)
            request_1 = rockets.Request('double', [2])
            request_2 = rockets.Request('double', [4])
            task = client.async_batch([request_1, request_2])
            task.add_done_callback(_on_done)
            await task

        asyncio.get_event_loop().run_until_complete(run_notebook_cell())
        assert_equal(asyncio.get_event_loop().run_until_complete(called), [4, 8])


if __name__ == '__main__':
    import nose
    nose.run(defaultTest=__name__)
