/*
 * Copyright 2012 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

// Required for Windows and Visual Studio 2012 and newer
#ifndef __linux__
#define _CRT_SECURE_NO_WARNINGS    1
#endif

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <miclib.h>
#ifdef __linux__
#include <unistd.h>
#else
#define usleep(arg)    Sleep(arg / 1000)
#define NAME_MAX       1000
#endif

#define MAX_DEVICES    (32)
#define MAX_CORES      (256)
#define NUM_SAMPLES    (3)


int do_pci_config_examples(struct mic_device *);

void print_error(const char *msg, const char *device_name)
{
    const char *mic_err_str = mic_get_error_string();

    fprintf(stderr, "Error");
    if (device_name != NULL)
        fprintf(stderr, ": %s", device_name);
    fprintf(stderr, ": %s", msg);
    if (strcmp("No error registered", mic_err_str) != 0)
        fprintf(stderr, ": %s", mic_err_str);
    if (errno == 0)
        fprintf(stderr, "\n");
    else
        fprintf(stderr, ": %s\n", strerror(errno));
}

int do_pci_config_examples(struct mic_device *mdh)
{
    struct mic_pci_config *pcfg;
    uint16_t vendor_id, device_id, subsys_id;
    uint8_t revision_id;
    uint32_t lwidth, payload, readreq;
    char speed[NAME_MAX];
    size_t size;

    /* PCI config examples */
    printf("PCI Config Examples:\n\n");
    if (mic_get_pci_config(mdh, &pcfg) != E_MIC_SUCCESS) {
        print_error("Failed to get PCI configuration information",
                    mic_get_device_name(mdh));
        return 1;
    }

    if (mic_get_vendor_id(pcfg, &vendor_id) != E_MIC_SUCCESS) {
        print_error("Failed to get vendor id", mic_get_device_name(mdh));
        (void)mic_free_pci_config(pcfg);
        return 2;
    }
    printf("    Vendor id: 0x%04x\n", vendor_id);

    if (mic_get_device_id(pcfg, &device_id) != E_MIC_SUCCESS) {
        print_error("Failed to get device id", mic_get_device_name(mdh));
        (void)mic_free_pci_config(pcfg);
        return 3;
    }
    printf("    Device id: 0x%04x\n", device_id);

    if (mic_get_revision_id(pcfg, &revision_id) != E_MIC_SUCCESS) {
        print_error("Failed to get revision id", mic_get_device_name(mdh));
        (void)mic_free_pci_config(pcfg);
        return 4;
    }
    printf("    Revision id: 0x%02x\n", revision_id);

    if (mic_get_subsystem_id(pcfg, &subsys_id) != E_MIC_SUCCESS) {
        print_error("Failed to get subsystem id", mic_get_device_name(mdh));
        (void)mic_free_pci_config(pcfg);
        return 5;
    }
    printf("    Subsystem id: 0x%04x\n", subsys_id);

    /* Has to be su to run this, else there is access violation error */
    if (mic_get_link_width(pcfg, &lwidth) != E_MIC_SUCCESS) {
        print_error("Failed to get link width: permission denied",
                    mic_get_device_name(mdh));
        (void)mic_free_pci_config(pcfg);
        return 7;
    }
    printf("    PCIe width: x%u\n", lwidth);

    if (mic_get_max_payload(pcfg, &payload) != E_MIC_SUCCESS) {
        print_error("Failed to get max payload: permission denied",
                    mic_get_device_name(mdh));
        (void)mic_free_pci_config(pcfg);
        return 8;
    }
    printf("    PCIe Max payload size: %u Bytes\n", payload);

    if (mic_get_max_readreq(pcfg, &readreq) != E_MIC_SUCCESS) {
        print_error("Failed to get max readreq: permission denied",
                    mic_get_device_name(mdh));
        (void)mic_free_pci_config(pcfg);
        return 9;
    }
    printf("    Max read req size: %u Bytes\n", readreq);

    size = sizeof(speed);
    if (mic_get_link_speed(pcfg, speed, &size) != E_MIC_SUCCESS) {
        print_error("Failed to get link speed: permission denied",
                    mic_get_device_name(mdh));
        (void)mic_free_pci_config(pcfg);
        return 10;
    }
    printf("    Link speed: %s\n\n", speed);

    (void)mic_free_pci_config(pcfg);
    return 0;
}


