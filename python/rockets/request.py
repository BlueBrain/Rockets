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
"""A JSON-RPC 2.0 request"""
from jsonrpc.jsonrpc2 import JSONRPC20Request

from .utils import random_string


class Request(JSONRPC20Request):
    """A JSON-RPC 2.0 request"""

    _id_generator = random_string()

    def __init__(self, method, params=None):
        super().__init__(
            method=method,
            params=params,
            _id=next(Request._id_generator),
            is_notification=False,
        )

    def request_id(self):
        """Return the request ID"""
        return super()._id
