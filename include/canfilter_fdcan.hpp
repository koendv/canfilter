#pragma once

#include "canfilter.hpp"
#include <cstring> // for std::memset

// Base class: canfilter_fdcan
template <uint32_t max_std_filter, uint32_t max_ext_filter, uint32_t dev_val> class canfilter_fdcan : public canfilter {
  public:
    // Hardware configuration struct (nested) with packed and aligned attributes
    struct hw_t {
        uint8_t dev; // CANFILTER_DEV_BXCAN = 0
        uint8_t std_count = 0;
        uint8_t ext_count = 0;
        uint8_t reserved[1];

        uint32_t std_filters[max_std_filter]; // Fixed-size array for standard filters
        uint64_t ext_filters[max_ext_filter]; // Fixed-size array for extended filters

        // Constructor initializes arrays to zero
        hw_t() {
            std::memset(this, 0, sizeof(hw_t));
        }
    } __attribute__((packed, aligned(4)));

    hw_t hw_config; // Hardware configuration for the specific CAN dev

    // Constructor that takes max_std_filter and max_ext_filter as arguments
    canfilter_fdcan();

    // Override methods (same for all versions)
    void begin() override;
    canfilter_error_t end() override;
    canfilter_error_t program() override;
    canfilter_error_t add_std_id(uint32_t id) override;
    canfilter_error_t add_ext_id(uint32_t id) override;
    canfilter_error_t add_std_range(uint32_t start, uint32_t end) override;
    canfilter_error_t add_ext_range(uint32_t start, uint32_t end) override;
    void debug_print_reg() const override;
    void debug_print() const override;

  private:
    // Extended IDs
    uint32_t ext_id[2];
    uint8_t ext_id_count = 0;

    // Standard IDs
    uint32_t std_id[2];
    uint8_t std_id_count = 0;
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
