#!/bin/bash
set -e

brew update
brew outdated cmake || brew upgrade cmake
brew install cppcheck doxygen ninja
brew install libwebsockets openssl
