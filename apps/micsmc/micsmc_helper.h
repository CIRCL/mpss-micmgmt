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

#ifndef MICSMC_HELPER_H_
#define MICSMC_HELPER_H_

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <memory>
using std::shared_ptr;

#include <miclib.h>
#include "mic_device_class.h"

class micsmc_helper {
public:
    micsmc_helper();
    virtual ~micsmc_helper();

    enum device_status {
        DEV_STATUS_UNKNOWN = 0,
        DEV_STATUS_UNAVAILABLE,
        DEV_STATUS_ONLINE,
        DEV_STATUS_LOADED,
        DEV_STATUS_READY,
        DEV_STATUS_STARTED
    };

    static string get_driver_version();
    static unsigned int get_device_num(const string &name);

    static bool wait_for_ready_state(mic_device_class &device,
                                     unsigned int timeout);
    static device_status get_device_status(mic_device_class &device);
    static void set_pm_config(mic_device_class &device, bool cpufreq_enabled,
                              bool corec6_enabled, bool pc3_enabled,
                              bool pc6_enabled);
    static void set_ecc_mode(mic_device_class &device, bool enabled);
    static bool wait_for_flash_op(mic_device_class &device,
                                  shared_ptr<struct mic_flash_op> &flash_op,
                                  unsigned int poll_interval,
                                  unsigned int timeout);

    struct device_util_info;
    static void compute_dev_utilization(mic_device_class &device, const
                                        mic_device_class::core_util_info &
                                        sample1, const
                                        mic_device_class::core_util_info &
                                        sample2, struct device_util_info *util);

    static const char *get_device_status_name(device_status status);
    static const char *const DEVICE_STATUS_NAMES[];

    struct device_util_info {
        double         user_util;
        double         sys_util;
        double         idle_util;
        vector<double> core_user_util;
        vector<double> core_sys_util;
        vector<double> core_idle_util;

        device_util_info() :
            user_util(0.0),
            sys_util(0.0),
            idle_util(0.0)
        {
        }
    };

private:
    static const unsigned int DEV_STATE_POLL_INTERVAL; // Seconds
    static const unsigned int ECC_MODE_OP_POLL_INTERVAL; // Seconds
    static const unsigned int ECC_MODE_OP_TIMEOUT; // Seconds
};

#endif /* MICSMC_HELPER_H_ */
