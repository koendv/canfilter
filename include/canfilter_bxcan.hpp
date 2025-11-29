#pragma once

#include "canfilter.hpp"
#include <cstdint>
#include <cstring>

/* bxCAN filter class */
class canfilter_bxcan : public canfilter {
  public:
    // Maximum number of bxCAN filter banks
    static constexpr uint8_t max_banks = 14;

    // Hardware configuration struct (nested)
    struct hw_t {
        uint8_t dev; // CANFILTER_DEV_BXCAN = 0
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
    int std_largest_prefix(uint32_t begin, uint32_t end);
    int ext_largest_prefix(uint32_t begin, uint32_t end);

    // Write filter banks
    canfilter_error_t emit_std_list(uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4);
    canfilter_error_t emit_std_mask(uint32_t id1, uint32_t mask1, uint32_t id2, uint32_t mask2);
    canfilter_error_t emit_ext_list(uint32_t id1, uint32_t id2);
    canfilter_error_t emit_ext_mask(uint32_t id1, uint32_t mask1);

    canfilter_error_t add_std_list(uint32_t id);
    canfilter_error_t add_std_mask(uint32_t id, uint32_t mask);
    canfilter_error_t add_ext_list(uint32_t id);
    canfilter_error_t add_ext_mask(uint32_t id, uint32_t mask);
};
