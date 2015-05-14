#!/bin/bash

`dirname $0`/configure CFLAGS='-O0 -g -DDEBUG' $*
