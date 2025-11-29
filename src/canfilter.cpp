#include "canfilter.hpp"
#include <charconv>
#include <string_view>

bool canfilter::parse(std::string_view input) {
    size_t pos = 0;

    if (input.empty()) {
        return true;
    }

    while (pos < input.length()) {
        // Parse first ID
        const char *start = input.data() + pos;
        char *end;
        errno = 0;
        uint32_t id1 = strtoul(start, &end, 0);

        if (end == start || errno == ERANGE) {
            return false;
        }

        pos += (end - start);

        // Check if this is a range
        if (pos < input.length() && input[pos] == '-') {
            pos++; // Skip '-'

            // Parse second ID
            start = input.data() + pos;
            errno = 0;
            uint32_t id2 = strtoul(start, &end, 0);

            if (end == start || errno == ERANGE) {
                return false;
            }

            pos += (end - start);
            if (id1 <= max_std_id && id2 <= max_std_id)
                add_std_range(id1, id2);
            else
                add_ext_range(id1, id2);
        } else {
            if (id1 <= max_std_id)
                add_std_id(id1);
            else
                add_ext_id(id1);
        }

        // Check for comma or end of string
        if (pos < input.length() && input[pos] == ',') {
            pos++;
        } else if (pos < input.length()) {
            return false;
        }
    }

    return true;
}
