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

"""Utils for the client"""

from random import choice
from string import ascii_lowercase, digits

HTTP = 'http://'
HTTPS = 'https://'
WS = 'ws://'
WSS = 'wss://'


def set_ws_protocol(url):
    """
    Set the WebSocket protocol according to the resource url.

    :param str url: Url to be checked
    :return: Url preprend with ws for http, wss for https for ws if no protocol was found
    :rtype: str
    """
    if url.startswith(WS) or url.startswith(WSS):
        return url
    if url.startswith(HTTP):
        return url.replace(HTTP, WS, 1)
    if url.startswith(HTTPS):
        return url.replace(HTTPS, WSS, 1)
    return WS + url


def copydoc(fromfunc, sep="\n"):
    """
    Decorator: Copy the docstring of `fromfunc`

    :return:
    :rtype:
    """
    def _decorator(func):
        sourcedoc = fromfunc.__doc__
        if func.__doc__:  # pragma: no cover
            func.__doc__ = sep.join([sourcedoc, func.__doc__])
        else:
            func.__doc__ = sourcedoc
        return func
    return _decorator


def random_string(length=8, chars=digits + ascii_lowercase):
    """
    A random string.

    Not unique, but has around 1 in a million chance of collision (with the default 8
    character length). e.g. 'fubui5e6'

    :param int length: Length of the random string.
    :param str chars: The characters to randomly choose from.
    :return: random string
    :rtype: str
    """
    while True:
        yield "".join([choice(chars) for _ in range(length)])
