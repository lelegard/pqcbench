//----------------------------------------------------------------------------
// pqcbench - Copyright (c) 2025, Thierry Lelegard
// BSD 2-Clause License, see LICENSE file.
//----------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <cinttypes>
#include <unistd.h>
#include <sys/resource.h>

#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#if defined(__APPLE__)
    #include <libproc.h>
#endif

constexpr int64_t USECPERSEC = 1000000;  // microseconds per second
constexpr int64_t MIN_CPU_TIME = 3 * USECPERSEC;
constexpr size_t  INNER_LOOP_COUNT = 10;


//----------------------------------------------------------------------------
// Get current CPU time resource usage in microseconds.
//----------------------------------------------------------------------------

int64_t cpu_time()
{
    rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) < 0) {
        perror("getrusage");
        exit(EXIT_FAILURE);
    }
    return ((int64_t)(ru.ru_utime.tv_sec) * USECPERSEC) + ru.ru_utime.tv_usec +
           ((int64_t)(ru.ru_stime.tv_sec) * USECPERSEC) + ru.ru_stime.tv_usec;
}


//----------------------------------------------------------------------------
// OpenSSL error, abort application.
//----------------------------------------------------------------------------

[[noreturn]] void fatal(const std::string& message)
{
    if (!message.empty()) {
        std::cerr << "openssl: " << message << std::endl;
    }
    ERR_print_errors_fp(stderr);
    std::exit(EXIT_FAILURE);
}


//----------------------------------------------------------------------------
// Print entry for OpenSSL version.
//----------------------------------------------------------------------------

void print_openssl_version()
{
    std::cout << "openssl: "
#if defined(OPENSSL_FULL_VERSION_STRING) // v3
              << OpenSSL_version(OPENSSL_FULL_VERSION_STRING) << ", " << OpenSSL_version(OPENSSL_CPU_INFO)
#elif defined(OPENSSL_VERSION)
              << OpenSSL_version(OPENSSL_VERSION)
#else
              << OPENSSL_VERSION_TEXT
#endif
              << std::endl;
}


//----------------------------------------------------------------------------
// Get current executable path.
//----------------------------------------------------------------------------

std::string current_exec()
{
#if defined(__APPLE__)
    char name[PROC_PIDPATHINFO_MAXSIZE];
    int length = proc_pidpath(getpid(), name, sizeof(name));
    return length < 0 ? "" : std::string(name, length);
#else
    return std::filesystem::weakly_canonical("/proc/self/exe");
#endif
}


//----------------------------------------------------------------------------
// Get directory of keys. Abort on error.
//----------------------------------------------------------------------------

std::string keys_directory()
{
    const std::string exe(current_exec());
    std::string dir(exe);
    size_t sep = 0;

    while ((sep = dir.rfind('/')) != std::string::npos) {
        dir.resize(sep);
        const std::string keys(dir + "/keys");
        if (std::filesystem::is_directory(keys)) {
            return keys;
        }
    }

    fatal("cannot find 'keys' directory from " + exe);
}


//----------------------------------------------------------------------------
// Load one public or private key.
//----------------------------------------------------------------------------

EVP_PKEY* load_key(const char* filename, bool is_public)
{
    const std::string path(keys_directory() + "/" + filename + (is_public ? "-pub.pem" : "-prv.pem"));

    std::FILE* fp = nullptr;
    if ((fp = std::fopen(path.c_str(), "r")) == nullptr) {
        perror(path.c_str());
        std::exit(EXIT_FAILURE);
    }

    EVP_PKEY* key = is_public ?
        PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr) :
        PEM_read_PrivateKey(fp, nullptr, nullptr, nullptr);

    if (key == nullptr) {
        fatal("error loading key from " + path);
    }

    fclose(fp);
    return key;
}


//----------------------------------------------------------------------------
// Print one test result.
//----------------------------------------------------------------------------

void print_result(const char* name, uint64_t count, uint64_t duration)
{
    std::cout << name << "-microsec: " << duration << std::endl;
    std::cout << name << "-count: " << count << std::endl;
    std::cout << name << "-persec: " << ((USECPERSEC * count) / duration) << std::endl;    
}


