#include "encrypt.h"
#include <cryptopp/aes.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/cast.h>
#include <cryptopp/chacha.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/des.h>
#include <cryptopp/crc.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/salsa.h>
#include <cryptopp/aes.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/cast.h>
#include <cryptopp/chacha.h>
#include <cryptopp/des.h>
#include <cryptopp/hex.h>
#include <cryptopp/salsa.h>
#include <cryptopp/pwdbased.h>

using CryptoPP::PKCS5_PBKDF2_HMAC;
using CryptoPP::SHA1;
using CryptoPP::AutoSeededRandomPool;
using CryptoPP::Exception;
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::ArraySink;
using CryptoPP::ArraySource;
using CryptoPP::StreamTransformationFilter;
using CryptoPP::HashFilter;
using CryptoPP::AES;
using CryptoPP::DES;
using CryptoPP::Blowfish;
using CryptoPP::CAST;
using CryptoPP::ChaCha20;
using CryptoPP::Salsa20;
using CryptoPP::CFB_Mode;
using CryptoPP::CRC32;

const byte iv[] = {167, 115, 79, 156, 18, 172, 27, 1, 164, 21, 242, 193, 252, 120, 230, 107};

std::string pbkdf2(std::string password) {
    byte salt[] = "kcp-go";
    size_t slen = strlen((const char *)salt);
    byte derived[32];
    PKCS5_PBKDF2_HMAC<CryptoPP::SHA1> pbk;
    pbk.DeriveKey(derived, sizeof(derived), 0, (const byte *)(password.c_str()), password.length(), salt, slen, 4096);
    return std::string((const char *)derived, 32);
}

template <typename T, const int keyLen, const int ivLen>
class Encrypter : public BaseEncrypter {
public:
    Encrypter(const std::string &pass) {
        assert(keyLen <= pass.length());
        assert(ivLen <= sizeof(iv));
        enc_.SetKeyWithIV((const byte *)(pass.c_str()), keyLen, iv, ivLen);
    }
    void encrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        ArraySource((byte *)dst, dlen, true, new StreamTransformationFilter(enc_, new ArraySink((byte *)src, slen)));
        return;
    }

private:
    typename CFB_Mode<T>::Encryption enc_;
};

template <typename T, const int keyLen, const int ivLen>
class Decrypter : public BaseDecrypter {
public:
    Decrypter(const std::string &pass) {
        assert(keyLen <= pass.length());
        assert(ivLen <= sizeof(iv));
        dec_.SetKeyWithIV((const byte *)(pass.c_str()), keyLen, iv, ivLen);
    }
    void decrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        ArraySource((byte *)dst, dlen, true, new StreamTransformationFilter(dec_, new ArraySink((byte *)src, slen)));
        return;
    }

private:
    typename CFB_Mode<T>::Decryption dec_;
};

void put_random_bytes(char *buffer, std::size_t length) {
    AutoSeededRandomPool prng;
    prng.GenerateBlock((byte *)buffer, length);
}

//template <const int keyLen, const int ivLen>
//class Decrypter<ChaCha20, keyLen, ivLen> : public BaseDecrypter {
//public:
//    Decrypter(const std::string &pwd) { key_ = evpBytesToKey(pwd, keyLen); }
//    void initIV(const std::string &iv) override {
//        dec_.SetKeyWithIV(
//            reinterpret_cast<const byte *>(key_.c_str()), key_.length(),
//            reinterpret_cast<const byte *>(iv.c_str()), iv.length());
//    }
//    std::size_t getIvLen() override { return ivLen; }
//    std::string decrypt(const std::string &str) override {
//        std::string plain;
//        StringSource(str, true, new StreamTransformationFilter(
//                                    dec_, new StringSink(plain)));
//        return plain;
//    }

//private:
//    std::string key_;
//    ChaCha20::Decryption dec_;
//};

