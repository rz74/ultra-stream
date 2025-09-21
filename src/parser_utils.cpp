#include "parser_utils.hpp"
#include <arpa/inet.h>
#include <endian.h>
#include <iostream>

bool parse_data_packet(const uint8_t* data, size_t len, ProcessedMessage& out_msg) {
    if (len < 10) return false; // Minimum: 4B len + 4B sid + 2B count

    size_t offset = 0;

    // Total message length (excluding this 4B header)
    uint32_t msg_len = ntohl(*reinterpret_cast<const uint32_t*>(data + offset));
    (void)msg_len; // Suppress unused variable warning - could be used for validation
    offset += 4;

    if (offset + 6 > len) return false;

    out_msg.subject_id = ntohl(*reinterpret_cast<const uint32_t*>(data + offset));
    offset += 4;

    uint16_t update_count = ntohs(*reinterpret_cast<const uint16_t*>(data + offset));
    offset += 2;

    out_msg.updates.clear();
    for (int i = 0; i < update_count; ++i) {
        if (offset + 14 > len) return false;

        DataLevel lvl;
        lvl.level = *(data + offset);
        offset += 1;
        lvl.side = *(data + offset);
        offset += 1;

        lvl.value = static_cast<int64_t>(be64toh(*reinterpret_cast<const uint64_t*>(data + offset)));
        offset += 8;

        lvl.volume = ntohl(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;

        out_msg.updates.push_back(lvl);
    }

    return true;
}
