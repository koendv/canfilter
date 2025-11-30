// main.cpp
//
// Command-line utility for generating and programming CAN hardware filters.
// This program uses the canfilter_* classes to build hardware-ready filter
// configurations for bxCAN (STM32 F0/F1/F3/F4/F7) or FDCAN (STM32 G0/H7) devices.
//
// Features:
//   - Parses individual CAN IDs and ranges from the command line (decimal or hex)
//   - Supports convenience options such as allow-all, verbose output, and dry-run
//   - Detects or selects the target hardware filter type (auto, bxcan_f0, bxcan_f4, fdcan_g0, fdcan_h7)
//   - Optionally programs a connected USB device using canfilter_usb
//   - Provides debug output of filter contents and hardware register layout
//
// Workflow:
//   1. Parse command-line arguments and options
//   2. Construct an appropriate canfilter_* instance for the selected hardware
//   3. Accumulate IDs/ranges and finalize the filter
//   4. Optionally program the filter into a USB-connected device
//
// All filter construction is compute-only. USB programming is handled separately
// via canfilter_usb and does not affect the filter-building logic.

#include "canfilter.hpp"
#include "canfilter_bxcan.hpp"
#include "canfilter_fdcan.hpp"
#include "canfilter_usb.hpp"
#include <format>
#include <iostream>
#include <memory>
#include <string>

// Print help message
void print_help(const char *prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS] [IDs/RANGES]\n"
              << "Generate and program hardware CAN filters\n\n"
              << "IDs: Single CAN IDs (0x100, 256, 0x1000)\n"
              << "RANGES: CAN ID ranges (0x100-0x1FF, 256-511, 0x1000-0x1FFF)\n\n"
              << "Options:\n"
              << "  -o, --output MODE      Output mode: auto, bxcan_f0, bxcan_f4, fdcan_g0, fdcan_h7\n"
              << "  -a, --allow-all        Allow all packets\n"
              << "  -v, --verbose          Enable verbose output\n"
              << "  -d, --dry-run          Do not program hardware; just print filter configuration\n"
              << "  -u, --usb vid:pid      Device in format vid:pid[@serial]\n"
              << "  -h, --help             Show this help\n"
              << "\nExamples:\n"
              << "  " << prog_name << " 0x100 0x200-0x2FF\n"
              << "  " << prog_name << " -a\n"
              << "  " << prog_name << " -o bxcan_f0 0x100,0x101,0x200-0x2FF --dry-run\n"
              << std::endl;
}

void print_error(canfilter_error_t err) {
    switch (err) {
        case CANFILTER_SUCCESS:
            std::cout << "operation completed successfully" << std::endl;
            break;
        case CANFILTER_ERROR_PARAM:
            std::cout << "invalid parameter (id out of range or invalid range)" << std::endl;
            break;
        case CANFILTER_ERROR_FULL:
            std::cout << "no more filter banks available" << std::endl;
            break;
        case CANFILTER_ERROR_PLATFORM:
            std::cout << "usb communication failed or hardware not found" << std::endl;
            break;
        default:
            std::cout << "invalid error code" << std::endl;
            break;
    }
}

bool parse_usb(const std::string &arg, uint16_t &vid, uint16_t &pid, std::string &serial) {
    serial.clear();

    // find optional @serial
    size_t atPos = arg.find('@');
    std::string mainPart = arg.substr(0, atPos);

    if (atPos != std::string::npos) {
        serial = arg.substr(atPos + 1);
    }

    // split vid:pid
    size_t colonPos = mainPart.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }

    std::string vidStr = mainPart.substr(0, colonPos);
    std::string pidStr = mainPart.substr(colonPos + 1);

    // convert from hex
    try {
        vid = static_cast<uint16_t>(std::stoul(vidStr, nullptr, 16));
        pid = static_cast<uint16_t>(std::stoul(pidStr, nullptr, 16));
    } catch (...) {
        return false;
    }

    return true;
}

