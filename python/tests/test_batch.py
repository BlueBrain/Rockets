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
from jsonrpcserver.aio import methods
from jsonrpcserver.response import BatchResponse, RequestResponse

from nose.tools import assert_true, assert_equal, raises
import rockets


got_cancel = asyncio.Future()

@methods.add
async def ping():
    return 'pong'

@methods.add
async def double(value):
    return value*2

@methods.add
async def foobar():
    pass

async def server_handle(websocket, path):
    request = await websocket.recv()

    json_request = json.loads(request)
    if isinstance(json_request, list):
        if any(i['method'] == 'test_cancel' for i in json_request):
            got_cancel.set_result(True)
            response = BatchResponse()
            for i in json_request:
                response.append(RequestResponse(i['id'], ['CANCELLED']))
        elif any(i['method'] == 'test_progress' for i in json_request):
            response = BatchResponse()
            for i in json_request:
                progress_notification = rockets.Request('progress', {
                    'amount': 0.5,
                    'operation': 'almost done',
                    'id': i['id']})
                await websocket.send(progress_notification.json)
                response.append(RequestResponse(i['id'], 'DONE'))
        else:
            response = await methods.dispatch(request)
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


def test_batch():
    client = rockets.Client(server_url)
    request_1 = rockets.Request('double', [2])
    request_2 = rockets.Request('double', [4])
    notification = rockets.Notification('foobar')
    assert_equal(client.batch([request_1, request_2, notification]), [4, 8])


@raises(TypeError)
def test_invalid_args():
    client = rockets.Client(server_url)
    client.batch('foo', 'bar')


@raises(ValueError)
def test_empty_request():
    client = rockets.Client(server_url)
    client.batch([], [])


def test_method_not_found():
    client = rockets.Client(server_url)
    request = rockets.Request('foo', ['bar'])
    assert_equal(client.batch([request]),
                 [{'code': -32601, 'message': 'Method not found'}])


@raises(rockets.request_error.RequestError)
def test_error_on_connection_lost():
    client = rockets.Client(server_url)
    # do one request, which closes the server, so the second request will throw an error
    assert_equal(client.request('ping'), 'pong')
    request_1 = rockets.Request('double', [2])
    request_2 = rockets.Request('double', [4])
    client.batch([request_1, request_2])


def test_progress_single_request():
    client = rockets.AsyncClient(server_url)
    request = rockets.Request('test_progress')
    request_task = client.async_batch([request])

    class ProgressTracker:
        def on_progress(self, progress):
            assert_equal(progress.amount, 0.5)
            assert_equal(progress.operation, 'Batch request')
            assert_equal(str(progress), "('Batch request', 0.5)")
            self.called = True

    tracker = ProgressTracker()

    request_task.add_progress_callback(tracker.on_progress)
    assert_equal(asyncio.get_event_loop().run_until_complete(request_task), ['DONE'])
    assert_true(tracker.called)


def test_progress_multiple_requests():
    start_test_server = websockets.serve(server_handle_two_requests, 'localhost')
    test_server = asyncio.get_event_loop().run_until_complete(start_test_server)
    client = rockets.AsyncClient('localhost:'+str(test_server.sockets[0].getsockname()[1]))
    request_a = rockets.Request('test_progress')
    request_b = rockets.Request('test_progress')
    request_task = client.async_batch([request_a, request_b])

    class ProgressTracker:
        def __init__(self):
            self._num_calls = 0

        def on_progress(self, progress):
            self._num_calls += 1
            if self._num_calls == 1:
                assert_equal(progress.amount, 0.25)
                assert_equal(progress.operation, 'Batch request')
                assert_equal(str(progress), "('Batch request', 0.25)")
            elif self._num_calls == 2:
                assert_equal(progress.amount, 0.5)
                assert_equal(progress.operation, 'Batch request')
                assert_equal(str(progress), "('Batch request', 0.5)")
                self.called = True

    tracker = ProgressTracker()

    request_task.add_progress_callback(tracker.on_progress)
    assert_equal(asyncio.get_event_loop().run_until_complete(request_task), ['DONE', 'DONE'])
    assert_true(tracker.called)


def test_cancel():
    client = rockets.AsyncClient(server_url)
    request_1 = rockets.Request('test_cancel')
    request_2 = rockets.Request('test_cancel')
    request_task = client.async_batch([request_1, request_2])

    def _on_done(value):
        assert_equal(value.result(), None)
        asyncio.get_event_loop().stop()

    async def _do_cancel():
        await got_cancel
        request_task.cancel()

    request_task.add_done_callback(_on_done)

    asyncio.ensure_future(request_task)
    asyncio.ensure_future(_do_cancel())

    asyncio.get_event_loop().run_forever()

    assert_true(request_task.done())


if __name__ == '__main__':
    import nose
    nose.run(defaultTest=__name__)
