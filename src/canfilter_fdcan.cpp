/*
 * canfilter_fdcan.cpp
 *
 * Implements CAN filters for FDCAN hardware (STM32G0/H7 series).
 *
 * Responsibilities:
 * - Use native hardware support for standard and extended ID filters and ranges.
 * - Translate logical filter IDs/ranges to FDCAN-specific filter registers.
 * - Manage filter counts and prevent overflow beyond hardware limits.
 * - Provide debug printing and usage statistics.
 *
 * STM32-specific configurations:
 * - STM32G0: 28 standard filters, 8 extended filters.
 * - STM32H7: 128 standard filters, 64 extended filters.
 *
 * Notes:
 * - Explicit template instantiations exist for G0 and H7 variants.
 *
 * See:
 *   STM RM044
 *   36.3.11 FDCAN standard message ID filter element
 *   36.3.12 FDCAN extended message ID filter element
 */

#include "canfilter_fdcan.hpp"
#include <iomanip>
#include <iostream>

// hex formatting macro
#define FORMAT_HEX(val, width)                                                                                         \
    "0x" << std::hex << std::setw(width) << std::setfill('0') << (val) << std::dec << std::setfill(' ')

// SFT Standard Filter Type
#define SFT_RANGE 0x0U
#define SFT_DUAL 0x1U

// SFEC Standard Filter Element Configuration
#define SFEC_RX_FIFO0 0x1U

// EFT Extended Filter Type
#define EFT_RANGE 0x0U
#define EFT_DUAL 0x1U

