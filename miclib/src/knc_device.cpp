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

#include <sstream>
#include <string>

#include "knc_device.h"

const static size_t KNC_FLASH_SIZE = 0x200000;
const static size_t KNC_FLASH_CSS_HEADER_OFFSET = 0x28000;
const static size_t KNC_FLASH_CSS_HEADER_SIZE = 0x284;
const static size_t KNC_FLASH_VERSION_OFFSET = 0xff0;
const static size_t KNC_FLASH_VERSION_STR_MAX = 256;

/* Support for SMC boot loader version register is provided starting from
 * SMC FW version 1.9 */
#ifdef __linux__
const union smc_fw_ver knc_device::SMC_BOOT_LOADER_VER_SUPPORT =
{ bits: { 0, 9, 1 } }; //$$Review asivaram- This does not work in windows, reason unknows
#else
const union smc_fw_ver knc_device::SMC_BOOT_LOADER_VER_SUPPORT = { 0x901 };
#endif
knc_device::knc_device(uint32_t id) : mic_device(id)
{
    std::stringstream ss;

    ss << "mic" << id;
    _name = ss.str();
}

void knc_device::get_device_num(uint32_t *device_num)
{
    *device_num = _devid;
}

const char *knc_device::get_device_name()
{
    return _name.c_str();
}

knc_device::~knc_device()
{
}

void knc_device::get_device_type(uint32_t &device_type)
{
    device_type = KNC_ID;
}

void knc_device::flash_active_offs(off_t &active)
{
    std::stringstream ss, filename;
    std::string offs_str = get_device_property(host_platform::FAIL_SAFE_OFFS);

    try {
        ss << std::hex << offs_str;
        ss >> active;
    } catch (...) {
        std::string filename_str =
            get_sysfs_attr_path(host_platform::FAIL_SAFE_OFFS);
        throw mic_exception(E_MIC_STACK,
                            "malformed file: " + filename_str, EINVAL);
    }
}

void knc_device::flash_size(size_t &size)
{
    size = KNC_FLASH_SIZE;
}

struct host_flash_op *knc_device::flash_update_start(void *buf, size_t bufsize)
{
    return host_platform::flash_update_start(buf, bufsize);
}

void knc_device::flash_update_done(struct host_flash_op *desc)
{
    host_platform::flash_update_done(desc);
}

struct host_flash_op *knc_device::flash_read_start(void *buf, size_t bufsize)
{
    return host_platform::flash_read_start(buf, bufsize);
}

void knc_device::flash_read_done(struct host_flash_op *desc)
{
    host_platform::flash_read_done(desc);
}

struct host_flash_op *knc_device::set_ecc_mode_start(
    uint16_t ecc_enabled)
{
    struct host_flash_op *desc = NULL;
    MIC_FLASH_CMD_TYPE ecc_cmd = RAS_CMD_ECC_DISABLE;
    uint8_t buf = 0;
    size_t size = 0;

    if (ecc_enabled)
        ecc_cmd = RAS_CMD_ECC_ENABLE;

    desc = host_platform::flash_init_fd(&buf, size, ecc_cmd, O_RDWR);
    try {
        host_platform::flash_ioctl(desc);
    } catch (const mic_exception &e) {
        host_platform::flash_close_fd(desc);
        throw e;
    }

    return desc;
}

void knc_device::set_ecc_mode_done(struct host_flash_op *desc)
{
    host_platform::flash_close_fd(desc);
}

void knc_device::flash_get_status(struct host_flash_op *desc,
                                  struct mic_flash_status_info *status)
{
    host_platform::flash_get_status(desc, status);
}

