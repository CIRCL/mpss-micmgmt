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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "main.h"
#include "mic.h"

extern int posix;
int version_info(struct mic_device *mdh);
int board_info(struct mic_device *mdh);
int cores_info(struct mic_device *mdh);
int thermal_info(struct mic_device *mdh);
int memory_info(struct mic_device *mdh);

int show_list()
{
    struct mic_devices_list *dev_list = NULL;
    struct mic_pci_config *pci_config = NULL;
    struct mic_device *mdh = NULL;
    int ndevices = 1;
    int indx = 0;
    int card = 0;
    uint16_t bus_no, domain, dev_no, dev_id, ven_id;
    uint32_t dev_ven;
    int ret = 0;

    if (mic_get_devices(&dev_list) != E_MIC_SUCCESS) {
        error_msg_start("No devices found : %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }

    if (mic_get_ndevices(dev_list, &ndevices) != E_MIC_SUCCESS) {
        error_msg_start("Failed to get devices : %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        if (dev_list)
            mic_free_devices(dev_list);
        return -1;
    }

    log_msg_start("\nList of Available Devices\n");
#ifdef __linux__
    log_msg_start("\ndeviceId |  domain  | bus# | pciDev# | hardwareId\n");
    log_msg_start("---------|----------|------|---------|-----------\n");
#else // Windows
    log_msg_start("\ndeviceId | bus# | pciDev# | hardwareId\n");
    log_msg_start("---------|------|---------|-----------\n");
#endif

    for (indx = 0; indx < ndevices; indx++) {
        if (mic_get_device_at_index(dev_list, indx, &card) !=
            E_MIC_SUCCESS) {
            error_msg_start
                ("Failed to get card at index %d: %s: %s\n",
                indx,
                mic_get_error_string(), strerror(errno));
            ret = -1;
            continue;
        }

        ret = mic_open_device(&mdh, card);
        if (ret != E_MIC_SCIF_ERROR && ret != E_MIC_SUCCESS) {
            error_msg_start("Failed to open device '%d': %s: %s\n",
                            card, mic_get_error_string(),
                            strerror(errno));
            ret = -1;
            continue;
        }

        ret = mic_get_pci_config(mdh, &pci_config);
        if (ret != E_MIC_SCIF_ERROR && ret != E_MIC_SUCCESS) {
            error_msg_start
                ("Failed to get pci config : %d : %s: %s\n",
                card,
                mic_get_error_string(), strerror(errno));
            mic_close_device(mdh);
            ret = -1;
            continue;
        }
#ifdef __linux__
        mic_get_pci_domain_id(pci_config, &domain);
#endif
        mic_get_bus_number(pci_config, &bus_no);
        mic_get_device_number(pci_config, &dev_no);
        mic_get_device_id(pci_config, &dev_id);
        mic_get_vendor_id(pci_config, &ven_id);
        dev_ven = dev_id;
        dev_ven <<= 16;
        dev_ven |= ven_id;
#ifdef __linux__
        log_msg_start("%8d | %8x | %4x | %7x |   %08X \n",
                      card, domain, bus_no, dev_no, dev_ven);
#else // Windows
        log_msg_start("%8d | %4x | %7x |   %08X \n",
                      card, bus_no, dev_no, dev_ven);
#endif
        mic_close_device(mdh);
    }

    log_msg_start("-------------------------------------------------\n");
    if (dev_list)
        mic_free_devices(dev_list);
    return ret;
}

int show_all(int card, int group_id)
{
    struct mic_devices_list *dev_list = NULL;
    int ndevices = 1;
    int dev_indx = 0;
    int ret = 0;

    if (mic_get_devices(&dev_list) != E_MIC_SUCCESS) {
        error_msg_start("No devices found : %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }

    if (mic_get_ndevices(dev_list, &ndevices) != E_MIC_SUCCESS) {
        error_msg_start("Failed to get devices : %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        if (dev_list)
            mic_free_devices(dev_list);
        return -1;
    }

    for (dev_indx = 0; dev_indx < ndevices; dev_indx++) {
        if (mic_get_device_at_index(dev_list, dev_indx, &card) !=
            E_MIC_SUCCESS) {
            error_msg_start
                ("Failed to get card at index %d: %s: %s\n",
                dev_indx, mic_get_error_string(),
                strerror(errno));
            ret = -1;
            continue;
        }

        if (mic_info(card, group_id) != E_MIC_SUCCESS) {
            ret = -1;
            continue;
        }
    }

    if (dev_list)
        mic_free_devices(dev_list);
    return ret;
}

int mic_info(int card, int group_id)
{
    struct mic_device *mdh = NULL;
    int ret = 0;

    ret = mic_open_device(&mdh, card);
    if (ret != E_MIC_SCIF_ERROR && ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to open device '%d': %s: %s\n",
                        card, mic_get_error_string(), strerror(errno));
        return -1;
    }
    /* print info to the screen */
    log_msg_start("\nDevice No: %d, Device Name: %s\n", card,
                  mic_get_device_name(mdh));

    switch (group_id) {
    case GRP_ALL:
        if (version_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("version info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
            break;
        }
        if (board_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("board info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
            break;
        }
        if (cores_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("cores info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
            break;
        }
        if (thermal_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("thermal info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
            break;
        }
        if (memory_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("memory info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
        }
        break;
    case GRP_VER:
        if (version_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("version info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
        }
        break;
    case GRP_BOARD:
        if (board_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("board info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
        }
        break;
    case GRP_CORES:
        if (cores_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("cores info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
        }
        break;
    case GRP_THER:
        if (thermal_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("thermal info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
        }
        break;
    case GRP_GDDR:
        if (memory_info(mdh) != E_MIC_SUCCESS) {
            error_msg_start("memory info failed: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            ret = -1;
        }
        break;
    default:
        ret = -1;
    }

    mic_close_device(mdh);
    return ret;
}

int version_info(struct mic_device *mdh)
{
    char str[NAME_MAX];
    char serial[NAME_MAX];
    size_t size = NAME_MAX;
    int no_driver = 0;
    int ret = 0;
    struct mic_version_info *version = NULL;
    struct mic_thermal_info *thermal = NULL;
    int bl_ver_supported;

    ret = mic_get_version_info(mdh, &version);
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to get version info: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }

    ret = mic_get_thermal_info(mdh, &thermal);
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to get thermal info: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }

    if (!no_driver) {
        log_msg_start("\n\tVersion\n");
        mic_get_flash_version(version, str, &size);
        log_msg_start("\t\tFlash Version \t\t : %s\n", str);
        mic_get_smc_fwversion(thermal, str, &size);
        log_msg_start("\t\tSMC Firmware Version\t : %s\n", str);

        mic_is_smc_boot_loader_ver_supported(thermal, &bl_ver_supported);
        if (bl_ver_supported) {
            mic_get_smc_boot_loader_ver(thermal, str, &size);
            log_msg_start("\t\tSMC Boot Loader Version\t : %s\n",
                          str);
        } else {
            log_msg_start(
                "\t\tSMC Boot Loader Version\t : NotAvailable\n");
        }

        mic_get_uos_version(version, str, &size);
        log_msg_start("\t\tCoprocessor OS Version \t : %s\n", str);
        mic_get_serial_number(mdh, serial, &size);
        if (!strncasecmp(serial, "update", strlen("update"))) {
            log_msg_start
                ("\t\tDevice Serial Number \t : NotAvailable\n");
        } else {
            log_msg_start("\t\tDevice Serial Number \t : %s\n",
                          serial);
        }
        if (thermal)
            mic_free_thermal_info(thermal);
        if (version)
            mic_free_version_info(version);
    } else if (!posix) {
        log_msg_start("\n\tVersion\n");
        log_msg_start("\t\tFlash Version \t\t : NotAvailable\n");
        log_msg_start("\t\tSMC Firmware Version\t : NotAvailable\n");
        log_msg_start("\t\tSMC Boot Loader Version\t : NotAvailable\n");
        log_msg_start("\t\tCoprocessor OS Version \t : NotAvailable\n");
        log_msg_start("\t\tDevice Serial Number \t : NotAvailable\n");
    }
    return 0;
}

int board_info(struct mic_device *mdh)
{
    char str[NAME_MAX];
    size_t size = NAME_MAX;
    uint32_t value32;
    uint16_t value16;
    uint16_t value16_ext;
    int no_driver = 0;
    int ret = 0;
    struct mic_pci_config *pci = NULL;
    struct mic_device_mem *memory = NULL;
    struct mic_processor_info *processor = NULL;
    struct mic_thermal_info *thermal = NULL;

    ret = mic_get_pci_config(mdh, &pci);
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to get pci config: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }
    ret = mic_get_processor_info(mdh, &processor);
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to get processor info: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }
    ret = mic_get_thermal_info(mdh, &thermal);
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to get thermal info: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }
    ret = mic_get_memory_info(mdh, &memory);
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to get memory info: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }
    log_msg_start("\n\tBoard\n");
    mic_get_vendor_id(pci, &value16);
    log_msg_start("\t\tVendor ID \t\t : 0x%x\n", value16);
    mic_get_device_id(pci, &value16);
    log_msg_start("\t\tDevice ID \t\t : 0x%x\n", value16);
    mic_get_subsystem_id(pci, &value16);
    log_msg_start("\t\tSubsystem ID \t\t : 0x%x\n", value16);

    mic_get_processor_steppingid(processor, &value32);
    log_msg_start("\t\tCoprocessor Stepping ID\t : %x\n", (value32 >> 4));
    if (mic_get_link_width(pci, &value32) == E_MIC_SUCCESS) {
        log_msg_start("\t\tPCIe Width \t\t : x%d\n", value32);
    } else {
        log_msg_start
            ("\t\tPCIe Width \t\t : Insufficient Privileges\n");
    }
    if (mic_get_link_speed(pci, str, &size) != E_MIC_ACCESS) {
        log_msg_start("\t\tPCIe Speed \t\t : %s\n", str);
    } else {
        log_msg_start
            ("\t\tPCIe Speed \t\t : Insufficient Privileges\n");
    }
    if (mic_get_max_payload(pci, &value32) == E_MIC_SUCCESS) {
        log_msg_start("\t\tPCIe Max payload size\t : %d bytes\n",
                      value32);
    } else {
        log_msg_start
        (
            "\t\tPCIe Max payload size\t : Insufficient Privileges\n");
    }
    if (mic_get_max_readreq(pci, &value32) == E_MIC_SUCCESS) {
        log_msg_start("\t\tPCIe Max read req size\t : %d bytes\n",
                      value32);
    } else {
        log_msg_start
        (
            "\t\tPCIe Max read req size\t : Insufficient Privileges\n");
    }
    mic_get_processor_model(processor, &value16, &value16_ext);
    log_msg_start("\t\tCoprocessor Model\t : 0x0%x\n", value16);
    log_msg_start("\t\tCoprocessor Model Ext\t : 0x0%x\n", value16_ext);
    mic_get_processor_type(processor, &value16);
    log_msg_start("\t\tCoprocessor Type\t : 0x0%x\n", value16);
    mic_get_processor_family(processor, &value16, &value16_ext);
    log_msg_start("\t\tCoprocessor Family\t : 0x0%x\n", value16);
    log_msg_start("\t\tCoprocessor Family Ext\t : 0x0%x\n", value16_ext);
    mic_get_processor_stepping(processor, str, &size);
    log_msg_start("\t\tCoprocessor Stepping \t : %s\n", str);
    mic_get_silicon_sku(mdh, str, &size);
    log_msg_start("\t\tBoard SKU \t\t : %s\n", str);
    if (!no_driver) {
        mic_get_ecc_mode(memory, &value16);
        log_msg_start("\t\tECC Mode \t\t : %s\n",
                      ((value16) ? "Enabled" : "Disabled"));
        mic_get_smc_hwrevision(thermal, str, &size);
        log_msg_start("\t\tSMC HW Revision \t : %s\n", str);
        if (thermal)
            mic_free_thermal_info(thermal);
        if (memory)
            mic_free_memory_info(memory);
    } else if (!posix) {
        log_msg_start("\t\tECC Mode \t\t : NotAvailable\n");
        log_msg_start("\t\tSMC HW Revision \t : NotAvailable\n");
    }
    if (pci)
        mic_free_pci_config(pci);
    if (processor)
        mic_free_processor_info(processor);
    return 0;
}

int cores_info(struct mic_device *mdh)
{
    uint32_t value32;
    int no_driver = 0;
    int ret = 0;
    struct mic_cores_info *cores = NULL;

    ret = mic_get_cores_info(mdh, &cores);
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to get cores info: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }
    if (!no_driver) {
        log_msg_start("\n\tCores\n");
        mic_get_cores_count(cores, &value32);
        log_msg_start("\t\tTotal No of Active Cores : %d\n", value32);
        mic_get_cores_voltage(cores, &value32);
        log_msg_start("\t\tVoltage \t\t : %d uV\n", value32);
        mic_get_cores_frequency(cores, &value32);
        log_msg_start("\t\tFrequency\t\t : %d kHz\n", value32);
        if (cores)
            mic_free_cores_info(cores);
    } else if (!posix) {
        log_msg_start("\n\tCores\n");
        log_msg_start("\t\tTotal No of Active Cores : NotAvailable\n");
        log_msg_start("\t\tVoltage \t\t : NotAvailable\n");
        log_msg_start("\t\tFrequency \t\t : NotAvailable\n");
    }
    return 0;
}

int thermal_info(struct mic_device *mdh)
{
    uint32_t value32;
    uint32_t fsc_status = 0;
    int no_driver = 0;
    int ret = 0;
    struct mic_thermal_info *thermal = NULL;

    ret = mic_get_thermal_info(mdh, &thermal);
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to get thermal info: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        mic_free_thermal_info(thermal);
        error_msg_start("Failed to get version info: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }
    if (!no_driver) {
        log_msg_start("\n\tThermal\n");
        mic_get_fsc_status(thermal, &fsc_status);
        if (fsc_status == 1) {
            mic_get_fsc_status(thermal, &value32);
            log_msg_start("\t\tFan Speed Control \t : %s\n",
                          ((value32) ? "On" : "Off"));
        } else {
            log_msg_start("\t\tFan Speed Control \t : N/A\n");
        }
        if (fsc_status == 1) {
            mic_get_fan_rpm(thermal, &value32);
            log_msg_start("\t\tFan RPM \t\t : %d\n", value32);
            mic_get_fan_pwm(thermal, &value32);
            log_msg_start("\t\tFan PWM \t\t : %d\n", value32);
        } else {
            log_msg_start("\t\tFan RPM \t\t : N/A\n");
            log_msg_start("\t\tFan PWM \t\t : N/A\n");
        }
        mic_get_die_temp(thermal, &value32);
        log_msg_start("\t\tDie Temp\t\t : %d C\n", value32);
        if (thermal)
            mic_free_thermal_info(thermal);
    } else if (!posix) {
        log_msg_start("\n\tThermal\n");
        log_msg_start("\t\tFan Speed Control \t : NotAvailable\n");
        log_msg_start("\t\tFan RPM \t\t : NotAvailable\n");
        log_msg_start("\t\tFan PWM \t\t : NotAvailable\n");
        log_msg_start("\t\tDie Temp\t\t : NotAvailable\n");
    }
    return 0;
}

int memory_info(struct mic_device *mdh)
{
    char str[NAME_MAX];
    size_t size = NAME_MAX;
    uint32_t value32;
    struct mic_device_mem *memory = NULL;
    int no_driver = 0;
    int ret = 0;

    ret = mic_get_memory_info(mdh, &memory);
    if (ret == E_MIC_SCIF_ERROR) {
        no_driver = 1;
    } else if (ret != E_MIC_SUCCESS) {
        error_msg_start("Failed to get memory info: %s: %s\n",
                        mic_get_error_string(), strerror(errno));
        return -1;
    }
    if (!no_driver) {
        log_msg_start("\n\tGDDR\n");
        mic_get_memory_vendor(memory, str, &size);
        log_msg_start("\t\tGDDR Vendor\t\t : %s\n", str);
        mic_get_memory_revision(memory, &value32);
        log_msg_start("\t\tGDDR Version\t\t : 0x%x\n", value32);
        mic_get_memory_density(memory, &value32);
        log_msg_start("\t\tGDDR Density\t\t : %d Mb\n", value32);
        mic_get_memory_size(memory, &value32);
        log_msg_start("\t\tGDDR Size\t\t : %d MB\n", value32 >> 10);
        mic_get_memory_type(memory, str, &size);
        log_msg_start("\t\tGDDR Technology\t\t : %s \n", str);
        mic_get_memory_speed(memory, &value32);
        log_msg_start("\t\tGDDR Speed\t\t : %f GT/s \n",
                      (float)value32 / 1000000);
        mic_get_memory_frequency(memory, &value32);
        log_msg_start("\t\tGDDR Frequency\t\t : %d kHz\n", value32);
        mic_get_memory_voltage(memory, &value32);
        log_msg_start("\t\tGDDR Voltage\t\t : %d uV\n", value32);
        if (memory)
            mic_free_memory_info(memory);
    } else if (!posix) {
        log_msg_start("\n\tGDDR\n");
        log_msg_start("\t\tGDDR Vendor\t\t : NotAvailable\n");
        log_msg_start("\t\tGDDR Version\t\t : NotAvailable\n");
        log_msg_start("\t\tGDDR Density\t\t : NotAvailable\n");
        log_msg_start("\t\tGDDR Size\t\t : NotAvailable\n");
        log_msg_start("\t\tGDDR Technology\t\t : NotAvailable\n");
        log_msg_start("\t\tGDDR Speed\t\t : NotAvailable\n");
        log_msg_start("\t\tGDDR Frequency\t\t : NotAvailable\n");
        log_msg_start("\t\tGDDR Voltage\t\t : NotAvailable\n");
    }
    return 0;
}
