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
from jsonrpcserver.response import RequestResponse
import json

from nose.tools import assert_true, assert_equal, raises
import rockets


@methods.add
async def ping():
    return 'pong'

@methods.add
async def double(value):
    return value*2

@methods.add
async def longrequest():
    asyncio.sleep(500)

async def server_handle(websocket, path):
    request = await websocket.recv()

    json_request = json.loads(request)
    method = json_request['method']
    if method == 'test_progress':
        progress_notification = rockets.Request('progress', {
            'amount': 0.5,
            'operation': 'almost done',
            'id': json_request['id']})
        await websocket.send(progress_notification.json)
        response = RequestResponse(json_request['id'], 'DONE')
    elif method == 'test_cancel':
        await websocket.recv()
        response = RequestResponse(json_request['id'], 'CANCELLED')
    else:
        response = await methods.dispatch(request)
    if not response.is_notification:
        await websocket.send(str(response))

async def server_handle_two_requests(websocket, path):
    for i in range(2):
        await server_handle(websocket, path)

server_url = None
def setup():
    start_server = websockets.serve(server_handle, 'localhost')
    server = asyncio.get_event_loop().run_until_complete(start_server)
    global server_url
    server_url = 'localhost:'+str(server.sockets[0].getsockname()[1])


def test_no_param():
    client = rockets.Client(server_url)
    assert_equal(client.request('ping'), 'pong')


def test_param():
    client = rockets.Client(server_url)
    assert_equal(client.request('double', [2]), 4)


def test_param_as_not_a_list():
    client = rockets.Client(server_url)
    assert_equal(client.request('double', 2), 4)


def test_method_not_found():
    client = rockets.Client(server_url)
    try:
        client.request('foo')
    except rockets.RequestError as e:
        assert_equal(e.code, -32601)
        assert_equal(e.message, 'Method not found')


@raises(rockets.request_error.RequestError)
def test_error_on_connection_lost():
    client = rockets.Client(server_url)
    # do one request, which finishes the server, so the second request will throw an error
    assert_equal(client.request('ping'), 'pong')
    client.request('ping')


def test_subsequent_request():
    start_test_server = websockets.serve(server_handle_two_requests, 'localhost')
    test_server = asyncio.get_event_loop().run_until_complete(start_test_server)
    client = rockets.Client('localhost:'+str(test_server.sockets[0].getsockname()[1]))
    assert_equal(client.request('ping'), 'pong')
    assert_equal(client.request('double', 2), 4)


def test_progress():
    client = rockets.AsyncClient(server_url)
    request_task = client.async_request('test_progress')

    class ProgressTracker:
        def on_progress(self, progress):
            assert_equal(progress.amount, 0.5)
            assert_equal(progress.operation, 'almost done')
            assert_equal(str(progress), "('almost done', 0.5)")
            self.called = True

    tracker = ProgressTracker()

    request_task.add_progress_callback(tracker.on_progress)
    assert_equal(asyncio.get_event_loop().run_until_complete(request_task), 'DONE')
    assert_true(tracker.called)


def test_cancel():
    client = rockets.AsyncClient(server_url)
    request_task = client.async_request('test_cancel')

    def _on_done(value):
        assert_equal(value.result(), None)
        asyncio.get_event_loop().stop()

    async def _do_cancel():
        asyncio.sleep(10)
        request_task.cancel()

    request_task.add_done_callback(_on_done)

    asyncio.ensure_future(request_task)
    asyncio.ensure_future(_do_cancel())

    asyncio.get_event_loop().run_forever()

    assert_true(request_task.done())


if __name__ == '__main__':
    import nose
    nose.run(defaultTest=__name__)
