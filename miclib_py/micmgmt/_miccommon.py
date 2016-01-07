# Copyright 2010-2013 Intel Corporation.
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, version 2.1.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# Disclaimer: The codes contained in these modules may be specific
# to the Intel Software Development Platform codenamed Knights Ferry,
# and the Intel product codenamed Knights Corner, and are not backward
# compatible with other Intel products. Additionally, Intel will NOT
# support the codes or instruction set in future products.
#
# Intel offers no warranty of any kind regarding the code. This code is
# licensed on an "AS IS" basis and Intel is not obligated to provide
# any support, assistance, installation, training, or other services
# of any kind. Intel is also not obligated to provide any updates,
# enhancements or extensions. Intel specifically disclaims any warranty
# of merchantability, non-infringement, fitness for any particular
# purpose, and any other warranty.
#
# Further, Intel disclaims all liability of any kind, including but
# not limited to liability for infringement of any proprietary rights,
# relating to the use of the code, even if Intel is notified of the
# possibility of such liability. Except as expressly stated in an Intel
# license agreement provided with this code and agreed upon with Intel,
# no license, express or implied, by estoppel or otherwise, to any
# intellectual property rights is granted herein.

import inspect
import ctypes

MAX_STRLEN = 512

#Error codes
MIC_ERRCODES = dict()
E_MIC_SUCCESS = 0
MIC_ERRCODES[1] = "E_MIC_INVAL"
MIC_ERRCODES[2] = "E_MIC_ACCESS"
MIC_ERRCODES[3] = "E_MIC_NOENT"
MIC_ERRCODES[4] = "E_MIC_UNSUPPORTED_DEV"
MIC_ERRCODES[5] = "E_MIC_NOT_IMPLEMENTED"
MIC_ERRCODES[6] = "E_MIC_DRIVER_INIT"
MIC_ERRCODES[7] = "E_MIC_DRIVER_NOT_LOADED"
MIC_ERRCODES[8] = "E_MIC_IOCTL_FAILED"
MIC_ERRCODES[9] = "E_MIC_ERROR_NOT_FOUND"
MIC_ERRCODES[10] = "E_MIC_NOMEM"
MIC_ERRCODES[11] = "E_MIC_RANGE"
MIC_ERRCODES[12] = "E_MIC_INTERNAL"
MIC_ERRCODES[13] = "E_MIC_SYSTEM"
MIC_ERRCODES[14] = "E_MIC_SCIF_ERROR"
MIC_ERRCODES[15] = "E_MIC_RAS_ERROR"
MIC_ERRCODES[16] = "E_MIC_STACK"
MIC_ERRCODES[17] = "E_MIC_GET_WMI_PARAM_VALUE_FAILED"
MIC_ERRCODES[18] = "E_NO_WINDOWS_SUPPORT"
MIC_ERRCODES[19] = "E_WMI_FAILED"

#constants to use with _set_feature method
PCI = 0
THERMAL = 1
MEMINFO = 2
PROC = 3
COREINFO = 4
VERSION = 5
POWERUTIL = 6
COREUTIL = 7
MEMUTIL = 8
TURBO = 9
POWERLIMIT = 10
THROTTLE = 11
PM = 12
MDH = 13

