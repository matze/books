#!/bin/sh
autoreconf --force --install
intltoolize --force --copy --automake || return 1
./configure "$@"
