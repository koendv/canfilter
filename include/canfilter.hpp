#ifndef CANFILTER_H
#define CANFILTER_H

// canfilter
//
// Controller-agnostic base class for constructing CAN hardware acceptance filters.
// This class defines the abstract operations common to all implementations
// without any hardware-specific behavior.
//
// Concrete subclasses translate the user’s high-level filter definitions into
// a hardware-ready format. Typical workflow:
//   1. begin()  – reset/clear filter state
//   2. add_*()  – add individual IDs or ID ranges
//   3. end()    – finalize the filter for hardware
//
// The class also provides:
//   * allow_all() – convenience to accept all standard and extended IDs
//   * parse()     – interpret text filter definitions (decimal or hex, single IDs or ranges)
//   * debug_*()   – inspect the internal state
//
// All operations are compute-only; no assumptions are made about the platform
// or execution environment.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/* Controller types - MUST MATCH CANDLELIGHT_FW */
typedef enum {
    CANFILTER_DEV_NONE = 0, /* no hardware filter */
    CANFILTER_DEV_BXCAN_F0, /* bxcan on F0/F1/F3, 14 filters */
    CANFILTER_DEV_BXCAN_F4, /* bxcan on F4/F7, 28 filters */
    CANFILTER_DEV_FDCAN_G0, /* bosch m_can, 28 standard, 8 extended filters */
    CANFILTER_DEV_FDCAN_H7, /* bosch m_can, 128 standard, 64 extended filters */
} canfilter_hardware_t;

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
    static constexpr uint32_t max_std_id = 0x7FFU;      // Standard CAN
    static constexpr uint32_t max_ext_id = 0x1FFFFFFFU; // Extended CAN

    virtual ~canfilter() = default;

    // Initialize filter
    virtual canfilter_error_t begin() = 0;

    // Add standard ID
    virtual canfilter_error_t add_std_id(uint32_t id) = 0;

    // Add extended ID
    virtual canfilter_error_t add_ext_id(uint32_t id) = 0;

    // Add standard range
    virtual canfilter_error_t add_std_range(uint32_t start, uint32_t end) = 0;

    // Add extended range
    virtual canfilter_error_t add_ext_range(uint32_t start, uint32_t end) = 0;

    // Finalize filter configuration
    virtual canfilter_error_t end() = 0;

    // Access to hardware config
    virtual void *get_hw_config() = 0;
    virtual size_t get_hw_size() = 0;

    // Print debug information
    virtual void debug_print_reg() const = 0;
    virtual void debug_print() const = 0;
    virtual void print_usage() const = 0;

    // Allow all traffic (standard + extended IDs)
    canfilter_error_t allow_all() {
        canfilter_error_t err = add_std_range(0, max_std_id);
        if (err != CANFILTER_SUCCESS)
            return err;
        return add_ext_range(0, max_ext_id);
    }

    // Parse list of ID's and ranges
    bool parse(const std::string &arg);
    bool parse(const std::vector<std::string> &args);
};

#endif
