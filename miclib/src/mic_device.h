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

#ifndef MICLIB_SRC_MIC_DEVICE_H_
#define MICLIB_SRC_MIC_DEVICE_H_

#include <stdint.h>
#include <memory>
#include "miclib_exception.h"
#include "miclib.h"
#ifdef __linux__
#include "host_platform.h"
#else
#include "windows_platform.h"
#endif


struct mic_device;

struct mic_device : public host_platform {
public:
    mic_device(uint32_t id);
    virtual ~mic_device();
    virtual void get_device_num(uint32_t *device_num) = 0;
    virtual const char *get_device_name() = 0;


    /* device flash operations */
    virtual void flash_active_offs(off_t &active) = 0;
    virtual void flash_size(size_t &size) = 0;
    virtual struct host_flash_op *flash_update_start(void *buffer, size_t
                                                     bufsize) = 0;
    virtual void flash_update_done(struct host_flash_op *desc) = 0;
    virtual struct host_flash_op *flash_read_start(void *buffer, size_t
                                                   bufsize) = 0;
    virtual void flash_read_done(struct host_flash_op *desc) = 0;
    virtual struct host_flash_op *set_ecc_mode_start(uint16_t ecc_enabled) = 0;
    virtual void set_ecc_mode_done(struct host_flash_op *desc) = 0;
    virtual void flash_get_status(struct host_flash_op *desc, struct
                                  mic_flash_status_info *status) = 0;
    virtual void flash_version_offs(off_t &offs) = 0;
    virtual std::string get_flash_vendor_device() = 0;

    virtual void get_device_type(uint32_t &device_type) = 0;
    virtual std::string get_state() = 0;
    virtual std::string get_mode() = 0;
    virtual void in_maint_mode(int *maint) = 0;
#ifdef __linux__
    virtual bool in_custom_boot() = 0;
#endif
    virtual void in_ready_state(int *ready) = 0;
    virtual void set_mode(int const &mode) = 0;
    virtual void set_mode(int const &mode, const std::string &elf_path) = 0;
    virtual void get_post_code(std::string &poststr) = 0;
    virtual void get_memory_info(struct mic_device_mem *mem) = 0;
    virtual void get_processor_info(struct mic_processor_info *processor) =
        0;
    virtual void get_cores_info(struct mic_cores_info *cores) = 0;
    virtual void get_thermal_info(struct mic_thermal_info *thermal) = 0;
    virtual void get_pci_config(struct mic_pci_config &config) = 0;
    virtual void get_silicon_sku(char *sku, size_t *size) = 0;
    virtual void get_serial_number(char *serial, size_t *size) = 0;
    virtual void get_version_info(struct mic_version_info *ver) = 0;
    virtual void get_power_utilization_info(struct mic_power_util_info *pwr)
        = 0;
    virtual void get_power_limit(struct mic_power_limit *limit) = 0;
    virtual void set_power_limit0(uint32_t power, uint32_t time_window) = 0;
    virtual void set_power_limit1(uint32_t power, uint32_t time_window) = 0;
    virtual void get_memory_utilization_info(struct
                                             mic_memory_util_info *memory) = 0;
    virtual void get_core_util(struct mic_core_util *cutil) = 0;
    virtual void get_led_alert(uint32_t *led_alert) = 0;
    virtual void set_led_alert(uint32_t *led_alert) = 0;
    virtual void get_turbo_state_info(struct mic_turbo_info *turbo) = 0;
    virtual void set_turbo_mode(uint32_t *mode) = 0;
    virtual void get_uuid(uint8_t *uuid, size_t *size) = 0;
    virtual void get_throttle_state_info(struct
                                         mic_throttle_state_info *ttl_state) =
        0;
    virtual void get_uos_pm_config(struct mic_uos_pm_config *pm_config) = 0;
#ifdef __linux__
    virtual void read_smc_reg(uint8_t reg, uint8_t *buffer, size_t *size)
                = 0;
    virtual void write_smc_reg(uint8_t reg, const uint8_t *buffer,
                                   size_t size) = 0;
#endif
    virtual void get_smc_persistence_flag(int *persist_flag) = 0;
    virtual void set_smc_persistence_flag(int persist_flag) = 0;

protected:
    std::string _name;

private:
    mic_device(mic_device const &);
    mic_device &operator=(mic_device const &);
};

#endif /* MICLIB_SRC_MIC_DEVICE_H_ */
