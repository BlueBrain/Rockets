#!/bin/bash
set -e

REMOTE_VERSION=$(npm show rockets-client version)
# https://gist.github.com/DarrenN/8c6a5b969481725a4413
LOCAL_VERSION=$(node -p "require('./js/package.json').version")

echo "Remote version: ${REMOTE_VERSION}"
echo "Local version: ${LOCAL_VERSION}"

if [ "${REMOTE_VERSION}" != "${LOCAL_VERSION}" ]; then
    export ROCKETS_CLIENT_VERSION=${LOCAL_VERSION}
    echo "Set version: ${LOCAL_VERSION}"
fi
