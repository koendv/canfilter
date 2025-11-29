/*
 * bxcan filters
 *
 * See: STM RM0431
 * 31.7.4 Identifier filtering
 *
 * - bxCAN lacks native range support, so ranges must be emulated
 * - CIDR algorithm breaks ranges into optimal mask filters
 * - Example: Range 0x100-0x1FF becomes: id=0x100, mask=0x700
 * - Single IDs become exact match filters (mask = 0x7FF or 0x1FFFFFFF)
 *
 * bxCAN Filter Limitations:
 * - Maximum 14 filter banks
 * - Each bank can hold:
 *   - 4 standard id's (list mode)
 *   - or 2 standard masks (mask mode)
 *   - or 2 extended id's (list mode)
 *   - or 1 extended mask (mask mode)
 */

#include "canfilter_bxcan.hpp"
#include "canfilter_usb.h"
#include <format>
#include <iomanip>

/* filter emission functions - one for each of four types */

/* Pack 4 std list filters into one bank (16-bit list mode) */
canfilter_error_t canfilter_bxcan::emit_std_list(uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4) {
    if (bank >= max_banks)
        return CANFILTER_ERROR_FULL;

    uint32_t fr1 = (id2 << 21) | (id1 << 5);
    uint32_t fr2 = (id4 << 21) | (id3 << 5);

    hw_config.fr1[bank] = fr1;
    hw_config.fr2[bank] = fr2;

    /* Configure bank as 16-bit list mode */
    hw_config.fs1r &= ~(1 << bank); /* 16-bit */
    hw_config.fm1r &= ~(1 << bank); /* List mode */
    hw_config.fa1r |= (1 << bank);  /* Enable */

    bank++;

    return CANFILTER_SUCCESS;
}

/* Pack 2 std mask filters into one bank (16-bit mask mode) */
canfilter_error_t canfilter_bxcan::emit_std_mask(uint32_t id1, uint32_t mask1, uint32_t id2, uint32_t mask2) {
    if (bank >= max_banks)
        return CANFILTER_ERROR_FULL;

    uint32_t fr1 = (mask1 << 21) | (id1 << 5);
    uint32_t fr2 = (mask2 << 21) | (id2 << 5);

    hw_config.fr1[bank] = fr1;
    hw_config.fr2[bank] = fr2;

    /* Configure bank as 16-bit list mode */
    hw_config.fs1r &= ~(1 << bank); /* 16-bit */
    hw_config.fm1r |= (1 << bank);  /* Mask mode */
    hw_config.fa1r |= (1 << bank);  /* Enable */

    bank++;

    return CANFILTER_SUCCESS;
}

/* Pack 2 ext list filters into one bank (32-bit list mode) */
canfilter_error_t canfilter_bxcan::emit_ext_list(uint32_t id1, uint32_t id2) {
    if (bank >= max_banks)
        return CANFILTER_ERROR_FULL;

    uint32_t fr1 = (id1 << 3);
    uint32_t fr2 = (id2 << 3);

    hw_config.fr1[bank] = fr1;
    hw_config.fr2[bank] = fr2;

    /* Configure bank as 32-bit list mode */
    hw_config.fs1r |= (1 << bank);  /* 32-bit */
    hw_config.fm1r &= ~(1 << bank); /* List mode */
    hw_config.fa1r |= (1 << bank);  /* Enable */

    bank++;

    return CANFILTER_SUCCESS;
}

/* Pack 1 ext mask filter into one bank (32-bit list mode) */
canfilter_error_t canfilter_bxcan::emit_ext_mask(uint32_t id1, uint32_t mask1) {
    if (bank >= max_banks)
        return CANFILTER_ERROR_FULL;

    uint32_t fr1 = (id1 << 3);
    uint32_t fr2 = (mask1 << 3);

    hw_config.fr1[bank] = fr1;
    hw_config.fr2[bank] = fr2;

    /* Configure bank as 32-bit mask mode */
    hw_config.fs1r |= (1 << bank); /* 32-bit */
    hw_config.fm1r |= (1 << bank); /* mask mode */
    hw_config.fa1r |= (1 << bank); /* Enable */

    bank++;

    return CANFILTER_SUCCESS;
}

/* Add to std/ext list/mask */
canfilter_error_t canfilter_bxcan::add_std_list(uint32_t id) {
    canfilter_error_t err = CANFILTER_SUCCESS;

    std_list[std_list_count++] = id;
    if (std_list_count == 1) {
        std_list[1] = id;
        std_list[2] = id;
        std_list[3] = id;
    } else if (std_list_count == 4) {
        err = emit_std_list(std_list[0], std_list[1], std_list[2], std_list[3]);
        std_list_count = 0;
    }

    return err;
}

