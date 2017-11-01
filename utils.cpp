#include "utils.h"

void Buffers::push_back(char *buf) {
    bufs_.insert(buf);
    auto s1 = bufs_.size();
    auto s2 = all_bufs_.size();
//    info("bufs: %ld, all_bufs: %ld", s1, s2);
    if (s1 * 4 > s2 * 3 && s2 > 16) {
        std::vector<char *> v;
        auto it = 0;
        for (auto buf : bufs_) {
            if (++it > s1 / 2) {
                break;
            }
            v.push_back(buf);
        }
        for (auto buf : v) {
            bufs_.erase(buf);
            all_bufs_.erase(buf);
            free(buf);
        }
    }
}

char *Buffers::get() {
    for (auto buf : bufs_) {
        bufs_.erase(buf);
        return buf;
    }
    char *buf = static_cast<char *>(malloc(n));
    all_bufs_.insert(buf);
    return buf;
}

static std::unordered_map<std::string, int *> kvars;
static std::unordered_map<std::string, int> kvarsRef;

kvar::kvar(const std::string &name) {
    auto it = kvars.find(name);
    if (it == kvars.end()) {
        p = new int(0);
        kvars.insert(std::make_pair(name, p));
        kvarsRef.insert(std::make_pair(name, 1));
    } else {
        p = kvars[name];
        kvarsRef[name]++;
    }
    name_ = name;
}

kvar::~kvar() {
    kvarsRef[name_]--;
    if (kvarsRef[name_] > 0) {
        return;
    }
    kvars.erase(name_);
    kvarsRef.erase(name_);
    delete p;
}

void printKvars() {
    for (auto &kvar : kvars) {
        auto name = kvar.first;
        if (kvar.second == nullptr) {
            continue;
        }
        auto value = *(kvar.second);
        LOG(INFO) << "name:" << name << " value:" << value;
    }
}

void run_kvar_printer(asio::io_service &service) {
    auto timer = std::make_shared<asio::high_resolution_timer>(service, std::chrono::seconds(1));
    timer->async_wait([&service, timer](const std::error_code &ec) {
        if (ec) {
            return;
        }
        printKvars();
        run_kvar_printer(service);
    });
}

static Buffers buffersCache(4096);

buffer::buffer() {
    init();
}

buffer::buffer(buffer &&other) noexcept {
    buf = other.buf;
    off = other.off;
    len = other.len;
    cap = other.cap;
    other.buf = nullptr;
    other.off = 0;
    other.len = 0;
    other.cap = 0;
}

buffer::~buffer() {
    if (buf == nullptr) {
        return;
    }
    buffersCache.push_back(buf);
}

void buffer::init() {
    if (buf != nullptr) {
        buffersCache.push_back(buf);
    }
    buf = buffersCache.get();
    cap = 4096;
    off = 0;
    len = 0;
}

LinearBuffer::LinearBuffer() {}

LinearBuffer::~LinearBuffer() {}

std::size_t LinearBuffer::size() {
    auto bufsSize = bufs_.size();
    if (bufsSize == 0) {
        return 0;
    } else if (bufsSize == 1) {
        return bufs_.front().size();
    } else {
        std::size_t n = 0;
        n = bufs_.front().size();
        n += bufs_.back().size();
        n += (bufsSize - 2) * 4096;
        return n;
    }
}

void LinearBuffer::append(char *buf, std::size_t len) {
    if (len == 0 || buf == nullptr) {
        return;
    }
    if (bufs_.empty() || bufs_.back().aval() == 0) {
        bufs_.emplace_back(buffer());
    }
    while (len > 0) {
        auto aval = bufs_.back().aval();
        if (aval >= len) {
            bufs_.back().append(buf, len);
            return;
        }
        bufs_.back().append(buf, aval);
        len -= aval;
        buf += aval;
        bufs_.emplace_back(buffer());
    }
}

void LinearBuffer::retrieve(char *buf, std::size_t len) {
    while (len > 0 && !bufs_.empty()) {
        auto sz = bufs_.front().size();
        if (sz > len) {
            bufs_.front().retrieve(buf, len);
            return;
        }
        bufs_.front().retrieve(buf, sz);
        len -= sz;
        buf += sz;
        bufs_.pop_front();
    }
}