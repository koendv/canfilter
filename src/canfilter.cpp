/*
 * canfilter.cpp
 *
 * Implements the base CAN filter parsing logic.
 *
 * Responsibilities:
 * - Parse single CAN IDs or ID ranges from strings or argument vectors.
 * - Distinguish between standard (11-bit) and extended (29-bit) IDs.
 * - Call virtual functions in derived classes to add IDs or ranges to the hardware configuration.
 *
 * Notes:
 * - No hardware-specific logic; this is purely parsing and classification.
 * - Relies on canfilter derived classes (bxCAN, FDCAN) to handle storage and hardware translation.
 */

#include "canfilter.hpp"
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>

bool canfilter::parse(const std::string &input) {
    if (input.empty()) {
        return true;
    }

    size_t pos = 0;
    size_t len = input.length();

    while (pos < len) {
        // Skip whitespace
        while (pos < len && std::isspace(input[pos])) {
            pos++;
        }
        if (pos >= len)
            break;

        // Parse first ID
        const char *start = input.c_str() + pos;
        char *end;
        errno = 0;
        uint32_t id1 = strtoul(start, &end, 0);

        if (end == start || errno == ERANGE) {
            return false;
        }

        pos += (end - start);

        // Skip whitespace after first ID
        while (pos < len && std::isspace(input[pos])) {
            pos++;
        }

        // Check if this is a range
        if (pos < len && input[pos] == '-') {
            pos++; // Skip '-'

            // Skip whitespace after dash
            while (pos < len && std::isspace(input[pos])) {
                pos++;
            }

            // Parse second ID
            start = input.c_str() + pos;
            errno = 0;
            uint32_t id2 = strtoul(start, &end, 0);

            if (end == start || errno == ERANGE) {
                return false;
            }

            pos += (end - start);

            if (id1 <= max_std_id && id2 <= max_std_id) {
                add_std_range(id1, id2);
            } else if (id1 <= max_ext_id && id2 <= max_ext_id) {
                add_ext_range(id1, id2);
            } else {
                return false;
            }
        } else {
            if (id1 <= max_std_id) {
                add_std_id(id1);
            } else if (id1 <= max_ext_id) {
                add_ext_id(id1);
            } else {
                return false;
            }
        }

        // Skip whitespace or comma after ID/range
        while (pos < len && (std::isspace(input[pos]) || (input[pos] == ','))) {
            pos++;
        }
    }

    return true;
}

bool canfilter::parse(const std::vector<std::string> &args) {
    for (const auto &arg : args) {
        if (!parse(arg)) {
            return false;
        }
    }
    return true;
}
