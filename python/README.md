# Rockets Python Client

> A small client for [Rockets](../README.md) using [JSON-RPC](https://www.jsonrpc.org) as
> communication contract over a [WebSocket](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket).

[![Travis CI](https://img.shields.io/travis/BlueBrain/Rockets/master.svg?style=flat-square)](https://travis-ci.org/BlueBrain/Rockets)
[![Updates](https://pyup.io/repos/github/BlueBrain/Rockets/shield.svg)](https://pyup.io/repos/github/BlueBrain/Rockets/)
[![Latest version](https://img.shields.io/pypi/v/rockets.svg)](https://pypi.org/project/rockets/)
[![Python versions](https://img.shields.io/pypi/pyversions/rockets.svg)](https://pypi.org/project/rockets/)
[![Code style: black](https://img.shields.io/badge/code%20style-black-000000.svg)](https://github.com/ambv/black)
[![Language grade: Python](https://img.shields.io/lgtm/grade/python/g/BlueBrain/Rockets.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/BlueBrain/Rockets/context:python)


# Table of Contents

* [Installation](#installation)
* [Usage](#usage)
    * [Connection](#connection)
    * [Notifications](#notifications)
    * [Requests](#requests)
    * [Batching](#batching)


### Installation
----------------
You can install this package from [PyPI](https://pypi.org/):
```bash
pip install rockets
```

### Usage
---------

#### `Client` vs. `AsyncClient`
Rockets provides two types of clients to support asychronous and synchronous usage.

The `AsyncClient` exposes all of its functionality as `async` functions, hence an `asyncio`
[event loop](https://docs.python.org/3/library/asyncio-eventloop.html) is needed to complete pending
execution via `await` or `run_until_complete()`.

For simplicity, a synchronous `Client` is provided which automagically executes in a synchronous,
blocking fashion.

#### Connection
Create a client and connect:
```py
from rockets import Client

# client does not connect during __init__;
# either explicit or automatically on any notify/request/send
client = Client('myhost:8080')

client.connect()
print(client.connected())
```

Close the connection with the socket cleanly:
```py
from rockets import Client

client = Client('myhost:8080')

client.connect()
client.disconnect()
print(client.connected())
```


#### Server messages
Listen to server notifications:
```py
from rockets import Client

client = Client('myhost:8080')

client.notifications.subscribe(lambda msg: print("Got message:", msg.data))
```

**NOTE**: The notification object is of type `Notification`.

Listen to any server message:
```py
from rockets import Client

client = Client('myhost:8080')

client.ws_observable.subscribe(lambda msg: print("Got message:", msg))
```


#### Notifications
Send notifications to the server:
```py
from rockets import Client

client = Client('myhost:8080')

client.notify('mymethod', {'ping': True})
```


#### Requests
Make a synchronous, blocking request:
```py
from rockets import Client

client = Client('myhost:8080')

response = client.request('mymethod', {'ping': True})
print(response)
```

Handle a request error:
```py
from rockets import Client, RequestError

client = Client('myhost:8080')

try:
    client.request('mymethod')
except RequestError as err:
    print(err.code, err.message)
```

**NOTE**: Any error that may occur will be a `RequestError`.


#### Asynchronous requests
Make an asynchronous request, using the `AsyncClient` and `asyncio`:
```py
import asyncio
from rockets import AsyncClient

client = AsyncClient('myhost:8080')

request_task = client.async_request('mymethod', {'ping': True})
asyncio.get_event_loop().run_until_complete(request_task)
print(request_task.result())
```

Alternatively, you can use `add_done_callback()` from the returned `RequestTask` which is called
once the request has finished:

```py
import asyncio
from rockets import AsyncClient

client = AsyncClient('myhost:8080')

request_task = client.async_request('mymethod', {'ping': True})
request_task.add_done_callback(lambda task: print(task.result()))
asyncio.get_event_loop().run_until_complete(request_task)
```

If the `RequestTask` is not needed, i.e. no `cancel()` or `add_progress_callback()` is desired, use
the `request()` coroutine:

```py
import asyncio
from rockets import AsyncClient

client = AsyncClient('myhost:8080')

coro = client.request('mymethod', {'ping': True})
result = asyncio.get_event_loop().run_until_complete(coro)
print(result)
```

If you are already in an `async` function or in a Jupyter notebook cell, you may use `await` to
execute an asynchronous request:
```py
# Inside a notebook cell here
import asyncio
from rockets import AsyncClient

client = AsyncClient('myhost:8080')

result = await client.request('mymethod', {'ping': True})
print(result)
```

Cancel a request:
```py
from rockets import AsyncClient

client = AsyncClient('myhost:8080')

request_task = client.async_request('mymethod')
request_task.cancel()
```

Get progress updates for a request:
```py
from rockets import AsyncClient

client = AsyncClient('myhost:8080')

request_task = client.async_request('mymethod')
request_task.add_progress_callback(lambda progress: print(progress))
```

**NOTE**: The progress object is of type `RequestProgress`.

#### Batching
Make a batch request:
```py
from rockets import Client, Request, Notification

client = Client('myhost:8080')

request = Request('myrequest')
notification = Notification('mynotify')
responses = client.batch([request, notification])

for response in responses:
    print(response)
```

Cancel a batch request:
```py
from rockets import AsyncClient

client = AsyncClient('myhost:8080')

request = Request('myrequest')
notification = Notification('mynotify')
request_task = client.async_batch([request, notification])
request_task.cancel()
```
