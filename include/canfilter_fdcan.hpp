#ifndef CANFILTER_FDCAN_H
#define CANFILTER_FDCAN_H

// canfilter_fdcan
//
// Templated builder for FDCAN (Bosch M_CAN) filter tables, used in STM32 G0/H7.
// Accumulates standard (11-bit) and extended (29-bit) CAN IDs and ranges,
// then serializes them into hardware table entries.
//
// Each table entry can encode either a pair of IDs or a start/end range. The
// builder handles accumulation of IDs into pairs, immediate emission of ranges,
// and ensures per-device limits (max_std_filter, max_ext_filter) are not exceeded.
//
// Key features:
//   • std_id / ext_id arrays hold IDs until they can be serialized into table entries
//   • emit_*() methods create raw filter descriptors in hw_config
//   • end() finalizes the table for hardware consumption
//
// This class is fully compute-only and platform-independent. It produces the
// hardware-ready table format expected by firmware or USB loaders, without
// accessing any MCU registers or relying on platform-specific headers.

#include "canfilter.hpp"
#include <cstring> // for std::memset

// Base class: canfilter_fdcan
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val> class canfilter_fdcan : public canfilter {
  public:
    // Hardware configuration struct (nested) with packed and aligned attributes
    struct hw_t {
        uint8_t dev;                // CANFILTER_DEV_FDCAN_G0/CANFILTER_DEV_FDCAN_H7
        uint8_t std_filter_nbr = 0; // number of standard filters
        uint8_t ext_filter_nbr = 0; // number of extended filters
        uint8_t reserved[1];

        uint32_t std_filter[max_std_filter];    // Fixed-size array for standard filters
        uint32_t ext_filter[max_ext_filter][2]; // Fixed-size array for extended filters

        // Constructor initializes arrays to zero
        hw_t() {
            std::memset(this, 0, sizeof(hw_t));
        }
    } __attribute__((packed, aligned(4)));

    hw_t hw_config; // Hardware configuration for the specific CAN dev

    // Constructor that takes max_std_filter and max_ext_filter as arguments
    canfilter_fdcan();

    // Override methods (same for all versions)
    canfilter_error_t begin() override;
    canfilter_error_t add_std_id(uint32_t id) override;
    canfilter_error_t add_ext_id(uint32_t id) override;
    canfilter_error_t add_std_range(uint32_t start, uint32_t end) override;
    canfilter_error_t add_ext_range(uint32_t start, uint32_t end) override;
    canfilter_error_t end() override;

    void *get_hw_config() override;
    size_t get_hw_size() override;

    void debug_print_reg() const override;
    void debug_print() const override;
    void print_usage() const override;

  private:
    // Extended IDs
    uint32_t ext_id[2];
    uint8_t ext_id_count = 0;

    // Standard IDs
    uint32_t std_id[2];
    uint8_t std_id_count = 0;

    // Write filter bank
    canfilter_error_t emit_std_id(uint32_t id1, uint32_t id2);
    canfilter_error_t emit_std_range(uint32_t id1, uint32_t id2);
    canfilter_error_t emit_ext_id(uint32_t id1, uint32_t id2);
    canfilter_error_t emit_ext_range(uint32_t id1, uint32_t id2);
};

// FD-CAN G0 specific implementation (specializing the template with specific filter sizes)
class canfilter_fdcan_g0 : public canfilter_fdcan<28, 8, CANFILTER_DEV_FDCAN_G0> {
  public:
    canfilter_fdcan_g0(); // Constructor
};

// FD-CAN H7 specific implementation (specializing the template with specific filter sizes)
class canfilter_fdcan_h7 : public canfilter_fdcan<128, 64, CANFILTER_DEV_FDCAN_H7> {
  public:
    canfilter_fdcan_h7(); // Constructor
};

#endif
