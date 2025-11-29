#include "canfilter.hpp"
#include "canfilter_bxcan.hpp"
#include "canfilter_fdcan.hpp"
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
              << "  -o, --output MODE      Output mode: bxcan, fdcan_g0, fdcan_h7\n"
              << "  -a, --allow-all        Allow all packets\n"
              << "  -v, --verbose          Enable verbose output\n"
              << "  -d, --dry-run          Do not program hardware; just print filter configuration\n"
              << "  -h, --help             Show this help\n"
              << "\nExamples:\n"
              << "  " << prog_name << " -o bxcan 0x100 0x200-0x2FF\n"
              << "  " << prog_name << " -o fdcan_g0 -a\n"
              << "  " << prog_name << " -o fdcan_h7 0x100,0x101,0x200-0x2FF --dry-run\n"
              << std::endl;
}

bool canfilter_cli(int argc, char *argv[]) {
    std::unique_ptr<canfilter> filter;
    std::string output_mode = "bxcan";
    int verbose = 0;
    bool dry_run = false;
    bool allow_all = false;
    bool has_data = false;

    /* first pass: options */
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                std::cerr << std::format("error: missing output mode\n");
                return false;
            }
            output_mode = argv[i];
        } else if (arg == "-v" || arg == "--verbose") {
            verbose++;
        } else if (arg == "-a" || arg == "--allow-all") {
            allow_all = true;
            has_data = true;
        } else if (arg == "-d" || arg == "--dry-run") {
            dry_run = true;
        } else if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
        } else if (isdigit(arg[0]))
            continue;
        else {
            std::cerr << std::format("error: invalid argument: {}\n", arg);
            return false;
        }
    }

    // create canbus filter
    if (output_mode == "bxcan") {
        filter = std::make_unique<canfilter_bxcan>();
    } else if (output_mode == "fdcan_g0") {
        filter = std::make_unique<canfilter_fdcan_g0>();
    } else if (output_mode == "fdcan_h7") {
        filter = std::make_unique<canfilter_fdcan_h7>();
    } else {
        std::cerr << std::format("error: invalid output mode {}\n", output_mode);
        return false;
    }
    filter->verbose = verbose;

    // second pass: add id's and ranges to filter
    filter->begin();

    if (allow_all)
        filter->allow_all();

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-')
            continue;
        if (filter->parse(argv[i]))
            has_data = true;
    }
    filter->end();

    if (!has_data) {
        if (verbose)
            std::cerr << std::format("filter empty\n");
        return false;
    }

    // debugging
    if (verbose) {
        filter->debug_print();

        if (verbose > 1)
            filter->debug_print_reg();
    }

    // program hardware filter
    if (dry_run) {
        if (verbose)
            std::cerr << std::format("not programming hardware\n");
        return true;
    }

    canfilter_error_t err = filter->program();

    // error code
    if (verbose) {
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

    return err;
}

int main(int argc, char *argv[]) {

    if (!canfilter_cli(argc, argv)) {
        return 1;
    }

    return 0;
}
