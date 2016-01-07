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

#ifndef MICLIB_SRC_KNC_DEVICE_H_
#define MICLIB_SRC_KNC_DEVICE_H_

#include <stdint.h>
#include <fcntl.h>
#include "mic_device.h"

class knc_device : public mic_device
{
public:
    knc_device(uint32_t id);
    virtual ~knc_device();
    virtual void flash_active_offs(off_t &active);
    virtual void flash_size(size_t &size);
    virtual struct host_flash_op *flash_update_start(void *buffer, size_t
                                                     bufsize);
    virtual void flash_update_done(struct host_flash_op *desc);
    virtual struct host_flash_op *flash_read_start(void *buffer, size_t
                                                   bufsize);
    virtual void flash_read_done(struct host_flash_op *desc);
    struct host_flash_op *set_ecc_mode_start(uint16_t ecc_enabled);
    void set_ecc_mode_done(struct host_flash_op *desc);
    virtual void flash_version_offs(off_t &offs);
    virtual void flash_get_status(struct host_flash_op *desc, struct
                                  mic_flash_status_info *status);
    virtual std::string get_flash_vendor_device();
    void get_device_num(uint32_t *device_num);
    const char *get_device_name();
    void get_device_type(uint32_t &device_type);
    void start_maint_mode(struct mic_device *mdh);
    void stop_maint_mode(struct mic_device *mdh);
    std::string get_state();
    std::string get_mode();
    void in_maint_mode(int *maint);
    bool in_custom_boot();
    void in_ready_state(int *ready);
    void set_mode(int const &mode);
    void set_mode(int const &mode, const std::string &elf_path);
    void get_post_code(std::string &poststr);
    void get_pci_config(struct mic_pci_config &config);

    /* dev memory info */
    void get_memory_info(struct mic_device_mem *mem);

    /* processor info */
    void get_processor_info(struct mic_processor_info *processor);

    /* ecc info */
    void get_silicon_sku(char *sku, size_t *size);

    /* serial info */
    void get_serial_number(char *serial, size_t *size);

    /* processor info */
    void get_cores_info(struct mic_cores_info *cores);

    /* thermal info */
    void get_thermal_info(struct mic_thermal_info *thermal);

    /* version info */
    void get_version_info(struct mic_version_info *ver);

    /* power utilization info */
    void get_power_utilization_info(struct mic_power_util_info *pwr);

    /* power limits */
    void get_power_limit(struct mic_power_limit *limit);
    void set_power_limit0(uint32_t power, uint32_t time_window);
    void set_power_limit1(uint32_t power, uint32_t time_window);


    /* memory utilization info */
    void get_memory_utilization_info(struct mic_memory_util_info *memory);

    /* core utilization */
    void get_core_util(struct mic_core_util *cutil);

    /* get led alert */
    void get_led_alert(uint32_t *led_alert);
    void set_led_alert(uint32_t *led_alert);

    /* turbo mode info */
    void get_turbo_state_info(struct mic_turbo_info *turbo);
    void set_turbo_mode(uint32_t *mode);

    /*uuid*/
    void get_uuid(uint8_t *uuid, size_t *size);

    /* throttle state info */
    void get_throttle_state_info(struct mic_throttle_state_info *ttl_state);

    /* uos pm config */
    void get_uos_pm_config(struct mic_uos_pm_config *pm_config);

#ifdef __linux__
    /* smc registers */
    virtual void read_smc_reg(uint8_t reg, uint8_t *buffer, size_t *size);
    virtual void write_smc_reg(uint8_t reg, const uint8_t *buffer,
                                   size_t size);
#endif

    /* smc config */
    void get_smc_persistence_flag(int *persist_flag);
    void set_smc_persistence_flag(int persist_flag);

    static const uint32_t FLASH_MFG_MASK = 0x0000ff;
    static const uint32_t FLASH_MFG_ATMEL = 0x1f;
    static const uint32_t FLASH_MFG_MACRONIX = 0xc2;
    static const uint32_t FLASH_MFG_WINBOND = 0xef;

    static const uint32_t FLASH_FAMILY_MASK = 0x00ff00;
    static const uint32_t FLASH_FAMILY_AT26DF = 0x4500;
    static const uint32_t FLASH_FAMILY_AT25DF = 0x4600;
    static const uint32_t FLASH_FAMILY_MX25 = 0x2000;
    static const uint32_t FLASH_FAMILY_W25X = 0x3000;
    static const uint32_t FLASH_FAMILY_W25Q = 0x4000;

    static const uint32_t FLASH_DEVICE_MASK = 0xff0000;
    static const uint32_t FLASH_DEVICE_081 = 0x010000;
    static const uint32_t FLASH_DEVICE_161 = 0x020000;
    static const uint32_t FLASH_DEVICE_L8005D = 0x140000;
    static const uint32_t FLASH_DEVICE_L1605D = 0x150000;
    static const uint32_t FLASH_DEVICE_L3205D = 0x160000;
    static const uint32_t FLASH_DEVICE_L6405D = 0x170000;
    static const uint32_t FLASH_DEVICE_80 = 0x140000;
    static const uint32_t FLASH_DEVICE_16 = 0x150000;
    static const uint32_t FLASH_DEVICE_32 = 0x160000;
    static const uint32_t FLASH_DEVICE_64 = 0x170000;

    /* Support for SMC boot loader version register is provided starting
     * from SMC FW version 1.9 */
    static const union smc_fw_ver SMC_BOOT_LOADER_VER_SUPPORT;

private:
    knc_device(knc_device const &);
    knc_device &operator=(knc_device const &);
};

#endif /* MICLIB_SRC_KNC_DEVICE_H_ */
