#include "utils.h"

void Buffers::push_back(char *buf) {
    bufs_.insert(buf);
    auto s1 = bufs_.size();
    auto s2 = all_bufs_.size();
//    info("bufs: %ld, all_bufs: %ld", s1, s2);
    if(s1 * 4 > s2 * 3 && s2 > 16) {
        std::vector<char *> v;
        auto it = 0;
        for(auto buf : bufs_) {
            if(++it > s1 / 2) {
                break;
            }
            v.push_back(buf);
        }
        for(auto buf : v) {
            bufs_.erase(buf);
            all_bufs_.erase(buf);
            free(buf);
        }
    }
}

char *Buffers::get() {
    for(auto buf : bufs_) {
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
    for(auto &kvar : kvars) {
        auto name = kvar.first;
        if (kvar.second == nullptr) {
            continue;
        }
        auto value = *(kvar.second);
        info("name:%s\tvalue:%d\n", name.data(), value);
    }
}