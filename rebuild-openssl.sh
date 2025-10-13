#!/usr/bin/env bash

# Create build environment.
cd $(dirname "$0")
mkdir -p .openssl
cd .openssl
ROOT=$(pwd)

# Get OpenSSL repo if necessary.
[[ -d openssl/.git ]] || git clone https://github.com/openssl/openssl.git

cd openssl
git checkout openssl-3.6.0 || exit 1
./config
make -j10
make install DESTDIR=$ROOT
