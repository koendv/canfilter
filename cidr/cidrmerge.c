#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>

typedef struct {
    uint32_t network;
    int prefix;
} CIDR;

typedef struct {
    uint32_t start;
    uint32_t end;
} Range;

// --- Convert CIDR → [start, end] ---
Range cidr_to_range(CIDR c) {
    uint32_t mask = c.prefix == 0 ? 0 : (0xFFFFFFFF << (32 - c.prefix));
    uint32_t start = c.network & mask;
    uint32_t end = start | ~mask;
    return (Range){ start, end };
}

// --- Find largest prefix fitting [start, end] ---
static int largest_prefix(uint32_t start, uint32_t end) {
    int prefix = 32;
    while (prefix > 0) {
        uint32_t mask_bit = (1U << (32 - prefix));
        if (start & mask_bit) break;
        prefix--;
    }
    while (prefix < 32) {
        uint32_t block_size = 1U << (32 - prefix);
        if (start + block_size - 1 > end) prefix++;
        else break;
    }
    return prefix;
}

// --- Convert [start, end] → CIDRs ---
CIDR* range_to_cidrs(uint32_t start, uint32_t end, size_t *count_out) {
    size_t cap = 16, count = 0;
    CIDR *out = malloc(cap * sizeof(CIDR));
    while (start <= end) {
        int prefix = largest_prefix(start, end);
        if (count >= cap) {
            cap *= 2;
            out = realloc(out, cap * sizeof(CIDR));
        }
        out[count++] = (CIDR){ start, prefix };
        uint32_t block_size = 1U << (32 - prefix);
        start += block_size;
    }
    *count_out = count;
    return out;
}

// --- Range merging ---
int cmp_ranges(const void *a, const void *b) {
    const Range *r1 = a, *r2 = b;
    return (r1->start > r2->start) - (r1->start < r2->start);
}

Range* merge_ranges(Range *ranges, size_t n, size_t *merged_count) {
    if (!n) return NULL;
    qsort(ranges, n, sizeof(Range), cmp_ranges);
    Range *out = malloc(n * sizeof(Range));
    size_t count = 0;
    out[count++] = ranges[0];
    for (size_t i = 1; i < n; i++) {
        Range *last = &out[count - 1];
        Range cur = ranges[i];
        if (cur.start <= last->end + 1) {
            if (cur.end > last->end) last->end = cur.end;
        } else {
            out[count++] = cur;
        }
    }
    *merged_count = count;
    return out;
}

// --- Recursive CIDR merge: collapse siblings ---
CIDR* collapse_cidrs(CIDR *cidrs, size_t count, size_t *new_count) {
    if (count < 2) { *new_count = count; return cidrs; }

    qsort(cidrs, count, sizeof(CIDR),
          (int(*)(const void*, const void*))[](const CIDR *a, const CIDR *b) {
              if (a->prefix != b->prefix) return a->prefix - b->prefix;
              return (a->network > b->network) - (a->network < b->network);
          });

    int merged = 1;
    while (merged) {
        merged = 0;
        size_t out_count = 0;
        CIDR *out = malloc(count * sizeof(CIDR));

        for (size_t i = 0; i < count; ) {
            if (i + 1 < count &&
                cidrs[i].prefix == cidrs[i + 1].prefix) {
                uint32_t block_size = 1U << (32 - cidrs[i].prefix);
                if (cidrs[i + 1].network == cidrs[i].network + block_size &&
                    (cidrs[i].network / (block_size * 2)) == (cidrs[i + 1].network / (block_size * 2))) {
                    // Merge siblings
                    out[out_count++] = (CIDR){ cidrs[i].network, cidrs[i].prefix - 1 };
                    i += 2;
                    merged = 1;
                    continue;
                }
            }
            out[out_count++] = cidrs[i++];
        }

        free(cidrs);
        cidrs = out;
        count = out_count;
    }

    *new_count = count;
    return cidrs;
}

// --- Merge two ISPs ---
CIDR* merge_isp_cidrs(const CIDR *isp1, size_t n1,
                      const CIDR *isp2, size_t n2,
                      size_t *final_count) {
    size_t total = n1 + n2;
    Range *ranges = malloc(total * sizeof(Range));
    for (size_t i = 0; i < n1; i++) ranges[i] = cidr_to_range(isp1[i]);
    for (size_t i = 0; i < n2; i++) ranges[n1 + i] = cidr_to_range(isp2[i]);

    size_t merged_count;
    Range *merged = merge_ranges(ranges, total, &merged_count);
    free(ranges);

    CIDR *result = NULL;
    size_t res_len = 0, res_cap = 0;
    for (size_t i = 0; i < merged_count; i++) {
        size_t n;
        CIDR *tmp = range_to_cidrs(merged[i].start, merged[i].end, &n);
        if (res_len + n > res_cap) {
            res_cap = (res_cap + n) * 2;
            result = realloc(result, res_cap * sizeof(CIDR));
        }
        for (size_t j = 0; j < n; j++)
            result[res_len++] = tmp[j];
        free(tmp);
    }
    free(merged);

    // Now recursively collapse siblings
    size_t collapsed_count;
    CIDR *collapsed = collapse_cidrs(result, res_len, &collapsed_count);

    *final_count = collapsed_count;
    return collapsed;
}

// --- Utility: IP conversion ---
uint32_t ip_to_int(const char *s) {
    struct in_addr a; inet_pton(AF_INET, s, &a); return ntohl(a.s_addr);
}
void int_to_ip(uint32_t ip, char *buf) {
    struct in_addr a = { htonl(ip) }; inet_ntop(AF_INET, &a, buf, INET_ADDRSTRLEN);
}

// --- Example main ---
int main(void) {
    CIDR isp1[] = {
        { ip_to_int("192.168.0.0"), 24 },
        { ip_to_int("192.168.1.0"), 24 },
    };
    CIDR isp2[] = {
        { ip_to_int("192.168.2.0"), 24 },
        { ip_to_int("192.168.3.0"), 24 },
    };

    size_t n_final;
    CIDR *final = merge_isp_cidrs(isp1, 2, isp2, 2, &n_final);

    printf("Unified CIDRs:\n");
    for (size_t i = 0; i < n_final; i++) {
        char buf[INET_ADDRSTRLEN];
        int_to_ip(final[i].network, buf);
        printf("%s/%d\n", buf, final[i].prefix);
    }
    free(final);
}