//template <const int keyLen, const int ivLen>
//class Encrypter<Salsa20, keyLen, ivLen> : public BaseEncrypter {
//public:
//    Encrypter(const std::string &pwd) {
//        auto key = evpBytesToKey(pwd, keyLen);
//        AutoSeededRandomPool prng;
//        byte iv[ivLen];
//        prng.GenerateBlock(iv, ivLen);
//        iv_ = std::string(std::begin(iv), std::end(iv));
//        enc_.SetKeyWithIV(
//            reinterpret_cast<const byte *>(key.c_str()), key.length(),
//            reinterpret_cast<const byte *>(iv_.c_str()), iv_.length());
//    }
//    std::string getIV() override { return iv_; }
//    std::string encrypt(const std::string &str) override {
//        std::string secret;
//        StringSource(str, true, new StreamTransformationFilter(
//                                    enc_, new StringSink(secret)));
//        return secret;
//    }
//    std::size_t getIvLen() override { return ivLen; }

//private:
//    std::string iv_;
//    Salsa20::Encryption enc_;
//};

//template <const int keyLen, const int ivLen>
//class Decrypter<Salsa20, keyLen, ivLen> : public BaseDecrypter {
//public:
//    Decrypter(const std::string &pwd) { key_ = evpBytesToKey(pwd, keyLen); }
//    void initIV(const std::string &iv) override {
//        dec_.SetKeyWithIV(
//            reinterpret_cast<const byte *>(key_.c_str()), key_.length(),
//            reinterpret_cast<const byte *>(iv.c_str()), iv.length());
//    }
//    std::size_t getIvLen() override { return ivLen; }
//    std::string decrypt(const std::string &str) override {
//        std::string plain;
//        StringSource(str, true, new StreamTransformationFilter(
//                                    dec_, new StringSink(plain)));
//        return plain;
//    }

//private:
//    std::string key_;
//    Salsa20::Decryption dec_;
//};

std::unique_ptr<BaseEncrypter> getEncrypter(const std::string &method,
                                            const std::string &pwd) {
    if (method == "aes-128-cfb") {
        return std::move(std::make_unique<Encrypter<AES, 16, 16>>(pwd));
    } else if (method == "aes-192-cfb") {
        return std::move(std::make_unique<Encrypter<AES, 24, 16>>(pwd));
//    } else if (method == "des-cfb") {
//        return std::move(std::make_unique<Encrypter<DES, 8, 8>>(pwd));
//    } else if (method == "bf-cfb") {
//        return std::move(std::make_unique<Encrypter<Blowfish, 16, 8>>(pwd));
//    } else if (method == "chacha20") {
//        return std::move(std::make_unique<Encrypter<ChaCha20, 32, 8>>(pwd));
//    } else if (method == "salsa20") {
//        return std::move(std::make_unique<Encrypter<Salsa20, 32, 8>>(pwd));
    } else {
        return std::move(std::make_unique<Encrypter<AES, 32, 16>>(pwd));
    }
}

std::unique_ptr<BaseDecrypter> getDecrypter(const std::string &method,
                                            const std::string &pwd) {
    if (method == "aes-128-cfb") {
        return std::move(std::make_unique<Decrypter<AES, 16, 16>>(pwd));
    } else if (method == "aes-192-cfb") {
        return std::move(std::make_unique<Decrypter<AES, 24, 16>>(pwd));
//    } else if (method == "des-cfb") {
//        return std::move(std::make_unique<Decrypter<DES, 8, 8>>(pwd));
//    } else if (method == "bf-cfb") {
//        return std::move(std::make_unique<Decrypter<Blowfish, 16, 8>>(pwd));
//    } else if (method == "chacha20") {
//        return std::move(std::make_unique<Decrypter<ChaCha20, 32, 8>>(pwd));
//    } else if (method == "salsa20") {
//        return std::move(std::make_unique<Decrypter<Salsa20, 32, 8>>(pwd));
    } else {
        return std::move(std::make_unique<Decrypter<AES, 32, 16>>(pwd));
    }
}