// EFEC Extended Filter Element Configuration
#define EFEC_RX_FIFO0 0x1U

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::emit_std_id(uint32_t id1, uint32_t id2) {
    if (hw_config.std_filter_nbr >= max_std_filter)
        return CANFILTER_ERROR_FULL;

    if (id1 > max_std_id || id2 > max_std_id)
        return CANFILTER_ERROR_PARAM;

    uint32_t sfr = (SFT_DUAL << 30) | (SFEC_RX_FIFO0 << 27) | (id1 << 16) | id2;
    hw_config.std_filter[hw_config.std_filter_nbr++] = sfr;
    return CANFILTER_SUCCESS;
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::emit_std_range(uint32_t id1, uint32_t id2) {
    if (hw_config.std_filter_nbr >= max_std_filter)
        return CANFILTER_ERROR_FULL;

    if (id1 > max_std_id || id2 > max_std_id || id1 > id2)
        return CANFILTER_ERROR_PARAM;

    uint32_t sfr = (SFT_RANGE << 30) | (SFEC_RX_FIFO0 << 27) | (id1 << 16) | id2;
    hw_config.std_filter[hw_config.std_filter_nbr++] = sfr;
    return CANFILTER_SUCCESS;
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::emit_ext_id(uint32_t id1, uint32_t id2) {
    if (hw_config.ext_filter_nbr >= max_ext_filter)
        return CANFILTER_ERROR_FULL;

    if (id1 > max_ext_id || id2 > max_ext_id)
        return CANFILTER_ERROR_PARAM;

    // Word 0: EFID1 (bits 28-0) + EFEC (bits 31-29)
    hw_config.ext_filter[hw_config.ext_filter_nbr][0] = (EFEC_RX_FIFO0 << 29) | id1;

    // Word 1: EFID2 (bits 28-0) + EFT_DUAL (bits 31-30)
    hw_config.ext_filter[hw_config.ext_filter_nbr][1] = (EFT_DUAL << 30) | id2;

    hw_config.ext_filter_nbr++;
    return CANFILTER_SUCCESS;
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::emit_ext_range(uint32_t id1, uint32_t id2) {
    if (hw_config.ext_filter_nbr >= max_ext_filter)
        return CANFILTER_ERROR_FULL;

    if (id1 > max_ext_id || id2 > max_ext_id || id1 > id2)
        return CANFILTER_ERROR_PARAM;

    // Word 0: EFID1 (bits 28-0) + EFEC (bits 31-29)
    hw_config.ext_filter[hw_config.ext_filter_nbr][0] = (EFEC_RX_FIFO0 << 29) | id1;

    // Word 1: EFID2 (bits 28-0) + EFT_RANGE (bits 31-30)
    hw_config.ext_filter[hw_config.ext_filter_nbr][1] = (EFT_RANGE << 30) | id2;

    hw_config.ext_filter_nbr++;
    return CANFILTER_SUCCESS;
}

// Constructor for base class, setting default values
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::canfilter_fdcan() {
    // no additional initialization code needed
}

// Begin function: Can initialize or reset hardware configuration
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::begin() {
    // zero out config
    hw_config = hw_t();
    hw_config.dev = dev_val;
    // no pending id's
    std_id_count = 0;
    ext_id_count = 0;
    // no filters written
    hw_config.std_filter_nbr = 0;
    hw_config.ext_filter_nbr = 0;
    return CANFILTER_SUCCESS;
}

// End function: Clean up or finalize hardware configuration
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::end() {
    canfilter_error_t err = CANFILTER_SUCCESS;

    if (std_id_count)
        err = emit_std_id(std_id[0], std_id[1]);

    if (ext_id_count && err == CANFILTER_SUCCESS)
        err = emit_ext_id(ext_id[0], ext_id[1]);

    return err;
}

// Add standard ID
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::add_std_id(uint32_t id) {
    canfilter_error_t err = CANFILTER_SUCCESS;

    if (id > max_std_id || std_id_count > 1)
        return CANFILTER_ERROR_PARAM;

    std_id[std_id_count++] = id;
    if (std_id_count == 1) {
        std_id[1] = id;
    } else {
        err = emit_std_id(std_id[0], std_id[1]);
        std_id_count = 0;
    }
    return err;
}

// Add extended ID
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::add_ext_id(uint32_t id) {
    canfilter_error_t err = CANFILTER_SUCCESS;

    if (id > max_ext_id || ext_id_count > 1)
        return CANFILTER_ERROR_PARAM;

    ext_id[ext_id_count++] = id;
    if (ext_id_count == 1) {
        ext_id[1] = id;
    } else {
        err = emit_ext_id(ext_id[0], ext_id[1]);
        ext_id_count = 0;
    }
    return err;
}

// Add standard ID range
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::add_std_range(uint32_t begin,
                                                                                          uint32_t end) {
    if (begin > max_std_id || end > max_std_id)
        return CANFILTER_ERROR_PARAM;

    if (begin <= end)
        return emit_std_range(begin, end);
    else
        return emit_std_range(end, begin);
}

// Add extended ID range
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::add_ext_range(uint32_t begin,
                                                                                          uint32_t end) {
    if (begin > max_ext_id || end > max_ext_id)
        return CANFILTER_ERROR_PARAM;

    if (begin <= end)
        return emit_ext_range(begin, end);
    else
        return emit_ext_range(end, begin);
}

// Access to hardware config
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
void *canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::get_hw_config() {
    return &hw_config;
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
size_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::get_hw_size() {
    return sizeof(hw_config);
}

// Debug print function: Show configuration (e.g., filter registers)
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
void canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::debug_print_reg() const {
    std::cout << std::endl << "fd-can registers:" << std::endl;
    std::cout << "standard filters: " << hw_config.std_filter_nbr << std::endl;
    for (uint32_t i = 0; i < hw_config.std_filter_nbr; ++i) {
        std::cout << "sf[" << i << "]: " << FORMAT_HEX(hw_config.std_filter[i], 8) << std::endl;
    }
    std::cout << "extended filters: " << hw_config.ext_filter_nbr << std::endl;
    for (uint32_t i = 0; i < hw_config.ext_filter_nbr; ++i) {
        std::cout << "ef[" << i << "]: f0=" << FORMAT_HEX(hw_config.ext_filter[i][0], 8)
                  << " f1=" << FORMAT_HEX(hw_config.ext_filter[i][1], 8) << std::endl;
    }
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
void canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::debug_print() const {
    static const char *ft_str[4] = {"range", "dual", "mask", "off"};
    static const char *fec_str[8] = {"off", "fifo0", "fifo1", "reject", "prio", "prio fifo0", "prio fifo1", "not used"};

    std::cout << std::endl << "fdcan debug:" << std::endl;
    for (uint32_t i = 0; i < hw_config.std_filter_nbr; ++i) {
        uint32_t sfid1 = (hw_config.std_filter[i] >> 16) & max_std_id;
        uint32_t sfid2 = (hw_config.std_filter[i] & max_std_id);
        uint32_t sfec = (hw_config.std_filter[i] >> 27) & 0x7;
        uint32_t sft = (hw_config.std_filter[i] >> 30) & 0x3;
        std::cout << "sf[" << i << "]: " << ft_str[sft] << " " << FORMAT_HEX(sfid1, 3) << " " << FORMAT_HEX(sfid2, 3)
                  << " " << fec_str[sfec] << std::endl;
    }
    for (uint32_t i = 0; i < hw_config.ext_filter_nbr; ++i) {
        uint32_t efid1 = hw_config.ext_filter[i][0] & max_ext_id;
        uint32_t efid2 = hw_config.ext_filter[i][1] & max_ext_id;
        uint32_t efec = (hw_config.ext_filter[i][0] >> 29) & 0x7;
        uint32_t eft = (hw_config.ext_filter[i][1] >> 30) & 0x3;
        std::cout << "ef[" << i << "]: " << ft_str[eft] << " " << FORMAT_HEX(efid1, 8) << " " << FORMAT_HEX(efid2, 8)
                  << " " << fec_str[efec] << std::endl;
    }
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
void canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::print_usage() const {
    uint32_t std_percent = (hw_config.std_filter_nbr * 100 + max_std_filter / 2) / max_std_filter;
    uint32_t ext_percent = (hw_config.ext_filter_nbr * 100 + max_ext_filter / 2) / max_ext_filter;

    std::cout << "Filter usage: " << (int)hw_config.std_filter_nbr << "/" << max_std_filter << " standard ("
              << std_percent << "%), " << (int)hw_config.ext_filter_nbr << "/" << max_ext_filter << " extended ("
              << ext_percent << "%)" << std::endl;
    return;
}

// Explicit template instantiation for G0 and H7
// fdcan for stm32g0: 28 standard filters, 8 extended filters, first byte of usb data CANFILTER_DEV_FDCAN_G0
template class canfilter_fdcan<28, 8, CANFILTER_DEV_FDCAN_G0>;
// fdcan for stm32h7: 128 standard filters, 64 extended filters, first byte of usb data CANFILTER_DEV_FDCAN_H7
template class canfilter_fdcan<128, 64, CANFILTER_DEV_FDCAN_H7>;

// Constructor implementations for G0 and H7
canfilter_fdcan_g0::canfilter_fdcan_g0() : canfilter_fdcan<28, 8, CANFILTER_DEV_FDCAN_G0>() {}
canfilter_fdcan_h7::canfilter_fdcan_h7() : canfilter_fdcan<128, 64, CANFILTER_DEV_FDCAN_H7>() {}