std::string knc_device::get_flash_vendor_device()
{
    uint32_t vendor_dev;
    std::stringstream vendor;

    vendor_dev = get_flash_vendor();
    switch (vendor_dev & FLASH_MFG_MASK) {
    case FLASH_MFG_ATMEL:
        vendor << "Atmel ";
        switch (vendor_dev & FLASH_FAMILY_MASK) {
        case FLASH_FAMILY_AT26DF:
            vendor << "AT26DF";
            break;

        case FLASH_FAMILY_AT25DF:
            vendor << "AT25DF";
            break;

        default:
            vendor << "<unknown>";
            break;
        }

        switch (vendor_dev & FLASH_DEVICE_MASK) {
        case FLASH_DEVICE_081:
            vendor << "081";
            break;

        case FLASH_DEVICE_161:
            vendor << "161";
            break;

        default:
            vendor << "xxx";
            break;
        }
        break;

    case FLASH_MFG_WINBOND:
        vendor << "Winbond ";
        switch (vendor_dev & FLASH_FAMILY_MASK) {
        case FLASH_FAMILY_W25Q:
            vendor << "W25Q";
            break;

        case FLASH_FAMILY_W25X:
            vendor << "W25X";
            break;

        default:
            vendor << "<unknown>";
            break;
        }

        switch (vendor_dev & FLASH_DEVICE_MASK) {
        case FLASH_DEVICE_80:
            vendor << "80";
            break;

        case FLASH_DEVICE_64:
            vendor << "64";
            break;

        case FLASH_DEVICE_32:
            vendor << "32";
            break;

        case FLASH_DEVICE_16:
            vendor << "16";
            break;

        default:
            vendor << "xx";
            break;
        }
        break;

    case FLASH_MFG_MACRONIX:
        vendor << "Macronix ";
        switch (vendor_dev & FLASH_FAMILY_MASK) {
        case FLASH_FAMILY_MX25:
            vendor << "MX25";
            break;
        default:
            vendor << "<unknown>";
            break;
        }

        switch (vendor_dev & FLASH_DEVICE_MASK) {
        case FLASH_DEVICE_L8005D:
            vendor << "L8005D";
            break;

        case FLASH_DEVICE_L1605D:
            vendor << "L1605D";
            break;

        case FLASH_DEVICE_L3205D:
            vendor << "L3205D";
            break;

        case FLASH_DEVICE_L6405D:
            vendor << "L6405D";
            break;

        default:
            vendor << "xxxxxx";
            break;
        }
        break;

    default:
        vendor << "<unknown> <unknown>";
        break;
    }

    return vendor.str();
}

void knc_device::flash_version_offs(off_t &offs)
{
    off_t fail_safe_offs;
    std::string offs_str;
    std::stringstream ss, filename;

    offs_str = get_device_property(host_platform::FAIL_SAFE_OFFS);

    try {
        ss << std::hex << offs_str;
        ss >> fail_safe_offs;
    } catch (...) {
        std::string filename_str = get_sysfs_attr_path(
            host_platform::FAIL_SAFE_OFFS);
        throw mic_exception(E_MIC_STACK,
                            "malformed file: " + filename_str, EINVAL);
    }

    offs = KNC_FLASH_CSS_HEADER_OFFSET + KNC_FLASH_CSS_HEADER_SIZE +
           KNC_FLASH_VERSION_OFFSET + fail_safe_offs;
}

void knc_device::get_memory_info(struct mic_device_mem *mem)
{
    std::string str_buf;
    std::stringstream ss;
    struct mr_rsp_gddr meminfo;
    struct mr_rsp_gfreq gddrfreq;
    struct mr_rsp_gvolt gddrvolt;
    struct mr_rsp_ecc eccinfo;
    uint32_t mem_size = 0;
    std::string mem_size_str;

    memset(&meminfo, 0, sizeof(struct mr_rsp_gddr));
    memset(&gddrfreq, 0, sizeof(struct mr_rsp_gfreq));
    memset(&gddrvolt, 0, sizeof(struct mr_rsp_gvolt));
    memset(&eccinfo, 0, sizeof(struct mr_rsp_ecc));

    /* Get dev_mem info using scif calls */
    scif_request(host_platform::GDDR_CMD, &meminfo,
                 host_platform::GDDR_SIZE);

    mem_size_str = get_device_property(host_platform::MEM_SIZE);

    if (!mem_size_str.empty()) {
        ss << std::hex << mem_size_str;
        ss >> mem_size;
    }
    strncpy(mem->vendor_name, (meminfo.dev + 1), (NAME_MAX));
    mem->vendor_name[NAME_MAX - 1] = '\0';

    mem->density = meminfo.size;
    mem->size = mem_size;
    mem->speed = meminfo.speed;
    mem->revision = meminfo.rev;
    strncpy(mem->memory_type, "GDDR5", (NAME_MAX));
    mem->memory_type[NAME_MAX] = '\0';

    /* memory_frequency */
    scif_request(host_platform::GFREQ_CMD,
                 &gddrfreq, host_platform::GFREQ_SIZE);
    mem->freq = gddrfreq.cur;

    /* memory_voltage */
    scif_request(host_platform::GVOLT_CMD,
                 &gddrvolt, host_platform::GVOLT_SIZE);
    mem->volt = gddrvolt.cur;

    /* Get dev_mem info using scif calls */
    scif_request(host_platform::ECC_CMD, &eccinfo,
                 host_platform::ECC_SIZE);

    mem->ecc = eccinfo.enable;
}

