/*
 * Copyright 2010-2013 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, version 2.1.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * Disclaimer: The codes contained in these modules may be specific
 * to the Intel Software Development Platform codenamed Knights Ferry,
 * and the Intel product codenamed Knights Corner, and are not backward
 * compatible with other Intel products. Additionally, Intel will NOT
 * support the codes or instruction set in future products.
 *
 * Intel offers no warranty of any kind regarding the code. This code is
 * licensed on an "AS IS" basis and Intel is not obligated to provide
 * any support, assistance, installation, training, or other services
 * of any kind. Intel is also not obligated to provide any updates,
 * enhancements or extensions. Intel specifically disclaims any warranty
 * of merchantability, non-infringement, fitness for any particular
 * purpose, and any other warranty.
 *
 * Further, Intel disclaims all liability of any kind, including but
 * not limited to liability for infringement of any proprietary rights,
 * relating to the use of the code, even if Intel is notified of the
 * possibility of such liability. Except as expressly stated in an Intel
 * license agreement provided with this code and agreed upon with Intel,
 * no license, express or implied, by estoppel or otherwise, to any
 * intellectual property rights is granted herein.
 */

#include "micsmc_cli.h"
using apps::micsmc_cli;

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <stdint.h>

#include <iostream>
using std::cout;
using std::endl;
using std::fixed;
using std::ios;
using std::hex;
using std::dec;
using std::left;
using std::right;

#include <iomanip>
using std::setprecision;
using std::setw;

#include <sstream>
using std::stringstream;
using std::ostringstream;

#include <exception>
using std::exception;

#include <algorithm>
using std::min;
using std::max;

#include "micsmc_helper.h"
#include "micmgmt_exception.h"
#include "micsmc_exception.h"

const char *micsmc_cli::PROGRAM_NAME = "micsmc";
const char *micsmc_cli::MICSMC_GUI_PATH = "micsmc-gui";

const unsigned int micsmc_cli::CORE_UTIL_SAMPLE_INTERVAL = 900;
const unsigned int micsmc_cli::ECC_OP_STATUS_INTERVAL = 900;
const unsigned int micsmc_cli::ECC_OP_LONG_STATUS_INTERVAL = 5000;

const unsigned int micsmc_cli::AVG_THROTTLE_SAMPLE_COUNT    = 10;
const unsigned int micsmc_cli::AVG_THROTTLE_SAMPLE_DELAY_MS = 100;

micsmc_cli::micsmc_cli(int argc, char *argv[]) :
    verbose_(false),
    timeout_(DEFAULT_TIMEOUT),
    exit_code_(0)
{
    try {
        configure_micsmc_cmd(&config_);
        args_.parse_args(argc, argv);
    } catch (const exception &e) {
        error_msg("an unexpected error has occurred during application "
                  "initialization: %s", e.what());
        exit(exit_code_);
    }
}

micsmc_cli::~micsmc_cli()
{
}

int micsmc_cli::run()
{
    if (!config_.process_cmd_line_args(&args_))
        exit_code_ = 1;

    if (args_.get_num_args() == 0) {
        // GUI
        exit_code_ = system(MICSMC_GUI_PATH);
    } else {
        // CLI
        cmd_line_args::opt_args *opt;

        // If the timeout option was specified, process it first
        if ((opt = args_.get_opt_args("--timeout")) != NULL) {
            perform_timeout_opt(*opt);
            opt->processed = true;
        }

        // Verbose?
        if ((opt = args_.get_opt_args("--verbose")) != NULL) {
            verbose_ = true;
            opt->processed = true;
        }

        for (size_t i = 0; i < args_.get_num_args(); i++) {
            opt = args_.get_opt_args(i);
            if (opt->error_code == cmd_line_args::OPT_OK && !opt->processed) {
                if (opt->duplicate) {
                    warning_msg(false, "Duplicate command-line option '%s' "
                                "was ignored", opt->option.c_str());
                }
                perform_option(*opt);
                opt->processed = true;
            }
        }
    }

    return exit_code_;
}

void micsmc_cli::perform_option(const cmd_line_args::opt_args &opt)
{
    if (opt.option == "-a" || opt.option == "--all") {
        perform_all(opt);
    } else if (opt.option == "-c" || opt.option == "--cores") {
        perform_cores_opt(opt);
    } else if (opt.option == "-f" || opt.option == "--freq") {
        perform_freq_opt(opt);
    } else if (opt.option == "-i" || opt.option == "--info") {
        perform_info_opt(opt);
    } else if (opt.option == "-l" || opt.option == "--lost") {
        perform_lost_opt();
    } else if (opt.option == "--online") {
        perform_online_opt();
    } else if (opt.option == "--offline") {
        perform_offline_opt();
    } else if (opt.option == "-m" || opt.option == "--mem") {
        perform_mem_opt(opt);
    } else if (opt.option == "-t" || opt.option == "--temp") {
        perform_temp_opt(opt);
    } else if (opt.option == "--ecc") {
        perform_ecc_opt(opt);
    } else if (opt.option == "--turbo") {
        perform_turbo_opt(opt);
    } else if (opt.option == "--led") {
        perform_led_opt(opt);
    } else if (opt.option == "--pthrottle") {
        perform_pthrottle_opt(opt);
    } else if (opt.option == "--tthrottle") {
        perform_tthrottle_opt(opt);
    } else if (opt.option == "--pwrenable") {
        perform_pwrenable_opt(opt);
    } else if (opt.option == "--pwrstatus") {
        perform_pwrstatus_opt(opt);
    } else if (opt.option == "--timeout") {
        perform_timeout_opt(opt);
    } else if (opt.option == "-h" || opt.option == "--help") {
        perform_help_opt(opt);
    } else if (opt.option == "-v" || opt.option == "--version") {
        perform_version_opt();
    } else {
        // Huh???  If this happens, the micsmc command options have not
        // been configured correctly :|
        error_msg("unexpected error: the command-line options could not be "
                  "parsed correctly");
    }
}

void micsmc_cli::perform_all(const cmd_line_args::opt_args &opt)
{
    mic_device_class *device;
    string dev_name;

    vector<unsigned int> device_nums;
    vector<unsigned int> avail_dev_nums;

    if (!get_device_nums(opt.args.begin(), opt.args.end(), &device_nums, "all"))
        return; // Unable to get devices

    // Check which devices are actually available
    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (validate_device_is_online(dev_name, *device))
                avail_dev_nums.push_back(*it);
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }

    // Call the other options only if there are any devices available
    if (!avail_dev_nums.empty()) {
        perform_info_opt(avail_dev_nums, true);
        perform_temp_opt(avail_dev_nums, true);
        perform_freq_opt(avail_dev_nums, true);
        perform_mem_opt(avail_dev_nums, true);
        perform_cores_opt(avail_dev_nums, true);
    }
}

void micsmc_cli::perform_cores_opt(const cmd_line_args::opt_args &opt)
{
    vector<unsigned int> device_nums;

    if (!get_device_nums(opt.args.begin(), opt.args.end(), &device_nums,
                         "cores"))
        return; // Unable to get devices

    perform_cores_opt(device_nums, false);
}

