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

#ifdef __linux__
#ifndef MICLIB_SRC_HOST_PLATFORM_H_
#define MICLIB_SRC_HOST_PLATFORM_H_


#include <scif.h>
#include <mic/io_interface.h>
#include <mic/micras_api.h>
#include <stdint.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <assert.h>
#include <cstring>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <sstream>
#include <fstream>

#include "miclib_int.h"
#include "miclib_exception.h"

struct host_flash_op {
    MIC_FLASH_CMD_TYPE op;
    void *             buf;
    size_t             bufsize;
    int                fd;
};

class host_platform {
public:
    host_platform(uint32_t device_num, bool init_scif = false);
    virtual ~host_platform();

    /* class static methods */
    static void get_devices(struct mic_devices_list *devices, uint16_t
                            n_allocated);

    static std::string get_sysfs_base_path();
    static void set_sysfs_base_path(std::string const &device_path);

    /* object methods */
    virtual void get_device_type(uint32_t &device_type);
    virtual void get_pci_config(struct mic_pci_config &pci_config);

    virtual void flash_get_status(struct host_flash_op *desc, struct
                                  mic_flash_status_info *status);
    virtual uint32_t get_flash_vendor(void);
    virtual host_flash_op *flash_update_start(void *buf, size_t bufsize);
    virtual void flash_update_done(struct host_flash_op *desc);
    virtual host_flash_op *flash_read_start(void *buf, size_t bufsize);
    virtual void flash_read_done(struct host_flash_op *desc);

    std::string get_sysfs_attribute(std::string const &attr);
    virtual int is_ras_avail();

    /* static consts, need to be moved to protected */
    static const char *STATE_ONLINE;
    static const char *MODE_FLASH;
    static const char *MODE_ELF;
    static const char *STATE_READY;

    static const int SMC_SENSOR_UNAVAILABLE = 0x3;

protected:
    std::string get_sysfs_device_path() const;
    std::string get_sysfs_attr_path(std::string const &attr) const;
    std::string get_sysfs_comp_path(std::string const &component, std::string
                                    const &attr);

    /* gets device's attributes in sysfs
     * the usual path for this is "/sys/class/mic/micX/property" */
    std::string get_device_property(std::string const &property);

    /* writes value in devices's sysfs
     * the usual path for this is "/sys/class/mic/micX/property" */
    virtual void set_device_property(std::string const &property, std::string
                                     const &value) const;

    virtual void scif_request(int cmd, void *buf, size_t size, uint32_t parm =
                                  0);
    virtual int scif_open_conn();
    virtual void read_postcode_property(std::string const &file, std::string &str);
    virtual void read_property(std::string const &file, std::string &str);
    virtual void read_property_token(std::string const &file,
                                     std::string &token, std::string &str);
    virtual host_flash_op *flash_init_fd(void *buf, size_t size,
                                         MIC_FLASH_CMD_TYPE op);
    virtual host_flash_op *flash_init_fd(void *buf, size_t size,
                                         MIC_FLASH_CMD_TYPE op, int open_flags);
    virtual void flash_close_fd(struct host_flash_op *desc);
    virtual void flash_ioctl(struct host_flash_op *desc);
    virtual void flash_ioctl(int fd, void *buf, size_t size, MIC_FLASH_CMD_TYPE
                             op);

    virtual void set_device_mode(const int &mode);
    virtual void set_device_mode(const int &mode, const std::string &elf_path) const;

    uint32_t _devid;
    scif_epd_t _scif_ep;
    bool _scif_inited;
    std::string _scif_err_msg;
    int _scif_errno;
    int _scif_mic_errno;
    pthread_mutex_t _mutex;