canfilter_error_t canfilter_bxcan::add_std_mask(uint32_t id, uint32_t mask) {
    canfilter_error_t err = CANFILTER_SUCCESS;

    std_mask[std_mask_count].id = id;
    std_mask[std_mask_count++].mask = mask;
    if (std_mask_count == 1) {
        std_mask[1].id = id;
        std_mask[1].mask = mask;
    } else if (std_mask_count == 2) {
        err = emit_std_mask(std_mask[0].id, std_mask[0].mask, std_mask[1].id, std_mask[1].mask);
        std_mask_count = 0;
    }

    return err;
}

canfilter_error_t canfilter_bxcan::add_ext_list(uint32_t id) {
    canfilter_error_t err = CANFILTER_SUCCESS;

    ext_list[ext_list_count++] = id;
    if (ext_list_count == 1) {
        ext_list[1] = id;
    } else if (ext_list_count == 2) {
        err = emit_ext_list(ext_list[0], ext_list[1]);
        ext_list_count = 0;
    }

    return err;
}

canfilter_error_t canfilter_bxcan::add_ext_mask(uint32_t id, uint32_t mask) {
    return emit_ext_mask(id, mask);
}

/* CIDR aggregation code */
int canfilter_bxcan::std_largest_prefix(uint32_t begin, uint32_t end) {
    int prefix = 11;
    while (prefix > 0) {
        uint32_t mask_bit = 1U << (11 - prefix);
        if (begin & mask_bit)
            break;
        prefix--;
    }
    while (prefix < 11) {
        uint32_t block_size = 1U << (11 - prefix);
        if (begin + block_size - 1 > end)
            prefix++;
        else
            break;
    }
    return prefix;
}

int canfilter_bxcan::ext_largest_prefix(uint32_t begin, uint32_t end) {
    int prefix = 29;
    while (prefix > 0) {
        uint32_t mask_bit = 1U << (29 - prefix);
        if (begin & mask_bit)
            break;
        prefix--;
    }
    while (prefix < 29) {
        uint32_t block_size = 1U << (29 - prefix);
        if (begin + block_size - 1 > end)
            prefix++;
        else
            break;
    }
    return prefix;
}

void canfilter_bxcan::begin() {
    std_list_count = 0;
    std_mask_count = 0;
    ext_list_count = 0;
    bank = 0;
    hw_config = hw_t(); // zero out
    hw_config.dev = CANFILTER_DEV_BXCAN;
}

canfilter_error_t canfilter_bxcan::end() {
    canfilter_error_t err = CANFILTER_SUCCESS;

    if (std_list_count != 0)
        err = emit_std_list(std_list[0], std_list[1], std_list[2], std_list[3]);

    if (std_mask_count != 0 && err == CANFILTER_SUCCESS)
        err = emit_std_mask(std_mask[0].id, std_mask[0].mask, std_mask[1].id, std_mask[1].mask);

    if (ext_list_count != 0 && err == CANFILTER_SUCCESS)
        err = emit_ext_list(ext_list[0], ext_list[1]);

    return err;
}

canfilter_error_t canfilter_bxcan::program() {
    bool retval = canfilter_send_usb(&hw_config, sizeof(hw_config));
    return retval ? CANFILTER_SUCCESS : CANFILTER_ERROR_PLATFORM;
}

canfilter_error_t canfilter_bxcan::add_std_range(uint32_t begin, uint32_t end) {
    canfilter_error_t err;

    if (begin > end) {
        uint32_t temp = end;
        end = begin;
        begin = temp;
    }

    if (end > max_std_id)
        return CANFILTER_ERROR_PARAM;

    /* CIDR aggregation: range to network algorithm */
    while (begin <= end) {
        int prefix = std_largest_prefix(begin, end);
        uint32_t mask = (~0U << (11 - prefix)) & max_std_id;
        uint32_t id = begin;
        if (mask == max_std_id) {
            err = add_std_list(id);
            if (verbose)
                std::cout << std::format("bxcan std list id 0x{:03x}\n", id);
        } else {
            err = add_std_mask(id, mask);
            if (verbose)
                std::cout << std::format("bxcan std mask id 0x{:03x} mask {:03x}\n", id, mask);
        }
        if (err != CANFILTER_SUCCESS) {
            std::cout << std::format("bxcan std filter fail\n");
            return err;
        }

        uint32_t block_size = 1U << (11 - prefix);
        begin += block_size;
    }

    return CANFILTER_SUCCESS;
}

