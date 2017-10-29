//
// Created by ccsexyz on 17-5-5.
//

#include "snappy_stream.h"
#include "snappy.h"

static const unsigned char magic_head[] = {0xff, 0x06, 0x00, 0x00, 0x73,
                                           0x4e, 0x61, 0x50, 0x70, 0x59};

snappy_stream_writer::snappy_stream_writer(asio::io_service &io_service,
                                           OutputHandler handler)
    : service_(io_service), AsyncInOutputer(handler) {
    memcpy(buf_, magic_head, snappy_magic_head_len);
    off_ = snappy_magic_head_len;
}

void snappy_stream_writer::async_input(char *buf, std::size_t len,
                                       Handler handler) {
    auto self = shared_from_this();
    if (len > snappy_max_block_size) {
        task_.buf = buf + snappy_max_block_size;
        task_.len = len - snappy_max_block_size;
        len -= snappy_max_block_size;
        task_.handler = handler;
        handler = nullptr;
    }
    auto chksum = crc32c_cast(0, (const unsigned char *)buf, len);
    encode32u((byte *)(buf_ + off_ + snappy_header_len), chksum);
    std::size_t compressed_length;
    char *payload = buf_ + off_ + snappy_header_len + snappy_checksum_size;
    snappy::RawCompress(buf, len, payload, &compressed_length);
    std::size_t length;
    if (len > compressed_length) {
        buf_[off_] = 0x0;
        length = compressed_length + snappy_checksum_size;
    } else {
        buf_[off_] = 0x1;
        length = len + snappy_checksum_size;
        memcpy(payload, buf, len);
    }
    buf_[off_ + 1] = length & 0xFF;
    buf_[off_ + 2] = (length >> 8) & 0xFF;
    buf_[off_ + 3] = (length >> 16) & 0xFF;
    off_ += snappy_header_len + length;
    output(buf_, off_, [this, self, handler](std::error_code ec, std::size_t) {
        off_ = 0;
        if (handler) {
            handler(ec, 0);
        }
        if (task_.check()) {
            auto b = task_.buf;
            auto l = task_.len;
            auto h = task_.handler;
            task_.reset();
            async_input(b, l, h);
        }
    });
}

void snappy_stream_reader::async_input(char *buf, std::size_t len,
                                       Handler handler) {
    auto self = shared_from_this();
    if (off_ < snappy_header_len) {
        if (len + off_ < snappy_header_len) {
            memcpy(chunk_ + off_, buf, len);
            off_ += len;
            if (handler) {
                handler(std::error_code(0, std::generic_category()), len);
            }
            return;
        }
        auto n = snappy_header_len - off_;
        memcpy(chunk_ + off_, buf, n);
        len -= n;
        buf += n;
        off_ += n;
        if (len == 0) {
            if (handler) {
                handler(std::error_code(0, std::generic_category()), len);
            }
            return;
        }
    }
    auto chunk_type = uint8_t(chunk_[0]);
    uint32_t chunk_len = uint32_t(uint8_t(chunk_[1])) |
                         (uint32_t(uint8_t(chunk_[2])) << 8) |
                         (uint32_t(uint8_t(chunk_[3])) << 16);
    if (!valid_) {
        if (chunk_type == chunk_type_stream_identifier &&
            chunk_len == snappy_magic_head_len - snappy_header_len &&
            off_ < snappy_magic_head_len) {
            if (len + off_ < snappy_magic_head_len) {
                memcpy(chunk_ + off_, buf, len);
                off_ += len;
                if (handler) {
                    handler(std::error_code(0, std::generic_category()), len);
                }
                return;
            }
            auto n = snappy_magic_head_len - off_;
            memcpy(chunk_ + off_, buf, n);
            len -= n;
            buf += n;
            off_ += n;
            for (int i = 0; i < snappy_magic_head_len; i++) {
                if (magic_head[i] != uint8_t(chunk_[i])) {
                    if (handler) {
                        handler(std::error_code(1, std::generic_category()),
                                len);
                    }
                    return;
                }
            }
            off_ = 0;
            valid_ = true;
            if (len == 0) {
                if (handler) {
                    handler(std::error_code(0, std::generic_category()), len);
                }
                return;
            }
            async_input(buf, len, handler);
            return;
        } else {
            if (handler) {
                handler(std::error_code(0, std::generic_category()), len);
            }
            return;
        }
    }
    if (off_ > snappy_header_len + chunk_len) {
        std::terminate();
    }
    if (len + off_ < snappy_header_len + chunk_len) {
        memcpy(chunk_ + off_, buf, len);
        off_ += len;
        if (handler) {
            handler(std::error_code(0, std::generic_category()), len);
        }
        return;
    }
    auto n = snappy_header_len + chunk_len - off_;
    memcpy(chunk_ + off_, buf, n);
    len -= n;
    buf += n;
    off_ += n;
    uint32_t crc32_chksum_2;
    decode32u((byte *)(chunk_ + snappy_header_len), &crc32_chksum_2);
    if (chunk_type == chunk_type_uncompressed_data) {
        uint32_t crc32_chksum =
            crc32c_cast(0, (unsigned char *)(chunk_ + snappy_header_len +
                                             snappy_checksum_size),
                        chunk_len - 4);
        if (crc32_chksum != crc32_chksum_2) {
            if (handler) {
                handler(std::error_code(1, std::generic_category()), len);
            }
            return;
        }
        output(
            chunk_ + snappy_header_len + snappy_checksum_size,
            off_ - (snappy_header_len + snappy_checksum_size),
            [this, self, buf, len, handler](std::error_code ec, std::size_t) {
                off_ = 0;
                if (ec) {
                    if (handler) {
                        handler(ec, len);
                    }
                    return;
                }
                if (len == 0) {
                    if (handler) {
                        handler(std::error_code(0, std::generic_category()),
                                len);
                    }
                    return;
                }
                async_input(buf, len, handler);
            });
    } else if (chunk_type == chunk_type_compressed_data) {
        char *compressed = chunk_ + snappy_header_len + snappy_checksum_size;
        std::size_t compressed_length =
            off_ - (snappy_header_len + snappy_checksum_size);
        std::size_t uncompressed_length;
        snappy::GetUncompressedLength(compressed, compressed_length,
                                      &uncompressed_length);
        snappy::RawUncompress(compressed, compressed_length, decode_buffer_);
        uint32_t crc32_chksum = crc32c_cast(
            0, (unsigned char *)(decode_buffer_), uncompressed_length);
        if (crc32_chksum != crc32_chksum_2) {
            if (handler) {
                handler(std::error_code(1, std::generic_category()), len);
            }
            return;
        }
        output(
            decode_buffer_, uncompressed_length,
            [this, self, buf, len, handler](std::error_code ec, std::size_t) {
                off_ = 0;
                if (ec) {
                    if (handler) {
                        handler(ec, len);
                    }
                    return;
                }
                if (len == 0) {
                    if (handler) {
                        handler(std::error_code(0, std::generic_category()),
                                len);
                    }
                    return;
                }
                async_input(buf, len, handler);
            });
    }
    if (chunk_type <= 0x7f) {
        if (handler) {
            handler(std::error_code(1, std::generic_category()), len);
        }
        return;
    }
    if (len == 0) {
        if (handler) {
            handler(std::error_code(0, std::generic_category()), len);
        }
        return;
    }
    async_input(buf, len, handler);
}
