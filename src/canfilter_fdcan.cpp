#include "canfilter_fdcan.hpp"
#include "canfilter_usb.h"
#include <format>

/*
 * fdcan filters
 *
 * STM RM044
 * 36.3.11 FDCAN standard message ID filter element
 * 36.3.12 FDCAN extended message ID filter element
 */

// SFT Standard Filter Type
#define SFT_RANGE 0x0U
#define SFT_DUAL 0x1U

// SFEC Standard Filter Element Configuration
#define SFEC_RX_FIFO0 0x1U

// EFT Extended Filter Type
#define EFT_RANGE 0x0ULL
#define EFT_DUAL 0x1ULL

// EFEC Extended Filter Element Configuration
#define EFEC_RX_FIFO0 0x1ULL

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::emit_std_id(uint32_t id1, uint32_t id2) {
    if (hw_config.std_count >= max_std_filter)
        return CANFILTER_ERROR_FULL;
    id1 &= max_std_id;
    id2 &= max_std_id;
    uint32_t sfr = (SFT_DUAL << 30) | (SFEC_RX_FIFO0 << 27) | (id1 << 16) | id2;
    hw_config.std_filters[hw_config.std_count++] = sfr;
    return CANFILTER_SUCCESS;
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::emit_std_range(uint32_t id1, uint32_t id2) {
    if (hw_config.std_count >= max_std_filter)
        return CANFILTER_ERROR_FULL;
    id1 &= max_std_id;
    id2 &= max_std_id;
    uint32_t sfr = (SFT_RANGE << 30) | (SFEC_RX_FIFO0 << 27) | (id1 << 16) | id2;
    hw_config.std_filters[hw_config.std_count++] = sfr;
    return CANFILTER_SUCCESS;
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::emit_ext_id(uint32_t id1, uint32_t id2) {
    if (hw_config.ext_count >= max_ext_filter)
        return CANFILTER_ERROR_FULL;
    uint64_t efid1 = id1 & max_ext_id;
    uint64_t efid2 = id2 & max_ext_id;
    uint64_t efr = (EFEC_RX_FIFO0 << 61) | (efid1 << 32) | (EFT_DUAL << 30) | efid2;
    hw_config.ext_filters[hw_config.ext_count++] = efr;
    return CANFILTER_SUCCESS;
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::emit_ext_range(uint32_t id1, uint32_t id2) {
    if (hw_config.ext_count >= max_ext_filter)
        return CANFILTER_ERROR_FULL;
    uint64_t efid1 = id1 & max_ext_id;
    uint64_t efid2 = id2 & max_ext_id;
    uint64_t efr = (EFEC_RX_FIFO0 << 61) | (efid1 << 32) | (EFT_RANGE << 30) | efid2;
    hw_config.ext_filters[hw_config.ext_count++] = efr;
    return CANFILTER_SUCCESS;
}

// Constructor for base class, setting default values
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::canfilter_fdcan() {
    // Any additional initialization code if needed
}

// Begin function: Can initialize or reset hardware configuration
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
void canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::begin() {
    // zero out config
    hw_config = hw_t();
    hw_config.dev = dev_val;
    // no pending id's
    std_id_count = 0;
    ext_id_count = 0;
    // no filters written
    hw_config.std_count = 0;
    hw_config.ext_count = 0;
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

// Program function: Apply configuration to hardware
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::program() {
    bool retval = canfilter_send_usb(&hw_config, sizeof(hw_config));
    return retval ? CANFILTER_SUCCESS : CANFILTER_ERROR_PLATFORM;
}

// Add standard ID
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::add_std_id(uint32_t id) {
    canfilter_error_t err = CANFILTER_SUCCESS;

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
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::add_std_range(uint32_t start,
                                                                                          uint32_t end) {
    if (start <= end)
        return emit_std_range(start, end);
    else
        return emit_std_range(end, start);
}

// Add extended ID range
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
canfilter_error_t canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::add_ext_range(uint32_t start,
                                                                                          uint32_t end) {
    if (start <= end)
        return emit_ext_range(start, end);
    else
        return emit_ext_range(end, start);
}

// Debug print function: Show configuration (e.g., filter registers)
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
void canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::debug_print_reg() const {
    std::cout << "fd-can debug print\n";
    std::cout << "standard filters:\n";
    for (uint32_t i = 0; i < hw_config.std_count; ++i) {
        std::cout << std::format("sf[{:2d}]: 0x{:03x}\n", i, hw_config.std_filters[i]);
    }
    std::cout << "Extended Filters:\n";
    for (uint32_t i = 0; i < hw_config.ext_count; ++i) {
        std::cout << std::format("ef[{:2d}]: 0x{:08x}\n", i, hw_config.ext_filters[i]);
    }
}

template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val>
void canfilter_fdcan<max_std_filter, max_ext_filter, dev_val>::debug_print() const {
    static const char *ft_str[4] = {"range", "dual", "mask", "off"};
    static const char *fec_str[8] = {"off", "fifo0", "fifo1", "reject", "prio", "prio fifo0", "prio fifo1", "not used"};

    std::cout << "fdcan debug\n";
    for (uint32_t i = 0; i < hw_config.std_count; ++i) {
        uint32_t sfid1 = (hw_config.std_filters[i] >> 16) & max_std_id;
        uint32_t sfid2 = (hw_config.std_filters[i] & max_std_id);
        uint32_t sfec = (hw_config.std_filters[i] >> 27) & 0x7;
        uint32_t sft = (hw_config.std_filters[i] >> 30) & 0x3;
        std::cout << std::format("sf[{:2d}]: {} 0x{:03x} 0x{:03x} {}\n", i, ft_str[sft], sfid1, sfid2, fec_str[sfec]);
    }
    for (uint32_t i = 0; i < hw_config.ext_count; ++i) {
        uint32_t efid1 = (hw_config.ext_filters[i] >> 32) & max_ext_id;
        uint32_t efid2 = (hw_config.ext_filters[i] & max_ext_id);
        uint32_t efec = (hw_config.ext_filters[i] >> 61) & 0x7;
        uint32_t eft = (hw_config.ext_filters[i] >> 30) & 0x3;
        std::cout << std::format("ef[{:2d}]: {} 0x{:08x} 0x{:08x} {}\n", i, ft_str[eft], efid1, efid2, fec_str[efec]);
    }
}

// Explicit template instantiation for G0 and H7
// fdcan for stm32g0: 28 standard filters, 8 extended filters, first byte of usb packet 1
template class canfilter_fdcan<28, 8, CANFILTER_DEV_FDCAN_G0>;
// fdcan for stm32h7: 128 standard filters, 64 extended filters, first byte of usb packet 2
template class canfilter_fdcan<128, 64, CANFILTER_DEV_FDCAN_H7>;

// Constructor implementations for G0 and H7
canfilter_fdcan_g0::canfilter_fdcan_g0() : canfilter_fdcan<28, 8, CANFILTER_DEV_FDCAN_G0>() {}
canfilter_fdcan_h7::canfilter_fdcan_h7() : canfilter_fdcan<128, 64, CANFILTER_DEV_FDCAN_H7>() {}
