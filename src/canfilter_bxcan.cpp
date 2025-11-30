/*
 * canfilter_bxcan.cpp
 *
 * Implements CAN filters for bxCAN hardware (STM32F0/F1/F3/F4/F7).
 *
 * Responsibilities:
 * - Emulate ID ranges using mask and list modes (bxCAN lacks native range support).
 * - CIDR-style algorithm converts ranges to optimal mask/list filter configurations.
 * - Supports both standard (11-bit) and extended (29-bit) CAN IDs.
 * - Manage filter banks and ensure hardware limits are respected.
 * - Provide debug printing and usage statistics.
 *
 * Limitations:
 * - STM32F0/F1/F3: maximum 14 filter banks.
 * - STM32F4/F7: maximum 28 filter banks.
 * - Each bank can hold up to 4 std IDs (list), 2 std masks, 2 ext IDs (list), or 1 ext mask.
 *
 * Notes:
 * - Explicit template instantiations exist for bxCAN F0 and F4 variants.
 *
 * See:
 *   STM RM0431
 *   31.7.4 Identifier filtering
 */

#include "canfilter_bxcan.hpp"
#include <iomanip>
#include <iostream>

// hex formatting macro
#define FORMAT_HEX(val, width)                                                                                         \
    "0x" << std::hex << std::setw(width) << std::setfill('0') << (val) << std::dec << std::setfill(' ')

/* Constructor */
template <uint8_t max_banks_t, uint8_t dev_val> canfilter_bxcan<max_banks_t, dev_val>::canfilter_bxcan() {
    // no additional initialization code needed
}

/* Constructor implementations for bxcan_f0 and bxcan_f4 */
canfilter_bxcan_f0::canfilter_bxcan_f0() : canfilter_bxcan<14, CANFILTER_DEV_BXCAN_F0>() {}
canfilter_bxcan_f4::canfilter_bxcan_f4() : canfilter_bxcan<28, CANFILTER_DEV_BXCAN_F4>() {}

/* filter emission functions - one for each of four types */

/* Pack 4 std list filters into one bank (16-bit list mode) */
template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::emit_std_list(uint32_t id1, uint32_t id2, uint32_t id3,
                                                                       uint32_t id4) {
    if (bank >= max_banks)
        return CANFILTER_ERROR_FULL;

    if (id1 > max_std_id || id2 > max_std_id || id3 > max_std_id || id4 > max_std_id)
        return CANFILTER_ERROR_PARAM;

    uint32_t fr1 = (id2 << 21) | (id1 << 5);
    uint32_t fr2 = (id4 << 21) | (id3 << 5);

    hw_config.fr1[bank] = fr1;
    hw_config.fr2[bank] = fr2;

    /* Configure bank as 16-bit list mode */
    hw_config.fs1r &= ~(1 << bank); /* 16-bit */
    hw_config.fm1r |= (1 << bank);  /* List mode */
    hw_config.fa1r |= (1 << bank);  /* Enable */

    bank++;

    return CANFILTER_SUCCESS;
}

/* Pack 2 std mask filters into one bank (16-bit mask mode) */
template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::emit_std_mask(uint32_t id1, uint32_t mask1, uint32_t id2,
                                                                       uint32_t mask2) {
    if (bank >= max_banks)
        return CANFILTER_ERROR_FULL;

    if (id1 > max_std_id || mask1 > max_std_id || id2 > max_std_id || mask2 > max_std_id)
        return CANFILTER_ERROR_PARAM;

    uint32_t fr1 = (mask1 << 21) | (id1 << 5);
    uint32_t fr2 = (mask2 << 21) | (id2 << 5);

    hw_config.fr1[bank] = fr1;
    hw_config.fr2[bank] = fr2;

    /* Configure bank as 16-bit list mode */
    hw_config.fs1r &= ~(1 << bank); /* 16-bit */
    hw_config.fm1r &= ~(1 << bank); /* Mask mode */
    hw_config.fa1r |= (1 << bank);  /* Enable */

    bank++;

    return CANFILTER_SUCCESS;
}

