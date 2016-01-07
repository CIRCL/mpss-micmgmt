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

#ifndef MIC_TOOLS_COMMON_H
#define MIC_TOOLS_COMMON_H

#include "miclib.h"
#include "assert.h"

extern char *progname;

inline void error_msg_start(char *, ...);
inline void error_msg_cont(char *, ...);

#define ARG_USED(arg)    while ((void)arg, 0)

#define CMD_LINE_ERR      (1)
#define CARD_NUM_ERR      (2)
#define MEM_ERR           (3)
#define FILE_ERR          (4)
#define NODEV_ERR         (5)
#define MODE_ERR          (6)
#define DEVOPEN_ERR       (7)
#define OTHER_ERR         (8)
#define IMAGE_MISMATCH    (9)
#define FLASH_UPDATED     (10)

long get_numl(char *, char **);

int check_smc_bootloader_image(int);


#endif  /* MIC_TOOLS_COMMON_H */
