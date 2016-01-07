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

#ifndef MIC_TOOLS_HELPER_H
#define MIC_TOOLS_HELPER_H

#include <stdarg.h>
#include <miclib.h>
#include <assert.h>

#ifdef DEBUG
#include <ut_instr_event.h>
#else
#define UT_INSTRUMENT_EVENT(event, statement)
#define SET_UT_INSTRUMENT_EVENT(event)
#define UNSET_UT_INSTRUMENT_EVENT(event)
#endif

#define MIC_COPYRIGHT "Copyright (c) 2015, Intel Corporation."

extern char *progname;

typedef int (action_func (void *, int *, int, char *[]));

struct cmd_actions {
    char *       cmd_name;
    action_func *cmd_func;
    char *       cmd_options;
    char *       cmd_help;
};

struct cmdopts {
    int   help_flag;
    int   device_flag;
    int   force_flag;
    int   file_flag;
    int   line_mode_flag;
    int   verbose_flag;
    int   test_mode_flag;
    int   smc_bootloader;
    int   maint_mode;
    int   no_reset;
    int   smc_ok;
    char *device;
    char *file;
};

extern struct cmdopts cmdopts;

struct cmd_actions *parse_actions(struct cmd_actions *, int *, int, char *[]);

long get_numl(char *, char **);

/* Allocate memory for 8 cards intially. */
#define N_ALLOCATE    (8)

int verify_image(const char *, struct mic_device *, void *, size_t, int *, int);

int poll_flash_op(struct    mic_device *, struct    mic_flash_op *, uint32_t,
                  int, int,
                  int);

int poll_maint_mode(struct mic_device *, uint32_t, int);

int poll_ready_state(struct mic_device *, uint32_t, int);

int flash_to_file_image(struct mic_device *, void *, size_t, void **, size_t *);

int flash_file_version(const char *, void *, char *, size_t);

int show_flash_info(struct mic_device *, void *, size_t);

int read_flash_device_maint_mode(struct mic_device *, void **, size_t *);

int check_smc_bootloader_image(char*, int);

inline void error_msg_start(char *, ...);
inline void error_msg_cont(char *, ...);

#define MAINT_MODE_POLL_INTERVAL      (1)       /* seconds */
#define READY_MODE_POLL_INTERVAL      (1)       /* seconds */
#define FLASH_UPDATE_POLL_INTERVAL    (1)       /* seconds */
#define FLASH_READ_POLL_INTERVAL      (1)       /* seconds */

#define MODE_CHANGE_TIMEOUT           (120)     /* seconds */
#define FLASH_READ_TIMEOUT            (120)     /* seconds */
#define FLASH_UPDATE_TIMEOUT          (240)     /* seconds */

#define ARG_USED(arg)    while ((void)arg, 0)

#define OFFSET_OF(field, structp) \
    ((uint8_t *)&(structp)->field - (uint8_t *)(structp))

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


#endif  /* MIC_TOOLS_HELPER_H */
