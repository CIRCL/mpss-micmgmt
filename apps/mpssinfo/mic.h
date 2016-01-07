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

#ifndef __MPSSINFO_MIC_H__
#define __MPSSINFO_MIC_H__
#include <miclib.h>
#include "helper.h"

#define GRP_ALL_STR      "all"
#define GRP_VER_STR      "versions"
#define GRP_BOARD_STR    "board"
#define GRP_CORES_STR    "cores"
#define GRP_THER_STR     "thermal"
#define GRP_GDDR_STR     "gddr"

#define GRP_ALL          0x0
#define GRP_VER          0x1
#define GRP_BOARD        0x2
#define GRP_CORES        0x3
#define GRP_THER         0x4
#define GRP_GDDR         0x5

int show_list();
int show_all(int card, int group_id);
int mic_info(int card, int group_id);
int version_info(struct mic_device *mdh);
int board_info(struct mic_device *mdh);
int cores_info(struct mic_device *mdh);
int thermal_info(struct mic_device *mdh);
int memory_info(struct mic_device *mdh);
#endif