#FEATURES returns feature based on API name
FEATURES = dict()
FEATURES["mic_get_pci_domain_id"] = PCI
FEATURES["mic_get_pci_class_code"] = PCI
FEATURES["mic_get_bus_number"] = PCI
FEATURES["mic_get_device_number"] = PCI
FEATURES["mic_get_vendor_id"] = PCI
FEATURES["mic_get_device_id"] = PCI
FEATURES["mic_get_revision_id"] = PCI
FEATURES["mic_get_subsystem_id"] = PCI
FEATURES["mic_get_link_speed"] = PCI
FEATURES["mic_get_link_width"] = PCI
FEATURES["mic_get_max_payload"] = PCI
FEATURES["mic_get_max_readreq"] = PCI
FEATURES["mic_get_pci_config"] = PCI
FEATURES["mic_update_pci_config"] = PCI
FEATURES["mic_get_thermal_info"] = THERMAL
FEATURES["mic_get_smc_hwrevision"] = THERMAL
FEATURES["mic_get_smc_fwversion"] = THERMAL
FEATURES["mic_is_smc_boot_loader_ver_supported"] = THERMAL
FEATURES["mic_get_smc_boot_loader_ver"] = THERMAL
FEATURES["mic_get_fsc_status"] = THERMAL
FEATURES["mic_get_die_temp"] = THERMAL
FEATURES["mic_is_die_temp_valid"] = THERMAL
FEATURES["mic_get_fan_rpm"] = THERMAL
FEATURES["mic_get_fan_pwm"] = THERMAL
FEATURES["mic_get_gddr_temp"] = THERMAL
FEATURES["mic_is_gddr_temp_valid"] = THERMAL
FEATURES["mic_get_fanin_temp"] = THERMAL
FEATURES["mic_is_fanin_temp_valid"] = THERMAL
FEATURES["mic_get_fanout_temp"] = THERMAL
FEATURES["mic_is_fanout_temp_valid"] = THERMAL
FEATURES["mic_get_vccp_temp"] = THERMAL
FEATURES["mic_is_vccp_temp_valid"] = THERMAL
FEATURES["mic_get_vddg_temp"] = THERMAL
FEATURES["mic_is_vddg_temp_valid"] = THERMAL
FEATURES["mic_get_vddq_temp"] = THERMAL
FEATURES["mic_is_vddq_temp_valid"] = THERMAL
FEATURES["mic_update_thermal_info"] = THERMAL
FEATURES["mic_get_memory_vendor"] = MEMINFO
FEATURES["mic_get_memory_revision"] = MEMINFO
FEATURES["mic_get_memory_density"] = MEMINFO
FEATURES["mic_get_memory_size"] = MEMINFO
FEATURES["mic_get_memory_speed"] = MEMINFO
FEATURES["mic_get_memory_type"] = MEMINFO
FEATURES["mic_get_memory_frequency"] = MEMINFO
FEATURES["mic_get_memory_voltage"] = MEMINFO
FEATURES["mic_get_ecc_mode"] = MEMINFO
FEATURES["mic_update_memory_info"] = MEMINFO
FEATURES["mic_get_memory_info"] = MEMINFO
FEATURES["mic_get_processor_model"] = PROC
FEATURES["mic_get_processor_family"] = PROC
FEATURES["mic_get_processor_type"] = PROC
FEATURES["mic_get_processor_steppingid"] = PROC
FEATURES["mic_get_processor_stepping"] = PROC
FEATURES["mic_update_processor_info"] = PROC
FEATURES["mic_get_processor_info"] = PROC
FEATURES["mic_get_cores_count"] = COREINFO
FEATURES["mic_get_cores_voltage"] = COREINFO
FEATURES["mic_get_cores_frequency"] = COREINFO
FEATURES["mic_update_cores_info"] = COREINFO
FEATURES["mic_get_cores_info"] = COREINFO
FEATURES["mic_get_uos_version"] = VERSION
FEATURES["mic_get_flash_version"] = VERSION
FEATURES["mic_get_fsc_strap"] = VERSION
FEATURES["mic_update_version_info"] = VERSION
FEATURES["mic_get_version_info"] = VERSION
FEATURES["mic_get_total_power_readings_w0"] = POWERUTIL
FEATURES["mic_get_total_power_sensor_sts_w0"] = POWERUTIL
FEATURES["mic_get_total_power_readings_w1"] = POWERUTIL
FEATURES["mic_get_total_power_sensor_sts_w1"] = POWERUTIL
FEATURES["mic_get_inst_power_readings"] = POWERUTIL
FEATURES["mic_get_inst_power_sensor_sts"] = POWERUTIL
FEATURES["mic_get_max_inst_power_readings"] = POWERUTIL
FEATURES["mic_get_max_inst_power_sensor_sts"] = POWERUTIL
FEATURES["mic_get_pcie_power_readings"] = POWERUTIL
FEATURES["mic_get_pcie_power_sensor_sts"] = POWERUTIL
FEATURES["mic_get_c2x3_power_readings"] = POWERUTIL
FEATURES["mic_get_c2x3_power_sensor_sts"] = POWERUTIL
FEATURES["mic_get_c2x4_power_readings"] = POWERUTIL
FEATURES["mic_get_c2x4_power_sensor_sts"] = POWERUTIL
FEATURES["mic_get_vccp_power_readings"] = POWERUTIL
FEATURES["mic_get_vccp_power_sensor_sts"] = POWERUTIL
FEATURES["mic_get_vccp_current_readings"] = POWERUTIL
FEATURES["mic_get_vccp_current_sensor_sts"] = POWERUTIL
FEATURES["mic_get_vccp_voltage_readings"] = POWERUTIL
FEATURES["mic_get_vccp_voltage_sensor_sts"] = POWERUTIL
FEATURES["mic_get_vddg_power_readings"] = POWERUTIL
FEATURES["mic_get_vddg_power_sensor_sts"] = POWERUTIL
FEATURES["mic_get_vddg_current_readings"] = POWERUTIL
FEATURES["mic_get_vddg_current_sensor_sts"] = POWERUTIL
FEATURES["mic_get_vddg_voltage_readings"] = POWERUTIL
FEATURES["mic_get_vddg_voltage_sensor_sts"] = POWERUTIL
FEATURES["mic_get_vddq_power_readings"] = POWERUTIL
FEATURES["mic_get_vddq_power_sensor_sts"] = POWERUTIL
FEATURES["mic_get_vddq_current_readings"] = POWERUTIL
FEATURES["mic_get_vddq_current_sensor_sts"] = POWERUTIL
FEATURES["mic_get_vddq_voltage_readings"] = POWERUTIL
FEATURES["mic_get_vddq_voltage_sensor_sts"] = POWERUTIL
FEATURES["mic_get_power_utilization_info"] = POWERUTIL
FEATURES["mic_update_power_utilization_info"] = POWERUTIL
FEATURES["mic_get_idle_counters"] = COREUTIL
FEATURES["mic_get_nice_counters"] = COREUTIL
FEATURES["mic_get_sys_counters"] = COREUTIL
FEATURES["mic_get_user_counters"] = COREUTIL
FEATURES["mic_get_idle_sum"] = COREUTIL
FEATURES["mic_get_sys_sum"] = COREUTIL
FEATURES["mic_get_nice_sum"] = COREUTIL
FEATURES["mic_get_user_sum"] = COREUTIL
FEATURES["mic_get_jiffy_counter"] = COREUTIL
FEATURES["mic_get_num_cores"] = COREUTIL
FEATURES["mic_get_threads_core"] = COREUTIL
FEATURES["mic_get_tick_count"] = COREUTIL
FEATURES["mic_update_core_util"] = COREUTIL
FEATURES["mic_get_core_util_info"] = COREUTIL
FEATURES["mic_get_total_memory_size"] = MEMUTIL
FEATURES["mic_get_available_memory_size"] = MEMUTIL
FEATURES["mic_get_memory_buffers_size"] = MEMUTIL
FEATURES["mic_update_memory_util"] = MEMUTIL
FEATURES["mic_get_memory_utilization_info"] = MEMUTIL
FEATURES["mic_get_turbo_state"] = TURBO
FEATURES["mic_get_turbo_state_valid"] = TURBO
FEATURES["mic_update_turbo_state_info"] = TURBO
FEATURES["mic_get_turbo_state_info"] = TURBO
FEATURES["mic_get_turbo_mode"] = TURBO
FEATURES["mic_get_power_limit"] = POWERLIMIT
FEATURES["mic_get_power_phys_limit"] = POWERLIMIT
FEATURES["mic_get_power_hmrk"] = POWERLIMIT
FEATURES["mic_get_power_lmrk"] = POWERLIMIT
FEATURES["mic_get_time_window0"] = POWERLIMIT
FEATURES["mic_get_time_window1"] = POWERLIMIT
FEATURES["mic_free_power_limit"] = POWERLIMIT
FEATURES["mic_get_throttle_state_info"] = THROTTLE
FEATURES["mic_get_thermal_ttl_active"] = THROTTLE
FEATURES["mic_get_thermal_ttl_current_len"] = THROTTLE
FEATURES["mic_get_thermal_ttl_count"] = THROTTLE
FEATURES["mic_get_thermal_ttl_time"] = THROTTLE
FEATURES["mic_get_power_ttl_active"] = THROTTLE
FEATURES["mic_get_power_ttl_current_len"] = THROTTLE
FEATURES["mic_get_power_ttl_count"] = THROTTLE
FEATURES["mic_get_power_ttl_time"] = THROTTLE
FEATURES["mic_free_throttle_state_info"] = THROTTLE
FEATURES["mic_update_ttl_state"] = THROTTLE
FEATURES["mic_get_uos_pm_config"] = PM
FEATURES["mic_get_cpufreq_mode"] = PM
FEATURES["mic_get_corec6_mode"] = PM
FEATURES["mic_get_pc3_mode"] = PM
FEATURES["mic_get_pc6_mode"] = PM
FEATURES["mic_free_uos_pm_config"] = PM
FEATURES["mic_update_uos_pm_config"] = PM
FEATURES["mic_get_silicon_sku"] = MDH
FEATURES["mic_get_serial_number"] = MDH
FEATURES["mic_get_post_code"] = MDH
FEATURES["mic_get_led_alert"] = MDH
FEATURES["mic_set_led_alert"] = MDH
FEATURES["mic_set_turbo_mode"] = MDH
FEATURES["mic_get_device_type"] = MDH
FEATURES["mic_is_ras_avail"] = MDH
FEATURES["mic_get_uuid"] = MDH
FEATURES["mic_get_smc_persistence_flag"] = MDH
FEATURES["mic_set_smc_persistence_flag"] = MDH

