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
#include <stdlib.h>//atof
#include <stdio.h>
#include "main.h"
#include "helper.h"

#define HOST_CPU_INFO        "/proc/cpuinfo"
#define HOST_MEM_INFO        "/proc/meminfo"
#define HOST_VERSION         "/proc/version"
#define HOST_SCIF_VERSION    "/sys/class/mic/ctrl/version"

/* Stub for Linux only; see 'host_windows.c' */
int is_current_user_valid()
{
    return 1;
}

int host_osversion()
{
	static const size_t SIZE = NAME_MAX;
    char pdata[SIZE];

    char *os_ver;
    FILE *ptr;

    /* Parse the proc/version file for host os information */
    memset(pdata, 0, SIZE);
    ptr = fopen(HOST_VERSION, "r");
    if (ptr == NULL) {
        log_msg_start("\t\tOS Version\t\t: NotAvailable\n");
        return 0;
    }
    fgets(pdata, SIZE, ptr);
    fclose(ptr);

    if (strlen(pdata) > 0) {
        os_ver = strtok(pdata, " ");
        /* Discard the second token, which is just the word "version" */
        strtok(NULL, " ");
        os_ver = strtok(NULL, " ");
        if (os_ver)
            log_msg_start("\t\tOS Version\t\t: %s\n", os_ver);
        else
            log_msg_start("\t\tOS Version\t\t: NotAvailable\n");
    }
    return 0;
}

int host_meminfo()
{
	static const size_t SIZE = NAME_MAX;
    char pdata[SIZE];

    char *meminfo = NULL;
    FILE *ptr = NULL;
    char *cmd = "cat /proc/meminfo | grep \"MemTotal\"";
    int len = 0;

    memset(pdata, 0, SIZE);
    ptr = popen(cmd, "r");
    if (ptr == NULL) {
        log_msg_start("\t\tHost Physical Memory\t: NotAvailable\n");
        return 0;
    }

    fgets(pdata, SIZE, ptr);
    if (strlen(pdata) > 0) {
        meminfo = strtok(pdata, ":");
        if (meminfo != NULL) {
            if (strncmp("MemTotal", meminfo, strlen("MemTotal")) ==
                0) {
                meminfo = strtok(NULL, " ");
                if (meminfo != NULL
                    && ((len = strlen(meminfo)) > 3)) {
		    //CONVERTING KB TO MB
		    snprintf(meminfo, 10, "%d", (atoi(meminfo)/1024));//Int to Char P
                    log_msg_start
                        ("\t\tHost Physical Memory\t: %s MB\n",
                        meminfo);
                } else {
                    log_msg_start
                        ("\t\tHost Physical Memory\t: NotAvailable\n");
                }
            }
        } else {
            log_msg_start
                ("\t\tHost Physical Memory\t: NotAvailable\n");
        }
    }

    fclose(ptr);
    return 0;
}

int host_mpssinfo()
{
	static const size_t SIZE = NAME_MAX;
    char pdata[SIZE];

    FILE *ptr = NULL;
    char *cmd = "rpm -qi mpss-daemon | grep Version";
    char *ver = NULL;

    memset(pdata, 0, SIZE);
    ptr = popen(cmd, "r");
    if (ptr == NULL) {
        log_msg_start("\t\tMPSS Version\t\t: NotAvailable\n");
        return 0;
    }

    fgets(pdata, SIZE, ptr); /* "Version      : <MPSS version>" */
    ver = strtok(pdata, " "); /* "Version" */
    ver = strtok(NULL, " "); /* ":" */
    ver = strtok(NULL, " "); /* "<MPSS version>" */

	if (ver != NULL)
		log_msg_start("\t\tMPSS Version\t\t: %s\n", ver);
	else
		log_msg_start("\t\tMPSS Version\t\t: NotAvailable\n");

    fclose(ptr);
    return 0;
}

int host_driver_version()
{
	static const size_t SIZE = NAME_MAX;
    char pdata[SIZE];

    FILE *ptr = NULL;
    char *ver = NULL;

    memset(pdata, 0, SIZE);
    ptr = fopen(HOST_SCIF_VERSION, "r");
    if (ptr == NULL) {
        log_msg_start("\t\tDriver Version\t\t: NotAvailable\n");
        return 0;
    }

    fgets(pdata, SIZE, ptr); /* <MPSS_VERSION> (<MPSS_BUILTBY>) */
	ver = strtok(pdata, " "); /* <MPSS_VERSION> */
	if (ver != NULL)
		log_msg_start("\t\tDriver Version\t\t: %s\n", ver);
	else
		log_msg_start("\t\tDriver Version\t\t: NotAvailable\n");

    fclose(ptr);
    return 0;
}

int host_info(void)
{
    log_msg_start("\n\tSystem Info\n");

    log_msg_start("\t\tHOST OS\t\t\t: Linux\n");
    host_osversion();
    host_driver_version();
    host_mpssinfo();
    host_meminfo();
    return 0;
}
#endif
