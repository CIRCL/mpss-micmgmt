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
#include "help.h"
#include "main.h"

#define ASSERT    assert

#define posix_help_help                                                  \
    "mpssinfo [-h, --help=][noargs|device|list|version]\n"               \
    "\n"                                                                 \
    "|  All options are POSIX compliant \n"                              \
    "|  Describe usage of mpssinfo. If topic specified, describe only\n" \
    "|  the desired topic\n"

#define posix_help_list       \
    "mpssinfo [-l, --list]\n" \
    "\n"                      \
    "|  List all detected devices in the system\n"

#define posix_help_device                                              \
    "mpssinfo [-d, --device <deviceNum>] [-g, --group <groupname>] \n" \
    "\n"                                                               \
    "|  with no args will print \n"                                    \
    "|  device information for each detected device in the system\n"   \
    "|\n"                                                              \
    "|  --device     indicates that only specified devices should\n"   \
    "|               have their device information printed.\n"         \
    "|  <deviceNum>  single device number from --list will print\n"    \
    "|               device information for that device\n"             \
    "|               If 'all' is given as deviceNum,\n"                \
    "|               the printout will be identical to the printout\n" \
    "|               provided by mpssinfo without arguments.\n"        \
    "|  --group      indicates that one of the following info groups\n"\
    "|               follows.\n"                                       \
    "|                   Versions\n"                                   \
    "|                   Board\n"                                      \
    "|                   Core\n"                                       \
    "|                   Thermal\n"                                    \
    "|                   GDDR\n"                                       \
    "|                   All (default)\n"                              \
    "|  <groupname>  one of the above info groups.Note: prefixes are\n"\
    "|               accepted for group names (e.g. ver for version)\n"

#define posix_help_version \
    "mpssinfo --version\n" \
    "\n"                   \
    "|  Display the tool version.\n"

#define help_help                                                           \
    "micinfo -Help [noargs deviceInfo listDevices version]\n"               \
    "\n"                                                                    \
    "|  Arguments beginning with a \"-\" are not case sensitive\n"          \
    "|  and need only be long enough to have a unique match\n"              \
    "|    Example: For \"-listDevices\", \"-l\" will also list devices\n"   \
    "|  Describe usage of micinfo. If topic specified, describe only the\n" \
    "|  desired topic\n"

#define  help_list           \
    "micinfo -listDevices\n" \
    "\n"                     \
    "|  List all detected devices\n"

#define help_device                                                      \
    "micinfo [-deviceInfo <deviceNum>] [-group <groupname>] \n"          \
    "\n"                                                                 \
    "|  with no args will print first SystemInfo section and then\n"     \
    "|  DeviceInfo   sections for each device detected\n"                \
    "|\n"                                                                \
    "|  -deviceInfo  indicates that only specified devices should\n"     \
    "|               have their device information printed.\n"           \
    "|  <deviceNum>  single device number from listDevices will print\n" \
    "|               DeviceInfo for that device\n"                       \
    "|               If 'all' is given as deviceNum,\n"                  \
    "|               the printout will be identical to the printout\n"   \
    "|               provided by micinfo without arguments.\n"           \
    "|  -group       indicates that one of the following info groups\n"  \
    "|               follows.\n"                                         \
    "|                   Versions\n"                                     \
    "|                   Board\n"                                        \
    "|                   Core\n"                                         \
    "|                   Thermal\n"                                      \
    "|                   GDDR\n"                                         \
    "|                   All (default)\n"                                \
    "|  <groupname>  one of the above info groups. Note: prefixes are\n" \
    "|               accepted for group names (e.g. ver for version)\n"


#define help_version      \
    "micinfo -version \n" \
    "\n"                  \
    "|  Display the tool version.\n"

#define posix_help_notes \
    "Note: prefixes are accepted for options (e.g. mpssinfo --dev 0)\n"

#define help_notes \
    "Note: prefixes are accepted for options (e.g. micinfo -dev 0)\n"

int show_help(char *help)
{
    int posix = 1;

    if (strncasecmp(progname, "micinfo", strlen("micinfo")) == 0)
        posix = 0;
    if (!help || !strcasecmp(help, "noargs"))
        log_msg_start("%s\n", (posix ? posix_help_help : help_help));

    if (!help || !strncasecmp(help, "listDevices", strlen(help)))
        log_msg_start("%s\n", (posix ? posix_help_list : help_list));

    if (!help || !strncasecmp(help, "deviceInfo", strlen(help)))
        log_msg_start("%s\n", (posix ? posix_help_device : help_device));

    if (!help || !strncasecmp(help, "versions", strlen(help)))
        log_msg_start("%s\n",
                      (posix ? posix_help_version : help_version));
    log_msg_start("%s\n", (posix ? posix_help_notes : help_notes));
    return 0;
}