void knc_device::get_processor_info(struct mic_processor_info *processor)
{
    std::string str_buf;
    std::stringstream ss;
    uint32_t value = 0;

    str_buf = get_device_property(host_platform::PROCESSOR_MODEL);

    ss << std::hex << str_buf;
    ss >> value;
    processor->model = value;
    ss << "";
    ss.clear();

    str_buf = get_device_property(host_platform::PROCESSOR_EXT_MODEL);
    ss << std::hex << str_buf;
    ss >> value;
    processor->model_ext = value;
    ss << "";
    ss.clear();

    str_buf = get_device_property(host_platform::PROCESSOR_TYPE);
    ss << std::hex << str_buf;
    ss >> value;
    processor->type = value;
    ss << "";
    ss.clear();

    str_buf = get_device_property(host_platform::PROCESSOR_FAMILY);
    ss << std::hex << str_buf;
    ss >> value;
    processor->family = value;
    ss << "";
    ss.clear();

    str_buf = get_device_property(host_platform::PROCESSOR_EXT_FAMILY);
    ss << std::hex << str_buf;
    ss >> value;
    processor->family_ext = value;
    ss << "";
    ss.clear();

    str_buf = get_device_property(host_platform::PROCESSOR_STEPPING);
    strncpy(processor->stepping, str_buf.c_str(), NAME_MAX);
    processor->stepping[NAME_MAX] = '\0';

    str_buf = get_device_property(host_platform::PROCESSOR_STEPPING_DATA);
    ss << std::hex << str_buf;
    ss >> value;
    processor->stepping_data = value;
    ss << "";
    ss.clear();

    str_buf = get_device_property(host_platform::PROCESSOR_SUBSTEPPING_DATA);
    ss << std::hex << str_buf;
    ss >> value;
    processor->substepping_data = value;
    ss << "";
    ss.clear();
}

void knc_device::get_cores_info(struct mic_cores_info *cores)
{
    struct mr_rsp_clst coreinfo;
    struct mr_rsp_freq corefreq;
    struct mr_rsp_volt corevolt;

    memset(&coreinfo, 0, sizeof(struct mr_rsp_clst));
    memset(&corefreq, 0, sizeof(struct mr_rsp_freq));
    memset(&corevolt, 0, sizeof(struct mr_rsp_volt));
    /* num_cores */
    scif_request(host_platform::CLST_CMD,
                 &coreinfo, host_platform::CLST_SIZE);
    cores->num_cores = coreinfo.count;

    /* cores_frequency */
    scif_request(host_platform::CFREQ_CMD,
                 &corefreq, host_platform::CFREQ_SIZE);
    cores->frequency = corefreq.cur;

    /* cores_voltage */
    scif_request(host_platform::CVOLT_CMD,
                 &corevolt, host_platform::CVOLT_SIZE);
    cores->voltage = corevolt.cur;
}

