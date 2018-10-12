# Rockets

> A library for easy HTTP and websockets messaging in C++ applications.

[![Travis CI](https://img.shields.io/travis/BlueBrain/Rockets/master.svg?style=flat-square)](https://travis-ci.org/BlueBrain/Rockets)


# Table of Contents

* [Features](#features)
* [Build](#build)
* [Clients](#clients)
* [Contribute](#contribute)


### Features
------------
It provides the following features:

* HTTP server with integrated websockets support
* HTTP client for making simple asynchronous requests (with or without payload)
* Websockets client for sending, broadcasting and receiving text and binary
  messages


### Build
---------
Rockets is a cross-platform library designed to run on any modern operating
system, including all Unix variants. It requires a C++11 compiler and uses CMake
to create a platform specific build environment. The following platforms and
build environments are tested:

* Linux: Ubuntu 16.04, RHEL 6.8
* Mac OS X: 10.9

Rockets requires the following external, pre-installed dependencies:

* libwebsockets
* Boost (for unit tests)

Building from source is as simple as:
```shell
git clone --recursive https://github.com/BlueBrain/Rockets.git
mkdir Rockets/build
cd Rockets/build
cmake -GNinja ..
ninja
```


### Clients
-----------
Rockets provides client libraries for the following languages:

* [Python](./python/README.md)
* [JavaScript](./js/README.md)


### Contribute
--------------
Follow the next guidelines when making a contribution:

* Use [Conventional Commits](https://www.conventionalcommits.org) spec whenever making a commit
* Keep PRs to a single feature or fix