void micsmc_cli::perform_cores_opt(const vector<unsigned int> &device_nums,
                                   bool devices_verified)
{
    mic_device_class *device;
    string dev_name;
    mic_device_class::core_util_info sample1;
    mic_device_class::core_util_info sample2;
    micsmc_helper::device_util_info util;
    ostringstream core;
    host_platform::os_utils *os = host_platform::os_utils::get_instance();

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!devices_verified &&
                    !validate_device_is_online(dev_name, *device))
                continue;

            device->get_num_cores();
            device->get_core_util_info(&sample1);
            os->sleep_ms(CORE_UTIL_SAMPLE_INTERVAL);
            device->update_info(mic_device_class::CORE_UTIL_INFO);
            device->get_core_util_info(&sample2);
            micsmc_helper::compute_dev_utilization(*device, sample1, sample2,
                                                   &util);

            // Save cout config
            ios save(NULL);
            save.copyfmt(cout);
            // Format output
            cout << fixed << setprecision(2);

            cout << endl;
            cout << dev_name << " (cores):" << endl;
            cout << "   Device Utilization: ";

            cout << setw(5) << "User:"
                 << setw(7) << util.user_util
                 << setw(1) << "%,";

            cout << setw(10) << "System:"
                 << setw(7) << util.sys_util
                 << setw(1) << "%,";

            cout << setw(8) << "Idle:"
                 << setw(7) << util.idle_util
                 << setw(1) << "%";

            cout << endl;
            cout << "   Per Core Utilization ("
                 << device->get_num_cores()
                 << " cores in use)";
            cout << setw(0);
            cout << endl;
            for (uint16_t i = 0; i < device->get_num_cores(); i++) {
                cout << "      ";
                core << "Core #" << i + 1 << ":";
                cout << setw(10) << left << core.str();
                cout << right;
                cout << setw(6) << "User:"
                     << setw(7) << util.core_user_util[i]
                     << setw(1) << "%,";
                cout << setw(10) << "System:"
                     << setw(7) << util.core_sys_util[i]
                     << setw(1) << "%,";
                cout << setw(8) << "Idle:"
                     << setw(7) << util.core_idle_util[i]
                     << setw(1) << "%";
                cout << endl;
                core.str("");
            }

            // Restore
            cout.copyfmt(save);
        } catch (const micsmc_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "%s",
                          e.get_composite_err_msg().c_str());
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device core data: "
                          "%s", e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_freq_opt(const cmd_line_args::opt_args &opt)
{
    vector<unsigned int> device_nums;

    if (!get_device_nums(opt.args.begin(), opt.args.end(), &device_nums,
                         "freq"))
        return; // Unable to get devices

    perform_freq_opt(device_nums, false);
}

void micsmc_cli::perform_freq_opt(const vector<unsigned int> &device_nums,
                                  bool devices_verified)
{
    mic_device_class *device;
    string dev_name;

    double freq;
    double total_pwr;

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!devices_verified &&
                    !validate_device_is_online(dev_name, *device))
                continue;

            device->update_info(mic_device_class::CORES_INFO);
            device->update_info(mic_device_class::POWER_UTIL_INFO);
            device->update_info(mic_device_class::POWER_LIMIT_INFO);

            // Convert frequency from kHz to GHz
            freq = device->get_core_frequency() / 1000000.0;
            // Convert total power from uWatts to Watts
            total_pwr = device->get_total_pwr_win0() / 1000000.0;

            // Save cout config
            ios save(NULL);
            save.copyfmt(cout);
            // Format output
            cout << fixed << setprecision(2);

            cout << endl;
            cout << dev_name << " (freq):" << endl;
            cout << "   Core Frequency: .......... "
                 << freq << " GHz" << endl;
            cout << "   Total Power: ............. "
                 << total_pwr << " Watts" << endl;
            cout << "   Low Power Limit: ......... "
                 << (double)device->get_power_lmrk()
                 << " Watts" << endl;
            cout << "   High Power Limit: ........ "
                 << (double)device->get_power_hmrk()
                 << " Watts" << endl;
            cout << "   Physical Power Limit: .... "
                 << (double)device->get_power_phys_limit()
                 << " Watts" << endl;
            cout << "Please note that the power levels displayed cannot be ";
            cout << "used to determine idle power consumption." << endl;

            // Restore
            cout.copyfmt(save);
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device frequency "
                          "data: %s", e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_info_opt(const cmd_line_args::opt_args &opt)
{
    vector<unsigned int> device_nums;

    if (!get_device_nums(opt.args.begin(), opt.args.end(), &device_nums,
                         "info"))
        return; // Unable to get devices

    perform_info_opt(device_nums, false);
}

void micsmc_cli::perform_info_opt(const vector<unsigned int> &device_nums,
                                  bool devices_verified)
{
    mic_device_class *device;
    string dev_name;

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!devices_verified &&
                    !validate_device_is_online(dev_name, *device))
                continue;

            device->update_info(mic_device_class::PCI_CONFIG_INFO);
            device->update_info(mic_device_class::CORE_UTIL_INFO);
            device->update_info(mic_device_class::VERSION_INFO);
            device->update_info(mic_device_class::PROC_INFO);

            cout << endl;
            cout << dev_name << " (info):" << endl;
            cout << "   Device Series: ........... "
                 << device->get_device_series()
                 << endl;
            cout << "   Device ID: ............... 0x"
                 << hex << device->get_device_id() << dec << endl;
            cout << "   Stepping: ................ 0x"
                 << device->get_stepping_id() << endl;
            cout << "   Substepping: ............. 0x"
                 << device->get_substepping_id() << endl;
            cout << "   Coprocessor OS Version: .. "
                 << device->get_uos_version() << endl;
            cout << "   Flash Version: ........... "
                 << device->get_flash_version() << endl;
            cout << "   Host Driver Version: ..... "
                 << micsmc_helper::get_driver_version() << endl;
            cout << "   Number of Cores: ......... "
                 << device->get_num_cores() << endl;
        } catch (const micsmc_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "%s",
                          e.get_composite_err_msg().c_str());
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device "
                          "information: %s", e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_lost_opt()
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;
    micsmc_helper::device_status status;

    if (!get_avail_device_nums(&device_nums))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            status = micsmc_helper::get_device_status(*device);

            cout << endl;
            cout << dev_name << " (lost): ";
            cout << micsmc_helper::get_device_status_name(status) << endl;
            cout << "   State: ................... "
                 << device->get_state() << endl;
            cout << "   Mode: .................... "
                 << device->get_mode() << endl;
            cout << "   Image: ................... "
                 << device->get_image() << endl;
            cout << "   SCIF Status: ............. "
                 << device->get_scif_status() << endl;
            cout << "   Post Code: ............... "
                 << device->get_post_code() << endl;
            cout << "   Boot Count: .............. "
                 << device->get_boot_count() << endl;
            cout << "   Crash Count: ............. "
                 << device->get_crash_count() << endl;
        } catch (const micsmc_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "%s",
                          e.get_composite_err_msg().c_str());
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device status "
                          "information: %s", e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_online_opt()
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;
    ostringstream oss;
    bool empty = true;

    if (!get_avail_device_nums(&device_nums))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (micsmc_helper::get_device_status(*device) ==
                    micsmc_helper::DEV_STATUS_ONLINE) {
                if (!empty) {
                    oss << ", " << dev_name;
                } else {
                    oss << dev_name;
                    empty = false;
                }
            }
        } catch (const micsmc_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "%s",
                          e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }

    if (!empty)
        cout << oss.str() << endl;
    else
        cout << "None" << endl;
}

void micsmc_cli::perform_offline_opt()
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;
    ostringstream oss;
    bool empty = true;

    if (!get_avail_device_nums(&device_nums))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (micsmc_helper::get_device_status(*device) !=
                    micsmc_helper::DEV_STATUS_ONLINE) {
                if (!empty) {
                    oss << ", " << dev_name;
                } else {
                    oss << dev_name;
                    empty = false;
                }
            }
        } catch (const micsmc_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "%s",
                          e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }

    if (!empty)
        cout << oss.str() << endl;
    else
        cout << "None" << endl;
}

void micsmc_cli::perform_mem_opt(const cmd_line_args::opt_args &opt)
{
    vector<unsigned int> device_nums;

    if (!get_device_nums(opt.args.begin(), opt.args.end(), &device_nums, "mem"))
        return; // Unable to get devices

    perform_mem_opt(device_nums, false);
}

