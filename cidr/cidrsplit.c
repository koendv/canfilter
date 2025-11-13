#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>

typedef struct {
    uint32_t network;  // Network address (in host byte order)
    int prefix;        // CIDR prefix length (e.g. /24)
} CIDR;

// Find largest CIDR prefix that fits between start and end
static int largest_prefix(uint32_t start, uint32_t end) {
    int prefix = 32;

    // Step 1: reduce prefix according to alignment (trailing zeros in start)
    while (prefix > 0) {
        uint32_t mask_bit = (1U << (32 - prefix));
        if (start & mask_bit)
            break;
        prefix--;
    }

    // Step 2: make sure block doesn't exceed end
    while (prefix < 32) {
        uint32_t block_size = 1U << (32 - prefix);
        if (start + block_size - 1 > end)
            prefix++;
        else
            break;
    }

    return prefix;
}

/**
 * Convert IP range [start, end] → array of CIDR blocks.
 *
 * @param start      starting IPv4 (host byte order)
 * @param end        ending IPv4 (host byte order)
 * @param count_out  output: number of CIDRs
 * @return           dynamically allocated array of CIDR structs (caller must free)
 */
CIDR* range_to_cidrs(uint32_t start, uint32_t end, size_t *count_out) {
    size_t capacity = 16;
    CIDR *list = malloc(capacity * sizeof(CIDR));
    if (!list) return NULL;

    size_t count = 0;

    while (start <= end) {
        int prefix = largest_prefix(start, end);

        if (count >= capacity) {
            capacity *= 2;
            CIDR *new_list = realloc(list, capacity * sizeof(CIDR));
            if (!new_list) {
                free(list);
                return NULL;
            }
            list = new_list;
        }

        list[count].network = start;
        list[count].prefix = prefix;
        count++;

        uint32_t block_size = 1U << (32 - prefix);
        start += block_size;
    }

    *count_out = count;
    return list;
}

// --- Example usage ---
uint32_t ip_to_int(const char *ip_str) {
    struct in_addr ip;
    inet_pton(AF_INET, ip_str, &ip);
    return ntohl(ip.s_addr);
}

void int_to_ip(uint32_t ip, char *buf) {
    struct in_addr addr;
    addr.s_addr = htonl(ip);
    inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN);
}

int main() {
    uint32_t start = ip_to_int("192.168.1.0");
    uint32_t end   = ip_to_int("192.168.1.130");

    size_t count = 0;
    CIDR *blocks = range_to_cidrs(start, end, &count);

    printf("CIDR blocks covering range:\n");
    for (size_t i = 0; i < count; i++) {
        char buf[INET_ADDRSTRLEN];
        int_to_ip(blocks[i].network, buf);
        printf("%s/%d\n", buf, blocks[i].prefix);
    }

    free(blocks);
    return 0;
}

