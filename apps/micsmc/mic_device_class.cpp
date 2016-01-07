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

#include "mic_device_class.h"
#include <cassert>

#include "micmgmt_exception.h"

const char *mic_device_class::KNC_ID_STR =
        "Intel(R) Xeon Phi(TM) coprocessor x100 family";

const char *mic_device_class::STATE_ATTRIB = "state";
const char *mic_device_class::MODE_ATTRIB = "mode";
const char *mic_device_class::SCIF_STATUS_ATTRIB = "scif_status";
const char *mic_device_class::IMAGE_ATTRIB = "image";
const char *mic_device_class::BOOT_COUNT_ATTRIB = "boot_count";
const char *mic_device_class::CRASH_COUNT_ATTRIB = "crash_count";

mic_device_class::mic_device_class()
{
    dev_num_ = 0;
    mdh_ = NULL;
    dev_type_ = 0;
}

mic_device_class::mic_device_class(unsigned int dev_num)
{
    open_device(dev_num);
}

mic_device_class::~mic_device_class()
{
    close_device();
}

void mic_device_class::open_device(unsigned int dev_num)
{
    close_device();

    errno = 0;
    int ret = mic_open_device(&mdh_, dev_num);
    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception("open device", ret);
    dev_num_ = dev_num;
    dev_name_ = mic_get_device_name(mdh_);
    dev_type_ = get_device_data<uint32_t>(&mic_get_device_type);
}

void mic_device_class::close_device()
{
    if (mdh_) {
        mic_close_device(mdh_);
        mdh_ = NULL;
    }
}

void mic_device_class::update_info(mic_feature feature)
{
    feature_map::iterator it = features_.find(feature);

    assert(it != features_.end());

    features_[feature]->update();
}

void mic_device_class::update_all_info()
{
    for (feature_map::iterator it = features_.begin();
         it != features_.end(); ++it)
        it->second->update();
}

void mic_device_class::get_avail_device_nums(vector<unsigned int> *device_nums)
{
    shared_ptr<struct mic_devices_list> devices;
    int num_devices;
    unsigned int dev_num;

    assert(device_nums);

    devices = get_devices();
    num_devices = get_number_of_devices(devices);

    device_nums->clear();
    for (int i = 0; i < num_devices; i++) {
        dev_num = get_device_at_index(devices, i);
        device_nums->push_back(dev_num);
    }
}

string mic_device_class::get_device_name() const
{
    assert(mdh_);
    return dev_name_;
}

unsigned int mic_device_class::get_device_num() const
{
    assert(mdh_);
    return dev_num_;
}

string mic_device_class::get_device_series() const
{
    assert(mdh_);
    switch (dev_type_) {
    case KNC_ID:
        return string(KNC_ID_STR);
    default:
        errno = 0;
        throw micmgmt_exception("unrecognized device type");
        break;
    }

    return string();
}

// Thermal info
uint32_t mic_device_class::get_die_temp()
{
    return thermal_info_.get_data<uint32_t>(&mic_get_die_temp,
                                            "get die temperature");
}

bool mic_device_class::die_temp_valid()
{
    return thermal_info_.get_data<int>(&mic_is_die_temp_valid);
}

uint16_t mic_device_class::get_mem_temp()
{
    return thermal_info_.get_data<uint16_t>(&mic_get_gddr_temp,
                                            "get memory temperature");
}

bool mic_device_class::mem_temp_valid()
{
    return thermal_info_.get_data<int>(&mic_is_gddr_temp_valid);
}

uint16_t mic_device_class::get_fanin_temp()
{
    return thermal_info_.get_data<uint16_t>(&mic_get_fanin_temp,
                                            "get fan-in temperature");
}

bool mic_device_class::fanin_temp_valid()
{
    return thermal_info_.get_data<int>(&mic_is_fanin_temp_valid);
}

uint16_t mic_device_class::get_fanout_temp()
{
    return thermal_info_.get_data<uint16_t>(&mic_get_fanout_temp,
                                            "get fan-out temperature");
}

bool mic_device_class::fanout_temp_valid()
{
    return thermal_info_.get_data<int>(&mic_is_fanout_temp_valid);
}

uint16_t mic_device_class::get_core_rail_temp()
{
    return thermal_info_.get_data<uint16_t>(&mic_get_vccp_temp,
                                            "get core rail temperature");
}

