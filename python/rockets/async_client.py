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
"""Asynchronous client implementation for asyncio event loop processing of JSON-RPC messages."""
import asyncio
import json
import sys
from functools import reduce

import websockets
from jsonrpc.jsonrpc2 import JSONRPC20Request
from rx import Observable

from .notification import Notification
from .request import Request
from .request_error import INVALID_REQUEST
from .request_error import RequestError
from .request_error import SOCKET_CLOSED_ERROR
from .request_progress import RequestProgress
from .request_task import RequestTask
from .response import Response
from .utils import is_json_rpc_notification
from .utils import is_json_rpc_response
from .utils import is_progress_notification
from .utils import set_ws_protocol


class AsyncClient:
    """Asynchronous client implementation for asyncio event loop processing of JSON-RPC messages."""

    def __init__(self, url, subprotocols=None, loop=None):
        """
        Initialize the state of the client.

        Convert the URL to a proper format. Does not establish the websocket connection yet. This
        will be postponed to the first notify or request.

        :param str url: The address of the Rockets server.
        :param list subprotocols: The websocket protocols to use
        :param asyncio.AbstractEventLoop loop: Event loop where this client should run in
        """
        self.url = set_ws_protocol(url)
        """The address of the connected Rockets server."""

        if not subprotocols:
            subprotocols = ["rockets"]
        self._subprotocols = subprotocols

        self._ws = None

        self.loop = loop
        """The event loop where this client is running in."""
        if not self.loop:
            self.loop = asyncio.get_event_loop()

        def _ws_loop(observer):
            """Internal: synchronous wrapper for async _ws_loop"""
            asyncio.ensure_future(self._ws_loop(observer), loop=self.loop)

        # pylint: disable=E1101
        self.ws_observable = Observable.create(_ws_loop).publish().auto_connect()
        """The websocket stream as an rx observable to subscribe to it."""
        # pylint: enable=E1101

        def _json_filter(value):
            try:
                json.loads(value)
                return True
            except ValueError:  # pragma: no cover
                return False

        # filter everything that is not JSON
        self._json_stream = self.ws_observable.filter(_json_filter).map(json.loads)

        def _notifications_filter(value):
            return is_json_rpc_notification(value) and not is_progress_notification(
                value
            )

        self.notifications = self._json_stream.filter(_notifications_filter).map(
            lambda x: Notification.from_json(json.dumps(x))
        )
        """The rx observable to subscribe to notifications from the server."""

    def connected(self):
        """
        Returns the connection state of this client.

        :return: true if the websocket is connected to the Rockets server.
        :rtype: bool
        """
        return bool(self._ws and self._ws.open)

    async def connect(self):
        """Connect this client to the Rockets server"""
        if self.connected():
            return

        self._ws = await websockets.connect(
            self.url,
            subprotocols=self._subprotocols,
            max_size=None,
            ping_timeout=None,
            loop=self.loop,
        )

    async def disconnect(self):
        """Disconnect this client from the Rockets server."""
        if not self.connected():
            return

        await self._ws.close()

    async def send(self, message):
        """
        Send any message to the connected Rockets server.

        :param str message: The message to send
        """
        await self.connect()
        await self._ws.send(message)

    async def notify(self, method, params):
        """
        Invoke an RPC on the Rockets server without expecting a response.

        :param str method: name of the method to invoke
        :param str params: params for the method
        """
        notification = Notification(method, params)
        await self.send(notification.json)

    async def request(self, method, params=None):
        """
        Invoke an RPC on the Rockets server and returns its response.

        :param str method: name of the method to invoke
        :param dict params: params for the method
        :return: future object
        :rtype: :class:`asyncio.Future`
        """
        if params and not isinstance(params, (list, tuple, dict)):
            params = [params]
        request = Request(method, params)
        try:
            response_future = self.loop.create_future()

            request_id = request.request_id()
            await self.connect()
            self._setup_response_filter(response_future, request_id)
            self._setup_progress_filter(response_future, request_id)

            await self.send(request.json)
            await response_future
            return response_future.result()
        except asyncio.CancelledError:
            await self.notify("cancel", {"id": request_id})

    async def batch(self, requests):
        """
        Invoke a batch RPC on the Rockets server and return its response(s).

        :param list requests: list of requests and/or notifications to send as batch
        :return: future object with list of responses
        :rtype: :class:`asyncio.Future`
        :raises RequestError: if methods and/or params are not a list
        :raises RequestError: if methods are empty
        """
        if not requests:
            raise INVALID_REQUEST

        # is valid for both, requests and notifications
        for request in requests:
            if not isinstance(request, JSONRPC20Request):
                raise INVALID_REQUEST

        request_ids = list()
        for request in requests:
            if isinstance(request, Request):
                request_ids.append(request.request_id())

        try:

            def _data(x):
                return x.data

            request = Request.from_data(list(map(_data, requests)))

            response_future = self.loop.create_future()

            await self.connect()
            self._setup_batch_response_filter(response_future, request_ids)
            self._setup_batch_progress_filter(response_future, request_ids)

            await self.send(request.json)
            await response_future
            return response_future.result()
        except asyncio.CancelledError:
            for request_id in request_ids:
                await self.notify("cancel", {"id": request_id})

    def async_request(self, method, params=None):
        """
        Invoke an RPC on the Rockets server and return the :class:`RequestTask`.

        :param str method: name of the method to invoke
        :param dict params: params for the method
        :return: :class:`RequestTask` object
        :rtype: :class:`RequestTask`
        """
        self.loop.set_task_factory(lambda loop, coro: RequestTask(coro=coro, loop=loop))

        task = self.request(method, params)
        return asyncio.ensure_future(task, loop=self.loop)

    def async_batch(self, requests):
        """
        Invoke a batch RPC on the Rockets server and return the :class:`RequestTask`.

        :param list requests: list of requests and/or notifications to send as batch
        :return: :class:`RequestTask` object
        :rtype: :class:`RequestTask`
        """
        self.loop.set_task_factory(lambda loop, coro: RequestTask(coro=coro, loop=loop))

        task = self.batch(requests)
        return asyncio.ensure_future(task, loop=self.loop)

    async def _ws_loop(self, observer):
        """Internal: The loop for feeding an rxpy observer."""
        try:
            await self.connect()
            if sys.version_info < (3, 6):
                while True:  # pragma: no cover
                    message = await self._ws.recv()
                    observer.on_next(message)
            else:  # pragma: no cover
                async for message in self._ws:
                    observer.on_next(message)
                observer.on_completed()
        except websockets.ConnectionClosed:  # pragma: no cover
            observer.on_completed()

    def _setup_response_filter(self, response_future, request_id):
        def _response_filter(value):
            return is_json_rpc_response(value) and value["id"] == request_id

        def _to_response(value):
            response = Response.from_json(json.dumps(value))
            if response.result:
                return response.result
            return response.error

        def _on_next(value):
            if not response_future.done():
                if isinstance(value, dict) and "code" in value:
                    response_future.set_exception(RequestError(**value))
                else:
                    response_future.set_result(value)

        def _on_completed():

            if not response_future.done():
                response_future.set_exception(SOCKET_CLOSED_ERROR)

        self._json_stream.filter(_response_filter).take(1).map(_to_response).subscribe(
            on_next=_on_next, on_completed=_on_completed
        )

    def _setup_batch_response_filter(self, response_future, request_ids):
        def _response_filter(value):
            if not isinstance(value, list):
                return False
            for response in value:
                if (
                    not is_json_rpc_response(response)
                    or response["id"] not in request_ids
                ):
                    return False  # pragma: no cover
            return True

        def _to_response(value):
            responses = [Response.from_json(json.dumps(i)) for i in value]
            return responses

        def _on_next(value):
            if not response_future.done():
                response_future.set_result(value)

        def _on_completed():
            if not response_future.done():
                response_future.set_exception(SOCKET_CLOSED_ERROR)

        self._json_stream.filter(_response_filter).take(1).map(_to_response).subscribe(
            on_next=_on_next, on_completed=_on_completed
        )

    def _setup_progress_filter(self, response_future, request_id):
        task = asyncio.Task.current_task()
        if task and isinstance(task, RequestTask):

            def _progress_filter(value):
                return (
                    is_progress_notification(value)
                    and value["params"]["id"] == request_id
                )

            def _to_progress(value):
                progress = Request.from_data(value).params
                return RequestProgress(progress["operation"], progress["amount"])

            progress_observable = (
                self._json_stream.filter(_progress_filter)
                .map(_to_progress)
                .subscribe(task._call_progress_callbacks)  # pylint: disable=W0212
            )

            def _done_callback(future):  # pylint: disable=W0613
                progress_observable.dispose()

            response_future.add_done_callback(_done_callback)

    def _setup_batch_progress_filter(self, response_future, request_ids):
        task = asyncio.Task.current_task()
        items = dict()
        if task and isinstance(task, RequestTask):

            def _progress_filter(value):
                return (
                    is_progress_notification(value)
                    and value["params"]["id"] in request_ids
                )

            def _to_progress(value):
                progress = Request.from_data(value).params
                items[progress["id"]] = progress["amount"]
                total = reduce((lambda x, value: x + value), items.values())
                return RequestProgress("Batch request", total / len(request_ids))

            progress_observable = (
                self._json_stream.filter(_progress_filter)
                .map(_to_progress)
                .subscribe(task._call_progress_callbacks)  # pylint: disable=W0212
            )

            def _done_callback(future):  # pylint: disable=W0613
                progress_observable.dispose()

            response_future.add_done_callback(_done_callback)