void knc_device::get_thermal_info(struct mic_thermal_info *thermal)
{
    struct mr_rsp_smc smc;
    struct mr_rsp_temp *temp = &(thermal->temp);

    memset(&smc, 0, sizeof(struct mr_rsp_smc));
    /* TODO(eamaro): should this struct be zero'ed before calling this? */
    memset(thermal, 0, sizeof(struct mic_thermal_info));

    /* FW Version */
    scif_request(host_platform::SMC_GET_CMD,
                 &smc,
                 host_platform::SMC_SIZE, host_platform::SMC_FW_VER);
    thermal->smc_version.value = smc.rtn.val;

    /* HW Revision */
    scif_request(host_platform::SMC_GET_CMD,
                 &smc,
                 host_platform::SMC_SIZE, host_platform::SMC_HW_REV);
    thermal->smc_revision.value = smc.rtn.val;

    /* Boot loader version (get the SMC boot loader version only if it is
     * supported by the current version of the SMC) */
    if (thermal->smc_version.value >= SMC_BOOT_LOADER_VER_SUPPORT.value) {
        scif_request(host_platform::SMC_GET_CMD,
                     &smc,
                     host_platform::SMC_SIZE,
                     host_platform::SMC_BOOT_LOADER_VER);
        thermal->smc_boot_loader.value = smc.rtn.val;
        thermal->smc_boot_loader_ver_supported = true;
    } else {
        thermal->smc_boot_loader.value = 0;
        thermal->smc_boot_loader_ver_supported = false;
    }

    /* Fan Status */
    if (thermal->smc_revision.bits.hsink_type == 0) {
        thermal->fsc_status = 1;
        /* PWM */
        scif_request(host_platform::SMC_GET_CMD,
                     &smc,
                     host_platform::SMC_SIZE,
                     host_platform::SMC_FAN_PWM);
        thermal->fan_pwm = smc.rtn.val;

        /* RPM */
        scif_request(host_platform::SMC_GET_CMD,
                     &smc,
                     host_platform::SMC_SIZE,
                     host_platform::SMC_FAN_RPM);
        thermal->fan_rpm = smc.rtn.val;
    }

    /* Temperature data */
    memset(temp, 0, sizeof(struct mr_rsp_temp));
    scif_request(host_platform::TEMP_CMD, temp, host_platform::TEMP_SIZE);
}

void knc_device::get_silicon_sku(char *sku, size_t *size)
{
    std::string ssku = get_device_property(host_platform::SI_SKU);

    if (sku != NULL && (*size > 0)) {
        strncpy(sku, ssku.c_str(), *size);
        sku[*size - 1] = '\0';
    }

    if (*size < ssku.length() + 1)
        *size = ssku.length() + 1;
}

void knc_device::get_version_info(struct mic_version_info *ver)
{
    struct mr_rsp_vers *version = &(ver->vers);

    memset(version, 0, sizeof(struct mr_rsp_vers));
    scif_request(host_platform::VER_CMD,
                 version, host_platform::VER_SIZE);
}

void knc_device::get_serial_number(char *serial, size_t *size)
{
    struct mr_rsp_hwinf hwinfo;
    /* the MR_SEN0_LEN does not take into account null terminator */
    size_t length = MR_SENO_LEN + 1;
     
    /* if we get a size greater than MR_SENQ_LEN, we need to set
     * size = MR_SENQ_LEN + 1, to include null terminator */
    if (*size > length)
        *size = length;

    memset(&hwinfo, 0, sizeof(struct mr_rsp_hwinf));
    scif_request(host_platform::SERNO_CMD, &hwinfo, host_platform::SERNO_SIZE);

    if ((serial != NULL) && (*size > 0)) {
        /* copy (size-1) bytes, so we can set the size-th byte to \0 */
        memcpy(serial, (void *) hwinfo.serial, *size - 1);
        serial[*size - 1] = '\0';
    }

    /* if requested size is less than what the complete string needs,
     * update value */
    if (*size < length)
        *size = length;
}

void knc_device::get_post_code(std::string &postcode)
{
    postcode = get_device_property(host_platform::POST_CODE);
}

std::string knc_device::get_state()
{
    return get_device_property(host_platform::STATE);
}

std::string knc_device::get_mode()
{
    return get_device_property(host_platform::MODE);
}

void knc_device::set_mode(const int &new_mode)
{
    host_platform::set_device_mode(new_mode);
}

void knc_device::set_mode(const int &mode, const std::string &elf_path)
{
    host_platform::set_device_mode(mode,elf_path);
}

