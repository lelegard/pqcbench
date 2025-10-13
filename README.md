# ML-KEM and ML-DSA Benchmarks

This project runs ML-KEM (CRYSTALS-Kyber) and ML-DSA (CRYSTALS-Dilithium) tests
on various CPU's using the OpenSSL cryptographic library.

The tested algorithms are ML-KEM-512, ML-KEM-768, ML-KEM-1024 (key encapsulation)
and ML-DSA-44, ML-DSA-65, ML-DSA-87 (signature).

The ML-KEM and ML-DSA keys were generated using the script `genkeys.sh`.

Note: equivalent [aesbench](https://github.com/lelegard/aesbench),
[shabench](https://github.com/lelegard/shabench),
[rsabench](https://github.com/lelegard/rsabench)
projects exist for AES, SHA hash functions, and RSA.

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

## OpenSSL support

OpenSSL supports ML-KEM and ML-DSA starting with version 3.5. All tests were
performed using version 3.6. When not available on the system, it was recompiled.

To download OpenSSL sources, recompile OpenSSL 3.6, build and run the `pqcbench`
test, run the following scripts:

~~~
./rebuild-openssl.sh
./make-with-rebuilt-openssl.ssh
./run-with-rebuilt-openssl.ssh
~~~