//----------------------------------------------------------------------------
// Perform one ML-KEM test.
//----------------------------------------------------------------------------

void one_test_kem(const char* key_file)
{
    // Load private and public keys.
    EVP_PKEY* kpriv = load_key(key_file, false);
    EVP_PKEY* kpub = load_key(key_file, true);

    const size_t data_size = EVP_PKEY_get_size(kpriv);
    std::cout << "algo: " << EVP_PKEY_get0_type_name(kpriv) << std::endl;
    std::cout << "data-size: " << data_size << std::endl;

    // Initialize encapsulation.
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(kpub, nullptr);
    if (ctx == nullptr) {
        fatal("error in EVP_PKEY_CTX_new(public-key)");
    }
    if (EVP_PKEY_encapsulate_init(ctx, nullptr) <= 0) {
        fatal("error in EVP_PKEY_encapsulate_init");
    }

    // Encapsulation test.
    std::vector<uint8_t> key(data_size);
    std::vector<uint8_t> wrapped(data_size);
    size_t key_len = 0;
    size_t wrapped_len = 0;
    uint64_t count = 0;
    uint64_t duration = 0;
    uint64_t start = cpu_time();

    do {
        for (size_t i = 0; i < INNER_LOOP_COUNT; i++) {
            key_len = key.size();
            wrapped_len = wrapped.size();
            if (EVP_PKEY_encapsulate(ctx, wrapped.data(), &wrapped_len, key.data(), &key_len) <= 0) {
                fatal("error in EVP_PKEY_encapsulate");
            }
            count++;
        }
        duration = cpu_time() - start;
    } while (duration < MIN_CPU_TIME);

    // End of encapsulation test.
    std::cout << "key-size: " << key_len << std::endl;
    std::cout << "wrapped-size: " << wrapped_len << std::endl;
    print_result("encap", count, duration);
    EVP_PKEY_CTX_free(ctx);

    // Initialize decryption.
    if ((ctx = EVP_PKEY_CTX_new(kpriv, nullptr)) == nullptr) {
        fatal("error in EVP_PKEY_CTX_new(private-key)");
    }
    if (EVP_PKEY_decapsulate_init(ctx, nullptr) <= 0) {
        fatal("error in EVP_PKEY_decapsulate_init");
    }

    // Decapsulation test.
    std::vector<uint8_t> unwrapped(data_size);
    size_t unwrapped_len = 0;
    count = 0;
    duration = 0;
    start = cpu_time();

    do {
        for (size_t i = 0; i < INNER_LOOP_COUNT; i++) {
            unwrapped_len = unwrapped.size();
            if (EVP_PKEY_decapsulate(ctx, unwrapped.data(), &unwrapped_len, wrapped.data(), wrapped_len) <= 0) {
                fatal("EVP_PKEY_decapsulate");
            }
            count++;
        }
        duration = cpu_time() - start;
    } while (duration < MIN_CPU_TIME);

    // End of decapsulation test.
    std::cout << "unwrapped-size: " << unwrapped_len << std::endl;
    print_result("decap", count, duration);
    EVP_PKEY_CTX_free(ctx);

    // Check decapsulated data.
    if (unwrapped_len != key_len || memcmp(key.data(), unwrapped.data(), key_len) != 0) {
        fatal("decapsulated data don't match input");
    }

    // Free keys.
    EVP_PKEY_free(kpub);
    EVP_PKEY_free(kpriv);
}


//----------------------------------------------------------------------------
// Perform one ML-DSA test.
//----------------------------------------------------------------------------