#STRUCTS returns the name of a struct given a feature
STRUCTS = dict()
STRUCTS[PCI] = "mic_pci_config"
STRUCTS[THERMAL] = "mic_thermal_info"
STRUCTS[MEMINFO] = "mic_device_mem"
STRUCTS[PROC] = "mic_processor_info"
STRUCTS[COREINFO] = "mic_cores_info"
STRUCTS[VERSION] = "mic_version_info"
STRUCTS[POWERUTIL] = "mic_power_util_info"
STRUCTS[COREUTIL] = "mic_core_util"
STRUCTS[MEMUTIL] = "mic_memory_util_info"
STRUCTS[TURBO] = "mic_turbo_info"
STRUCTS[POWERLIMIT] = "mic_power_limit"
STRUCTS[THROTTLE] = "mic_throttle_state_info"
STRUCTS[PM] = "mic_uos_pm_config"
STRUCTS[MDH] = "mdh"

#INIT_STRUCT returns the name of the function that
#allocates memory for a certain struct, given the feature
INIT_STRUCTS = dict()
INIT_STRUCTS[PCI] = "mic_get_pci_config"
INIT_STRUCTS[THERMAL] = "mic_get_thermal_info"
INIT_STRUCTS[MEMINFO] = "mic_get_memory_info"
INIT_STRUCTS[PROC] = "mic_get_processor_info"
INIT_STRUCTS[COREINFO] = "mic_get_cores_info"
INIT_STRUCTS[VERSION] = "mic_get_version_info"
INIT_STRUCTS[POWERUTIL] = "mic_get_power_utilization_info"
INIT_STRUCTS[COREUTIL] = "mic_alloc_core_util"
INIT_STRUCTS[MEMUTIL] = "mic_get_memory_utilization_info"
INIT_STRUCTS[TURBO] = "mic_get_turbo_state_info"
INIT_STRUCTS[POWERLIMIT] = "mic_get_power_limit"
INIT_STRUCTS[THROTTLE] = "mic_get_throttle_state_info"
INIT_STRUCTS[PM] = "mic_get_uos_pm_config"

