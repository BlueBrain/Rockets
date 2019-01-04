#!/bin/bash
set -e

brew update
brew outdated cmake || brew upgrade cmake
brew install ninja libwebsockets openssl