bool canfilter_cli(int argc, char *argv[]) {
    std::unique_ptr<canfilter> filter;
    std::vector<std::string> filter_args;
    std::string output_mode = "auto";
    int verbose = 0;
    bool dry_run = false;
    bool allow_all = false;

    canfilter_usb usb_device;
    uint16_t usb_vid = 0;
    uint16_t usb_pid = 0;
    std::string usb_serial;
    bool usb_specified = false;

    /* parse options */
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                std::cerr << "error: missing output mode" << std::endl;
                return false;
            }
            output_mode = argv[i];
        } else if (arg == "-v" || arg == "--verbose") {
            verbose++;
        } else if (arg == "-a" || arg == "--allow-all") {
            allow_all = true;
        } else if (arg == "-d" || arg == "--dry-run") {
            dry_run = true;
        } else if (arg == "-u" || arg == "--usb") {
            if (++i >= argc) {
                std::cerr << "error: missing usb vid:pid" << std::endl;
                return false;
            }
            if (!parse_usb(argv[i], usb_vid, usb_pid, usb_serial)) {
                std::cerr << "error: not vid:pid[@serial]" << std::endl;
                return false;
            }
            usb_specified = true;
        } else if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
        } else if (arg[0] == '-') {
            std::cerr << "error: invalid argument: " << arg << std::endl;
            return false;
        } else {
            filter_args.push_back(arg); // filter argument
        }
    }

    // open usb device if vid:pid given
    if (usb_specified) {
        if (!usb_device.open(usb_vid, usb_pid, usb_serial)) {
            std::cerr << "error: could not open device" << std::endl;
            return false;
        } else if (verbose)
            std::cerr << "usb device open success" << std::endl;
    }

    // create canbus filter
    canfilter_hardware_t hw_filter = CANFILTER_DEV_NONE;
    if (output_mode == "auto") {
        if (usb_device.hasHardwareFilter()) {
            hw_filter = (canfilter_hardware_t)usb_device.getFilterInfo();
        } else {
            std::cerr << "error: no hardware filter\n";
            return false;
        }
    } else if (output_mode == "bxcan_f0") {
        hw_filter = CANFILTER_DEV_BXCAN_F0;
    } else if (output_mode == "bxcan_f4") {
        hw_filter = CANFILTER_DEV_BXCAN_F4;
    } else if (output_mode == "fdcan_g0") {
        hw_filter = CANFILTER_DEV_FDCAN_G0;
    } else if (output_mode == "fdcan_h7") {
        hw_filter = CANFILTER_DEV_FDCAN_H7;
    } else {
        std::cerr << "error: invalid output mode " << output_mode << std::endl;
        return false;
    }

    switch (hw_filter) {
        case CANFILTER_DEV_BXCAN_F0:
            filter.reset(new canfilter_bxcan_f0());
            if (verbose)
                std::cerr << "Using bxCAN (F0/F1/F3) with 14 filter banks" << std::endl;
            break;
        case CANFILTER_DEV_BXCAN_F4:
            filter.reset(new canfilter_bxcan_f4());
            if (verbose)
                std::cerr << "Using bxCAN (F4/F7) with 28 filter banks" << std::endl;
            break;
        case CANFILTER_DEV_FDCAN_G0:
            filter.reset(new canfilter_fdcan_g0());
            if (verbose)
                std::cerr << "Using FDCAN (G0) with 28 standard, 8 extended filters" << std::endl;
            break;
        case CANFILTER_DEV_FDCAN_H7:
            filter.reset(new canfilter_fdcan_h7());
            if (verbose)
                std::cerr << "Using FDCAN (H7) with 128 standard, 64 extended filters" << std::endl;
            break;
        case CANFILTER_DEV_NONE:
        default:
            std::cerr << "error: invalid output mode " << output_mode << std::endl;
            return false;
            break;
    }

    filter->verbose = verbose;

    // parse filter arguments
    filter->begin();

    if (allow_all)
        filter->allow_all();

    if (!filter_args.empty() && !filter->parse(filter_args)) {
        std::cerr << "error: failed to parse filter arguments\n";
        return false;
    }

    filter->end();

    if (!allow_all && filter_args.empty()) {
        if (verbose)
            std::cerr << "no filter specified" << std::endl;
        return false;
    }

    // debugging
    if (verbose > 1) {
        // print registers as ranges and ids
        filter->debug_print();

        if (verbose > 2)
            // dump registers in hex
            filter->debug_print_reg();
    }

    // usage
    if (verbose)
        std::cout << "\n";
    filter->print_usage();

    // no programming if dry run.
    if (dry_run) {
        if (verbose)
            std::cerr << "not programming hardware" << std::endl;
        return true;
    }

    // program hardware filter
    bool success = usb_device.programFilter(filter->get_hw_config(), filter->get_hw_size());
    if (!success)
        std::cerr << "usb programming fail\n";
    else if (verbose)
        std::cerr << "usb programming success\n";

    return success;
}

int main(int argc, char *argv[]) {

    if (!canfilter_cli(argc, argv)) {
        return 1;
    }

    return 0;
}
