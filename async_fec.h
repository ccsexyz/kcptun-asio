#ifndef KCPTUN_ASYNC_FEC
#define KCPTUN_ASYNC_FEC

#include "utils.h"
#include "fec.h"

class AsyncFECInputer : public AsyncInOutputer {
public:
    AsyncFECInputer(OutputHandler o = nullptr);
    void async_input(char *buf, std::size_t len, Handler handler) override;
    void output_recovered(std::size_t len, std::shared_ptr<std::vector<row_type>> recovered, Handler handler);

private:
    std::unique_ptr<FEC> fec_;
};

class AsyncFECOutputer : public AsyncInOutputer {
public:
    AsyncFECOutputer(OutputHandler o = nullptr);
    void async_input(char *buf, std::size_t len, Handler handler) override;

private:
    byte buf_[2048];
    uint32_t pkt_idx_ = 0;
    std::unique_ptr<FEC> fec_;
    std::unique_ptr<std::vector<row_type>> shards_;
};

#endif