void knc_device::get_pci_config(struct mic_pci_config &config)
{
    host_platform::get_pci_config(config);
}

void knc_device::in_maint_mode(int *maint)
{
    *maint = get_state() == host_platform::STATE_ONLINE &&
             get_mode() == host_platform::MODE_FLASH;
}

bool knc_device::in_custom_boot()
{
    return get_state() == host_platform::STATE_ONLINE &&
             get_mode() == host_platform::MODE_ELF;
}

void knc_device::in_ready_state(int *ready)
{
    *ready = get_state() == host_platform::STATE_READY;
}

void knc_device::get_power_utilization_info(struct mic_power_util_info *power)
{
    struct mr_rsp_power *pwr = &(power->pwr);

    memset(pwr, 0, sizeof(struct mr_rsp_power));
    scif_request(host_platform::PWRUT_CMD,
                 pwr, host_platform::PWRUT_SIZE);
}

void knc_device::get_power_limit(struct mic_power_limit *limit)
{
        struct mr_rsp_plim plim;
        struct mr_rsp_ptrig ptrig;

        memset(&plim, 0, sizeof(struct mr_rsp_plim));
        memset(&ptrig, 0, sizeof(struct mr_rsp_ptrig));

        scif_request(host_platform::PWRLIM_CMD,
                     &plim, host_platform::PWRLIM_SIZE);
        limit->phys = plim.phys;
        limit->hmrk = plim.hmrk;
        limit->lmrk = plim.lmrk;

        scif_request(host_platform::PWRLIM0_GET_CMD,
                     &ptrig, host_platform::PTRIG_SIZE);
        limit->time_win0 = ptrig.time;

        memset(&ptrig, 0, sizeof(struct mr_rsp_ptrig));
        scif_request(host_platform::PWRLIM1_GET_CMD,
                     &ptrig, host_platform::PTRIG_SIZE);
        limit->time_win1 = ptrig.time;
}

void knc_device::set_power_limit0(uint32_t power, uint32_t time_window)
{
        struct mr_rsp_ptrig ptrig;
        uint8_t dummy_buf = 0;
        uint32_t *param = (uint32_t *)(&ptrig);

        ptrig.power = power;
        ptrig.time = time_window;

        scif_request(host_platform::PWRLIM0_SET_CMD, &dummy_buf, 0, *param);
}

void knc_device::set_power_limit1(uint32_t power, uint32_t time_window)
{
        struct mr_rsp_ptrig ptrig;
        uint8_t dummy_buf = 0;
        uint32_t *param = (uint32_t *)(&ptrig);

        ptrig.power = power;
        ptrig.time = time_window;

        scif_request(host_platform::PWRLIM1_SET_CMD, &dummy_buf, 0, *param);
}

void knc_device::get_memory_utilization_info(
    struct mic_memory_util_info *memory)
{
    struct mr_rsp_mem *mem = &(memory->mem);

    memset(mem, 0, sizeof(struct mr_rsp_mem));
    scif_request(host_platform::MEMUT_CMD,
                 mem, host_platform::MEMUT_SIZE);
}

void knc_device::get_core_util(mic_core_util *cutil)
{
    MrRspCutl *c_util = &(cutil->c_util);

    memset(c_util, 0, sizeof(MrRspCutl));
    scif_request(host_platform::CUTIL_REQUEST, c_util,
                 sizeof(MrRspCutl));
}

void knc_device::get_led_alert(uint32_t *led_alert)
{
    struct mr_rsp_led led;

    memset(&led, 0, sizeof(struct mr_rsp_led));

    scif_request(host_platform::LED_READ,
                 &led, host_platform::LED_SIZE,
                 host_platform::SMC_LED_ALERT);
    *led_alert = led.led;
}

void knc_device::set_led_alert(uint32_t *led_alert)
{
    uint32_t size = 0;
    struct mr_rsp_led led;

    if (*led_alert == 0 || *led_alert == 1)
        scif_request(host_platform::LED_WRITE, &led, size, *led_alert);
    else
        throw mic_exception(E_MIC_INVAL,
                            "invalid args: " + *led_alert, EINVAL);
}

