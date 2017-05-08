//
// Created by ccsexyz on 17-5-5.
//

#ifndef KCPTUN_SNAPPY_STREAM_H
#define KCPTUN_SNAPPY_STREAM_H

#include "utils.h"

// snappy stream

enum {
    snappy_max_block_size = 65536,
    snappy_max_encoded_len_of_max_block_size = 76490,
    snappy_checksum_size = 4,
    snappy_header_len = 4,
    snappy_magic_head_len = 10,
    chunk_type_compressed_data = 0x0,
    chunk_type_uncompressed_data = 0x1,
    chunk_type_padding = 0xfe,
    chunk_type_stream_identifier = 0xff
};

class snappy_stream_reader final
    : public std::enable_shared_from_this<snappy_stream_reader>,
      public AsyncInOutputer {
public:
    snappy_stream_reader(asio::io_service &io_service, OutputHandler handler)
        : service_(io_service), AsyncInOutputer(handler) {}
    void async_input(char *buf, std::size_t len, Handler handler) override;

private:
    bool valid_ = false;
    Task task_;
    std::size_t off_;
    asio::io_service &service_;
    char chunk_[snappy_max_block_size + snappy_checksum_size +
                snappy_header_len];
    char decode_buffer_[snappy_max_block_size];
};

class snappy_stream_writer final
    : public std::enable_shared_from_this<snappy_stream_writer>,
      public AsyncInOutputer {
public:
    snappy_stream_writer(asio::io_service &io_service, OutputHandler handler);
    void async_input(char *buf, std::size_t len, Handler handler);

private:
    asio::io_service &service_;
    char buf_[snappy_max_block_size + snappy_checksum_size + snappy_header_len +
              snappy_magic_head_len];
    std::size_t off_;
    Task task_;
};

#endif // KCPTUN_SNAPPY_STREAM_H