bool mic_device_class::core_rail_temp_valid()
{
    return thermal_info_.get_data<int>(&mic_is_vccp_temp_valid);
}

uint16_t mic_device_class::get_uncore_rail_temp()
{
    return thermal_info_.get_data<uint16_t>(&mic_get_vddg_temp,
                                            "get uncore rail temperature");
}

bool mic_device_class::uncore_rail_temp_valid()
{
    return thermal_info_.get_data<int>(&mic_is_vddg_temp_valid);
}

uint16_t mic_device_class::get_mem_rail_temp()
{
    return thermal_info_.get_data<uint16_t>(&mic_get_vddq_temp,
                                            "get memory rail temperature");
}

bool mic_device_class::mem_rail_temp_valid()
{
    return thermal_info_.get_data<int>(&mic_is_vddq_temp_valid);
}

// Cores info
uint32_t mic_device_class::get_core_count()
{
    return cores_info_.get_data<uint32_t>(&mic_get_cores_count,
                                          "get core count");
}

uint32_t mic_device_class::get_core_frequency()
{
    return cores_info_.get_data<uint32_t>(&mic_get_cores_frequency,
                                          "get core frequency");
}

// Power usage
uint32_t mic_device_class::get_total_pwr_win0()
{
    return power_util_info_.get_data<uint32_t>(&mic_get_total_power_readings_w0,
                                               "get total power (window 0)");
}

// Power limits
uint32_t mic_device_class::get_power_phys_limit()
{
    return power_limit_info_.get_data<uint32_t>(&mic_get_power_phys_limit,
                                                "get power physical limit");
}

uint32_t mic_device_class::get_power_hmrk()
{
    return power_limit_info_.get_data<uint32_t>(&mic_get_power_hmrk,
                                        "get power high water mark (CPU hot)");
}

uint32_t mic_device_class::get_power_lmrk()
{
    return power_limit_info_.get_data<uint32_t>(&mic_get_power_lmrk,
                                    "get power low water mark (power alert)");
}

// Memory usage
uint32_t mic_device_class::get_total_memory()
{
    return mem_util_info_.get_data<uint32_t>(&mic_get_total_memory_size,
                                             "get total memory");
}

uint32_t mic_device_class::get_available_memory()
{
    return mem_util_info_.get_data<uint32_t>(&mic_get_available_memory_size,
                                             "get available memory");
}

// Throttle state info
bool mic_device_class::thermal_ttl_active()
{
    return throttle_info_.get_data<int>(&mic_get_thermal_ttl_active,
                                        "check thermal throttling state");
}

uint32_t mic_device_class::get_thermal_ttl_current_len()
{
    return throttle_info_.get_data<uint32_t>(&mic_get_thermal_ttl_current_len,
                                         "get current thermal throttle length");
}

uint32_t mic_device_class::get_thermal_ttl_count()
{
    return throttle_info_.get_data<uint32_t>(&mic_get_thermal_ttl_count,
                                             "get thermal throttle count");
}

uint32_t mic_device_class::get_thermal_ttl_time()
{
    return throttle_info_.get_data<uint32_t>(&mic_get_thermal_ttl_time,
                                             "get total thermal throttle time");
}

bool mic_device_class::power_ttl_active()
{
    return throttle_info_.get_data<int>(&mic_get_power_ttl_active,
                                        "check power throttling state");
}

uint32_t mic_device_class::get_power_ttl_current_len()
{
    return throttle_info_.get_data<uint32_t>(&mic_get_power_ttl_current_len,
                                         "get current power throttle length");
}

uint32_t mic_device_class::get_power_ttl_count()
{
    return throttle_info_.get_data<uint32_t>(&mic_get_power_ttl_count,
                                             "get power throttle count");
}

uint32_t mic_device_class::get_power_ttl_time()
{
    return throttle_info_.get_data<uint32_t>(&mic_get_power_ttl_time,
                                             "get total power throttle time");
}

// PCI configuration
uint16_t mic_device_class::get_device_id()
{
    return pci_config_info_.get_data<uint16_t>(&mic_get_device_id,
                                               "get device ID");
}

// Version info
string mic_device_class::get_uos_version()
{
    return version_info_.get_str_data(&mic_get_uos_version,
                                      "get uOS version");
}

