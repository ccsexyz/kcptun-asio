#include "async_fec.h"
#include "config.h"
#include "fec.h"

AsyncFECInputer::AsyncFECInputer(OutputHandler o)
    : AsyncInOutputer(o),
      fec_(my_make_unique<FEC>(
          FEC::New(3 * (FLAGS_datashard + FLAGS_parityshard), FLAGS_datashard, FLAGS_parityshard))) {}

void AsyncFECInputer::output_recovered(
    std::size_t len, std::shared_ptr<std::vector<row_type>> recovered,
    Handler handler) {
    if (!recovered || recovered->size() == 0) {
        if (handler) {
            handler(std::error_code(0, std::generic_category()), len);
        }
        return;
    }
    auto first = recovered->begin();
    auto r = *first;
    recovered->erase(first);
    if (r->size() <= 2) {
        output_recovered(len, recovered, handler);
        return;
    }
    auto ptr = r->data();
    uint16_t sz;
    decode16u(ptr, &sz);
    if (sz < 2 || sz > r->size()) {
        output_recovered(len, recovered, handler);
        return;
    }
    output((char *)(ptr + 2), sz - 2,
           [this, len, recovered, handler](std::error_code ec, std::size_t) {
               if (ec) {
                   if (handler) {
                       handler(ec, len);
                   }
                   return;
               }
               output_recovered(len, recovered, handler);
           });
}

void AsyncFECInputer::async_input(char *buf, std::size_t len, Handler handler) {
    auto pkt = fec_->Decode((byte *)buf, len);
    if (pkt.flag != typeData && pkt.flag != typeFEC) {
        return;
    }
    auto f = [pkt, handler, len, this](std::error_code ec,
                                       std::size_t) mutable {
        if (ec) {
            if (handler) {
                handler(ec, len);
            }
            return;
        }
        auto recovered =
            std::make_shared<std::vector<row_type>>(fec_->Input(pkt));
        output_recovered(len, recovered, handler);
    };
    if (pkt.flag == typeData) {
        auto ptr = pkt.data->data();
        output((char *)(ptr + 2), pkt.data->size() - 2, f);
    } else {
        f(std::error_code(0, std::generic_category()), len);
    }
}

static Buffers fec_buffers(2048);

static Buffers fec_header_buffers;

static char *get_fec_header() {
    static ConstructCaller nopCaller([](){
        fec_header_buffers.reset(fecHeaderSize*FLAGS_parityshard);
    });
    return fec_header_buffers.get();
}

static void push_fec_header_back(char *buf) {
    fec_header_buffers.push_back(buf);
}



AsyncFECOutputer::AsyncFECOutputer(OutputHandler o)
    : AsyncInOutputer(o),
      fec_(my_make_unique<FEC>(
          FEC::New(3 * (FLAGS_datashard + FLAGS_parityshard), FLAGS_datashard, FLAGS_parityshard))),
      shards_(my_make_unique<std::vector<row_type>>(FLAGS_datashard + FLAGS_parityshard,
                                                      nullptr)) {}

void AsyncFECOutputer::async_input(char *buf, std::size_t len,
                                   Handler handler) {
    memcpy(buf_ + fecHeaderSizePlus2, buf, len);
    fec_->MarkData(buf_, len + fecHeaderSizePlus2);
    auto slen = len + 2;
    assert(pkt_idx_ < (FLAGS_datashard + FLAGS_parityshard));
    (*shards_)[pkt_idx_] = std::make_shared<std::vector<byte>>(
        &(buf_[fecHeaderSize]), &(buf_[fecHeaderSize + slen]));
    pkt_idx_++;
    if (pkt_idx_ < FLAGS_datashard) {
        output((char *)buf_, len + fecHeaderSizePlus2,
               [this, len, handler](std::error_code ec, std::size_t) {
                   if (handler) {
                       handler(ec, len);
                   }
                   return;
               });
        return;
    }
    pkt_idx_ = 0;
    fec_->Encode(*shards_);
    char *buffer = fec_buffers.get();
    std::vector<row_type> shards(FLAGS_parityshard, nullptr);
    char *fec_headers = get_fec_header();
    for (int i = 0; i < FLAGS_parityshard; i++) {
        shards[i] = (*shards_)[FLAGS_datashard + i];
        (*shards_)[FLAGS_datashard + i] = nullptr;
        fec_->MarkFEC((byte *)(fec_headers + fecHeaderSize * i));
    }
    output((char *)buf_, len + fecHeaderSizePlus2,
           [this, len, handler, buffer, fec_headers, shards](std::error_code ec,
                                                             std::size_t) {
               DeferCaller defer([buffer, fec_headers, ec, handler, len] {
                   fec_buffers.push_back(buffer);
                   push_fec_header_back(fec_headers);
                   if (handler) {
                       handler(ec, len);
                   }
               });
               for (int i = 0; i < FLAGS_parityshard; i++) {
                   memcpy(buffer, fec_headers + i * fecHeaderSize,
                          fecHeaderSize);
                   memcpy(buffer + fecHeaderSize, shards[i]->data(),
                          shards[i]->size());
                   output(buffer, shards[i]->size() + fecHeaderSize, nullptr);
               }
           });
}