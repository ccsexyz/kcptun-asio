#ifndef KCPTUN_FRAME_H
#define KCPTUN_FRAME_H

#include "utils.h"

enum { cmdSyn = 0, cmdFin = 1, cmdPsh = 2, cmdNop = 3 };

enum { VERSION = 1 };

enum {
    sizeOfVer = 1,
    sizeOfCmd = 1,
    sizeOfLength = 2,
    sizeOfSid = 4,
    headerSize = sizeOfVer + sizeOfCmd + sizeOfSid + sizeOfLength
};

struct frame final {
    uint8_t version;
    uint8_t cmd;
    uint16_t length;
    uint32_t id;
    char *data;

    static frame unmarshal(const char *b) {
        frame f;
        f.version = b[0];
        f.cmd = b[1];
        decode16u((byte *)(b + 2), &(f.length));
        decode32u((byte *)(b + 4), &(f.id));
        return f;
    }
    void marshal(char *b) {
        b[0] = version;
        b[1] = cmd;
        encode16u((byte *)(b + 2), length);
        encode32u((byte *)(b + 4), id);
    }
};

#endif
