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
"""Reports the error code and message of a request that has failed."""


class RequestError(Exception):
    """Reports the error code and message of a request that has failed."""

    def __init__(self, code, message, data=None):
        super(RequestError, self).__init__(message)

        self.code = code
        self.message = message
        self.data = data


SOCKET_CLOSED_ERROR = RequestError(-30100, "Socket connection closed")
INVALID_REQUEST = RequestError(-32600, "Invalid Request")