string mic_device_class::get_flash_version()
{
    return version_info_.get_str_data(&mic_get_flash_version,
                                      "get flash version");
}

// Processor info
uint16_t mic_device_class::get_stepping_id()
{
    uint32_t stepping_data =
        proc_info_.get_data<uint32_t>(&mic_get_processor_steppingid,
                                      "get stepping ID");
    uint16_t stepping_id = stepping_data >> 4;

    return stepping_id;
}

uint16_t mic_device_class::get_substepping_id()
{
    uint32_t stepping_data =
        proc_info_.get_data<uint32_t>(&mic_get_processor_steppingid,
                                      "get substepping ID");
    uint16_t substepping_id = stepping_data & 0xF;

    return substepping_id;
}

// Core utilization info
uint16_t mic_device_class::get_num_cores()
{
    return core_util_info_.get_data<uint16_t>(&mic_get_num_cores,
                                              "get number of cores");
}

uint16_t mic_device_class::get_threads_per_core()
{
    return core_util_info_.get_data<uint16_t>(&mic_get_threads_core,
                                              "get threads per core");
}

uint64_t mic_device_class::get_jiffy_counter()
{
    return core_util_info_.get_data<uint64_t>(&mic_get_jiffy_counter,
                                              "get jiffy counter");
}

uint64_t mic_device_class::get_user_sum()
{
    return core_util_info_.get_data<uint64_t>(&mic_get_user_sum,
                                              "get user utilization sum");
}

uint64_t mic_device_class::get_nice_sum()
{
    return core_util_info_.get_data<uint64_t>(&mic_get_nice_sum,
                                              "get nice utilization sum");
}

uint64_t mic_device_class::get_sys_sum()
{
    return core_util_info_.get_data<uint64_t>(&mic_get_sys_sum,
                                              "get system utilization sum");
}

uint64_t mic_device_class::get_idle_sum()
{
    return core_util_info_.get_data<uint64_t>(&mic_get_idle_sum,
                                              "get idle time sum");
}

void mic_device_class::get_user_counters(vector<uint64_t> *counters)
{
    counters->resize(get_num_cores());
    core_util_info_.get_array_data<uint64_t>(&mic_get_user_counters,
                                             counters->data(),
                                             "get user utilization counters");
}

void mic_device_class::get_nice_counters(vector<uint64_t> *counters)
{
    counters->resize(get_num_cores());
    core_util_info_.get_array_data<uint64_t>(&mic_get_nice_counters,
                                             counters->data(),
                                             "get nice utilization counters");
}

void mic_device_class::get_sys_counters(vector<uint64_t> *counters)
{
    counters->resize(get_num_cores());
    core_util_info_.get_array_data<uint64_t>(&mic_get_sys_counters,
                                             counters->data(),
                                             "get system utilization counters");
}

void mic_device_class::get_idle_counters(vector<uint64_t> *counters)
{
    counters->resize(get_num_cores());
    core_util_info_.get_array_data<uint64_t>(&mic_get_idle_counters,
                                             counters->data(),
                                             "get idle time counters");
}

void mic_device_class::get_core_util_info(
    struct mic_device_class::core_util_info *info)
{
    info->jiffy_counter = get_jiffy_counter();
    info->user_sum = get_user_sum();
    info->nice_sum = get_nice_sum();
    info->sys_sum = get_sys_sum();
    info->idle_sum = get_idle_sum();
    get_user_counters(&info->user_counters);
    get_nice_counters(&info->nice_counters);
    get_sys_counters(&info->sys_counters);
    get_idle_counters(&info->idle_counters);
}

// Memory info
bool mic_device_class::ecc_mode_enabled()
{
    return mem_info_.get_data<uint16_t>(&mic_get_ecc_mode,
                                        "get ECC mode");
}

// Coprocessor OS power management configuration
bool mic_device_class::cpufreq_enabled()
{
    return pm_config_info_.get_data<int>(&mic_get_cpufreq_mode,
                                         "get P state (cpufreq)");
}

bool mic_device_class::corec6_enabled()
{
    return pm_config_info_.get_data<int>(&mic_get_corec6_mode,
                                         "get core C6 state (corec6)");
}

bool mic_device_class::pc3_enabled()
{
    return pm_config_info_.get_data<int>(&mic_get_pc3_mode,
                                         "get package C3 state (pc3)");
}

