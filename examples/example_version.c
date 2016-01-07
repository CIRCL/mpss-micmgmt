/*
 * Copyright 2012 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include <miclib.h>

const size_t MAX_FORMAT_LEN = 64;

void error_message(const char *extra_str /* = NULL for no string*/,
                   int mic_dev_num /* =-1 to ignore */)
{
    if (mic_dev_num != -1 && extra_str != NULL) {
        fprintf(stderr, "example_version: %s: %d: %s: %s\n", extra_str,
                mic_dev_num, mic_get_error_string(),
                strerror(errno));
    } else if (mic_dev_num == -1 && extra_str != NULL) {
        fprintf(stderr, "example_version: %s :%s: %s\n", extra_str,
                mic_get_error_string(), strerror(errno));
    } else {
        fprintf(stderr, "example_version: %s: %s\n", mic_get_error_string(),
                strerror(errno));
    }
}

void print_value_pair(const char *name, const char *val)
{
    printf("%20s : %-20s\n", name, val);
}

int main()
{
    struct mic_devices_list *dev_list = NULL;
    struct mic_device *mdh = NULL;
    struct mic_version_info *version = NULL;
    int ndevices = 0;
    int indx = 0;
    int card = 0;
    int ret = 0;
    char str[NAME_MAX] = { '\0' };
    size_t size = NAME_MAX;
    int rv = 0;

    ret = mic_get_devices(&dev_list);
    if (ret == E_MIC_DRIVER_NOT_LOADED) {
        fprintf(stderr, "Error: The driver is not loaded! ");
        fprintf(stderr, "Load the driver before using this tool.\n");
        return 1; /* Return a general error to the shell */
    }
    else if (ret == E_MIC_ACCESS) {
        fprintf(stderr, "Error: Access is denied to the driver! ");
        fprintf(stderr, "Do you have permissions to access the driver?\n");
        return 1; /* Return a general error to the shell */
    }
    else if (ret != E_MIC_SUCCESS) {
        error_message("No devices found", -1);
        return 1;
    }

    if (mic_get_ndevices(dev_list, &ndevices) != E_MIC_SUCCESS) {
        error_message("Failed to get devices", -1);
        mic_free_devices(dev_list);
        return 1;
    }

    for (indx = 0; indx < ndevices; indx++) {
        int returned_index = 0;
        if (mic_get_device_at_index(dev_list, card,
                                    &returned_index) != E_MIC_SUCCESS) {
            error_message("Failed to get card at index", indx);
            card++;
            rv = 1;
            continue;
        }

        ret = mic_open_device(&mdh, card);
        if (ret != E_MIC_SUCCESS) {
            error_message("Failed to open device", card);
            card++;
            rv = 1;
            continue;
        }
        ret = mic_get_version_info(mdh, &version);
        if (ret != E_MIC_SUCCESS) {
            error_message("Failed to get version info", -1);
            mic_close_device(mdh);
            card++;
            rv = 1;
            continue;
        }

        printf("\n Version Information:  mic%d\n", indx);
        printf(" ------------------------------------------------\n");
        mic_get_flash_version(version, str, &size);
        print_value_pair("Flash Version", str);
        mic_get_uos_version(version, str, &size);
        print_value_pair("Coprocessor OS Version", str);
        mic_free_version_info(version);
        mic_get_silicon_sku(mdh, str, &size);
        print_value_pair("SKU", str);
        mic_get_serial_number(mdh, str, &size);
        print_value_pair("Serial Number", str);
        mic_close_device(mdh);
        card++;
    }
    mic_free_devices(dev_list);
    return rv;
}
