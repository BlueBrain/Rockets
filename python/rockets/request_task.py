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

"""Extend asyncio.Task to add callbacks for progress reporting while the request is not done."""

import asyncio


class RequestTask(asyncio.Task):
    """Extend asyncio.Task to add callbacks for progress reporting while the request is not done."""

    def __init__(self, coro, *, loop=None):
        super().__init__(coro=coro, loop=loop)
        self._progress_callbacks = []

    def add_progress_callback(self, fn):
        """
        Add a callback to be run everytime a progress update arrives.

        The callback is called with a single argument - the :class:`RequestProgress` object.
        """
        self._progress_callbacks.append(fn)

    def _call_progress_callbacks(self, value):
        """Internal: Calls registered progress callbacks."""
        for callback in self._progress_callbacks:
            callback(value)
