#!/usr/bin/env bash

export OSSLROOT=$(cd $(dirname "$BASH_SOURCE[0]"); pwd)/.openssl/usr/local
export PATH="$OSSLROOT/bin:$PATH"
export LD_LIBRARY_PATH="$OSSLROOT/lib64:$OSSLROOT/lib:$LD_LIBRARY_PATH"
