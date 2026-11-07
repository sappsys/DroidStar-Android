#!/bin/sh
# The published APK is the same release-signed build as a local install.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
exec "$root/scripts/build-android.sh"
