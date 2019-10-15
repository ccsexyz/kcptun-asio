#ifndef SHADOWSOCKS_ASIO_ENCRYPT_H
#define SHADOWSOCKS_ASIO_ENCRYPT_H

#include "utils.h"

std::string pbkdf2(std::string password);

class BaseDecEncrypter {
public:
    virtual ~BaseDecEncrypter() = default;
    virtual void encrypt(char *dst, std::size_t dlen, char *src,
                         std::size_t slen) = 0;
    virtual void decrypt(char *dst, std::size_t dlen, char *src,
                         std::size_t slen) = 0;
};

class AsyncDecrypter : public AsyncInOutputer {
public:
    AsyncDecrypter(std::unique_ptr<BaseDecEncrypter> &&dec, OutputHandler o = nullptr)
        : AsyncInOutputer(o), dec_(std::move(dec)) {}
    void async_input(char *buf, std::size_t len, Handler handler) override {
        dec_->decrypt(buf, len, buf, len);
        output(buf, len, handler);
    }

private:
    std::unique_ptr<BaseDecEncrypter> dec_;
};

class AsyncEncrypter : public AsyncInOutputer {
public:
    AsyncEncrypter(std::unique_ptr<BaseDecEncrypter> &&enc, OutputHandler o = nullptr)
        : AsyncInOutputer(o), enc_(std::move(enc)) {}
    void async_input(char *buf, std::size_t len, Handler handler) override {
        enc_->encrypt(buf, len, buf, len);
        output(buf, len, handler);
    }

private:
    std::unique_ptr<BaseDecEncrypter> enc_;
};

std::unique_ptr<BaseDecEncrypter> getDecEncrypter(const std::string &method,
                                                  const std::string &pwd);

void put_random_bytes(char *buffer, std::size_t length);

static inline std::shared_ptr<AsyncEncrypter>
getAsyncEncrypter(std::unique_ptr<BaseDecEncrypter> &&enc,
                  OutputHandler handler) {
    return std::make_shared<AsyncEncrypter>(std::move(enc), handler);
}

static inline std::shared_ptr<AsyncDecrypter>
getAsyncDecrypter(std::unique_ptr<BaseDecEncrypter> &&dec,
                  OutputHandler handler) {
    return std::make_shared<AsyncDecrypter>(std::move(dec), handler);
}

uint32_t crc32c_cast(const unsigned char *buf, size_t len);

#endif // SHADOWSOCKS_ASIO_ENCRYPT_H