void micsmc_cli::perform_mem_opt(const vector<unsigned int> &device_nums,
                                 bool devices_verified)
{
    mic_device_class *device;
    string dev_name;
    double total_mem;
    double free_mem;
    double usage;

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!devices_verified &&
                    !validate_device_is_online(dev_name, *device))
                continue;

            device->update_info(mic_device_class::MEM_UTIL_INFO);

            total_mem = device->get_total_memory() / 1024.0;
            free_mem = device->get_available_memory() / 1024.0;
            usage = total_mem - free_mem;

            // Save cout config
            ios save(NULL);
            save.copyfmt(cout);
            // Format output
            cout << fixed << setprecision(2);

            cout << endl;
            cout << dev_name << " (mem):" << endl;
            cout << "   Free Memory: ............. "
                 << free_mem << " MB" << endl;
            cout << "   Total Memory: ............ "
                 << total_mem << " MB" << endl;
            cout << "   Memory Usage: ............ "
                 << usage << " MB" << endl;
            // Restore
            cout.copyfmt(save);
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device memory "
                          "data: %s", e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_temp_opt(const cmd_line_args::opt_args &opt)
{
    vector<unsigned int> device_nums;

    if (!get_device_nums(opt.args.begin(), opt.args.end(), &device_nums,
                         "temp"))
        return; // Unable to get devices

    perform_temp_opt(device_nums, false);
}

void micsmc_cli::perform_temp_opt(const vector<unsigned int> &device_nums,
                                  bool devices_verified)
{
    mic_device_class *device;
    string dev_name;

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!devices_verified &&
                    !validate_device_is_online(dev_name, *device))
                continue;

            device->update_info(mic_device_class::THERMAL_INFO);

            // Save cout config
            ios save(NULL);
            save.copyfmt(cout);
            // Format output
            cout << fixed << setprecision(2);

            cout << endl;
            cout << dev_name << " (temp):" << endl;
            cout << "   Cpu Temp: ................ "
                 << (double)device->get_die_temp() << " C";
            if (!device->die_temp_valid())
                cout << " (SMC reports sensor read invalid)";
            cout << endl;

            cout << "   Memory Temp: ............. "
                 << (double)device->get_mem_temp() << " C";
            if (!device->mem_temp_valid())
                cout << " (SMC reports sensor read invalid)";
            cout << endl;

            cout << "   Fan-In Temp: ............. "
                 << (double)device->get_fanin_temp() << " C";
            if (!device->fanin_temp_valid())
                cout << " (SMC reports sensor read invalid)";
            cout << endl;

            cout << "   Fan-Out Temp: ............ "
                 << (double)device->get_fanout_temp() << " C";
            if (!device->fanout_temp_valid())
                cout << " (SMC reports sensor read invalid)";
            cout << endl;

            cout << "   Core Rail Temp: .......... "
                 << (double)device->get_core_rail_temp() << " C";
            if (!device->core_rail_temp_valid())
                cout << " (SMC reports sensor read invalid)";
            cout << endl;

            cout << "   Uncore Rail Temp: ........ "
                 << (double)device->get_uncore_rail_temp()
                 << " C";
            if (!device->uncore_rail_temp_valid())
                cout << " (SMC reports sensor read invalid)";
            cout << endl;

            cout << "   Memory Rail Temp: ........ "
                 << (double)device->get_mem_rail_temp() << " C";
            if (!device->mem_rail_temp_valid())
                cout << " (SMC reports sensor read invalid)";
            cout << endl;

            // Restore
            cout.copyfmt(save);
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device "
                          "temperature data: %s",
                          e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_ecc_opt(const cmd_line_args::opt_args &opt)
{
    if (opt.args.empty()) {
        // No args: by default show ECC status for all devices
        perform_ecc_status(opt.args.begin(), opt.args.end());
    } else if (opt.args[0] == "status") {
        // Status option explicitly selected: show status for specified
        // devices
        perform_ecc_status(opt.args.begin() + 1, opt.args.end());
    } else if (opt.args[0] == "enable") {
        // Enable ECC mode for specified devices
        perform_ecc_set(true, opt.args.begin() + 1, opt.args.end());
    } else if (opt.args[0] == "disable") {
        // Disable ECC mode for specified devices
        perform_ecc_set(false, opt.args.begin() + 1, opt.args.end());
    } else {
        // No status operation selected, only devices were specified.
        // By default, show ECC status for specified devices
        perform_ecc_status(opt.args.begin(), opt.args.end());
    }
}

void micsmc_cli::perform_ecc_status(vector<string>::const_iterator begin,
                                    vector<string>::const_iterator end)
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;

    if (!get_device_nums(begin, end, &device_nums, "ecc"))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!validate_device_is_online(dev_name, *device))
                continue;

            device->update_info(mic_device_class::MEM_INFO);

            cout << endl;
            cout << dev_name << " (ecc):" << endl;
            cout << "   ECC is "
                 << (device->ecc_mode_enabled() ? "enabled" : "disabled")
                 << endl;
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device ECC mode "
                          "status: %s", e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_ecc_set(bool enabled,
                                 vector<string>::const_iterator begin,
                                 vector<string>::const_iterator end)
{
    vector<unsigned int> device_nums;
    string dev_name;
    ostringstream devices;
    unsigned int num_devices;
    vector<shared_ptr<host_platform::thread>> threads;

    if (!get_device_nums(begin, end, &device_nums, "ecc"))
        return; // Unable to get devices

    num_devices = device_nums.end() - device_nums.begin();

    try {
        ecc_op_master_work_area master(num_devices, this);
        ecc_status_work_area op_status(this, &master);

        for (vector<unsigned int>::const_iterator it = device_nums.begin();
                it != device_nums.end(); ++it) {
            dev_name.clear();
            set_device_name(&dev_name, *it);
            devices << dev_name;
            if (device_nums.end() - it > 1)
                devices << ", ";
        }
        info_msg("%s ECC on devices: %s...",
                 (enabled ? "enabling" : "disabling"),
                 devices.str().c_str());
        cout << endl;

        // Spawn status reporting thread
        if (verbose_)
            threads.push_back(host_platform::thread::new_thread(
                    ecc_verbose_status_thread, (void *)&op_status));
        else
            threads.push_back(host_platform::thread::new_thread(
                    ecc_status_thread, (void *)&op_status));

        // Spawn ECC operation threads...
        size_t i = 0;
        for (vector<unsigned int>::const_iterator it = device_nums.begin();
                it != device_nums.end(); ++it, i++) {
            try {
                master.op_work_areas_[i].assign_work(*it, enabled, timeout_);

                threads.push_back(host_platform::thread::new_thread(
                                  ecc_set_op_thread,
                                  (void *)&master.op_work_areas_[i]));
            } catch (const exception &e) {
                master.op_work_areas_[i].finish_with_msg(
                        MSG_ERROR,
                        string("an unexpected error has occurred: ") +
                        string(e.what()));
            }
        }

        // Wait for everyone to be done
        for (vector<shared_ptr<host_platform::thread>>::iterator it =
                threads.begin(); it != threads.end(); ++it) {
            (*it)->join();
        }

        // Report final status...
        ecc_op_step step;
        msg_type type;
        string msg;
        for (vector<ecc_op_work_area>::iterator it =
                master.op_work_areas_.begin();
                it != master.op_work_areas_.end(); ++it) {
            dev_name.clear();
            set_device_name(&dev_name, it->get_dev_num());
            it->get_status(&step, &type, &msg);
            status_msg_dev(dev_name.c_str(), type, msg.c_str());
        }

        if (master.at_least_one_success()) {
            cout << endl;
            info_msg("you must restart your Intel(R) Xeon Phi(TM) "
                     "Coprocessor(s) now for the ECC mode changes to take "
                     "effect");
        }
    } catch (const exception &e) {
        error_msg("an unexpected error has occurred: %s", e.what());
    }
}

