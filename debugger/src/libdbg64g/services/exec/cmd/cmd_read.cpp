/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Read/write memory.
 */

#include "cmd_read.h"

namespace debugger {

CmdRead::CmdRead(ITap *tap, ISocInfo *info) 
    : ICommand ("read", tap, info) {

    briefDescr_.make_string("Read memory");
    detailedDescr_.make_string(
        "Description:\n"
        "    32-bits aligned memory reading. Default bytes = 4 bytes.\n"
        "Usage:\n"
        "    read <addr> <bytes>\n"
        "Example:\n"
        "    read 0xfffff004 16\n"
        "    read 0xfffff004\n");

    rdData_.make_data(1024);
}

CmdRead::~CmdRead() {
}

bool CmdRead::isValid(AttributeType *args) {
    if ((*args)[0u].is_equal(cmdName_.to_string()) 
     && (args->size() == 2 || args->size() == 3)) {
        return CMD_VALID;
    }
    return CMD_INVALID;
}

bool CmdRead::exec(AttributeType *args, AttributeType *res) {
    res->make_uint64(~0);
    if (!isValid(args)) {
        return CMD_FAILED;
    }

    uint64_t addr = (*args)[1].to_uint64();
    unsigned bytes = 4;
    if (args->size() == 3) {
        bytes = static_cast<unsigned>((*args)[2].to_uint64());
    }
    /** Use 4 x Buffer size to store formatted output */
    if (4 * bytes > rdData_.size()) {
        rdData_.make_data(4 * bytes);
    }

    tap_->read(addr, bytes, rdData_.data());

    res->make_data(bytes, rdData_.data());
    return CMD_SUCCESS;
}

bool CmdRead::format(AttributeType *args, AttributeType *res, AttributeType *out) {
    uint64_t addr = (*args)[1].to_uint64();
    int bytes = 4;
    if (args->size() == 3) {
        bytes = static_cast<int>((*args)[2].to_uint64());
    }

    const uint64_t MSK64 = 0x7ull;
    uint64_t addr_start, addr_end, inv_i;
    addr_start = addr & ~MSK64;
    addr_end = (addr + bytes + 7) & ~MSK64;

    int strsz = 0;
    int rdBufSz = static_cast<int>(rdData_.size());
    char *strbuf = reinterpret_cast<char *>(rdData_.data());
    for (uint64_t i = addr_start; i < addr_end; i++) {
        if ((i & MSK64) == 0) {
            // Output address:
            strsz += RISCV_sprintf(&strbuf[strsz], rdBufSz - strsz,
                                "[%016" RV_PRI64 "x]: ", i);
        }
        inv_i = (i & ~MSK64) | (MSK64 - (i & MSK64));
        if ((addr <= inv_i) && (inv_i < (addr + bytes))) {
            strsz += RISCV_sprintf(&strbuf[strsz], rdBufSz - strsz, " %02x",
                                    res->data()[inv_i - addr]);
        } else {
            strsz += RISCV_sprintf(&strbuf[strsz], rdBufSz - strsz, " ..");
        }
        if ((i & MSK64) == MSK64) {
            strsz += RISCV_sprintf(&strbuf[strsz], rdBufSz - strsz, "\n");
        }
    }
    out->make_string(strbuf);
    return CMD_IS_OUTPUT;
}

}  // namespace debugger