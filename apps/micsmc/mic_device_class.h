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

#ifndef MIC_DEVICE_CLASS_H_
#define MIC_DEVICE_CLASS_H_

#include <cstdlib>
#include <miclib.h>

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <memory>
using std::shared_ptr;

#include "mic_device_features.h"

class mic_device_class;
typedef shared_ptr<mic_device_class> mic_device_ptr;

class mic_device_class : public mic_device_features {
public:
    mic_device_class();
    mic_device_class(unsigned int dev_num);
    virtual ~mic_device_class();

    void open_device(unsigned int dev_num);
    void close_device();

    void update_info(mic_feature feature);
    void update_all_info();

    static void get_avail_device_nums(vector<unsigned int> *device_nums);

    // Device info
    string get_device_name() const;
    unsigned int get_device_num() const;
    string get_device_series() const;

    // Thermal info
    uint32_t get_die_temp();
    bool die_temp_valid();
    uint16_t get_mem_temp();
    bool mem_temp_valid();
    uint16_t get_fanin_temp();
    bool fanin_temp_valid();
    uint16_t get_fanout_temp();
    bool fanout_temp_valid();
    uint16_t get_core_rail_temp();
    bool core_rail_temp_valid();
    uint16_t get_uncore_rail_temp();
    bool uncore_rail_temp_valid();
    uint16_t get_mem_rail_temp();
    bool mem_rail_temp_valid();

    // Cores info
    uint32_t get_core_count();
    uint32_t get_core_frequency();

    // Power usage
    uint32_t get_total_pwr_win0();

    // Power limits
    uint32_t get_power_phys_limit();
    uint32_t get_power_hmrk();
    uint32_t get_power_lmrk();

    // Memory usage
    uint32_t get_total_memory();
    uint32_t get_available_memory();

    // Throttle state info
    bool thermal_ttl_active();
    uint32_t get_thermal_ttl_current_len();
    uint32_t get_thermal_ttl_count();
    uint32_t get_thermal_ttl_time();
    bool power_ttl_active();
    uint32_t get_power_ttl_current_len();
    uint32_t get_power_ttl_count();
    uint32_t get_power_ttl_time();

    // PCI configuration
    uint16_t get_device_id();

    // Version info
    string get_uos_version();
    string get_flash_version();

    // Processor info
    uint16_t get_stepping_id();
    uint16_t get_substepping_id();

    // Core utilization info
    uint16_t get_num_cores();
    uint16_t get_threads_per_core();
    uint64_t get_jiffy_counter();
    uint64_t get_user_sum();
    uint64_t get_nice_sum();
    uint64_t get_sys_sum();
    uint64_t get_idle_sum();
    void get_user_counters(vector<uint64_t> *counters);
    void get_nice_counters(vector<uint64_t> *counters);
    void get_sys_counters(vector<uint64_t> *counters);
    void get_idle_counters(vector<uint64_t> *counters);

    struct core_util_info;
    void get_core_util_info(struct core_util_info *info);

    // Memory info
    bool ecc_mode_enabled();

    // uOS power management configuration
    bool cpufreq_enabled();
    bool corec6_enabled();
    bool pc3_enabled();
    bool pc6_enabled();

    // Turbo mode info
    bool turbo_mode_available();
    bool turbo_mode_active();
    bool turbo_mode_enabled();
    void set_turbo_mode(bool enabled);

    // LED alert
    bool led_alert_enabled();
    void set_led_alert(bool enabled);

    // RAS availability
    bool ras_available();

    // Device attributes
    string get_post_code();
    string get_image();
    int get_boot_count();
    int get_crash_count();
    string get_state();
    string get_mode();
    string get_scif_status();

    // Maintenance mode
    void enter_maintenance_mode();
    void leave_maintenance_mode();
    bool in_maintenance_mode();
    bool in_ready_state();

    // Flash operations
    shared_ptr<struct mic_flash_op> set_ecc_mode_start(bool enabled);
    void get_flash_status_info(shared_ptr<struct mic_flash_op> &flash_op,
                               int *percent_complete, int *cmd_status,
                               int *ext_status);
    void set_ecc_mode_done(shared_ptr<struct mic_flash_op> &flash_op);

    struct core_util_info {
        uint64_t         jiffy_counter;
        uint64_t         user_sum;
        uint64_t         nice_sum;
        uint64_t         sys_sum;
        uint64_t         idle_sum;
        vector<uint64_t> user_counters;
        vector<uint64_t> nice_counters;
        vector<uint64_t> sys_counters;
        vector<uint64_t> idle_counters;

        core_util_info() :
            jiffy_counter(0), user_sum(0), nice_sum(0), sys_sum(0),
            idle_sum(0)
        {
        }
    };

private:
    static const char *KNC_ID_STR;

    static const char *STATE_ATTRIB;
    static const char *MODE_ATTRIB;
    static const char *SCIF_STATUS_ATTRIB;
    static const char *IMAGE_ATTRIB;
    static const char *BOOT_COUNT_ATTRIB;
    static const char *CRASH_COUNT_ATTRIB;

    unsigned int dev_num_;
    string dev_name_;
    uint32_t dev_type_;
};

#endif /* MIC_DEVICE_CLASS_H_ */