void micsmc_cli::perform_turbo_opt(const cmd_line_args::opt_args &opt)
{
    if (opt.args.empty()) {
        // No args: by default show turbo mode status for all devices
        perform_turbo_status(opt.args.begin(), opt.args.end());
    } else if (opt.args[0] == "status") {
        // Status option explicitly selected: show status for specified
        // devices
        perform_turbo_status(opt.args.begin() + 1, opt.args.end());
    } else if (opt.args[0] == "enable") {
        // Enable turbo mode for specified devices
        perform_turbo_set(true, opt.args.begin() + 1, opt.args.end());
    } else if (opt.args[0] == "disable") {
        // Disable turbo mode for specified devices
        perform_turbo_set(false, opt.args.begin() + 1, opt.args.end());
    } else {
        // No status operation selected, only devices were specified.
        // By default, show turbo mode status for specified devices
        perform_turbo_status(opt.args.begin(), opt.args.end());
    }
}

void micsmc_cli::perform_turbo_status(vector<string>::const_iterator begin,
                                      vector<string>::const_iterator end)
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;
    uint16_t dev_id;

    if (!get_device_nums(begin, end, &device_nums, "turbo"))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!validate_device_is_online(dev_name, *device))
                continue;

            device->update_info(mic_device_class::TURBO_INFO);

            if (device->turbo_mode_available()) {
                cout << endl;
                cout << dev_name << " (turbo):"
                     << endl;
                cout << "   Turbo mode is "
                     << (device->turbo_mode_enabled() ? "enabled" : "disabled")
                     << endl;
            } else {
                dev_id = device->get_device_id();
                warning_msg_dev(dev_name.c_str(), true, "Turbo mode not "
                                "supported by this device:\n       Device "
                                "ID: 0x%x, stepping: 0x%x, substepping: 0x%x",
                                dev_id, device->get_stepping_id(),
                                device->get_substepping_id());
            }
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device turbo mode "
                          "status: %s", e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_turbo_set(bool enabled,
                                   vector<string>::const_iterator begin,
                                   vector<string>::const_iterator end)
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;
    uint16_t dev_id;

    if (!get_device_nums(begin, end, &device_nums, "turbo"))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!validate_device_is_online(dev_name, *device))
                continue;

            device->update_info(mic_device_class::TURBO_INFO);

            if (device->turbo_mode_available()) {
                device->set_turbo_mode(enabled);
                info_msg_dev(dev_name.c_str(), "turbo mode %s succeeded",
                             (enabled ? "enable" : "disable"));
            } else {
                dev_id = device->get_device_id();
                warning_msg_dev(dev_name.c_str(), true, "Turbo mode not "
                                "supported by this device:\n       Device "
                                "ID: 0x%x, stepping: 0x%x, substepping: 0x%x",
                                dev_id, device->get_stepping_id(),
                                device->get_substepping_id());
            }
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "unable to set device turbo mode: "
                          "%s", e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_led_opt(const cmd_line_args::opt_args &opt)
{
    if (opt.args.empty()) {
        // No args: by default show LED alert status for all devices
        perform_led_status(opt.args.begin(), opt.args.end());
    } else if (opt.args[0] == "status") {
        // Status option explicitly selected: show status for specified
        // devices
        perform_led_status(opt.args.begin() + 1, opt.args.end());
    } else if (opt.args[0] == "enable") {
        // Enable LED alert for specified devices
        perform_led_set(true, opt.args.begin() + 1, opt.args.end());
    } else if (opt.args[0] == "disable") {
        // Disable LED alert for specified devices
        perform_led_set(false, opt.args.begin() + 1, opt.args.end());
    } else {
        // No status operation selected, only devices were specified.
        // By default, show LED alert status for specified devices
        perform_led_status(opt.args.begin(), opt.args.end());
    }
}

void micsmc_cli::perform_led_status(vector<string>::const_iterator begin,
                                    vector<string>::const_iterator end)
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;
    bool led_enabled;

    if (!get_device_nums(begin, end, &device_nums, "led"))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!validate_device_is_online(dev_name, *device))
                continue;

            led_enabled = device->led_alert_enabled();

            cout << endl;
            cout << dev_name << " (led):" << endl;
            cout << "   LED alert is "
                 << (led_enabled ? "enabled" : "disabled")
                 << endl;
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device LED alert "
                          "status: %s",
                          e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_led_set(bool enabled,
                                 vector<string>::const_iterator begin,
                                 vector<string>::const_iterator end)
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;

    if (!get_device_nums(begin, end, &device_nums, "led"))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!validate_device_is_online(dev_name, *device))
                continue;

            device->set_led_alert(enabled);
            info_msg_dev(dev_name.c_str(), "LED alert %s succeeded",
                         (enabled ? "enable" : "disable"));
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "unable to set device LED alert: "
                          "%s", e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_pthrottle_opt(const cmd_line_args::opt_args &opt)
{
    mic_device_class *device = 0;

    host_platform::os_utils *os = host_platform::os_utils::get_instance();
    vector<unsigned int> device_nums;
    string dev_name;

    if (!get_device_nums(opt.args.begin(), opt.args.end(), &device_nums,
                         "pthrottle"))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!validate_device_is_online(dev_name, *device))
                continue;

            uint32_t  avg_current_len = 0;
            uint32_t  avg_count       = 0;
            uint32_t  sample_count    = 0;

            for (unsigned int i=0; i<AVG_THROTTLE_SAMPLE_COUNT; i++)
            {
                if (i > 0)
                    os->sleep_ms(AVG_THROTTLE_SAMPLE_DELAY_MS);

                device->update_info(mic_device_class::THROTTLE_INFO);

                avg_current_len += device->get_power_ttl_current_len();
                avg_count       += device->get_power_ttl_count();
                ++sample_count;
            }

            if (sample_count > 0)
            {
                avg_current_len /= sample_count;
                avg_count       /= sample_count;
            }

            cout << endl;
            cout << dev_name << " (pthrottle):" << endl;
            cout << "   Throttle state: ......... "
                 << (device->power_ttl_active() ? "active" : "inactive")
                 << endl;
            cout << "   Current throttle time: .. "
                 << avg_current_len << " msec" << endl;
            cout << "   Throttle event count: ... "
                 << avg_count << endl;
            cout << "   Total throttle time: .... "
                 << device->get_power_ttl_time() << " msec" << endl;

        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device power "
                          "throttling state: %s",
                          e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_tthrottle_opt(const cmd_line_args::opt_args &opt)
{
    mic_device_class *device = 0;

    host_platform::os_utils *os = host_platform::os_utils::get_instance();
    vector<unsigned int> device_nums;
    string dev_name;

    if (!get_device_nums(opt.args.begin(), opt.args.end(), &device_nums,
                         "tthrottle"))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!validate_device_is_online(dev_name, *device))
                continue;

            uint32_t  avg_current_len = 0;
            uint32_t  avg_count       = 0;
            uint32_t  sample_count    = 0;

            for (unsigned int i=0; i<AVG_THROTTLE_SAMPLE_COUNT; i++)
            {
                if (i > 0)
                    os->sleep_ms(AVG_THROTTLE_SAMPLE_DELAY_MS);

                device->update_info(mic_device_class::THROTTLE_INFO);

                avg_current_len += device->get_thermal_ttl_current_len();
                avg_count       += device->get_thermal_ttl_count();
                ++sample_count;
            }

            if (sample_count > 0)
            {
                avg_current_len /= sample_count;
                avg_count       /= sample_count;
            }

            cout << endl;
            cout << dev_name << " (tthrottle):"
                 << endl;
            cout << "   Throttle state: ......... "
                 << (device->thermal_ttl_active() ? "active" : "inactive")
                 << endl;
            cout << "   Current throttle time: .. "
                 << avg_current_len << " msec" << endl;
            cout << "   Throttle event count: ... "
                 << avg_count << endl;
            cout << "   Total throttle time: .... "
                 << device->get_thermal_ttl_time() << " msec" << endl;

        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device thermal "
                          "throttling state: %s",
                          e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_pwrenable_opt(const cmd_line_args::opt_args &opt)
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;
    bool cpufreq_enabled;
    bool corec6_enabled;
    bool pc3_enabled;
    bool pc6_enabled;
    vector<string>::const_iterator begin = opt.args.begin();
    vector<string>::const_iterator end = opt.args.end();
    bool success = false;

    // Defaults
    cpufreq_enabled = false;
    corec6_enabled = false;
    pc3_enabled = false;
    pc6_enabled = false;

    for (; begin != end; ++begin) {
        const string &pm_feature = *begin;
        if (pm_feature == "cpufreq") {
            cpufreq_enabled = true;
        } else if (pm_feature == "corec6") {
            corec6_enabled = true;
        } else if (pm_feature == "pc3") {
            pc3_enabled = true;
        } else if (pm_feature == "pc6") {
            pc6_enabled = true;
        } else if (pm_feature == "all") {
            cpufreq_enabled = true;
            corec6_enabled = true;
            pc3_enabled = true;
            pc6_enabled = true;
        } else {
            // We're done collecting PM features, the rest is the
            // device list
            break;
        }
    }

    if (!get_device_nums(begin, end, &device_nums, "pwrenable"))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            micsmc_helper::set_pm_config(*device, cpufreq_enabled,
                                         corec6_enabled, pc3_enabled,
                                         pc6_enabled);

            info_msg_dev(dev_name.c_str(), "power management state change "
                         "succeeded");

            success = true;
        } catch (const micsmc_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "%s",
                          e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }

    if (success)
        info_msg("you must restart your Intel(R) Xeon Phi(TM) Coprocessor(s) "
                 "now for the power management state changes to take effect");
}