bool mic_device_class::pc6_enabled()
{
    return pm_config_info_.get_data<int>(&mic_get_pc6_mode,
                                         "get package C6 state (pc6)");
}

// Turbo mode info
bool mic_device_class::turbo_mode_available()
{
    return turbo_info_.get_data<uint32_t>(&mic_get_turbo_state_valid,
                                          "check if turbo mode is supported");
}

bool mic_device_class::turbo_mode_active()
{
    return turbo_info_.get_data<uint32_t>(&mic_get_turbo_state,
                                          "check if turbo mode is active");
}

bool mic_device_class::turbo_mode_enabled()
{
    return turbo_info_.get_data<uint32_t>(&mic_get_turbo_mode,
                                          "get turbo mode");
}

void mic_device_class::set_turbo_mode(bool enabled)
{
    uint32_t mode = enabled ? 1 : 0;

    set_device_data<uint32_t *>(&mic_set_turbo_mode, &mode,
                                "set turbo mode");
}

// LED alert
bool mic_device_class::led_alert_enabled()
{
    return get_device_data<uint32_t>(&mic_get_led_alert,
                                     "get LED alert");
}

void mic_device_class::set_led_alert(bool enabled)
{
    uint32_t led_alert = enabled ? 1 : 0;

    set_device_data<uint32_t *>(&mic_set_led_alert, &led_alert,
                                "set LED alert");
}

// RAS availability
bool mic_device_class::ras_available()
{
    return get_device_data<int>(&mic_is_ras_avail,
                                "check RAS");
}

// Device attributes
string mic_device_class::get_post_code()
{
    return get_device_str_data(&mic_get_post_code,
                               "get post code");
}

string mic_device_class::get_image()
{
    return get_device_attrib(IMAGE_ATTRIB);
}

int mic_device_class::get_boot_count()
{
    return get_device_attrib_as_int(BOOT_COUNT_ATTRIB);
}

int mic_device_class::get_crash_count()
{
    return get_device_attrib_as_int(CRASH_COUNT_ATTRIB);
}

string mic_device_class::get_state()
{
    return get_device_attrib(STATE_ATTRIB);
}

string mic_device_class::get_mode()
{
    return get_device_attrib(MODE_ATTRIB);
}

string mic_device_class::get_scif_status()
{
    return get_device_attrib(SCIF_STATUS_ATTRIB);
}

// Maintenance mode
void mic_device_class::enter_maintenance_mode()
{
    do_device_operation(&mic_enter_maint_mode,
                        "enter maintenance mode");
}

void mic_device_class::leave_maintenance_mode()
{
    do_device_operation(&mic_leave_maint_mode,
                        "leave maintenance mode");
}

bool mic_device_class::in_maintenance_mode()
{
    return get_device_data<int>(&mic_in_maint_mode,
                                "check if in maintenance mode");
}

bool mic_device_class::in_ready_state()
{
    return get_device_data<int>(&mic_in_ready_state,
                                "check if in ready state");
}

shared_ptr<struct mic_flash_op> mic_device_class::set_ecc_mode_start(
    bool enabled)
{
    uint16_t ecc_mode = enabled ? 1 : 0;
    struct mic_flash_op *flash_op = NULL;
    errno = 0;
    int ret = mic_set_ecc_mode_start(mdh_, ecc_mode, &flash_op);

    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception("set ECC mode", ret);
    shared_ptr<struct mic_flash_op> flash_op_sp(flash_op,
                                                &mic_set_ecc_mode_done);
    return flash_op_sp;
}

void mic_device_class::get_flash_status_info(
    shared_ptr<struct mic_flash_op> &flash_op, int *percent_complete,
    int *cmd_status, int *ext_status)
{
    struct mic_flash_status_info *status_info = NULL;
    uint32_t progress = 0;
    errno = 0;
    int ret = mic_get_flash_status_info(flash_op.get(), &status_info);

    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception("get flash status info", ret);
    mic_get_progress(status_info, &progress);
    *percent_complete = progress;
    mic_get_status(status_info, cmd_status);
    mic_get_ext_status(status_info, ext_status);
    mic_free_flash_status_info(status_info);
}

void mic_device_class::set_ecc_mode_done(
    shared_ptr<struct mic_flash_op> &flash_op)
{
    flash_op.reset();
}