#FREE_STRUCTS returns the name of the function
#that frees memory for a certain struct
FREE_STRUCTS = dict()
FREE_STRUCTS[PCI] = "mic_free_pci_config"
FREE_STRUCTS[THERMAL] = "mic_free_thermal_info"
FREE_STRUCTS[MEMINFO] = "mic_free_memory_info"
FREE_STRUCTS[PROC] = "mic_free_processor_info"
FREE_STRUCTS[COREINFO] = "mic_free_cores_info"
FREE_STRUCTS[VERSION] = "mic_free_version_info"
FREE_STRUCTS[POWERUTIL] = "mic_free_power_utilization_info"
FREE_STRUCTS[COREUTIL] = "mic_free_core_util"
FREE_STRUCTS[MEMUTIL] = "mic_free_memory_utilization_info"
FREE_STRUCTS[TURBO] = "mic_free_turbo_info"
FREE_STRUCTS[POWERLIMIT] = "mic_free_power_limit"
FREE_STRUCTS[THROTTLE] = "mic_free_throttle_state_info"
FREE_STRUCTS[PM] = "mic_free_uos_pm_config"

#List to contain names for flash APIs
FLASH_APIS = list()
FLASH_APIS.append("mic_read_flash")

MICMGMT_LIBRARY = "libmicmgmt.so.0"

