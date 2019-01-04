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
"""A JSON-RPC 2.0 notification"""
from jsonrpc.jsonrpc2 import JSONRPC20Response


class Response(JSONRPC20Response):
    """A JSON-RPC 2.0 response"""

    @classmethod
    def from_json(cls, json_str):
        """Create Response from JSON string"""
        data = cls.deserialize(json_str)
        return cls.from_data(data)

    @classmethod
    def from_data(cls, data):
        """Create Response from dict"""
        data["_id"] = data["id"]
        response = Response(**data)
        return response
