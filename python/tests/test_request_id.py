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
from nose.tools import assert_equal
from nose.tools import assert_not_equal

from rockets import Request


def test_request_id_length():
    request = Request("foo")
    assert_equal(len(request.request_id()), 8)


def test_request_ids_different():
    request_a = Request("foo")
    request_b = Request("bar")
    assert_not_equal(request_a.request_id(), request_b.request_id())


if __name__ == "__main__":
    import nose

    nose.run(defaultTest=__name__)
