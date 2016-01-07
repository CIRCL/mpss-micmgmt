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

#ifndef MICLIB_SRC_MICLIB_INT_H_
#define MICLIB_SRC_MICLIB_INT_H_

#include <assert.h>
#include <map>
#include <string>
#include <limits.h>
#include <mic/micras_api.h>
#ifndef __linux__
#define bzero(b, len)    (memset((b), '\0', (len)), (void)0)
#define NAME_MAX    (1000)
#define EBADE       EBADMSG
#endif

#define ASSERT      assert


#ifdef __cplusplus
extern "C" {
#endif
struct mic_devices_list {
    int n_devices;
    int devices[1];
};

struct mic_device_mem {
    char     vendor_name[NAME_MAX + 1];
    uint16_t revision;
    uint32_t density;
    uint32_t size;
    uint32_t speed;
    char     memory_type[NAME_MAX + 1];
    uint32_t freq;
    uint32_t volt;
    uint16_t ecc;
};


struct mic_processor_info {
    uint16_t model;
    uint16_t model_ext;
    uint16_t family;
    uint16_t family_ext;
    uint16_t stepping_data;
    uint16_t substepping_data;
    uint16_t type;
    char     stepping[NAME_MAX + 1];
};

struct mic_cores_info {
    uint32_t num_cores;
    uint32_t voltage;
    uint32_t frequency;
};

struct smc_hw_rev_bits {
    uint32_t mem_cfg     : 1,
             power       : 1,
             hsink_type  : 1,
             reserved1   : 5,
             fab_version : 3,
             reserved2   : 5,
             board_type  : 2,
             reserved3   : 14;
};

union smc_hw_rev {
    uint32_t               value;
    struct smc_hw_rev_bits bits;
};

struct smc_fw_ver_bits {
    uint32_t buildno : 16,
             minor   : 8,
             major   : 8;
};

union smc_fw_ver {
    uint32_t               value;
    struct smc_fw_ver_bits bits;
};

struct smc_boot_loader_ver_bits {
    uint32_t buildno : 16,
             minor   : 8,
             major   : 8;
};

union smc_boot_loader_ver {
    uint32_t                        value;
    struct smc_boot_loader_ver_bits bits;
};

struct mic_version_info {
    struct mr_rsp_vers vers;
};

struct mic_thermal_info {
    union smc_fw_ver          smc_version;
    union smc_hw_rev          smc_revision;
    union smc_boot_loader_ver smc_boot_loader;
    int                       smc_boot_loader_ver_supported;
    uint32_t                  fsc_status;
    uint32_t                  fsc_strap;
    uint32_t                  fan_rpm;
    uint32_t                  fan_pwm;
    struct mr_rsp_temp        temp;
};

struct mic_pci_config {
    uint16_t domain;
    uint16_t bus_no;
    uint16_t device_no;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  revision_id;
    uint16_t subsystem_id;
    uint32_t link_speed;
    uint32_t link_width;
    uint32_t payload_size;
    uint32_t read_req_size;
    uint32_t access_violation;
    char     class_code[NAME_MAX + 1];
    uint16_t domain_info_implemented;
};

struct mic_flash_status_info {
    uint32_t complete;
    int      flash_status;
    int      ext_status;
};

struct mic_power_util_info {
    struct mr_rsp_power pwr;
};

struct mic_power_limit {
    uint32_t phys;          /* Physical limit, in W */
    uint32_t hmrk;          /* High water mark, in W (power limit 0/cpu hot) */
    uint32_t lmrk;          /* Low water mark, in W  (power limit 1/pwr alt) */
    uint32_t time_win0;     /* Time window 0 in msec */
    uint32_t time_win1;     /* Time window 1 in msec */
};

struct mic_memory_util_info {
    struct mr_rsp_mem mem;
};

struct mic_core_util {
    MrRspCutl c_util;
};

struct mic_flash_op {
    struct mic_device *   mdh;
    struct host_flash_op *h_desc;
};

struct mic_turbo_info {
    struct mr_rsp_trbo trbo;
};

struct mic_throttle_state_info {
    struct mr_rsp_ttl ttl_state;
};

struct uos_pm_config_bits {
    uint32_t cpufreq    : 1;         /* [0]   0 = disabled, 1 = enabled */
    uint32_t corec6     : 1;         /* [1]   0 = disabled, 1 = enabled */
    uint32_t pc3        : 1;         /* [2]   0 = disabled, 1 = enabled */
    uint32_t pc6        : 1;         /* [3]   0 = disabled, 1 = enabled */
    uint32_t not_in_use : 28;        /* [31:4] not currently in use */
};

union uos_pm_config {
    uint32_t                  mode;
    struct uos_pm_config_bits bits;
};

struct mic_uos_pm_config {
    union uos_pm_config pm_config;
};

enum mic_device_modes {
	MIC_MODE_MAINT = 0,
	MIC_MODE_NORMAL = 1,
	MIC_MODE_READY = 2,
	MIC_MODE_ELF = 3
};

#ifdef __cplusplus
}
#endif
#endif /* MICLIB_SRC_MICLIB_INT_H_ */
