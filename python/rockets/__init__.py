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
"""A small client for Rockets using JSON-RPC as communication contract over a WebSocket."""
from .async_client import AsyncClient
from .client import Client
from .notification import Notification
from .request import Request
from .request_error import RequestError
from .request_progress import RequestProgress
from .request_task import RequestTask
from .response import Response
from .version import VERSION as __version__

__all__ = [
    "AsyncClient",
    "Client",
    "Notification",
    "Request",
    "RequestError",
    "RequestProgress",
    "RequestTask",
    "Response",
]