class MicException(Exception):
    """Base class for errors returned by libmicmgmt"""
    pass

class MicFlashException(Exception):
    """Base class for flash errors returned by libmicmgmt"""
    pass

def mic_generic_func(mic_obj, ctype):
    """Generic function for interacting with libmicmgmt.

    Expects a MicDevice object, and a type (ctypes, bool, str) - which is in
    turn parameter to a libmicmgmt function.

    This function does three simple things:
    1. Instances a ctypes object to pass to libmicmgmt function
    2. Gets pointers to libmicmgmt function and opaque struct via _get_func()
    3. Calls libmicmgmt function

    """
    ret_type = ctype
    stack_frame = inspect.stack()[1]
    calling_func = stack_frame[3]
    if ret_type is str:
        func, struct = mic_obj._get_func(calling_func)
        size = ctypes.c_long(MAX_STRLEN)
        ret_str = ctypes.create_string_buffer(MAX_STRLEN)
        ret_code = func(struct, ret_str, ctypes.byref(size))
        mic_obj._check_success(ret_code, calling_func=calling_func)
        return ret_str.value

    if ret_type is bool:
        var = ctypes.c_uint()
    else:
        var = ctype()

    func, struct = mic_obj._get_func(calling_func)
    ret_code = func(struct, ctypes.byref(var))
    mic_obj._check_success(ret_code, calling_func=calling_func)
    if ret_type is bool:
        return bool(var.value)
    else:
        return var.value

def mic_generic_set(mic_obj, value, ctype, pointer=False):

    stack_frame = inspect.stack()[1]
    calling_func = stack_frame[3]
    func, struct = mic_obj._get_func(calling_func)
    if pointer:
        ret_code = func(struct, ctypes.byref(ctype(value)))
    else:
        ret_code = func(struct, ctype(value))

    mic_obj._check_success(ret_code, calling_func=calling_func)
    return ret_code
