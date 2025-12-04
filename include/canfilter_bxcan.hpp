#pragma once

#include "canfilter.hpp"
#include <cstdint>
#include <cstring>

/* bxCAN filter template class */
template <uint8_t max_banks_t, uint8_t dev_val> class canfilter_bxcan : public canfilter {
  public:
    // Maximum number of bxCAN filter banks (template parameter)
    static constexpr uint8_t max_banks = max_banks_t;

    // Hardware configuration struct (nested)
    struct hw_t {
        uint8_t dev; // CANFILTER_DEV_BXCAN_F0 or CANFILTER_DEV_BXCAN_F4
        uint8_t reserved[3];
        uint32_t fs1r;
        uint32_t fm1r;
        uint32_t ffa1r;
        uint32_t fa1r;
        uint32_t fr1[max_banks];
        uint32_t fr2[max_banks];

        // Initialize to zero
        hw_t() {
            std::memset(this, 0, sizeof(hw_t));
        }
    } __attribute__((packed, aligned(4)));

    hw_t hw_config;

    canfilter_bxcan();
    void begin() override;
    canfilter_error_t end() override;
    canfilter_error_t program() const override;

    canfilter_error_t add_std_id(uint32_t id) override;
    canfilter_error_t add_ext_id(uint32_t id) override;
    canfilter_error_t add_std_range(uint32_t begin, uint32_t end) override;
    canfilter_error_t add_ext_range(uint32_t begin, uint32_t end) override;

    void debug_print_reg() const override;
    void debug_print() const override;
    void print_usage() const override;

  private:
    uint32_t bank = 0; /* current register bank */

    // Extended IDs
    uint32_t ext_list[2];
    uint32_t ext_list_count = 0;

    // Standard masks for ranges
    struct std_mask_t {
        uint32_t id;
        uint32_t mask;
    };
    std_mask_t std_mask[2];
    uint32_t std_mask_count = 0;

    // Standard individual IDs
    uint32_t std_list[4];
    uint32_t std_list_count = 0;

    // helpers
    int std_largest_prefix(uint32_t begin, uint32_t end) const;
    int ext_largest_prefix(uint32_t begin, uint32_t end) const;

    // Write filter banks
    canfilter_error_t emit_std_list(uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4);
    canfilter_error_t emit_std_mask(uint32_t id1, uint32_t mask1, uint32_t id2, uint32_t mask2);
    canfilter_error_t emit_ext_list(uint32_t id1, uint32_t id2);
    canfilter_error_t emit_ext_mask(uint32_t id1, uint32_t mask1);

    // Add to list or mask
    canfilter_error_t add_std_list(uint32_t id);
    canfilter_error_t add_std_mask(uint32_t id, uint32_t mask);
    canfilter_error_t add_ext_list(uint32_t id);
    canfilter_error_t add_ext_mask(uint32_t id, uint32_t mask);
};

// bxCAN for STM32F0/F1/F3 (14 banks)
class canfilter_bxcan_f0 : public canfilter_bxcan<14, CANFILTER_DEV_BXCAN_F0> {
  public:
    canfilter_bxcan_f0();
};

// bxCAN for STM32F4/F7 (28 banks)
class canfilter_bxcan_f4 : public canfilter_bxcan<28, CANFILTER_DEV_BXCAN_F4> {
  public:
    canfilter_bxcan_f4();
};