void one_test_dsa(const char* key_file)
{
    // Load private and public keys.
    EVP_PKEY* kpriv = load_key(key_file, false);
    EVP_PKEY* kpub = load_key(key_file, true);

    const size_t data_size = EVP_PKEY_get_size(kpriv);
    std::cout << "algo: " << EVP_PKEY_get0_type_name(kpriv) << std::endl;
    std::cout << "data-size: " << data_size << std::endl;

    // Signature parameters.
    char contextstring[] = "0123456789ABCDEF";
    const OSSL_PARAM params[] = {
        OSSL_PARAM_octet_string("context-string", contextstring, sizeof(contextstring) - 1),
        OSSL_PARAM_END
    };
    EVP_SIGNATURE* sig_alg = EVP_SIGNATURE_fetch(nullptr, EVP_PKEY_get0_type_name(kpriv), nullptr);
    if (sig_alg == nullptr) {
        fatal("error in EVP_SIGNATURE_fetch");
    }

    // Initialize signature.
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(kpriv, nullptr);
    if (ctx == nullptr) {
        fatal("error in EVP_PKEY_CTX_new(private-key)");
    }

    // Signature test.
    std::vector<uint8_t> to_be_signed(data_size / 2, 0x5A);
    std::vector<uint8_t> signature(data_size);
    size_t signature_len = 0;
    uint64_t count = 0;
    uint64_t duration = 0;
    uint64_t start = cpu_time();

    do {
        for (size_t i = 0; i < INNER_LOOP_COUNT; i++) {
            signature_len = signature.size();
            if (EVP_PKEY_sign_message_init(ctx, sig_alg, params) <= 0) {
                fatal("error in EVP_PKEY_sign_message_init");
            }
            if (EVP_PKEY_sign(ctx, signature.data(), &signature_len, to_be_signed.data(), to_be_signed.size()) <= 0) {
                fatal("error in EVP_PKEY_sign");
            }
            count++;
        }
        duration = cpu_time() - start;
    } while (duration < MIN_CPU_TIME);

    // End of signature test.
    std::cout << "signature-size: " << signature_len << std::endl;
    print_result("sign", count, duration);
    EVP_PKEY_CTX_free(ctx);

    // Initialize signature verification.
    if ((ctx = EVP_PKEY_CTX_new(kpub, nullptr)) == nullptr) {
        fatal("error in EVP_PKEY_CTX_new(public-key)");
    }

    // Signature verification test.
    count = 0;
    duration = 0;
    start = cpu_time();

    do {
        for (size_t i = 0; i < INNER_LOOP_COUNT; i++) {
            if (EVP_PKEY_verify_message_init(ctx, sig_alg, params) <= 0) {
                fatal("error in EVP_PKEY_sign_message_init");
            }
            // Status: 1=verified, 0=not verified, <0 = error
            const int res = EVP_PKEY_verify(ctx, signature.data(), signature_len, to_be_signed.data(), to_be_signed.size());
            if (res <= 0) {
                fatal("RSA verify error");
            }
            count++;
        }
        duration = cpu_time() - start;
    } while (duration < MIN_CPU_TIME);

    // End of signature verification test.
    print_result("verify", count, duration);
    EVP_PKEY_CTX_free(ctx);

    // Free keys.
    EVP_SIGNATURE_free(sig_alg);
    EVP_PKEY_free(kpub);
    EVP_PKEY_free(kpriv);
}


//----------------------------------------------------------------------------
// Application entry point
//----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // OpenSSL initialization.
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    print_openssl_version();

    // Run tests.
    one_test_kem("mlkem-512");
    one_test_kem("mlkem-768");
    one_test_kem("mlkem-1024");
    one_test_dsa("mldsa-44");
    one_test_dsa("mldsa-65");
    one_test_dsa("mldsa-87");
    one_test_dsa("slhdsa-sha2-128f");
    one_test_dsa("slhdsa-sha2-128s");
    one_test_dsa("slhdsa-sha2-192f");
    one_test_dsa("slhdsa-sha2-192s");
    one_test_dsa("slhdsa-sha2-256f");
    one_test_dsa("slhdsa-sha2-256s");
    one_test_dsa("slhdsa-shake-128f");
    one_test_dsa("slhdsa-shake-128s");
    one_test_dsa("slhdsa-shake-192f");
    one_test_dsa("slhdsa-shake-192s");
    one_test_dsa("slhdsa-shake-256f");
    one_test_dsa("slhdsa-shake-256s");

    // OpenSSL cleanup.
    EVP_cleanup();
    ERR_free_strings();
    return EXIT_SUCCESS;
}