void micsmc_cli::perform_pwrstatus_opt(const cmd_line_args::opt_args &opt)
{
    mic_device_class *device;

    vector<unsigned int> device_nums;
    string dev_name;

    if (!get_device_nums(opt.args.begin(), opt.args.end(), &device_nums,
                         "pwrstatus"))
        return; // Unable to get devices

    for (vector<unsigned int>::const_iterator it = device_nums.begin();
         it != device_nums.end(); ++it) {
        try {
            dev_name.clear();
            device = get_device(*it);
            dev_name = device->get_device_name();

            if (!validate_device_is_online(dev_name, *device))
                continue;

            device->update_info(mic_device_class::PM_CONFIG_INFO);

            cout << endl;
            cout << dev_name << " (pwrstatus):" << endl;
            cout << "   cpufreq power management feature: .. "
                 << (device->cpufreq_enabled() ? "enabled" :
                "disabled") << endl;
            cout << "   corec6 power management feature: ... "
                 << (device->corec6_enabled() ? "enabled" :
                "disabled") << endl;
            cout << "   pc3 power management feature: ...... "
                 << (device->pc3_enabled() ? "enabled" :
                "disabled") << endl;
            cout << "   pc6 power management feature: ...... "
                 << (device->pc6_enabled() ? "enabled" :
                "disabled") << endl;
        } catch (const micmgmt_exception &e) {
            set_device_name(&dev_name, *it);
            error_msg_dev(dev_name.c_str(), "while accessing device power "
                          "management status: %s",
                          e.get_composite_err_msg().c_str());
        } catch (const exception &e) {
            error_msg("an unexpected error has occurred: %s", e.what());
        }
    }
}

void micsmc_cli::perform_timeout_opt(const cmd_line_args::opt_args &opt)
{
    stringstream ss;
    unsigned int new_timeout;

    if (opt.args.empty()) {
        error_msg("no timeout value specified");
        return;
    }

    ss << opt.args[0];
    ss >> new_timeout;
    if (!ss) {
        error_msg("invalid value for timeout specified");
        return;
    }

    timeout_ = new_timeout;
}

void micsmc_cli::perform_help_opt(const cmd_line_args::opt_args &opt)
{
    try {
        if (!opt.args.empty()) {
            // Show help for recognized options
            stringstream unrecognized;
            for (vector<string>::const_iterator it = opt.args.begin();
                    it != opt.args.end(); ++it) {
                const cmd_config::opt_config *opt =
                    config_.get_opt_by_name(*it);
                if (opt != NULL)
                    cout << opt->help << endl;
                else
                    unrecognized << " " << *it;
            }

            if (unrecognized.rdbuf()->in_avail() > 0)
                error_msg("Unrecognized option(s):%s",
                          unrecognized.str().c_str());
        } else {
            // No options specified, show the entire help
            cout << endl;
            show_about();
            cout << endl;
            config_.print_help();
        }
    } catch (const exception &e) {
        error_msg("an unexpected error has occurred: %s", e.what()); // what???
    }
}

void micsmc_cli::show_about()
{
    host_platform::mpss_utils *mpss = host_platform::mpss_utils::get_instance();

    cout << "Intel(R) Xeon Phi(TM) Coprocessor Platform "
         << "Control Panel" << endl;
    cout << "VERSION: " << MY_VERSION << endl;
    cout << MIC_COPYRIGHT << endl;
    cout << "Developed by Intel Corporation. Intel, Xeon, and Intel Xeon "
         << "Phi are trademarks" << endl
         << "of Intel Corporation in the U.S and/or other "
         << "countries." << endl;
    cout << endl;
    cout << "This application monitors device performance, including "
         << "driver info," << endl
         << "temperatures, core usage, etc." << endl;
    cout << endl;
    cout << "The Control Panel User Guide is available in all supported "
         << "languages, in PDF and "
         << "HTML formats, at:" << endl << endl;
    cout << "   \"" << mpss->get_micsmc_doc_path() << "\"" << endl;
}

void micsmc_cli::perform_version_opt()
{
    cout << endl;
    cout << PROGRAM_NAME << endl;
    cout << "Version: " << MY_VERSION << endl;
    cout << MIC_COPYRIGHT << endl << endl;
}