/* Pack 2 ext list filters into one bank (32-bit list mode) */
template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::emit_ext_list(uint32_t id1, uint32_t id2) {
    if (bank >= max_banks)
        return CANFILTER_ERROR_FULL;

    if (id1 > max_ext_id || id2 > max_ext_id)
        return CANFILTER_ERROR_PARAM;

    uint32_t fr1 = (id1 << 3) | (0x1U << 2);
    uint32_t fr2 = (id2 << 3) | (0x1U << 2);

    hw_config.fr1[bank] = fr1;
    hw_config.fr2[bank] = fr2;

    /* Configure bank as 32-bit list mode */
    hw_config.fs1r |= (1 << bank); /* 32-bit */
    hw_config.fm1r |= (1 << bank); /* List mode */
    hw_config.fa1r |= (1 << bank); /* Enable */

    bank++;

    return CANFILTER_SUCCESS;
}

/* Pack 1 ext mask filter into one bank (32-bit list mode) */
template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::emit_ext_mask(uint32_t id1, uint32_t mask1) {
    if (bank >= max_banks)
        return CANFILTER_ERROR_FULL;

    if (id1 > max_ext_id || mask1 > max_ext_id)
        return CANFILTER_ERROR_PARAM;

    uint32_t fr1 = (id1 << 3) | (0x1U << 2);
    uint32_t fr2 = (mask1 << 3);

    hw_config.fr1[bank] = fr1;
    hw_config.fr2[bank] = fr2;

    /* Configure bank as 32-bit mask mode */
    hw_config.fs1r |= (1 << bank);  /* 32-bit */
    hw_config.fm1r &= ~(1 << bank); /* mask mode */
    hw_config.fa1r |= (1 << bank);  /* Enable */

    bank++;

    return CANFILTER_SUCCESS;
}

