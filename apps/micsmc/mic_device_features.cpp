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

#include "mic_device_features.h"

#include <sstream>
using std::stringstream;

#include <memory>
using std::shared_ptr;

mic_device_features::mic_device_features() :
    thermal_info_(&mdh_, "thermal info", &mic_get_thermal_info,
                  &mic_free_thermal_info),
    cores_info_(&mdh_, "cores info", &mic_get_cores_info, &mic_free_cores_info),
    power_util_info_(&mdh_, "power utilization info",
                     &mic_get_power_utilization_info,
                     &mic_free_power_utilization_info),
    power_limit_info_(&mdh_, "power limits info", &mic_get_power_limit,
                      &mic_free_power_limit),
    mem_util_info_(&mdh_, "memory utilization info",
                   &mic_get_memory_utilization_info,
                   &mic_free_memory_utilization_info),
    throttle_info_(&mdh_, "throttle state info",
                   &mic_get_throttle_state_info,
                   &mic_free_throttle_state_info),
    pci_config_info_(&mdh_, "PCI configuration", &mic_get_pci_config,
                     &mic_free_pci_config),
    version_info_(&mdh_, "version info", &mic_get_version_info,
                  &mic_free_version_info),
    proc_info_(&mdh_, "processor info", &mic_get_processor_info,
               &mic_free_processor_info),
    core_util_info_(&mdh_, "core utilization info", &mic_alloc_core_util,
                    &mic_update_core_util,
                    &mic_free_core_util),
    mem_info_(&mdh_, "memory info", &mic_get_memory_info,
              &mic_free_memory_info),
    pm_config_info_(&mdh_, "coprocessor OS power management configuration",
                    &mic_get_uos_pm_config, &mic_free_uos_pm_config),
    turbo_info_(&mdh_, "turbo mode info", &mic_get_turbo_state_info,
                &mic_free_turbo_info)
{
    mdh_ = NULL;
    features_[THERMAL_INFO] = &thermal_info_;
    features_[CORES_INFO] = &cores_info_;
    features_[POWER_UTIL_INFO] = &power_util_info_;
    features_[POWER_LIMIT_INFO] = &power_limit_info_;
    features_[MEM_UTIL_INFO] = &mem_util_info_;
    features_[THROTTLE_INFO] = &throttle_info_;
    features_[PCI_CONFIG_INFO] = &pci_config_info_;
    features_[VERSION_INFO] = &version_info_;
    features_[PROC_INFO] = &proc_info_;
    features_[CORE_UTIL_INFO] = &core_util_info_;
    features_[MEM_INFO] = &mem_info_;
    features_[PM_CONFIG_INFO] = &pm_config_info_;
    features_[TURBO_INFO] = &turbo_info_;
}

mic_device_features::~mic_device_features()
{
}

shared_ptr<struct mic_devices_list> mic_device_features::get_devices()
{
    struct mic_devices_list *devices;
    errno = 0;
    int ret = mic_get_devices(&devices);

    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception("get devices", ret);

    shared_ptr<struct mic_devices_list> devices_sp(devices, &mic_free_devices);
    return devices_sp;
}

int mic_device_features::get_number_of_devices(
    shared_ptr<struct mic_devices_list> &devices)
{
    int n = 0;
    errno = 0;
    int ret = mic_get_ndevices(devices.get(), &n);

    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception("get number of devices", ret);

    return n;
}

unsigned int mic_device_features::get_device_at_index(
    shared_ptr<struct mic_devices_list> &devices, int index)
{
    int dev_num = 0;
    errno = 0;
    int ret = mic_get_device_at_index(devices.get(), index, &dev_num);

    if (ret != E_MIC_SUCCESS || dev_num < 0)
        throw micmgmt_exception("get device", ret);

    return dev_num;
}

string mic_device_features::get_device_str_data(
    int (*func)(struct mic_device *, char *, size_t *),
    const char *desc)
{
    assert(mdh_);

    size_t size = 0;
    errno = 0;
    int ret = func(mdh_, NULL, &size);
    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception(desc, ret);
    shared_ptr<char> buffer(new char[size]);
    errno = 0;
    ret = func(mdh_, buffer.get(), &size);
    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception(desc, ret);

    return string(buffer.get());
}

string mic_device_features::get_device_attrib(const char *attrib)
{
    assert(mdh_);

    size_t size = 0;
    errno = 0;
    int ret = mic_get_sysfs_attribute(mdh_, attrib, NULL, &size);
    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception(attrib, ret);
    shared_ptr<char> buffer(new char[size]);
    errno = 0;
    ret = mic_get_sysfs_attribute(mdh_, attrib, buffer.get(), &size);
    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception(attrib, ret);

    return string(buffer.get());
}

int mic_device_features::get_device_attrib_as_int(const char *attrib)
{
    string str_value = get_device_attrib(attrib);
    int value;
    stringstream ss;

    ss << str_value;
    ss >> value;
    if (!ss)
        throw micmgmt_exception(string(attrib) + ": non-numeric value");

    return value;
}

void mic_device_features::do_device_operation(
    int (*func)(struct mic_device *), const char *desc)
{
    errno = 0;
    int ret = func(mdh_);

    if (ret != E_MIC_SUCCESS)
        throw micmgmt_exception(desc, ret);
}