void micsmc_cli::info_msg(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf(stdout, "Information: ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    fflush(stdout);

    va_end(args);
}

void micsmc_cli::info_msg_dev(const char *dev_name, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf(stdout, "Information: %s: ", dev_name);
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    fflush(stdout);

    va_end(args);
}

void micsmc_cli::warning_msg(bool error, const char *format, ...)
{
    va_list args;
    FILE *out;

    if (error) {
        out = stderr;
        exit_code_ = 1;
    } else {
        out = stdout;
    }

    va_start(args, format);
    fprintf(out, "Warning: ");
    vfprintf(out, format, args);
    fprintf(out, "\n");
    fflush(out);

    va_end(args);
}

void micsmc_cli::warning_msg_dev(const char *dev_name, bool error,
                                 const char *format, ...)
{
    va_list args;
    FILE *out;

    if (error) {
        out = stderr;
        exit_code_ = 1;
    } else {
        out = stdout;
    }

    va_start(args, format);
    fprintf(out, "Warning: %s: ", dev_name);
    vfprintf(out, format, args);
    fprintf(out, "\n");
    fflush(out);

    va_end(args);
}

void micsmc_cli::error_msg(const char *format, ...)
{
    va_list args;

    exit_code_ = 1;

    va_start(args, format);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    fflush(stderr);

    va_end(args);
}

void micsmc_cli::error_msg_dev(const char *dev_name, const char *format, ...)
{
    va_list args;

    exit_code_ = 1;

    va_start(args, format);
    fprintf(stderr, "Error: %s: ", dev_name);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    fflush(stderr);

    va_end(args);
}

void micsmc_cli::status_msg_dev(const char *dev_name, msg_type type,
                                const char *msg)
{
    switch (type) {
    case MSG_WARNING:
        fprintf(stdout, "Status: %s: warning: %s\n", dev_name, msg);
        fflush(stdout);
        break;
    case MSG_ERROR:
        fprintf(stderr, "Status: %s: error: %s\n", dev_name, msg);
        fflush(stderr);
        exit_code_ = 1;
        break;
    default:
        fprintf(stdout, "Status: %s: %s\n", dev_name, msg);
        fflush(stdout);
        break;
    }
}

mic_device_class *micsmc_cli::get_device(unsigned int dev_num)
{
    map<unsigned int, mic_device_ptr>::iterator it = devices_.find(dev_num);

    if (it != devices_.end())
        return it->second.get();

    // Add it to the map, as it hasn't been added yet
    mic_device_ptr dev(new mic_device_class(dev_num));
    devices_[dev_num] = dev;

    return dev.get();
}

bool micsmc_cli::get_device_nums(vector<string>::const_iterator begin,
                                 vector<string>::const_iterator end,
                                 vector<unsigned int> *device_nums,
                                 const char *opt)
{
    if (begin != end) {
        if (*begin == "device")
            ++begin;
        for (vector<string>::const_iterator it = begin;
             it != end; ++it) {
            try {
                device_nums->push_back(micsmc_helper::get_device_num(*it));
            } catch (const micsmc_exception &e) {
                error_msg("in option '%s': %s", opt,
                          e.get_composite_err_msg().c_str());
            } catch (const exception &e) {
                error_msg("an unexpected error has occurred: %s", e.what());
                return false;
            }
        }

        if (device_nums->empty()) {
            warning_msg(true, "no valid device names were specified with "
                        "option '%s'", opt);
            return false;
        }
    } else {
        return get_avail_device_nums(device_nums);
    }

    // Success
    return true;
}

bool micsmc_cli::get_avail_device_nums(vector<unsigned int> *device_nums)
{
    try {
        // Get all the devices available in the system
        mic_device_class::get_avail_device_nums(device_nums);
    } catch (const micsmc_exception &e) {
        error_msg("unable to query devices: %s",
                  e.get_composite_err_msg().c_str());
        return false;
    } catch (const exception &e) {
        error_msg("an unexpected error has occurred: %s", e.what());
        return false;
    }

    if (device_nums->empty()) {
        cout << endl;
        error_msg("attempt to query devices to determine the number of "
                  "Intel(R) Xeon Phi(TM)\n"
                  "Coprocessors in your system failed.\n"
                  "Please make sure your devices are initialized and running.\n"
                  "No devices detected in target system: exiting program.");
        return false;
    }

    return true;
}

void micsmc_cli::set_device_name(string *dev_name, unsigned int dev_num)
{
    if (dev_name->empty()) {
        ostringstream convert;

        convert << "mic" << dev_num;
        *dev_name = convert.str();
    }
}

bool micsmc_cli::validate_device_is_online(const string &dev_name,
        mic_device_class &device)
{
    try {
        micsmc_helper::device_status status =
                micsmc_helper::get_device_status(device);;

        if (status != micsmc_helper::DEV_STATUS_ONLINE) {
            warning_msg_dev(dev_name.c_str(), true, "cannot access device "
                            "information: device is not available");
            return false;
        }

        return true;
    } catch (const micsmc_exception &e) {
        error_msg_dev(dev_name.c_str(), "%s",
                      e.get_composite_err_msg().c_str());
        return false;
    } catch (const exception &e) {
        error_msg("an unexpected error has occurred: %s", e.what());
        return false;
    }
}

void *micsmc_cli::ecc_set_op_thread(void *work_area)
{
    struct ecc_op_work_area *work =
            (struct ecc_op_work_area *)work_area;
    mic_device_class *device = NULL;
    host_platform::os_utils *os = host_platform::os_utils::get_instance();

    try {
        device = work->this_cli_->get_device(work->get_dev_num());

        if (!device->in_ready_state()) {
            work->finish_with_msg(MSG_ERROR,
                                  "cannot set ECC mode: device is not in "
                                  "'ready' state");
            os->thread_exit();
            return NULL;
        }

        work->next_step(ECC_OP_ENTER_MAINT_MODE,
                        "putting device in maintenance mode...");
        device->enter_maintenance_mode();

        work->next_step(ECC_OP_SET_ECC_MODE,
                        string(work->enable_ecc() ? "enabling" : "disabling") +
                        string(" ECC..."));
        micsmc_helper::set_ecc_mode(*device, work->enable_ecc());
        
        // Let status thread know we're done setting ECC mode
        work->signal_ecc_op_complete(true);

        work->next_step(ECC_OP_RESET, "resetting to 'ready' state");
        device->leave_maintenance_mode();

        if (!micsmc_helper::wait_for_ready_state(*device,
                work->get_timeout())) {
            work->finish_with_msg(MSG_WARNING,
                                  "timeout reached while waiting for the "
                                  "device to become ready. Please check the "
                                  "device's state before executing further "
                                  "commands.");
        } else {
            work->finish_with_msg(MSG_INFO,
                                  string("ECC ") +
                                  string(work->enable_ecc() ?
                                         "enabled" : "disabled") +
                                  string(" successfully"));
        }
    } catch (const micsmc_exception &e) {
        work->finish_with_msg(MSG_ERROR, e.get_composite_err_msg());
    } catch (const micmgmt_exception &e) {
        work->finish_with_msg(MSG_ERROR,
                              string("unable to set device ECC mode: ") +
                              string(e.get_composite_err_msg()));
    } catch (const exception &e) {
        work->finish_with_msg(MSG_ERROR,
                              string("an unexpected error has occurred: ") +
                              string(e.what()));
    }

    os->thread_exit();
    return NULL;
}

void *micsmc_cli::ecc_status_thread(void *work_area)
{
    struct ecc_status_work_area *work =
            (struct ecc_status_work_area *)work_area;
    host_platform::os_utils *os = host_platform::os_utils::get_instance();
    unsigned int complete = 1;

    try {
        work->master_->lock_op_complete_mtx();
        while (!work->master_->all_ops_complete()) {
            work->master_->wait_for_op_complete();

            for (; complete <= work->master_->get_op_complete(); complete++)
                work->this_cli_->info_msg("%u of %u complete", complete,
                                          work->master_->get_num_op_threads());
        }
        if (work->master_->at_least_one_success())
            work->this_cli_->info_msg("please wait as some devices may still "
                                      "be resetting...");
        cout << endl;

        work->master_->unlock_op_complete_mtx();
    } catch (const exception &e) {
        work->this_cli_->error_msg("the ECC status thread has exited "
                                   "unexpectedly with error: %s", e.what());
    }

    os->thread_exit();
    return NULL;
}

void *micsmc_cli::ecc_verbose_status_thread(void *work_area)
{
    struct ecc_status_work_area *work =
                (struct ecc_status_work_area *)work_area;
    host_platform::os_utils *os = host_platform::os_utils::get_instance();
    vector<ecc_op_work_area> *op_work_areas_ = &work->master_->op_work_areas_;
    bool resetting_or_done = false;
    
    bool all_done;
    string dev_name;
    ecc_op_step step;
    msg_type type;
    string msg;

    try {
        do {
            // If all are resetting or done, don't check the status as often
            os->sleep_ms(resetting_or_done ?
                         ECC_OP_LONG_STATUS_INTERVAL : ECC_OP_STATUS_INTERVAL);

            all_done = work->master_->all_threads_done();
            if (!all_done) {
                resetting_or_done = true;
                for (vector<ecc_op_work_area>::iterator it =
                        op_work_areas_->begin();
                        it != op_work_areas_->end(); ++it) {
                    dev_name.clear();
                    set_device_name(&dev_name, it->get_dev_num());

                    it->get_status(&step, &type, &msg);
                    work->this_cli_->status_msg_dev(dev_name.c_str(),
                                                    type, msg.c_str());
                    if (step != ECC_OP_RESET && step != ECC_OP_FINISH)
                        resetting_or_done = false;
                }
                cout << endl;
            }
        } while (!all_done);
    } catch (const exception &e) {
        work->this_cli_->error_msg("the ECC status thread has exited "
                                   "unexpectedly with error: %s", e.what());
    }

    os->thread_exit();
    return NULL;
}

void micsmc_cli::configure_micsmc_cmd(cmd_config *config)
{
    shared_ptr<cmd_config::opt_config> opt;

    config->set_silent(false);
    config->set_stop_on_first_error(false);
    config->set_on_error_show_opt_help(true);
    config->set_ignore_duplicates(true);
    config->set_error_msg_header("Error: ");

    config->set_header("USAGE:\n======");
    config->set_footer(
        "=======================================\n"
        "Common Argument: [[device] device_list]\n"
        "   Specifies the device name arguments for a given "
        "command option. The\n"
        "   'device_list' specifies one or more 'micN' values "
        "where 'N' is the device\n"
        "   number: 'mic2 mic5 ...' When no device names are "
        "specified, the option\n"
        "   operates on all devices in the system.");

    // Option: -a, --all
    opt.reset(new cmd_config::opt_config("-a", "--all"));
    opt->help =
        "   -a, --all [[device] <device_list>]\n"
        "         Displays all/selected device status data. Equivalent "
        "to: -i -t -f -m\n"
        "         -c.";
    config->set_opt(opt);

    // Option: -c, --cores
    opt.reset(new cmd_config::opt_config("-c", "--cores"));
    opt->help =
        "   -c, --cores [[device] <device_list>]\n"
        "         Displays the average and per core utilization levels "
        "for all/selected\n"
        "         devices.";
    config->set_opt(opt);

    // Option: -f, --freq
    opt.reset(new cmd_config::opt_config("-f", "--freq"));
    opt->help =
        "   -f, --freq [[device] <device_list>]\n"
        "         Displays the clock frequency and power levels for "
        "all/selected\n"
        "         devices.";
    config->set_opt(opt);

    // Option: -i, --info
    opt.reset(new cmd_config::opt_config("-i", "--info"));
    opt->help =
        "   -i, --info [[device] <device_list>]\n"
        "         Displays general system information for all/selected "
        "devices.";
    config->set_opt(opt);

    // Option: -l, --lost
    opt.reset(new cmd_config::opt_config("-l", "--lost"));
    opt->help =
        "   -l, --lost\n"
        "         Displays all Intel(R) Xeon Phi(TM) "
        "Coprocessors in the system and\n"
        "         whether they are currently in the Lost Node "
        "condition.";
    opt->max_args = 0;
    config->set_opt(opt);

    // Option: --online
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--online";
    opt->help =
        "   --online\n"
        "         Displays all Intel(R) Xeon Phi(TM) "
        "Coprocessors in the system that are\n"
        "         currently online.";
    opt->max_args = 0;
    config->set_opt(opt);

    // Option: --offline
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--offline";
    opt->help =
        "   --offline\n"
        "         Displays all Intel(R) Xeon Phi(TM) "
        "Coprocessors in the system that are\n"
        "         currently offline, lost, or otherwise unavailable.";
    opt->max_args = 0;
    config->set_opt(opt);

    // Option: -m, --mem
    opt.reset(new cmd_config::opt_config("-m", "--mem"));
    opt->help =
        "   -m, --mem [[device] <device_list>]\n"
        "         Displays the memory utilization data for "
        "all/selected devices.";
    config->set_opt(opt);

    // Option: -t, --temp
    opt.reset(new cmd_config::opt_config("-t", "--temp"));
    opt->help =
        "   -t, --temp [[device] <device_list>]\n"
        "         Displays the temperature levels for all/selected "
        "devices.";
    config->set_opt(opt);

    // Option: --ecc
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--ecc";
    opt->help =
        "   --ecc [status | enable | disable] [[device] "
        "<device_list>]\n"
        "         Optional arguments:\n"
        "            enable  - enables ECC Mode\n"
        "            disable - disables ECC Mode\n"
        "            status  - displays the ECC Mode\n"
        "         Enables, disables or displays the ECC Mode for "
        "all/selected devices.\n"
        "         NOTE: If no arguments are provided, status is "
        "displayed.";
    config->set_opt(opt);

    // Option: --turbo
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--turbo";
    opt->help =
        "   --turbo [status | enable | disable] [[device] "
        "<device_list>]\n"
        "         Optional arguments:\n"
        "            enable  - enables Turbo Mode\n"
        "            disable - disables Turbo Mode\n"
        "            status  - displays Turbo Mode status\n"
        "         Enables, disables or displays the Turbo Mode for "
        "all/selected devices.\n"
        "         NOTE: If no arguments are provided, status is "
        "displayed.";
    config->set_opt(opt);

    // Option: --led
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--led";
    opt->help =
        "   --led [status | enable | disable] [[device] "
        "<device_list>]\n"
        "         Optional arguments:\n"
        "            enable  - enables LED Alert\n"
        "            disable - disables LED Alert\n"
        "            status  - displays LED Alert status\n"
        "         Enables, disables or displays the LED Alert for "
        "all/selected devices.\n"
        "         NOTE: If no arguments are provided, status is "
        "displayed.";
    config->set_opt(opt);

    // Option: --pthrottle
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--pthrottle";
    opt->help =
        "   --pthrottle [[device] <device_list>]\n"
        "         Displays the Power Throttle State for all/selected "
        "devices.";
    config->set_opt(opt);

    // Option: --tthrottle
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--tthrottle";
    opt->help =
        "   --tthrottle [[device] <device_list>]\n"
        "         Displays the Thermal Throttle State for all/selected "
        "devices.";
    config->set_opt(opt);

    // Option: --pwrenable
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--pwrenable";
    opt->help =
        "   --pwrenable [cpufreq | corec6 | pc3 | pc6 | all] "
        "[[device] <device_list>]\n"
        "         Optional arguments:\n"
        "            cpufreq - enables the cpufreq power management "
        "feature\n"
        "            corec6  - enables the corec6 power management "
        "feature\n"
        "            pc3     - enables the pc3 power management "
        "feature\n"
        "            pc6     - enables the pc6 power management "
        "feature\n"
        "            all     - enables all four power management "
        "features\n"
        "         Enables/disables the Power Management Features for "
        "all/selected\n"
        "         devices.\n"
        "         NOTE: Each feature not specified will automatically "
        "be disabled. If no\n"
        "         features are specified, then all Power "
        "Management Features are\n"
        "         disabled.";
    config->set_opt(opt);

    // Option: --pwrstatus
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--pwrstatus";
    opt->help =
        "   --pwrstatus [[device] <device_list>]\n"
        "         Displays the Power Management Feature status for "
        "all/selected devices.";
    config->set_opt(opt);

    // Option: --timeout
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--timeout";
    opt->help =
        "   --timeout <value>\n"
        "         Required argument:\n"
        "            value - integer timeout value in seconds.\n"
        "         Sets the sub-process timeout value for the current "
        "invocation. Affects\n"
        "         only command option(s) requiring sub-process "
        "execution.";
    opt->min_args = 1;
    opt->max_args = 1;
    opt->validator = &cmd_config::basic_uint_validator;
    config->set_opt(opt);

    // Option: --verbose
    opt.reset(new cmd_config::opt_config);
    opt->long_opt = "--verbose";
    opt->help =
        "   --verbose\n"
        "         Request verbose output.\n"
        "         NOTE: At the moment this only applies to the --ecc option.";
    opt->min_args = 0;
    opt->max_args = 0;
    config->set_opt(opt);

    // Option: -h, --help
    opt.reset(new cmd_config::opt_config("-h", "--help"));
    opt->help =
        "   -h, --help [<options_list>]\n"
        "         Displays full/selected usage information and then "
        "exits.";
    config->set_opt(opt);

    // Option: -v, --version
    opt.reset(new cmd_config::opt_config("-v", "--version"));
    opt->help =
        "   -v, --version\n"
        "         Displays the tool version and then exits.";
    opt->max_args = 0;
    config->set_opt(opt);
}

cmd_line_args::opt_error_code micsmc_cli::status_cmd_validator(
    const cmd_line_args::opt_args &args, string *err_msg)
{
    if (args.args.empty())
        return cmd_line_args::OPT_OK;

    const string &first = args.args[0];
    string devices;
    if (first != "device" && first.find("mic") == string::npos) {
        // First arg should be: status | enable | disable
        if (first != "status" && first != "enable" &&
            first != "disable") {
            *err_msg = "Unrecognized argument for status: " +
                       first;
            return cmd_line_args::OPT_UNRECOGNIZED_ARG;
        }
        // Devices list could come next
        if (args.get_num_args() > 1)
            devices = args.text.substr(first.length());
    } else {
        // Devices?
        devices = args.text;
    }

    return cmd_line_args::OPT_OK;
}

cmd_line_args::opt_error_code micsmc_cli::pwrenable_validator(
    const cmd_line_args::opt_args &args, string *err_msg)
{
    if (args.args.empty())
        return cmd_line_args::OPT_OK;

    vector<string>::const_iterator begin = args.args.begin();
    vector<string>::const_iterator end = args.args.end();
    bool pm_args_done = false;
    // size_t devices_pos = string::npos;
    string devices;

    for (; begin != end && !pm_args_done; ++begin) {
        const string &arg = *begin;
        if (arg != "cpufreq" && arg != "corec6" && arg != "pc3" &&
            arg != "pc6" && arg != "all") {
            // We're done collecting PM features, the rest should
            // be the device list
            pm_args_done = true;
            if (arg != "device" &&
                arg.find("mic") == string::npos) {
                *err_msg = "Unrecognized power management "
                           "feature: " + arg;
                return cmd_line_args::OPT_UNRECOGNIZED_ARG;
            }
            // Check device list starting from here:
            // devices_pos = args.text.rfind(arg);
        }
    }

    return cmd_line_args::OPT_OK;
}

micsmc_cli::ecc_op_work_area::ecc_op_work_area():
        this_cli_(NULL),
        master_(NULL),
        dev_num_(0),
        enable_ecc_(false),
        timeout_(DEFAULT_TIMEOUT),
        status_mtx_(host_platform::mutex::new_mutex()),
        step_(ECC_OP_START),
        status_msg_type_(MSG_INFO),
        status_msg_("starting...")
{
}

micsmc_cli::ecc_op_work_area::~ecc_op_work_area()
{
}

void micsmc_cli::ecc_op_work_area::assign_work(unsigned int dev_num,
                                               bool enable_ecc,
                                               unsigned int timeout)
{
    dev_num_ = dev_num;
    enable_ecc_ = enable_ecc;
    timeout_ = timeout;
}

unsigned int micsmc_cli::ecc_op_work_area::get_dev_num() const
{
    return dev_num_;
}

bool micsmc_cli::ecc_op_work_area::enable_ecc() const
{
    return enable_ecc_;
}

unsigned int micsmc_cli::ecc_op_work_area::get_timeout() const
{
    return timeout_;
}

void micsmc_cli::ecc_op_work_area::next_step(ecc_op_step step,
                                             const string &info_msg)
{
    status_mtx_->lock();
    step_ = step;
    status_msg_type_ = MSG_INFO;
    status_msg_ = info_msg;
    status_mtx_->unlock();
}

void micsmc_cli::ecc_op_work_area::signal_ecc_op_complete(bool success)
{
    master_->signal_ecc_op_complete(success);
}

void micsmc_cli::ecc_op_work_area::finish_with_msg(msg_type type,
                                                   const string &msg)
{
    bool need_to_signal = step_ < ECC_OP_RESET;

    status_mtx_->lock();
    step_ = ECC_OP_FINISH;
    status_msg_type_ = type;
    status_msg_ = msg;
    status_mtx_->unlock();

    if (need_to_signal)
        signal_ecc_op_complete(false);

    master_->signal_thread_done();
}

void micsmc_cli::ecc_op_work_area::get_status(ecc_op_step *step,
                                              msg_type *type,
                                              string *msg)
{
    *step = step_;
    *type = status_msg_type_;
    *msg = status_msg_;
}

micsmc_cli::ecc_op_master_work_area::ecc_op_master_work_area(
        unsigned int num_devices,
        micsmc_cli *this_cli):

        op_work_areas_(num_devices),
        op_complete_(0),
        op_complete_mtx_(host_platform::mutex::new_mutex()),
        op_complete_cv_(host_platform::condition_var::new_condition_var()),
        op_threads_done_(0),
        op_threads_done_mtx_(host_platform::mutex::new_mutex()),
        at_least_one_success_(false)
{
    for (vector<ecc_op_work_area>::iterator it = op_work_areas_.begin();
            it != op_work_areas_.end(); ++it) {
        it->this_cli_ = this_cli;
        it->master_ = this;
    }
}

micsmc_cli::ecc_op_master_work_area::~ecc_op_master_work_area()
{
}

unsigned int micsmc_cli::ecc_op_master_work_area::get_num_op_threads() const
{
    return op_work_areas_.size();
}

void micsmc_cli::ecc_op_master_work_area::signal_ecc_op_complete(bool success)
{
    if (success)
        at_least_one_success_ = true;

    op_complete_mtx_->lock();
    op_complete_++;
    op_complete_cv_->signal_one();
    op_complete_mtx_->unlock();
}
void micsmc_cli::ecc_op_master_work_area::signal_thread_done()
{
    op_threads_done_mtx_->lock();
    op_threads_done_++;
    op_threads_done_mtx_->unlock();
}

void micsmc_cli::ecc_op_master_work_area::lock_op_complete_mtx()
{
    op_complete_mtx_->lock();
}

void micsmc_cli::ecc_op_master_work_area::wait_for_op_complete()
{
    op_complete_cv_->wait(op_complete_mtx_.get());
}

unsigned int micsmc_cli::ecc_op_master_work_area::get_op_complete() const
{
    return op_complete_;
}

void micsmc_cli::ecc_op_master_work_area::unlock_op_complete_mtx()
{
    op_complete_mtx_->unlock();
}

bool micsmc_cli::ecc_op_master_work_area::at_least_one_success() const
{
    return at_least_one_success_;
}

bool micsmc_cli::ecc_op_master_work_area::all_ops_complete() const
{
    return op_complete_ >= op_work_areas_.size();
}

bool micsmc_cli::ecc_op_master_work_area::all_threads_done()
{
    bool all_done;

    op_threads_done_mtx_->lock();
    all_done = op_threads_done_ >= op_work_areas_.size();
    op_threads_done_mtx_->unlock();

    return all_done;
}

micsmc_cli::ecc_status_work_area::ecc_status_work_area(
        micsmc_cli *this_cli,
        ecc_op_master_work_area *master):

        this_cli_(this_cli),
        master_(master)
{
}
