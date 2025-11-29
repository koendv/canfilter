#pragma once

#include <cstdint>
#include <iostream>
#include <string_view>

/* Controller types - MUST MATCH CANDLELIGHT_FW */
#define CANFILTER_DEV_BXCAN 0
#define CANFILTER_DEV_FDCAN_G0 1
#define CANFILTER_DEV_FDCAN_H7 2

/* Error codes */
typedef enum {
    CANFILTER_SUCCESS = 0,
    CANFILTER_ERROR_PARAM,
    CANFILTER_ERROR_FULL,
    CANFILTER_ERROR_PLATFORM,
} canfilter_error_t;

class canfilter {
  protected:
  public:
    uint8_t verbose = 0; // Verbosity level (0 = no output, 1 = verbose)

    // Maximum IDs
    static constexpr uint32_t max_std_id = 0x7FF;      // Standard CAN
    static constexpr uint32_t max_ext_id = 0x1FFFFFFF; // Extended CAN

    virtual ~canfilter() = default;

    // Initialize filter
    virtual void begin() = 0;

    // Finalize filter configuration
    virtual canfilter_error_t end() = 0;

    // Program hardware
    virtual canfilter_error_t program() = 0;

    // Add standard ID
    virtual canfilter_error_t add_std_id(uint32_t id) = 0;

    // Add extended ID
    virtual canfilter_error_t add_ext_id(uint32_t id) = 0;

    // Add standard range
    virtual canfilter_error_t add_std_range(uint32_t start, uint32_t end) = 0;

    // Add extended range
    virtual canfilter_error_t add_ext_range(uint32_t start, uint32_t end) = 0;

    // Print debug information
    virtual void debug_print_reg() const = 0;
    virtual void debug_print() const = 0;

    // Allow all traffic (standard + extended IDs)
    canfilter_error_t allow_all() {
        canfilter_error_t err = add_std_range(0, max_std_id);
        if (err != CANFILTER_SUCCESS)
            return err;
        return add_ext_range(0, max_ext_id);
    }

    bool parse(std::string_view input);
};
