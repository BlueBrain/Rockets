# Rockets Python Client

> A small client for [Rockets](../README.md) using [JSON-RPC](https://www.jsonrpc.org) as
> communication contract over a [WebSocket](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket).

[![Travis (.org) branch](https://img.shields.io/travis/BlueBrain/Rockets/master.svg?style=flat-square)](https://github.com/BlueBrain/Rockets)


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


#### Notifications
Listen to server notifications:
```py
from rockets import Client

client = Client('myhost:8080')

client.as_observable().subscribe(lambda msg: print("Got message:", msg))
```

Send notifications:
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

Make an asynchronous request using `asyncio`:
```py
import asyncio
from rockets import Client

client = Client('myhost:8080')

request_task = client.async_request('mymethod', {'ping': True})
asyncio.get_event_loop().run_until_complete(request_task)
print(request_task.result())
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

Cancel a request:
```py
from rockets import Client

client = Client('myhost:8080')

request_task = client.async_request('mymethod')
request_task.cancel()
```

Get progress updates for a request:
```py
from rockets import Client

client = Client('myhost:8080')

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
from rockets import Client

client = Client('myhost:8080')

request = Request('myrequest')
notification = Notification('mynotify')
request_task = client.async_batch([request, notification])
request_task.cancel()
```
