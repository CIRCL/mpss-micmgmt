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

#include "micsmc_helper.h"

#include <sstream>
using std::stringstream;

#include <algorithm>
using std::min;
using std::max;

#include <exception>
using std::exception;

#include "micmgmt_exception.h"
#include "micsmc_exception.h"
#include "host_platform.h"

const char *const micsmc_helper::DEVICE_STATUS_NAMES[] = {
    "unknown",
    "unavailable",
    "online",
    "loaded",
    "ready",
    "started"
};

const unsigned int micsmc_helper::DEV_STATE_POLL_INTERVAL = 5;
const unsigned int micsmc_helper::ECC_MODE_OP_POLL_INTERVAL = 1;
const unsigned int micsmc_helper::ECC_MODE_OP_TIMEOUT = 10;

micsmc_helper::micsmc_helper()
{
}

micsmc_helper::~micsmc_helper()
{
}

string micsmc_helper::get_driver_version()
{
    try {
        host_platform::mpss_utils *mpss =
                host_platform::mpss_utils::get_instance();
        return mpss->get_mic_driver_version();
    } catch (const exception &e) {
        throw micsmc_exception("unable to get driver version", e);
    }
}

unsigned int micsmc_helper::get_device_num(const string &name)
{
    if (name.length() < 4)
        throw micsmc_exception("invalid device name: " + name);

    // Expecting the format: mic<n> (this may change in the future)
    string mic = name.substr(0, 3);
    if (mic.compare("mic") != 0)
        throw micsmc_exception("invalid device name: " + name);

    string num_str = name.substr(3);

    // Make sure that the mic<n> is a number
    size_t error = num_str.find_first_not_of("0123456789");
    if (error != string::npos)
        throw micsmc_exception("invalid device name: " + name);

    stringstream ss;
    unsigned int dev_num;

    ss << num_str;
    ss >> dev_num;

    if (!ss)
        throw micsmc_exception("invalid device name: " + name);

    return dev_num;
}

bool micsmc_helper::wait_for_ready_state(mic_device_class &device,
                                         unsigned int timeout)
{
    unsigned int time_left = timeout;
    host_platform::os_utils *os = host_platform::os_utils::get_instance();

    try {
        // Check the state by intervals
        while (time_left > 0) {
            if (device.in_ready_state())
                return true;

            if (time_left >= DEV_STATE_POLL_INTERVAL) {
                os->sleep_sec(DEV_STATE_POLL_INTERVAL);
                time_left -= DEV_STATE_POLL_INTERVAL;
            } else {
                os->sleep_sec(time_left);
                time_left = 0;
            }
        }

        // Check one last time
        if (device.in_ready_state())
            return true;
    } catch (const micmgmt_exception &e) {
        throw micsmc_exception("error while querying device state", e);
    }

    return false;
}

micsmc_helper::device_status micsmc_helper::get_device_status(
    mic_device_class &device)
{
    try {
        string state = device.get_state();
        string mode = device.get_mode();
        string scif_status = device.get_scif_status();
        string post_code = device.get_post_code();
        bool ras_avail = device.ras_available();
        device_status status = DEV_STATUS_UNKNOWN;

        // Interpret overall device status
        if (!ras_avail) {
            status = DEV_STATUS_UNAVAILABLE;
        } else if (state == "online") {
            status = DEV_STATUS_ONLINE;
        } else if (state == "ready") {
            if (mode == "N/A") {
                status = DEV_STATUS_LOADED;
            } else if (mode == "linux") {
                if (scif_status == "initializing")
                    status = DEV_STATUS_READY;
                else if (scif_status == "online")
                    status = DEV_STATUS_STARTED;
            }
        }

        return status;
    } catch (const micmgmt_exception &e) {
        throw micsmc_exception("unable to determine device status", e);
    }
}

void micsmc_helper::set_pm_config(mic_device_class &device,
                                  bool cpufreq_enabled, bool corec6_enabled,
                                  bool pc3_enabled,
                                  bool pc6_enabled)
{
    try {
        host_platform::mpss_utils *mpss =
                host_platform::mpss_utils::get_instance();
        mpss->set_mic_pm_config(device, cpufreq_enabled, corec6_enabled,
                                pc3_enabled, pc6_enabled);
    } catch (const exception &e) {
        throw micsmc_exception("unable to set power management configuration",
                               e);
    }
}

