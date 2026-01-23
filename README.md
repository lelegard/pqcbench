# Post-Quantum Cryptography Benchmarks

This project runs PQC algorithms tests on various CPU's using the OpenSSL
cryptographic library.

The tested algorithms are:

- ML-KEM, Module-Lattice-Based Key Encapsulation Mechanism, aka CRYSTALS-Kyber
  - Sizes: 512, 768, 1024
- ML-DSA, Module-Lattice-Based Digital Signature Algorithm, aka CRYSTALS-Dilithium
  - Security levels: 44, 65, 87
- SLH-DSA, Stateless Hash-Based Digital Signature Algorithm, aka SPHINCS+
  - Associated hash: SHA-2, SHAKE
  - Sizes: 128, 192, 256
  - Variants: "s" (small), "f" (fast)

Note: this project is part of a series of cryptographic benchmarks:
- [aesbench](https://github.com/lelegard/aesbench) for AES
- [shabench](https://github.com/lelegard/shabench) for SHA-x hash functions
- [rsabench](https://github.com/lelegard/rsabench) for RSA
- [eccbench](https://github.com/lelegard/rsabench) for ECC (signature only)
- [pqcbench](https://github.com/lelegard/pqcbench) for PQC (ML-KEM, ML-DSA, SLH-DSA)

## Performance results

The performances are displayed and sorted in number of operations: key
encapsulation or decapsulation, signature generation or verification.

The results are summarized in file [RESULTS.txt](RESULTS.txt).
It is generated using the Python script `analyze.py`.

Two tables are provided:

- Number of operations per second.
- Number of operations per CPU cycle. This metrics is independent of the
  CPU frequency and demonstrates the quality of implementation.

In each table, the ranking of each CPU in the line is added between brackets.

## Key pairs generation

The key pairs in the subdirectory `keys` of this repository are used to run the
tests. The same keys are used on all platforms. These keys were generated using
the script `genkeys.sh`.

To view the content of a private or public key file, use the following commands:

~~~
openssl pkey -in keys/ALGO-prv.pem -text
openssl pkey -in keys/ALGO-pub.pem -pubin -text
~~~

## OpenSSL support

OpenSSL supports ML-KEM, ML-DSA, and SLH-DSA starting with version 3.5. All tests
were performed using version 3.6. When not available on the system, it was recompiled.

To download OpenSSL sources, recompile OpenSSL 3.6, build and run the `pqcbench` test,
run the following commands:

~~~
./rebuild-openssl.sh
source setenv-rebuilt-openssl.sh
make
build/pqcbench
~~~

The script `setenv-rebuilt-openssl.sh` is not executable. It must be source'd from
the calling shell. It sets (or unsets with option `-u`) the environment variables
`PATH` and `LD_LIBRARY_PATH` with the directories of the rebuilt OpenSSL.
