#!/usr/bin/env bash
# Generate keys for the ML-KEM and ML-DSA tests.
cd $(dirname "$0")
mkdir -p keys

# ML-KEM keys
for size in 512 768 1024; do
    openssl genpkey -quiet -algorithm ML-KEM-$size \
            -out keys/mlkem-$size-prv.pem -outpubkey keys/mlkem-$size-pub.pem -outform PEM 
done

# ML-DSA keys
for size in 44 65 87; do
    openssl genpkey -quiet -algorithm ML-DSA-$size \
            -out keys/mldsa-$size-prv.pem -outpubkey keys/mldsa-$size-pub.pem -outform PEM 
done

# SLH-DSA keys
for speed in s f; do
    for size in 128 192 256; do
        for hash in SHA2 SHAKE; do
            algo=$hash-$size$speed
            file=$(tr <<<$algo A-Z a-z)
            openssl genpkey -quiet -algorithm SLH-DSA-$algo \
                    -out keys/slhdsa-$file-prv.pem -outpubkey keys/slhdsa-$file-pub.pem -outform PEM 
        done
    done
done
