#include "encrypt.h"
#include <cryptopp/aes.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/cast.h>
#include <cryptopp/chacha.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/des.h>
#include <cryptopp/crc.h>
#include <cryptopp/filters.h>
#include <cryptopp/twofish.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/salsa.h>
#include <cryptopp/aes.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/cast.h>
#include <cryptopp/des.h>
#include <cryptopp/hex.h>
#include <cryptopp/salsa.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/tea.h>

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
using CryptoPP::Salsa20;
using CryptoPP::CFB_Mode;
using CryptoPP::CRC32;
using CryptoPP::Twofish;
using CryptoPP::DES_EDE3;
using CryptoPP::XTEA;
using CryptoPP::CAST128;

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
class DecEncrypter : public BaseDecEncrypter {
public:
    DecEncrypter(const std::string &pass) : pass_(pass) {
        assert(keyLen <= pass.length());
        assert(ivLen <= sizeof(iv));
    }
    void encrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        typename CFB_Mode<T>::Encryption enc;
        enc.SetKeyWithIV((const byte *)(pass_.c_str()), keyLen, iv, ivLen);
        ArraySource((byte *)dst, dlen, true, new StreamTransformationFilter(enc, new ArraySink((byte *)src, slen)));
        return;
    }

    void decrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        typename CFB_Mode<T>::Decryption dec;
        dec.SetKeyWithIV((const byte *)(pass_.c_str()), keyLen, iv, ivLen);
        ArraySource((byte *)dst, dlen, true, new StreamTransformationFilter(dec, new ArraySink((byte *)src, slen)));
        return;
    }

private:
    std::string pass_;
};

void put_random_bytes(char *buffer, std::size_t length) {
    AutoSeededRandomPool prng;
    prng.GenerateBlock((byte *)buffer, length);
}

template <const int keyLen, const int ivLen>
class DecEncrypter<Salsa20, keyLen, ivLen> : public BaseDecEncrypter {
public:
    DecEncrypter(const std::string &pass) : pass_(pass) {
        assert(keyLen <= pass.length());
    }
    void encrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        Salsa20::Encryption enc;
        enc.SetKeyWithIV((const byte *)(pass_.c_str()), keyLen, (const byte *)src, ivLen);
        memmove(dst, src, ivLen);
        dst += ivLen;
        src += ivLen;
        dlen -= ivLen;
        slen -= ivLen;
        ArraySource((byte *)dst, dlen, true, new StreamTransformationFilter(enc, new ArraySink((byte *)src, slen)));
        return;
    }

    void decrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        Salsa20::Decryption dec;
        dec.SetKeyWithIV((const byte *)(pass_.c_str()), keyLen, (const byte *)src, ivLen);
        memmove(dst, src, ivLen);
        dst += ivLen;
        src += ivLen;
        dlen -= ivLen;
        slen -= ivLen;
        ArraySource((byte *)dst, dlen, true, new StreamTransformationFilter(dec, new ArraySink((byte *)src, slen)));
        return;
    }

private:
    std::string pass_;
};

class NoneDecEncrypter final : public BaseDecEncrypter {
public:
    void encrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        memmove(dst, src, slen);
    }
    void decrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        memmove(dst, src, slen);
    }
};

class XorBase {
public:
    XorBase(const std::string &pwd) {
        xortbl = new char[mtu_limit];
        byte salt[] = "sH3CIVoF#rWLtJo6";
        size_t slen = strlen((const char *)salt);
        PKCS5_PBKDF2_HMAC<CryptoPP::SHA1> pbk;
        pbk.DeriveKey((byte *)xortbl, mtu_limit, 0, (const byte *)(pwd.c_str()), pwd.length(), salt, slen, 32);
    }
    virtual ~XorBase() {
        delete[] xortbl;
    }
public:
    char *xortbl = nullptr;
};

class SimpleXorDecEncrypter final : public BaseDecEncrypter, public XorBase {
public:
    SimpleXorDecEncrypter(const std::string &pwd) : XorBase(pwd) {}
    void encrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        for(auto i = 0; i < slen; i++) {
            dst[i] = src[i] ^ xortbl[i];
        }
    }
    void decrypt(char *dst, std::size_t dlen, char *src, std::size_t slen) override {
        for(auto i = 0; i < slen; i++) {
            dst[i] = src[i] ^ xortbl[i];
        }
    }
};

std::unique_ptr<BaseDecEncrypter> getDecEncrypter(const std::string &method,
                                            const std::string &pwd) {
    if (method == "aes-128") {
        return std::move(std::make_unique<DecEncrypter<AES, 16, 16>>(pwd));
    } else if (method == "aes-192") {
        return std::move(std::make_unique<DecEncrypter<AES, 24, 16>>(pwd));
    } else if(method == "none") {
        return std::move(std::make_unique<NoneDecEncrypter>());
    } else if(method == "xor") {
        return std::move(std::make_unique<SimpleXorDecEncrypter>(pwd));
    } else if (method == "3des") {
        return std::move(std::make_unique<DecEncrypter<DES_EDE3, 24, 8>>(pwd));
    } else if (method == "blowfish") {
        return std::move(std::make_unique<DecEncrypter<Blowfish, 32, 8>>(pwd));
    } else if (method == "twofish") {
        return std::move(std::make_unique<DecEncrypter<Twofish, 32, 16>>(pwd));
    } else if (method == "salsa20") {
        return std::move(std::make_unique<DecEncrypter<Salsa20, 32, 8>>(pwd));
    } else if (method == "xtea") {
        return std::move(std::make_unique<DecEncrypter<XTEA, 16, 8>>(pwd));
    } else if (method == "cast5") {
        return std::move(std::make_unique<DecEncrypter<CAST128, 16, 8>>(pwd));
    } else {
        return std::move(std::make_unique<DecEncrypter<AES, 32, 16>>(pwd));
    }
}
