# Rockets

> A library for easy HTTP and websockets messaging in C++ applications.

[![Travis CI](https://img.shields.io/travis/BlueBrain/Rockets/master.svg?style=flat-square)](https://travis-ci.org/BlueBrain/Rockets)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/BlueBrain/Rockets.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/BlueBrain/Rockets/context:cpp)


# Table of Contents

* [Features](#features)
* [Build](#build)
* [Usage](#usage)
* [Contribute](#contribute)
* [Funding & Acknowledgment](#Funding-&-Acknowledgment)


## Features
`Rockets` provides the following features:

* [HTTP server](rockets/server.h) with integrated websockets support
* [HTTP client](rockets/http/client.h) for making simple asynchronous requests (with or without payload)
* [Websockets client](rockets/ws/client.h) for sending, broadcasting and receiving text and binary messages
* Support for [JSON-RPC](https://www.jsonrpc.org) as a communication protocol over HTTP and websockets. `Rockets` extends the [2.0 specification](https://www.jsonrpc.org/specification) by providing support for [Cancellation and Progress notifications](rockets/jsonrpc/cancellableReceiver.h) of pending requests.
  * [Server](rockets/jsonrpc/server.h)
  * [C++ client](rockets/jsonrpc/client.h)
  * [Javascript client](js/README.md)
  * [Python client](python/README.md)
* Event loop integration for [Qt](rockets/qt) and `libuv` through the appropriate `Server` constructor


## Build
`Rockets` is a cross-platform library designed to run on any modern operating system, including all Unix variants.
It requires a C++11 compiler and uses CMake to create a platform specific build environment.
The following platforms and build environments are tested:

* Linux: Ubuntu 16.04, 18.04
* Mac OS X: 10.9 - 10.14

`Rockets` requires the following external, pre-installed dependencies:

* [libwebsockets](https://libwebsockets.org/)
* [Boost.test](https://www.boost.org/doc/libs/1_69_0/libs/test/doc/html/index.html) (for unit tests)

Building from source is as simple as:
```shell
git clone --recursive https://github.com/BlueBrain/Rockets.git
mkdir Rockets/build
cd Rockets/build
cmake -GNinja ..
ninja
```


## Usage

### Hello world REST server
```cpp
#include <rockets/server.h>
#include <iostream>

int main(int , char** )
{
    rockets::Server server;
    std::cout << "Rockets REST server running on " << server.getURI() << std::endl;

    server.handle(rockets::http::Method::GET, "hello", [](auto) {
        return rockets::http::make_ready_response(rockets::http::Code::OK, "world");
    });
    for(;;)
        server.process(100);
    return 0;
}
```

### Hello world websockets server
```cpp
#include <rockets/server.h>
#include <iostream>

int main(int , char** )
{
    rockets::Server server("", "myws");
    std::cout << "Rockets websockets server running on " << server.getURI() << std::endl;

    server.handleText([](const auto& request) {
        return "server echo: " + request.message;
    });
    for(;;)
        server.process(100);
    return 0;
}
```

### Production REST server example

For a more elaborate use of a `Rockets` REST server, check out the [RestServer](https://github.com/BlueBrain/Tide/blob/master/tide/master/rest/RestServer.h) of [Tide](https://github.com/BlueBrain/Tide).

### Production websockets server example

For a more elaborate use of a `Rockets` websockets server, check out the [RocketsPlugin](https://github.com/BlueBrain/Brayns/blob/master/plugins/Rockets/RocketsPlugin.cpp) of [Brayns](https://github.com/BlueBrain/Brayns).


## Contribute
Follow the next guidelines when making a contribution:

* Install our [pre-commit](https://pre-commit.com/#install) hooks with `pre-commit install` prior the first commit
* Use [Conventional Commits](https://www.conventionalcommits.org) spec whenever making a commit
* Keep PRs to a single feature or fix


## Funding & Acknowledgment
 
The development of this software was supported by funding to the Blue Brain Project,
a research center of the École polytechnique fédérale de Lausanne (EPFL), from the
Swiss government’s ETH Board of the Swiss Federal Institutes of Technology.
 
