# Rockets JS Client

> A small client for [Rockets](../README.md) using [JSON RPC](https://www.jsonrpc.org) as communication contract over a [WebSocket](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket).

[![Travis CI](https://img.shields.io/travis/BlueBrain/Rockets/master.svg?style=flat-square)](https://travis-ci.org/BlueBrain/Rockets)
[![Language grade: JavaScript](https://img.shields.io/lgtm/grade/javascript/g/BlueBrain/Rockets.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/BlueBrain/Rockets/context:javascript)


# Table of Contents

* [Installation](#installation)
* [Usage](#usage)
    * [Connection](#connection)
    * [Notifications](#notifications)
    * [Requests](#requests)
    * [Batching](#batching)
* [Release](#release)
* [Learning Material](#learning-material)


### Installation
----------------
You can install this package from [NPM](https://www.npmjs.com):
```bash
npm add rxjs rockets-client
```

Or with [Yarn](https://yarnpkg.com/en):
```bash
yarn add rxjs rockets-client
```

#### CDN
For CDN, you can use [unpkg](https://unpkg.com):

[https://unpkg.com/rockets-client/dist/bundles/rockets-client.umd.min.js](https://unpkg.com/rockets-client/dist/bundles/rockets-client.umd.min.js)

The global namespace for rockets is `rocketsClient`:
```js
const {Client} = rocketsClient;

const rockets = new Client({
    url: 'myhost'
});
```

### Usage
---------

#### Connection
Create a client and connect:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({
    url: 'myhost',
    onConnected() {
        console.info('I just connected');
    },
    onClosed(evt) {
        console.log(`Socket connection closed with code ${evt.code}`);
    }
});
```

Close the connection with the socket cleanly:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

rockets.subscribe({
    next(notification) {
        console.log(notification);
    },
    complete() {
        console.log('Socket connection closed');
    }
});

rockets.disconnect();
```


#### Notifications
Listen to server notifications:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

rockets.subscribe(notification => {
    console.log(notification);
});
```

Send notifications:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

rockets.notify('mymethod', {
    ping: true
});
```

Send a notification using the `Notification` object:
```ts
import {Client, Notification} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

const notification = new Notification('mymethod', {
    ping: true
});
rockets.notify(notification);
```


#### Requests
Make a request:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

const response = await rockets.request('mymethod', {
    ping: true
});
console.log(response);
```

**NOTE**: There is no need to wait for a connection to be established with the socket as requests will be buffered and sent once the connection is alive.
In case of a socket error, the request promise will reject.
The same is true for batch requests and notifications.

Or make a request using the `Request` object:
```ts
import {Client, Request} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

const request = new Request('mymethod', {
    ping: true
});
const response = await rockets.request(request);
console.log(response);
```

Handle a request error:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

try {
    await rockets.request('mymethod');
} catch (err) {
    console.log(err.code);
    console.log(err.message);
    console.log(err.data);
}
```

**NOTE**: Any error that may occur will be a `JsonRpcError`.

Cancel a request:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

const task = rockets.request('mymethod');
task.cancel();
```

Get progress updates for a request:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

const task = rockets.request('mymethod');
task.on('progress')
    .subscribe(progress => {
        console.log(progress);
    });
```

#### Batching
Make a batch request:
```ts
import {Client, Notification, Request} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

const request = new Request('mymethod');
const notification = new Notification('mymethod');
const [response] = await rockets.batch(...[
    request,
    notification
]);

const result = await response.json();
console.log(result);
```

**NOTE**: Notifications will not return any responses and if no requests are sent, the batch request will resolve without any arguments.

Handle a batch request error:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

try {
    const request = new Request('mymethod');
    await rockets.batch(request);
} catch (err) {
    console.log(err.code);
    console.log(err.message);
    console.log(err.data);
}
```

**NOTE**: The batch promise will not reject for response errors, but you can catch the response error when using the response `.json()` method:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

try {
    const request = new Request('mymethod');
    const [response] = await rockets.batch(request);
    await response.json();
} catch (err) {
    console.log(err.code);
    console.log(err.message);
    console.log(err.data);
}
```

Cancel a batch request:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

const request = new Request('mymethod');
const task = rockets.batch(request);

task.cancel();
```

**NOTE**: Notifications cannot be canceled.

Get progress updates for a batch request:
```ts
import {Client} from 'rockets-client';

const rockets = new Client({url: 'myhost'});

const request = new Request('mymethod');
const task = rockets.batch(request);

task.on('progress')
    .subscribe(progress => {
        console.log(progress);
    });
```


### Release
-----------
If you're a contributor and wish to make a release of this package use:
```bash
# Cut a minor release
# A release can be: patch, minor, major;
yarn release --release-as minor
```

See [release as a target type imperatively like npm-version](https://github.com/conventional-changelog/standard-version#release-as-a-target-type-imperatively-like-npm-version) for more release options.

After the changes land in master, the new version will be automatically published by the CI.

**IMPORTANT**: Follow the [semver](https://semver.org) for versioning.


### Learning Material
---------------------
* [JSON RPC 2.0](https://www.jsonrpc.org/specification)
* [RxJs](http://reactivex.io/rxjs)