int do_memory_examples(struct mic_device *mdh)
{
    struct mic_device_mem *minfo;
    char str[NAME_MAX];
    size_t size;
    uint32_t revision, density, msize, freq, volt;
    uint16_t ecc;

    /* Memory device examples */
    printf("Memory API Examples:\n\n");
    if (mic_get_memory_info(mdh, &minfo) != E_MIC_SUCCESS) {
        print_error("Failed to get memory information",
                    mic_get_device_name(mdh));
        return 1;
    }
    size = sizeof(str);
    if (mic_get_memory_vendor(minfo, str, &size) != E_MIC_SUCCESS) {
        print_error("Failed to get memory vendor information",
                    mic_get_device_name(mdh));
        (void)mic_free_memory_info(minfo);
        return 2;
    }
    printf("    Memory vendor: %s\n", str);

    if (mic_get_memory_revision(minfo, &revision) != E_MIC_SUCCESS) {
        print_error("Failed to get memory revision", mic_get_device_name(mdh));
        (void)mic_free_memory_info(minfo);
        return 3;
    }
    printf("    Memory revision: %u\n", revision);

    if (mic_get_memory_density(minfo, &density) != E_MIC_SUCCESS) {
        print_error("Failed to get memory density", mic_get_device_name(mdh));
        (void)mic_free_memory_info(minfo);
        return 4;
    }
    printf("    Memory density: %u Kbits/device\n", density);

    if (mic_get_memory_size(minfo, &msize) != E_MIC_SUCCESS) {
        print_error("Failed to get memory size", mic_get_device_name(mdh));
        (void)mic_free_memory_info(minfo);
        return 5;
    }
    printf("    Memory size: %u KBytes\n", msize);

    size = sizeof(str);
    if (mic_get_memory_type(minfo, str, &size) != E_MIC_SUCCESS) {
        print_error("Failed to get memory type", mic_get_device_name(mdh));
        (void)mic_free_memory_info(minfo);
        return 6;
    }
    printf("    Memory type: %s\n", str);

    if (mic_get_memory_frequency(minfo, &freq) != E_MIC_SUCCESS) {
        print_error("Failed to get memory frequency", mic_get_device_name(mdh));
        (void)mic_free_memory_info(minfo);
        return 7;
    }
    printf("    Memory frequency: %u KHz\n", freq); //Add unit. measured in what?

    if (mic_get_memory_voltage(minfo, &volt) != E_MIC_SUCCESS) {
        print_error("Failed to get memory voltage", mic_get_device_name(mdh));
        (void)mic_free_memory_info(minfo);
        return 8;
    }
    printf("    Memory voltage: %u volt\n", volt / 1000000);

    if (mic_get_ecc_mode(minfo, &ecc) != E_MIC_SUCCESS) {
        print_error("Failed to get ecc mode", mic_get_device_name(mdh));
        (void)mic_free_memory_info(minfo);
        return 9;
    }
    printf("    ECC mode: %d\n\n", ecc);     //Unit?

    (void)mic_free_memory_info(minfo);
    return 0;
}

int do_core_examples(struct mic_device *mdh)
{
    struct mic_cores_info *cinfo;
    uint32_t ncores, freq, volt;

    /* Core Info examples */
    printf("Core Info Examples:\n\n");
    if (mic_get_cores_info(mdh, &cinfo) != E_MIC_SUCCESS) {
        print_error("Failed to get core information", mic_get_device_name(mdh));
        return 1;
    }

    if (mic_get_cores_count(cinfo, &ncores) != E_MIC_SUCCESS) {
        print_error("Failed to get number of cores", mic_get_device_name(mdh));
        (void)mic_free_cores_info(cinfo);
        return 2;
    }
    printf("    Number of cores: %u\n", ncores);

    if (mic_get_cores_voltage(cinfo, &volt) != E_MIC_SUCCESS) {
        print_error("Failed to get core voltage", mic_get_device_name(mdh));
        (void)mic_free_cores_info(cinfo);
        return 3;
    }
    printf("    Core voltage: %u volt\n", volt / 1000000);

    if (mic_get_cores_frequency(cinfo, &freq) != E_MIC_SUCCESS) {
        print_error("Failed to get core frequency", mic_get_device_name(mdh));
        (void)mic_free_cores_info(cinfo);
        return 4;
    }
    printf("    Core frequency: %u\n\n", freq);     //Add Unit.

    (void)mic_free_cores_info(cinfo);
    return 0;
}