/* Add to std/ext list/mask */
template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::add_std_list(uint32_t id) {
    canfilter_error_t err = CANFILTER_SUCCESS;

    if (std_list_count > 3)
        return CANFILTER_ERROR_PARAM;

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

template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::add_std_mask(uint32_t id, uint32_t mask) {
    canfilter_error_t err = CANFILTER_SUCCESS;

    if (std_mask_count > 1)
        return CANFILTER_ERROR_PARAM;

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

template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::add_ext_list(uint32_t id) {
    canfilter_error_t err = CANFILTER_SUCCESS;

    if (ext_list_count > 1)
        return CANFILTER_ERROR_PARAM;

    ext_list[ext_list_count++] = id;
    if (ext_list_count == 1) {
        ext_list[1] = id;
    } else if (ext_list_count == 2) {
        err = emit_ext_list(ext_list[0], ext_list[1]);
        ext_list_count = 0;
    }

    return err;
}

template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::add_ext_mask(uint32_t id, uint32_t mask) {
    return emit_ext_mask(id, mask);
}

/* CIDR aggregation code */
template <uint8_t max_banks_t, uint8_t dev_val>
int canfilter_bxcan<max_banks_t, dev_val>::std_largest_prefix(uint32_t begin, uint32_t end) const {
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

template <uint8_t max_banks_t, uint8_t dev_val>
int canfilter_bxcan<max_banks_t, dev_val>::ext_largest_prefix(uint32_t begin, uint32_t end) const {
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

template <uint8_t max_banks_t, uint8_t dev_val> canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::begin() {
    std_list_count = 0;
    std_mask_count = 0;
    ext_list_count = 0;
    bank = 0;
    hw_config = hw_t(); // zero out
    hw_config.dev = dev_val;

    return CANFILTER_SUCCESS;
}

template <uint8_t max_banks_t, uint8_t dev_val> canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::end() {
    canfilter_error_t err = CANFILTER_SUCCESS;

    if (std_list_count != 0)
        err = emit_std_list(std_list[0], std_list[1], std_list[2], std_list[3]);

    if (std_mask_count != 0 && err == CANFILTER_SUCCESS)
        err = emit_std_mask(std_mask[0].id, std_mask[0].mask, std_mask[1].id, std_mask[1].mask);

    if (ext_list_count != 0 && err == CANFILTER_SUCCESS)
        err = emit_ext_list(ext_list[0], ext_list[1]);

    return err;
}

template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::add_std_range(uint32_t begin, uint32_t end) {
    canfilter_error_t err;

    if (begin > max_std_id || end > max_std_id)
        return CANFILTER_ERROR_PARAM;

    if (begin > end) {
        uint32_t temp = end;
        end = begin;
        begin = temp;
    }

    /* CIDR aggregation: range to network algorithm */
    while (begin <= end) {
        int prefix = std_largest_prefix(begin, end);
        uint32_t mask = (~0U << (11 - prefix)) & max_std_id;
        uint32_t id = begin;
        if (mask == max_std_id) {
            err = add_std_list(id);
            if (verbose)
                std::cout << "bxcan std list id " << FORMAT_HEX(id, 3) << std::endl;
        } else {
            err = add_std_mask(id, mask);
            if (verbose)
                std::cout << "bxcan std mask id " << FORMAT_HEX(id, 3) << " mask " << FORMAT_HEX(mask, 3) << std::endl;
        }
        if (err != CANFILTER_SUCCESS) {
            std::cout << "bxcan std filter fail" << std::endl;
            return err;
        }

        uint32_t block_size = 1U << (11 - prefix);
        begin += block_size;
    }

    return CANFILTER_SUCCESS;
}

template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::add_ext_range(uint32_t begin, uint32_t end) {
    canfilter_error_t err;

    if (begin > max_ext_id || end > max_ext_id)
        return CANFILTER_ERROR_PARAM;

    if (begin > end) {
        uint32_t temp = end;
        end = begin;
        begin = temp;
    }

    /* CIDR aggregation: range to network algorithm */
    while (begin <= end) {
        int prefix = ext_largest_prefix(begin, end);
        uint32_t mask = (~0U << (29 - prefix)) & max_ext_id;
        uint32_t id = begin;
        if (mask == max_ext_id) {
            err = add_ext_list(id);
            if (verbose)
                std::cout << "bxcan ext list id " << FORMAT_HEX(id, 8) << std::endl;
        } else {
            err = add_ext_mask(id, mask);
            if (verbose)
                std::cout << "bxcan ext mask id " << FORMAT_HEX(id, 8) << " mask " << FORMAT_HEX(mask, 8) << std::endl;
        }
        if (err != CANFILTER_SUCCESS) {
            std::cout << "bxcan ext filter fail" << std::endl;
            return err;
        }

        uint32_t block_size = 1U << (29 - prefix);
        begin += block_size;
    }

    return CANFILTER_SUCCESS;
}

template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::add_std_id(uint32_t id) {
    return add_std_range(id, id);
}

template <uint8_t max_banks_t, uint8_t dev_val>
canfilter_error_t canfilter_bxcan<max_banks_t, dev_val>::add_ext_id(uint32_t id) {
    return add_ext_range(id, id);
}

template <uint8_t max_banks_t, uint8_t dev_val> void *canfilter_bxcan<max_banks_t, dev_val>::get_hw_config() {
    return &hw_config;
}

template <uint8_t max_banks_t, uint8_t dev_val> size_t canfilter_bxcan<max_banks_t, dev_val>::get_hw_size() {
    return sizeof(hw_config);
}

template <uint8_t max_banks_t, uint8_t dev_val> void canfilter_bxcan<max_banks_t, dev_val>::debug_print_reg() const {
    std::cout << std::endl << "bxcan registers:" << std::endl;

    std::cout << "FS1R:  " << FORMAT_HEX(hw_config.fs1r, 8) << std::endl;
    std::cout << "FM1R:  " << FORMAT_HEX(hw_config.fm1r, 8) << std::endl;
    std::cout << "FFA1R: " << FORMAT_HEX(hw_config.ffa1r, 8) << std::endl;
    std::cout << "FA1R:  " << FORMAT_HEX(hw_config.fa1r, 8) << std::endl;

    // Print the filter registers
    for (uint32_t i = 0; i < max_banks; ++i) {
        uint32_t r1 = hw_config.fr1[i];
        uint32_t r2 = hw_config.fr2[i];
        if (r1 != 0 || r2 != 0) {
            std::cout << "FR1[" << i << "]: " << FORMAT_HEX(r1, 8) << " FR2[" << i << "]: " << FORMAT_HEX(r2, 8)
                      << std::endl;
        }
    }
}

template <uint8_t max_banks_t, uint8_t dev_val> void canfilter_bxcan<max_banks_t, dev_val>::debug_print() const {
    std::cout << "\nbxcan debug:" << std::endl;
    for (int i = 0; i < max_banks; i++) {
        bool is_active = hw_config.fa1r & (1 << i);
        if (!is_active)
            continue;
        std::cout << "bank [" << i << "]: ";
        bool is_32bit = hw_config.fs1r & (1 << i);
        bool is_list = hw_config.fm1r & (1 << i);
        if (is_32bit) {
            uint32_t id1 = (hw_config.fr1[i] >> 3) & max_ext_id;
            uint32_t id2 = (hw_config.fr2[i] >> 3) & max_ext_id;
            if (is_list) {
                std::cout << "ext list " << FORMAT_HEX(id1, 8) << ", " << FORMAT_HEX(id2, 8) << std::endl;
            } else {
                uint32_t base1 = id1;
                uint32_t mask1 = id2;
                uint32_t begin1 = base1 & mask1;
                uint32_t end1 = (begin1 | ~mask1) & max_ext_id;
                std::cout << "ext mask " << FORMAT_HEX(begin1, 8) << "-" << FORMAT_HEX(end1, 8) << std::endl;
            }
        } else {
            uint32_t id1 = (hw_config.fr1[i] >> 5) & max_std_id;
            uint32_t id2 = (hw_config.fr1[i] >> 21) & max_std_id;
            uint32_t id3 = (hw_config.fr2[i] >> 5) & max_std_id;
            uint32_t id4 = (hw_config.fr2[i] >> 21) & max_std_id;
            if (is_list) {
                std::cout << "std list " << FORMAT_HEX(id1, 3) << ", " << FORMAT_HEX(id2, 3) << ", "
                          << FORMAT_HEX(id3, 3) << ", " << FORMAT_HEX(id4, 3) << std::endl;
            } else {
                uint32_t base1 = id1;
                uint32_t mask1 = id2;
                uint32_t begin1 = base1 & mask1;
                uint32_t end1 = (begin1 | ~mask1) & max_std_id;
                uint32_t base2 = id3;
                uint32_t mask2 = id4;
                uint32_t begin2 = base2 & mask2;
                uint32_t end2 = (begin2 | ~mask2) & max_std_id;
                std::cout << "std mask " << FORMAT_HEX(base1, 3) << "-" << FORMAT_HEX(end1, 3) << ", "
                          << FORMAT_HEX(base2, 3) << "-" << FORMAT_HEX(end2, 3) << std::endl;
            }
        }
    }
}

template <uint8_t max_banks_t, uint8_t dev_val> void canfilter_bxcan<max_banks_t, dev_val>::print_usage() const {
    uint32_t percent = (bank * 100 + max_banks / 2) / max_banks;
    std::cout << "Filter usage: " << (int)bank << "/" << (int)max_banks << " (" << percent << "%)" << std::endl;
    return;
}

// Explicit template instantiation for bxcan_f0 and bxcan_f4
template class canfilter_bxcan<14, CANFILTER_DEV_BXCAN_F0>;
template class canfilter_bxcan<28, CANFILTER_DEV_BXCAN_F4>;