void micsmc_helper::set_ecc_mode(mic_device_class &device, bool enabled)
{
    try {
        shared_ptr<struct mic_flash_op> flash_op =
            device.set_ecc_mode_start(enabled);
        if (!wait_for_flash_op(device, flash_op, ECC_MODE_OP_POLL_INTERVAL,
                               ECC_MODE_OP_TIMEOUT)) {
            throw micsmc_exception("unable to set ECC mode: operation "
                                   "timed out");
        }
        device.set_ecc_mode_done(flash_op);
    } catch (const micsmc_exception &e) {
        throw;
    } catch (const micmgmt_exception &e) {
        throw micsmc_exception("unable to set ECC mode", e);
    }
}

bool micsmc_helper::wait_for_flash_op(mic_device_class &device,
                                      shared_ptr<struct mic_flash_op> &flash_op,
                                      unsigned int poll_interval,
                                      unsigned int timeout)
{
    int percent = 0;
    int cmd_status;
    int ext_status;
    unsigned int time_left = timeout;
    host_platform::os_utils *os = host_platform::os_utils::get_instance();

    try {
        while (time_left > 0) {
            device.get_flash_status_info(flash_op, &percent, &cmd_status,
                                         &ext_status);

            if (percent >= 100)
                return true;

            if (time_left >= poll_interval) {
                os->sleep_sec(poll_interval);
                time_left -= poll_interval;
            } else {
                os->sleep_sec(time_left);
                time_left = 0;
            }
        }

        device.get_flash_status_info(flash_op, &percent, &cmd_status,
                                     &ext_status);
        if (percent >= 100)
            return true;
    } catch (const micmgmt_exception &e) {
        throw micsmc_exception("error while querying flash operation status",
                               e);
    }

    return false;
}

void micsmc_helper::compute_dev_utilization(
    mic_device_class &device,
    const mic_device_class::core_util_info &sample1,
    const mic_device_class::core_util_info &sample2,
    struct device_util_info *util)
{
    try {
        double jiffy_delta = sample2.jiffy_counter - sample1.jiffy_counter;
        double thr_core = device.get_threads_per_core();
        uint16_t num_cores = device.get_num_cores();

        double user_delta = sample2.user_sum - sample1.user_sum;
        double sys_delta = sample2.sys_sum - sample1.sys_sum;

        if (jiffy_delta == 0.0 || thr_core == 0.0 || num_cores == 0)
            throw micsmc_exception("unable to compute device utilization: "
                                   "bad samples");

        // Device user utilization
        double user_util = (user_delta * 100.0 / (thr_core * jiffy_delta)) /
                           (double)num_cores;
        // Normalize device utilization
        user_util = min<double>(user_util, 100.0);

        // System device utilization
        double sys_util = ((sys_delta * 100.0) / (thr_core * jiffy_delta)) /
                          (double)num_cores;
        // Normalize
        sys_util = min<double>(sys_util, 100.0);

        // Device idle time
        double idle_time = 100.0 - (user_util + sys_util);
        // Normalize
        idle_time = max<double>(idle_time, 0.0);

        util->user_util = user_util;
        util->sys_util = sys_util;
        util->idle_util = idle_time;

        util->core_user_util.resize(num_cores);
        util->core_sys_util.resize(num_cores);
        util->core_idle_util.resize(num_cores);

        for (uint16_t i = 0; i < num_cores; i++) {
            double core_user_delta = sample2.user_counters[i]
                                     - sample1.user_counters[i];
            double core_sys_delta = sample2.sys_counters[i]
                                    - sample1.sys_counters[i];

            // Core user utilization
            double core_user_util = (core_user_delta * 100.0) /
                                    (thr_core * jiffy_delta);
            core_user_util = min<double>(core_user_util, 100.0);
            util->core_user_util[i] = core_user_util;

            // Core system utilization
            double core_sys_util = (core_sys_delta * 100.00) /
                                   (thr_core * jiffy_delta);
            core_sys_util = min<double>(core_sys_util, 100.0);
            util->core_sys_util[i] = core_sys_util;

            // Core idle time
            double core_idle_time = 100.0 -
                                    (core_user_util + core_sys_util);
            core_idle_time = max<double>(core_idle_time, 0.0);
            util->core_idle_util[i] = core_idle_time;
        }
    } catch (const micsmc_exception &e) {
        throw;
    } catch (const micmgmt_exception &e) {
        throw micsmc_exception("unable to compute device utilization", e);
    } catch (const exception &e) {
        throw micsmc_exception("unexpected error while computing device "
                               "utilization", e);
    }
}

const char *micsmc_helper::get_device_status_name(device_status status)
{
    return DEVICE_STATUS_NAMES[(int)status];
}