int do_power_examples(struct mic_device *mdh)
{
    struct mic_power_util_info *pinfo;
    uint32_t pwr, sts;

    /* Power utilization examples */
    printf("Power utilization Examples:\n\n");
    if (mic_get_power_utilization_info(mdh, &pinfo) != E_MIC_SUCCESS) {
        print_error("Failed to get power utilization information",
                    mic_get_device_name(mdh));
        return 1;
    }

    if (mic_get_total_power_readings_w0(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get w0 total power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 2;
    }
    printf("    Total power reading w0: %u watts\n", pwr / 1000000);

    if (mic_get_total_power_sensor_sts_w0(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get w0 power sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 3;
    }
    printf("    w0 power sensor status: %u\n", sts);

    if (mic_get_total_power_readings_w1(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get w1 total power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 4;
    }
    printf("    Total power reading w1: %u watts\n", pwr / 1000000);

    if (mic_get_total_power_sensor_sts_w1(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get w1 power sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 5;
    }
    printf("    w1 power sensor status: %u\n", sts);

    if (mic_get_inst_power_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get instant power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 6;
    }
    printf("    Instant power reading: %u watts\n", pwr / 1000000);
    if (mic_get_inst_power_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get instant power sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 6;
    }
    printf("    Instant power sensor status: %u\n", sts);

    if (mic_get_max_inst_power_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get max instant power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 7;
    }
    printf("    Max power reading: %u watts\n", pwr / 1000000);

    if (mic_get_max_inst_power_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get max instant power sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 8;
    }
    printf("    Max power sensor status: %u\n", sts);

    if (mic_get_pcie_power_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get PCIe power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 9;
    }
    printf("    pcie power reading: %u watts\n", pwr / 1000000);

    if (mic_get_pcie_power_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get PCIe power sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 10;
    }
    printf("    pcie power sensor status: %u\n", sts);

    if (mic_get_c2x3_power_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get c2x3 power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 11;
    }
    printf("    2X3 connector power reading: %u watts\n", pwr / 1000000);

    if (mic_get_c2x3_power_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get c2x3 power sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 12;
    }
    printf("    2X3 connector power sensor status: %u\n", sts);

    if (mic_get_c2x4_power_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get c2x4 power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 13;
    }
    printf("    2x4 connector power reading: %u watts\n", pwr / 1000000);

    if (mic_get_c2x4_power_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get c2x4 power sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 14;
    }
    printf("    2x4 connector power sensor status: %u\n", sts);


    if (mic_get_vccp_power_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get vccp(core rail) power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 15;
    }
    printf("    Vccp(core rail) power reading: %u watts\n", pwr / 1000000);

    if (mic_get_vccp_power_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get vccp(core rail) power sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 16;
    }
    printf("    Vccp(core rail) power sensor status: %u \n", sts);

    if (mic_get_vccp_current_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get vccp(core rail) current readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 17;
    }
    printf("    Vccp(core rail) current reading: %u mA\n", pwr);

    if (mic_get_vccp_current_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get vccp(core rail) current sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 18;
    }
    printf("    Vccp(core rail) current sensor status: %u \n", sts);

    if (mic_get_vccp_voltage_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get vccp(core rail) voltage readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 19;
    }
    printf("    Vccp(core rail) voltage reading: %u volts\n", pwr / 1000000);

    if (mic_get_vccp_voltage_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get vccp(core rail) voltage sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 20;
    }
    printf("    Vccp(core rail) voltage sensor status: %u\n", sts);

    if (mic_get_vddg_power_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get vddg(uncore rail) power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 21;
    }
    printf("    Vddg(uncore rail) power reading: %u watts\n", pwr / 1000000);

    if (mic_get_vddg_power_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get vddg(uncore rail) power sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 22;
    }
    printf("    Vddg(uncore rail) power sensor status: %u \n", sts);

    if (mic_get_vddg_current_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get vddg(uncore rail) current readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 23;
    }
    printf("    Vddg(uncore rail) current reading: %u mA\n", pwr);

    if (mic_get_vddg_current_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get vddg(uncore rail) current sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 24;
    }
    printf("    Vddg(uncore rail) current sensor status: %u\n", sts);


    if (mic_get_vddg_voltage_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get vddg(uncore rail) voltage readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 25;
    }
    printf("    Vddg(uncore rail) voltage reading: %u mA\n", pwr);

    if (mic_get_vddg_voltage_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error("Failed to get vddg(uncore rail) voltage sensor status",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 26;
    }
    printf("    Vddg(uncore rail) voltage sensor status : %u\n", sts);

    if (mic_get_vddq_power_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get vddq(Memory subsystem rail) power readings",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 27;
    }
    printf("    Vddq(Memory subsystem rail) power reading: %u watts\n",
           pwr / 1000000);

    if (mic_get_vddq_power_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error(
            "Failed to get vddq(Memory subsystem rail) power sensor status",
            mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 28;
    }
    printf("    Vddq(Memory subsystem rail) power sensor status : %u\n", sts);

    if (mic_get_vddq_current_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error("Failed to get vddq(Memory subsystem rail) current reading",
                    mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 29;
    }
    printf("    Vddq(Memory subsystem rail) current reading: %u mA\n", pwr);

    if (mic_get_vddq_current_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error(
            "Failed to get vddq(Memory subsystem rail) current sensor status",
            mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 30;
    }
    printf("    Vddq(Memory subsystem rail) current sensor status: %u \n",
           sts);


    if (mic_get_vddq_voltage_readings(pinfo, &pwr) != E_MIC_SUCCESS) {
        print_error(
            "Failed to get vddg(Memory subsystem rail) voltage readings",
            mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 31;
    }
    printf("    Vddq(Memory subsystem rail) voltage reading: %u mA\n", pwr);

    if (mic_get_vddq_voltage_sensor_sts(pinfo, &sts) != E_MIC_SUCCESS) {
        print_error(
            "Failed to get vddg(Memory subsystem rail) voltage sensor status",
            mic_get_device_name(mdh));
        (void)mic_free_power_utilization_info(pinfo);
        return 32;
    }
    printf("    Vddq(Memory subsystem rail) voltage sensor status: %u\n\n",
           sts);

    (void)mic_free_power_utilization_info(pinfo);
    return 0;
}

int do_power_config_examples(struct mic_device *mdh)
{
    struct mic_power_limit *limit;
    uint32_t phys_lim;
    uint32_t hmrk;
    uint32_t lmrk;
    uint32_t time_window0;
    uint32_t time_window1;

    /* Power Configuration Examples */
    printf("Power Configuration Examples:\n\n");
    if (mic_get_power_limit(mdh, &limit) != E_MIC_SUCCESS) {
        print_error("Failed to get power limit data", mic_get_device_name(mdh));
        (void)mic_free_power_limit(limit);
        return 1;
    }

    if (mic_get_power_phys_limit(limit, &phys_lim) != E_MIC_SUCCESS) {
        print_error("Failed to get physical power limit",
                    mic_get_device_name(mdh));
        (void)mic_free_power_limit(limit);
        return 2;
    }
    printf("    Physical power limit: %u\n", phys_lim);

    if (mic_get_power_hmrk(limit, &hmrk) != E_MIC_SUCCESS) {
        print_error("Failed to get power high mark", mic_get_device_name(mdh));
        (void)mic_free_power_limit(limit);
        return 3;
    }
    printf("    Power limit high mark: %u\n", hmrk);

    if (mic_get_power_lmrk(limit, &lmrk) != E_MIC_SUCCESS) {
        print_error("Failed to get power low mark", mic_get_device_name(mdh));
        (void)mic_free_power_limit(limit);
        return 4;
    }
    printf("    Power limit low mark: %u\n", lmrk);

    if (mic_get_time_window0(limit, &time_window0)) {
        print_error("Failed to get time window value 0",
                    mic_get_device_name(mdh));
        (void)mic_free_power_limit(limit);
        return 5;
    }
    printf("    Time window value 0: %u\n", time_window0);

    if (mic_get_time_window1(limit, &time_window1)) {
        print_error("Failed to get time window value 1",
                    mic_get_device_name(mdh));
        (void)mic_free_power_limit(limit);
        return 6;
    }
    printf("    Time window value 1: %u\n", time_window1);

    if (mic_set_power_limit0(mdh, hmrk, time_window0) != E_MIC_SUCCESS) {
        print_error("Failed to set power limit 0", mic_get_device_name(mdh));
        (void)mic_free_power_limit(limit);
        return 7;
    }
    printf("    Power limit 0 set: %u %u\n", hmrk, time_window0);

    if (mic_set_power_limit1(mdh, lmrk, time_window1) != E_MIC_SUCCESS) {
        print_error("Failed to set power limit 1", mic_get_device_name(mdh));
        (void)mic_free_power_limit(limit);
        return 8;
    }
    printf("    Power limit 1 set: %u %u\n", lmrk, time_window1);

    (void)mic_free_power_limit(limit);
    return 0;
}

int do_processor_examples(struct mic_device *mdh)
{
    struct mic_processor_info *pinfo;
    uint32_t id;
    size_t size;
    char stepping[NAME_MAX];
    uint16_t model, model_ext, type;

    /* Processor Info Examples */
    printf("Processor Info Examples:\n\n");
    if (mic_get_processor_info(mdh, &pinfo) != E_MIC_SUCCESS) {
        print_error("Failed to get processor information",
                    mic_get_device_name(mdh));
        return 1;
    }

    if (mic_get_processor_model(pinfo, &model,
                                &model_ext) != E_MIC_SUCCESS) {
        print_error(
            "Failed to get processor model and processor extended model",
            mic_get_device_name(mdh));
        (void)mic_free_processor_info(pinfo);
        return 2;
    }
    printf("    Processor model: %u\n", model);
    printf("    Processor model Extension: %u\n", model_ext);

    if (mic_get_processor_type(pinfo, &type) != E_MIC_SUCCESS) {
        print_error("Failed to get processor type", mic_get_device_name(mdh));
        (void)mic_free_processor_info(pinfo);
        return 3;
    }
    printf("    Processor type: %u\n", type);

    if (mic_get_processor_steppingid(pinfo, &id) != E_MIC_SUCCESS) {
        print_error("Failed to get processor stepping id",
                    mic_get_device_name(mdh));
        (void)mic_free_processor_info(pinfo);
        return 4;
    }
    printf("    Processor stepping id: %u\n", id);

    size = sizeof(stepping);
    if (mic_get_processor_stepping(pinfo, stepping,
                                   &size) != E_MIC_SUCCESS) {
        print_error("Failed to get processor stepping",
                    mic_get_device_name(mdh));
        (void)mic_free_processor_info(pinfo);
        return 5;
    }
    printf("    Processor stepping: %s\n\n", stepping);

    (void)mic_free_processor_info(pinfo);
    return 0;
}

int do_thermal_examples(struct mic_device *mdh)
{
    struct mic_thermal_info *tinfo;
    uint32_t fsc, rpm, pwm, dtemp;
    char fwversion[NAME_MAX], hwrevision[NAME_MAX];
    size_t size;

    /* Thermal Info examples */
    printf("Thermal Info Examples:\n\n");
    if (mic_get_thermal_info(mdh, &tinfo) != E_MIC_SUCCESS) {
        print_error("Failed to get thermal information",
                    mic_get_device_name(mdh));
        return 1;
    }

    if (mic_get_fsc_status(tinfo, &fsc) != E_MIC_SUCCESS) {
        print_error("Failed to get fan speed controller status",
                    mic_get_device_name(mdh));
        (void)mic_free_thermal_info(tinfo);
        return 2;
    }
    printf("    Fan speed controller status: %u\n", fsc);

    if (mic_get_fan_rpm(tinfo, &rpm) != E_MIC_SUCCESS) {
        print_error("Failed to get fan rpm", mic_get_device_name(mdh));
        (void)mic_free_thermal_info(tinfo);
        return 3;
    }
    printf("    Fan Speed: %u revolutions per minute\n", rpm);

    if (mic_get_fan_pwm(tinfo, &pwm) != E_MIC_SUCCESS) {
        print_error("Failed to get fan pwm", mic_get_device_name(mdh));
        (void)mic_free_thermal_info(tinfo);
        return 4;
    }
    printf("    Fan PWM: %u\n", pwm);

    if (mic_get_die_temp(tinfo, &dtemp) != E_MIC_SUCCESS) {
        print_error("Failed to get die temperature", mic_get_device_name(mdh));
        (void)mic_free_thermal_info(tinfo);
        return 5;
    }
    printf("    Die Temperature: %uC\n", dtemp);

    size = sizeof(fwversion);
    if (mic_get_smc_fwversion(tinfo, fwversion, &size) != E_MIC_SUCCESS) {
        print_error("Failed to get smc Firmware version",
                    mic_get_device_name(mdh));
        (void)mic_free_thermal_info(tinfo);
        return 6;
    }
    printf("    SMC Firmware Version: %s\n", fwversion);

    size = sizeof(hwrevision);
    if (mic_get_smc_hwrevision(tinfo, hwrevision,
                               &size) != E_MIC_SUCCESS) {
        print_error("Failed to get smc Hardware version",
                    mic_get_device_name(mdh));
        (void)mic_free_thermal_info(tinfo);
        return 7;
    }
    printf("    SMC Hardware Revision: %s\n\n", hwrevision);

    (void)mic_free_thermal_info(tinfo);
    return 0;
}

int do_core_util_examples(struct mic_device *mdh)
{
    struct mic_core_util *cutil = NULL;
    uint64_t idle_sum, nice_sum, sys_sum, user_sum, jiffy_counter;
    uint32_t tick_count;
    uint16_t num_cores, thread_core;
    uint64_t *nice_counters, *idle_counters, *sys_counters, *user_counters;
    uint16_t core_idx;

    /* Core Util examples */
    printf("Core Utilization Examples:\n\n");

    if (mic_alloc_core_util(&cutil) != E_MIC_SUCCESS) {
        print_error("Failed to allocate Core utilization information",
                    mic_get_device_name(mdh));
        return 1;
    }

    if (mic_update_core_util(mdh, cutil) != E_MIC_SUCCESS) {
        print_error("Failed to update Core utilization information",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 1;
    }

    if (mic_get_num_cores(cutil, &num_cores) != E_MIC_SUCCESS) {
        print_error("Failed to get the number of cores",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 12;
    }

    printf("    Number of cores: %u\n", num_cores);

    if (mic_get_threads_core(cutil, &thread_core) != E_MIC_SUCCESS) {
        print_error("Failed to get the Number of threads per core",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 13;
    }

    printf("    Number of threads per core: %u\n", thread_core);

    if (mic_get_jiffy_counter(cutil, &jiffy_counter) != E_MIC_SUCCESS) {
        print_error("Failed to get jiffy count", mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 11;
    }

    printf("    Jiffy count at Query time: %d\n", (int)jiffy_counter);

    if (mic_get_tick_count(cutil, &tick_count) != E_MIC_SUCCESS) {
        print_error("Failed to get tick count", mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 14;
    }

    printf("    tick_count at Query time: %d\n", (int)tick_count);

    if ((idle_counters =
             (uint64_t *)malloc(sizeof(uint64_t) * num_cores)) ==
        NULL) {
        print_error("Malloc failed with bad alloc",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 2;
    }

    if ((nice_counters =
             (uint64_t *)malloc(sizeof(uint64_t) * num_cores)) ==
        NULL) {
        print_error("Malloc failed with bad alloc",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        free(idle_counters);
        return 2;
    }

    if ((sys_counters =
             (uint64_t *)malloc(sizeof(uint64_t) * num_cores)) ==
        NULL) {
        print_error("Malloc failed with bad alloc",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        free(idle_counters);
        free(nice_counters);
        return 2;
    }

    if ((user_counters =
             (uint64_t *)malloc(sizeof(uint64_t) * num_cores)) ==
        NULL) {
        print_error("Malloc failed with bad alloc",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        free(idle_counters);
        free(nice_counters);
        free(sys_counters);
        return 2;
    }

    if (mic_get_idle_counters(cutil, idle_counters) != E_MIC_SUCCESS) {
        print_error("Failed to get idle counters",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        free(idle_counters);
        free(nice_counters);
        free(sys_counters);
        free(user_counters);
        return 3;
    }
    if (mic_get_nice_counters(cutil, nice_counters) != E_MIC_SUCCESS) {
        print_error("Failed to get the number of nice counters",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        free(idle_counters);
        free(nice_counters);
        free(sys_counters);
        free(user_counters);
        return 4;
    }
    if (mic_get_sys_counters(cutil, sys_counters) != E_MIC_SUCCESS) {
        print_error("Failed to get system counters",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        free(idle_counters);
        free(nice_counters);
        free(sys_counters);
        free(user_counters);
        return 5;
    }
    if (mic_get_user_counters(cutil, user_counters) != E_MIC_SUCCESS) {
        print_error("Failed to get the number of user counters",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        free(idle_counters);
        free(nice_counters);
        free(sys_counters);
        free(user_counters);
        return 6;
    }
    for (core_idx = 0; core_idx < num_cores; ++core_idx) {
        printf(
            "    idle_counters[%u]: %lu\t nice_counters[%u]: %lu\t"
            "sys_counters[%u]: %lu\t user_counters[%u]: %lu\n",
            core_idx,
            (unsigned long)idle_counters[core_idx], core_idx,
            (unsigned long)nice_counters[core_idx], core_idx,
            (unsigned long)sys_counters[core_idx], core_idx,
            (unsigned long)user_counters[core_idx]);
    }

    free(idle_counters);
    free(nice_counters);
    free(sys_counters);
    free(user_counters);

    usleep(900);

    if (mic_get_idle_sum(cutil, &idle_sum) != E_MIC_SUCCESS) {
        print_error("Failed to get the idle sum", mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 7;
    }
    printf("    Idle Sum: %d \n", (int)idle_sum);

    if (mic_get_nice_sum(cutil, &nice_sum) != E_MIC_SUCCESS) {
        print_error("Failed to get the nice sum",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 8;
    }
    printf("    Nice Sum: %d\n", (int)nice_sum);

    if (mic_get_sys_sum(cutil, &sys_sum) != E_MIC_SUCCESS) {
        print_error("Failed to get the system sum",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 9;
    }
    printf("    System Sum: %d\n", (int)sys_sum);

    if (mic_get_user_sum(cutil, &user_sum) != E_MIC_SUCCESS) {
        print_error("Failed to get the user sum",
                    mic_get_device_name(mdh));
        (void)mic_free_core_util(cutil);
        return 10;
    }
    printf("    User Sum: %d\n\n", (int)user_sum);

    (void)mic_free_core_util(cutil);
    return 0;
}

int do_led_examples(struct mic_device *mdh)
{
    uint32_t value, new_value;

    new_value = 1;

    printf("\nLED Status Information Examples:\n\n");
    if (mic_get_led_alert(mdh, &value) != E_MIC_SUCCESS)
        print_error("Failed to get led alert", mic_get_device_name(mdh));
    printf("    LED Mode: ");
    (value == 0) ? printf("Normal Mode\n") : printf("Identify Mode\n");
    if (mic_set_led_alert(mdh, &new_value) != E_MIC_SUCCESS)
        print_error("Failed to set led alert", mic_get_device_name(mdh));
    if (mic_get_led_alert(mdh, &value) != E_MIC_SUCCESS)
        print_error("Failed to get led alert", mic_get_device_name(mdh));
    printf("    LED Mode: ");
    (value == 0) ? printf("Normal Mode\n") : printf("Identify Mode\n");
    new_value = 0;
    if (mic_set_led_alert(mdh, &new_value) != E_MIC_SUCCESS)
        print_error("Failed to set led alert", mic_get_device_name(mdh));
    if (mic_get_led_alert(mdh, &value) != E_MIC_SUCCESS)
        print_error("Failed to get led alert", mic_get_device_name(mdh));
    printf("    LED Mode: ");
    (value == 0) ? printf("Normal Mode\n") : printf("Identify Mode\n");
    return 0;
}

int do_turbo_util_examples(struct mic_device *mdh)
{
    struct mic_turbo_info *turbo;
    uint32_t state = 1;

    printf("\nTurbo Information Examples:\n\n");
    if (mic_get_turbo_state_info(mdh, &turbo) != E_MIC_SUCCESS) {
        print_error("Failed to get turbo state info",
                    mic_get_device_name(mdh));
        return 1;
    }
    if (mic_get_turbo_state(turbo, &state) != E_MIC_SUCCESS) {
        print_error("Failed to get turbo state",
                    mic_get_device_name(mdh));
        (void)mic_free_turbo_info(turbo);
        return 2;
    }
    printf("    Turbo State: ");
    (state == 0) ? printf("Disabled\n") : printf("Enabled\n");
    state = 1;
    if (mic_set_turbo_mode(mdh, &state) != E_MIC_SUCCESS) {
        print_error("Failed to set turbo state info",
                    mic_get_device_name(mdh));
        (void)mic_free_turbo_info(turbo);
        return 1;
    }
    (void)mic_free_turbo_info(turbo);
    if (mic_get_turbo_state_info(mdh, &turbo) != E_MIC_SUCCESS) {
        print_error("Failed to get turbo state info",
                    mic_get_device_name(mdh));
        return 1;
    }
    if (mic_get_turbo_state(turbo, &state) != E_MIC_SUCCESS) {
        print_error("Failed to get turbo state",
                    mic_get_device_name(mdh));
        (void)mic_free_turbo_info(turbo);
        return 2;
    }
    printf("    Turbo State: ");
    (state == 0) ? printf("Disabled\n") : printf("Enabled\n");
    //Setting turbo state back to disabled.
    state = 0;
    if (mic_get_turbo_mode(turbo, &state) != E_MIC_SUCCESS) {
        print_error("Failed to get turbo state",
                    mic_get_device_name(mdh));
        (void)mic_free_turbo_info(turbo);
        return 2;
    }
    printf("    Turbo Mode:  ");
    (state == 0) ? printf("Disabled\n") : printf("Enabled\n");
    //Setting turbo state back to disabled.
    state = 0;
    if (mic_set_turbo_mode(mdh, &state) != E_MIC_SUCCESS) {
        print_error("Failed to get turbo state info",
                    mic_get_device_name(mdh));
        (void)mic_free_turbo_info(turbo);
        return 1;
    }
    (void)mic_free_turbo_info(turbo);
    if (mic_get_turbo_state_info(mdh, &turbo) != E_MIC_SUCCESS) {
        print_error("Failed to get turbo state info",
                    mic_get_device_name(mdh));
        return 1;
    }
    if (mic_get_turbo_state(turbo, &state) != E_MIC_SUCCESS) {
        print_error("Failed to get turbo state",
                    mic_get_device_name(mdh));
        (void)mic_free_turbo_info(turbo);
        return 2;
    }
    printf("    Turbo State: ");
    (state == 0) ? printf("Disabled\n") : printf("Enabled\n");

    if (mic_get_turbo_mode(turbo, &state) != E_MIC_SUCCESS) {
        print_error("Failed to get turbo state",
                    mic_get_device_name(mdh));
        (void)mic_free_turbo_info(turbo);
        return 2;
    }
    printf("    Turbo Mode:  ");
    (state == 0) ? printf("Disabled\n") : printf("Enabled\n");
    (void)mic_free_turbo_info(turbo);
    return 0;
}

int do_get_uuid(struct mic_device *mdh)
{
    uint8_t str[NAME_MAX];
    size_t size = NAME_MAX;
    int i = 0;

    printf("\nGet UUID Examples:\n\n");
    if (mic_get_uuid(mdh, str, &size) != E_MIC_SUCCESS) {
        fprintf(stderr, "%s: Failed to get uuid : %s: %s\n",
                mic_get_device_name(mdh), mic_get_error_string(), strerror(
                    errno));
        return 1;
    }

    printf("    UUID: ");
    for (i = 0; i < 16; i++)
        printf("0x%02x ", str[i] & 0xff);
    printf("\n");
    return 0;
}

int main(void)
{
    int ncards, card_num, card;
    struct mic_devices_list *mdl;
    struct mic_device *mdh;
    int ret;
    uint32_t device_type;

    ret = mic_get_devices(&mdl);
    if (ret == E_MIC_DRIVER_NOT_LOADED) {
        fprintf(stderr, "Error: The driver is not loaded! ");
        fprintf(stderr, "Load the driver before using this tool.\n");
        return 1; /* Return a general error to the shell */
    } else if (ret == E_MIC_ACCESS) {
        fprintf(stderr, "Error: Access is denied to the driver! ");
        fprintf(stderr, "Do you have permissions to access the driver?\n");
        return 1; /* Return a general error to the shell */
    } else if (ret != E_MIC_SUCCESS) {
        fprintf(stderr, "Failed to get cards list: %s: %s\n",
                mic_get_error_string(), strerror(errno));
        return 1;
    }

    if (mic_get_ndevices(mdl, &ncards) != E_MIC_SUCCESS) {
        print_error("Failed to get number of cards", NULL);
        (void)mic_free_devices(mdl);
        return 2;
    }

    if (ncards == 0) {
        print_error("No MIC card found", NULL);
        (void)mic_free_devices(mdl);
        return 3;
    }

    /* Get card at index 0 */
    card_num = 0;
    if (mic_get_device_at_index(mdl, card_num, &card) != E_MIC_SUCCESS) {
        fprintf(stderr, "Error: Failed to get card at index %d: %s: %s\n",
                card_num, mic_get_error_string(), strerror(errno));
        mic_free_devices(mdl);
        return 4;
    }

    (void)mic_free_devices(mdl);

    if (mic_open_device(&mdh, card) != E_MIC_SUCCESS) {
        fprintf(stderr, "Error: Failed to open card %d: %s: %s\n",
                card_num, mic_get_error_string(), strerror(errno));
        return 5;
    }

    if (mic_get_device_type(mdh, &device_type) != E_MIC_SUCCESS) {
        print_error("Failed to get device type", mic_get_device_name(mdh));
        (void)mic_close_device(mdh);
        return 6;
    }

    if (device_type != KNC_ID) {
        fprintf(stderr, "Error: Unknown device Type: %u\n", device_type);
        (void)mic_close_device(mdh);
        return 7;
    }
    printf("    Found KNC device '%s'\n", mic_get_device_name(mdh));

    if ((ret = do_pci_config_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 20 + ret;
    }

    if ((ret = do_memory_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 40 + ret;
    }

    if ((ret = do_core_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 60 + ret;
    }

    if ((ret = do_power_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 80 + ret;
    }
    if ((ret = do_power_config_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 90 + ret;
    }
    if ((ret = do_processor_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 100 + ret;
    }
    if ((ret = do_thermal_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 120 + ret;
    }
    if ((ret = do_core_util_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 120 + ret;
    }

    if ((ret = do_led_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 150 + ret;
    }
    if ((ret = do_turbo_util_examples(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 160 + ret;
    }
    if ((ret = do_get_uuid(mdh)) != 0) {
        (void)mic_close_device(mdh);
        return 170 + ret;
    }
    (void)mic_close_device(mdh);
    return 0;
}