canfilter_error_t canfilter_bxcan::add_ext_range(uint32_t begin, uint32_t end) {
    canfilter_error_t err;

    if (begin > end) {
        uint32_t temp = end;
        end = begin;
        begin = temp;
    }

    if (end > max_ext_id)
        return CANFILTER_ERROR_PARAM;

    /* CIDR aggregation: range to network algorithm */
    while (begin <= end) {
        int prefix = ext_largest_prefix(begin, end);
        uint32_t mask = (~0U << (29 - prefix)) & max_ext_id;
        uint32_t id = begin;
        if (mask == max_ext_id) {
            err = add_ext_list(id);
            if (verbose)
                std::cout << std::format("bxcan ext list id 0x{:08x}\n", id);
        } else {
            err = add_ext_mask(id, mask);
            if (verbose)
                std::cout << std::format("bxcan ext mask id 0x{:08x} mask {:08x}\n", id, mask);
        }
        if (err != CANFILTER_SUCCESS) {
            std::cout << std::format("bxcan ext filter fail\n");
            return err;
        }

        uint32_t block_size = 1U << (29 - prefix);
        begin += block_size;
    }

    return CANFILTER_SUCCESS;
}

canfilter_error_t canfilter_bxcan::add_std_id(uint32_t id) {
    return add_std_range(id, id);
}

canfilter_error_t canfilter_bxcan::add_ext_id(uint32_t id) {
    return add_ext_range(id, id);
}

void canfilter_bxcan::debug_print_reg() const {
    std::cout << "bxcan registers:\n";

    std::cout << std::format("FS1R:  0x{:08x}\n", hw_config.fs1r);
    std::cout << std::format("FM1R:  0x{:08x}\n", hw_config.fm1r);
    std::cout << std::format("FFA1R: 0x{:08x}\n", hw_config.ffa1r);
    std::cout << std::format("FA1R:  0x{:08x}\n", hw_config.fa1r);

    // Print the filter registers
    for (uint8_t i = 0; i < max_banks; ++i) {
        uint32_t r1 = hw_config.fr1[i];
        uint32_t r2 = hw_config.fr2[i];
        if (r1 != 0 || r2 != 0)
            std::cout << std::format("bank[{:2d}]: fr1: 0x{:08x} fr2: 0x{:08x}\n", i, r1, r2);
    }
}

void canfilter_bxcan::debug_print() const {
    std::cout << "bxcan debug:\n";
    for (int i = 0; i < max_banks; i++) {
        bool is_active = hw_config.fa1r & (1 << i);
        if (!is_active)
            continue;
        std::cout << std::format("bank [{:2d}]: ", i);
        bool is_32bit = hw_config.fs1r & (1 << i);
        bool is_mask = hw_config.fm1r & (1 << i);
        if (is_32bit) {
            uint32_t id1 = (hw_config.fr1[i] >> 3) & max_ext_id;
            uint32_t id2 = (hw_config.fr2[i] >> 3) & max_ext_id;
            if (is_mask) {
                uint32_t base1 = id1;
                uint32_t mask1 = id2;
                uint32_t begin1 = base1 & mask1;
                uint32_t end1 = (begin1 | ~mask1) & max_ext_id;
                std::cout << std::format("ext mask 0x{:08x}-0x{:08x}\n", begin1, end1);
            } else {
                std::cout << std::format("ext list 0x{:08x}, 0x{:08x}\n", id1, id2);
            }
        } else {
            uint32_t id1 = (hw_config.fr1[i] >> 5) & max_std_id;
            uint32_t id2 = (hw_config.fr1[i] >> 21) & max_std_id;
            uint32_t id3 = (hw_config.fr2[i] >> 5) & max_std_id;
            uint32_t id4 = (hw_config.fr2[i] >> 21) & max_std_id;
            if (is_mask) {
                uint32_t base1 = id1;
                uint32_t mask1 = id2;
                uint32_t begin1 = base1 & mask1;
                uint32_t end1 = (begin1 | ~mask1) & max_std_id;
                uint32_t base2 = id3;
                uint32_t mask2 = id4;
                uint32_t begin2 = base2 & mask2;
                uint32_t end2 = (begin2 | ~mask2) & max_std_id;
                std::cout << std::format("std mask 0x{:03x}-0x{:03x}, 0x{:03x}-0x{:03x}\n", base1, end1, base2, end2);
            } else {
                std::cout << std::format("std list 0x{:03x}, 0x{:03x}, 0x{:03x}, 0x{:03x}\n", id1, id2, id3, id4);
            }
        }
    }
}
