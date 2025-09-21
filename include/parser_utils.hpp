#pragma once

#include <cstdint>
#include <cstddef>
#include "types.hpp"

// Parses a binary Source Endpoint packet into ProcessedMessage.
// Returns true if successful, false if format/length is invalid.
bool parse_data_packet(const uint8_t* data, size_t len, ProcessedMessage& out_msg);