    /* static consts */
    static const char *KSYSFS_DEVICE_PREFIX;
    static const char *KHOST_DRIVER_PATH;
    static const char *PCI_PREFIX;
    static const char *SYSFS_STATE_CHANGE;
    static const size_t CLST_SIZE = sizeof(struct mr_rsp_clst);
    static const size_t CFREQ_SIZE = sizeof(struct mr_rsp_freq);
    static const size_t CVOLT_SIZE = sizeof(struct mr_rsp_volt);
    static const size_t GDDR_SIZE = sizeof(struct mr_rsp_gddr);
    static const size_t GFREQ_SIZE = sizeof(struct mr_rsp_freq);
    static const size_t GVOLT_SIZE = sizeof(struct mr_rsp_volt);
    static const size_t SMC_SIZE = sizeof(struct mr_rsp_smc);
    static const size_t ECC_SIZE = sizeof(struct mr_rsp_ecc);
    static const size_t VER_SIZE = sizeof(struct mr_rsp_vers);
    static const size_t PWRUT_SIZE = sizeof(struct mr_rsp_power);
    static const size_t PWRLIM_SIZE = sizeof(struct mr_rsp_plim);
    static const size_t PTRIG_SIZE = sizeof(struct mr_rsp_ptrig);
    static const size_t MEMUT_SIZE = sizeof(struct mr_rsp_mem);
    static const size_t SERNO_SIZE = sizeof(struct mr_rsp_hwinf);
    static const size_t TURBO_SIZE = sizeof(struct mr_rsp_trbo);
    static const size_t TTL_SIZE = sizeof(struct mr_rsp_ttl);
    static const size_t TEMP_SIZE = sizeof(struct mr_rsp_temp);
    static const size_t PMCFG_SIZE = sizeof(struct mr_rsp_pmcfg);
    static const size_t LED_SIZE = sizeof(struct mr_rsp_led);
    static const size_t PERSIST_FLAG_SIZE = sizeof(struct mr_rsp_perst);
    static const size_t PVER_SIZE = sizeof(struct mr_rsp_pver);
    static const int CLST_CMD = MR_REQ_CLST;
    static const int CFREQ_CMD = MR_REQ_CFREQ;
    static const int CVOLT_CMD = MR_REQ_CVOLT;
    static const int GDDR_CMD = MR_REQ_GDDR;
    static const int GFREQ_CMD = MR_REQ_GFREQ;
    static const int GVOLT_CMD = MR_REQ_GVOLT;
    static const int ECC_CMD = MR_REQ_ECC;
    static const int SMC_GET_CMD = MR_GET_SMC;
    static const int SMC_SET_CMD = MR_SET_SMC;
    static const int VER_CMD = MR_REQ_VERS;
    static const int PWRUT_CMD = MR_REQ_PWR;
    static const int PWRALT_CMD = MR_REQ_PWRALT;
    static const int CPUHOT_CMD = MR_REQ_GPUHOT;
    static const int PWRLIM_CMD = MR_REQ_PLIM;
    static const int PWRLIM0_GET_CMD = MR_REQ_PROCHOT;
    static const int PWRLIM0_SET_CMD = MR_SET_PROCHOT;
    static const int PWRLIM1_GET_CMD = MR_REQ_PWRALT;
    static const int PWRLIM1_SET_CMD = MR_SET_PWRALT;
    static const int MEMUT_CMD = MR_REQ_MEM;
    static const int SERNO_CMD = MR_REQ_HWINF;
    static const int TURBO_CMD = MR_REQ_TRBO;
    static const int CUTIL_REQUEST = MR_REQ_CUTL;
    static const int TTL_CMD = MR_REQ_TTL;
    static const int TEMP_CMD = MR_REQ_TEMP;
    static const int PMCFG_CMD = MR_REQ_PMCFG;
    static const int LED_READ = MR_REQ_LED;
    static const int LED_WRITE = MR_SET_LED;
    static const int TURBO_WRITE = MR_SET_TRBO;
    static const int PERSIST_FLAG_GET_CMD = MR_REQ_PERST;
    static const int PERSIST_FLAG_SET_CMD = MR_SET_PERST;
    static const int PVER_CMD = MR_REQ_PVER;
    static const MIC_FLASH_CMD_TYPE FLASH_READ = FLASH_CMD_READ;
    static const MIC_FLASH_CMD_TYPE FLASH_WRITE = FLASH_CMD_WRITE;
    static const MIC_FLASH_CMD_TYPE FLASH_READ_DATA = FLASH_CMD_READ_DATA;
    static const int SMC_UUID = 0x10;
    static const int SMC_FW_VER = 0x11;
    static const int SMC_HW_REV = 0x14;
    static const int SMC_BOOT_LOADER_VER = 0x16;
    static const int SMC_TEMP_CPU = 0x40;
    static const int SMC_FAN_RPM = 0x49;
    static const int SMC_FAN_PWM = 0x4A;
    static const int SMC_LED_ALERT = 0x60;
    static const char *STATE;
    static const char *MODE;
    static const char *MODE_MAINT_CMD;
    static const char *MODE_MAINT_CMD_AB_STEP;
    static const char *MODE_MAINT_CMD_C_STEP;
    static const char *MODE_READY_CMD;
    static const char *POST_CODE;
    static const char *FLASH_UPDATE;
    static const char *ENABLE_VMM_UPDATE;
    static const char *FAIL_SAFE_OFFS;
    static const char *MEM_SIZE;
    static const char *PROCESSOR_MODEL;
    static const char *PROCESSOR_EXT_MODEL;
    static const char *PROCESSOR_TYPE;
    static const char *PROCESSOR_FAMILY;
    static const char *PROCESSOR_EXT_FAMILY;
    static const char *PROCESSOR_STEPPING;
    static const char *PROCESSOR_STEPPING_DATA;
    static const char *PROCESSOR_SUBSTEPPING_DATA;
    static const char *SI_SKU;
    static std::string SYSFS_DEVICE_PATH;
    static const std::string MIC_MODULE;
    static const std::string SYSFS_SCIF_PATH;
    static const std::string KNC_IDENTIFIER;

    static const uint32_t WAIT_TIME_EAGAIN  = 20000;
    static const uint32_t WAIT_COUNT_EAGAIN = 50;

private:

    host_platform(host_platform const &);
    host_platform &operator=(host_platform const &);
};

#endif /* MICLIB_SRC_HOST_PLATFORM_H_ */
#endif
