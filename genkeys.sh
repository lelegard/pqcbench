#!/usr/bin/env bash
# Generate keys for the ML-KEM and ML-DSA tests.
cd $(dirname "$0")
mkdir -p keys

for size in 512 768 1024; do
    openssl genpkey -quiet -algorithm ML-KEM-$size \
        -out keys/mlkem-$size-prv.pem -outpubkey keys/mlkem-$size-pub.pem -outform PEM 
done

for size in 44 65 87; do
    openssl genpkey -quiet -algorithm ML-DSA-$size \
        -out keys/mldsa-$size-prv.pem -outpubkey keys/mldsa-$size-pub.pem -outform PEM 
done