void knc_device::get_turbo_state_info(struct mic_turbo_info *turbo)
{
    struct mr_rsp_trbo *trbo = &(turbo->trbo);

    memset(trbo, 0, sizeof(struct mr_rsp_trbo));
    scif_request(host_platform::TURBO_CMD, trbo,
                 host_platform::TURBO_SIZE);
}

void knc_device::set_turbo_mode(uint32_t *turbo_mode)
{
    uint32_t size = 0;
    struct mr_rsp_trbo turbo;

    if (*turbo_mode == 0 || *turbo_mode == 1) {
        scif_request(host_platform::TURBO_WRITE, &turbo, size,
                     *turbo_mode);
        /* eamaro: sleep call needs to be removed */
        sleep(2);
    } else {
        throw mic_exception(E_MIC_INVAL,
                            "invalid args: " + *turbo_mode, EINVAL);
    }
}

void knc_device::get_uuid(uint8_t *uuid, size_t *size)
{
    struct mr_rsp_smc smc;

    memset(&smc, 0, sizeof(struct mr_rsp_smc));

    scif_request(host_platform::SMC_GET_CMD, &smc, host_platform::SMC_SIZE,
                 host_platform::SMC_UUID);

    if (*size > smc.width)
        *size = smc.width;
    for (uint8_t i = 0; i < *size; ++i)
        uuid[i] = smc.rtn.uuid[i];
    *size = smc.width;
}

void knc_device::get_throttle_state_info(
    struct mic_throttle_state_info *ttl_state)
{
    struct mr_rsp_ttl *ttl = &(ttl_state->ttl_state);

    memset(ttl, 0, sizeof(struct mr_rsp_ttl));
    scif_request(host_platform::TTL_CMD, ttl, host_platform::TTL_SIZE);
}

void knc_device::get_uos_pm_config(struct mic_uos_pm_config *pm_config)
{
    struct mr_rsp_pmcfg pmcfg;

    memset(pm_config, 0, sizeof(struct mr_rsp_pmcfg));
    scif_request(host_platform::PMCFG_CMD, &pmcfg,
                 host_platform::PMCFG_SIZE);
    pm_config->pm_config.mode = pmcfg.mode;
}

#ifdef __linux
void knc_device::read_smc_reg(uint8_t reg, uint8_t *buffer, size_t *size)
{
        struct mr_rsp_smc smc;

        memset(&smc, 0, sizeof(struct mr_rsp_smc));
        scif_request(host_platform::SMC_GET_CMD, &smc, host_platform::SMC_SIZE,
                     reg);

        if (buffer != NULL && *size > 0)
                memcpy((void *)buffer, (void *)(&smc.rtn),
                       *size < smc.width ? *size : smc.width);
        *size = smc.width;
}

void knc_device::write_smc_reg(uint8_t reg, const uint8_t *buffer, size_t size)
{
        struct mr_rsp_smc smc;
        uint32_t param = 0;
        uint32_t val = 0;

        memset(&smc, 0, sizeof(struct mr_rsp_smc));
        if (size > 2)
                size = 2;  // RAS interface supports writing at most 2 bytes.
        memcpy((void *)&val, (void *)buffer, size);
        param = reg;
        param <<= 24;
        param |= val;

        scif_request(host_platform::SMC_SET_CMD, &smc, 0, param);
}
#endif

void knc_device::get_smc_persistence_flag(int *persist_flag)
{
        struct mr_rsp_perst perst;

        memset(&perst, 0, sizeof(struct mr_rsp_perst));
        scif_request(host_platform::PERSIST_FLAG_GET_CMD, &perst,
                     host_platform::PERSIST_FLAG_SIZE);
        if (perst.perst)
                *persist_flag = true;
        else
                *persist_flag = false;
}

void knc_device::set_smc_persistence_flag(int persist_flag)
{
        struct mr_rsp_perst perst;
        uint32_t size = 0;

        memset(&perst, 0, sizeof(struct mr_rsp_perst));
        scif_request(host_platform::PERSIST_FLAG_SET_CMD, &perst, size,
                     persist_flag ? 1 : 0);
}
