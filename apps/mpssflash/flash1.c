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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <signal.h>
#include <limits.h>
#include <time.h>
#include "helper.h"
#include <miclib.h>

#define TEST_MODE         (1)
#define SMC_BOOTLOADER    (2)
#define MAINT_MODE        (3)
#define NO_RESET          (4)
#define SMC_OK            (5)

static struct option options[] = {
    { "device",        1, NULL, 'd'            },
    { "file",          1, NULL, 'f'            },
    { "read",          1, NULL, 'r'            },
    { "help",          0, NULL, 'h'            },
    { "oldimage",      0, NULL, 'm'            },
    { "line",          0, NULL, 'l'            },
    { "verbose",       0, NULL, 'v'            },
    { "test",          0, NULL, TEST_MODE      },
    { "smcbootloader", 0, NULL, SMC_BOOTLOADER },
    { "maintmode",     0, NULL, MAINT_MODE     },
    { "noreset",       0, NULL, NO_RESET       },
    { "smcok",         0, NULL, SMC_OK         },
    { NULL,            0, 0,    0              }
};

struct cmdopts cmdopts;

static action_func flash_update;
static action_func flash_version;
static action_func flash_read;
static action_func flash_device;
static action_func flash_check;
static int flash_help(struct cmd_actions *);

static struct cmd_actions actions[] = {
    {
        "update", flash_update,
        "[{--device|-d} <device>] {--file|-f} <file> "
        "[{--oldimage|-m}] [{--verbose|-v}] update",
        "update flash image using specified file"
    },
    {
        "version", flash_version,
        "[{--device|-d} <device>] [{--file|-f} <file>] version",
        "read version from flash or given file"
    },
    {
        "read", flash_read,
        "[{--device|-d} <device>] {--file|-f} <file> read",
        "read contents of flash into the specified file"
    },
    {
        "device", flash_device,
        "[{--device|-d} <device>] device",
        "get vendor and device type information"
    },
    {
        "check", flash_check,
        "--file|-f <file> [{--device|-d} <device>] check",
        "check if specified file is compatible with the card"
    },
    { NULL, NULL, NULL, NULL }
};

#define MSG_START    (0)
#define MSG_CONT     (1)


/*
 * Called by main of flash1 and also by micflash child process.
 */
int flash1_start(int argc, char *argv[])
{
    int c, option_index = 0;
    struct cmd_actions *ca;

    if (progname == NULL)
        progname = argv[0];
    bzero((void *)&cmdopts, sizeof(cmdopts));

    setlinebuf(stdout);
    setlinebuf(stderr);

    optind = 1;

    for (;; ) {
        c = getopt_long(argc, argv, "d:f:hmv", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case TEST_MODE:
            /* Test mode */
            cmdopts.test_mode_flag++;
            break;

        case SMC_BOOTLOADER:
            cmdopts.smc_bootloader++;
            break;

        case MAINT_MODE:
            cmdopts.maint_mode++;
            break;

        case NO_RESET:
            cmdopts.no_reset++;
            break;

        case SMC_OK:
            cmdopts.smc_ok++;
            break;

        case 'd':
            cmdopts.device_flag++;
            cmdopts.device = optarg;
            break;

        case 'f':
            cmdopts.file_flag++;
            cmdopts.file = optarg;
            break;

        case 'm':
            cmdopts.force_flag++;
            break;

        case 'h':
            cmdopts.help_flag++;
            break;

        case 'l':
            cmdopts.line_mode_flag++;
            break;

        case 'v':
            cmdopts.verbose_flag++;
            break;

        default:
            error_msg_start("Unrecognized option: %c\n", c);
        /* FALL THRU */
        case '?':
            return CMD_LINE_ERR;
        }
    }

    if (cmdopts.device_flag > 1) {
        error_msg_start("Multiple '{--device|-d} <device>' "
                        "options\n");
        return CMD_LINE_ERR;
    }

    if (cmdopts.file_flag > 1) {
        error_msg_start("Multiple '{--file|-f} <file>' options\n");
        return CMD_LINE_ERR;
    }

    if (cmdopts.help_flag > 1) {
        error_msg_start("Multiple '{--help|-h}' options\n");
        return CMD_LINE_ERR;
    }

    if (cmdopts.force_flag > 1) {
        error_msg_start("Multiple '{--oldimage|-m}' options\n");
        return CMD_LINE_ERR;
    }

    if (cmdopts.help_flag && (cmdopts.device_flag || cmdopts.file_flag)) {
        error_msg_start("Device or file must not be specified with "
                        "the help command\n");
        return CMD_LINE_ERR;
    }

    if (optind >= argc) {
        if (cmdopts.help_flag)
            return flash_help(NULL);

        error_msg_start("No action specified\n");
        return CMD_LINE_ERR;
    }

    option_index = optind;

    if ((ca = parse_actions(actions, &option_index, argc, argv)) == NULL) {
        error_msg_start("Unrecognized action - '%s'\n",
                        argv[option_index]);
        return CMD_LINE_ERR;
    }

    if (cmdopts.help_flag) {
        return flash_help(ca);
    } else {
        if (cmdopts.force_flag &&
            (strcmp(ca->cmd_name, "update") != 0)) {
            error_msg_start("'{--oldimage|-m}' may only be "
                            "specified with update command\n");
            return CMD_LINE_ERR;
        }

        if (cmdopts.smc_bootloader &&
            (strcmp(ca->cmd_name, "update") != 0)) {
            error_msg_start("'smcbootloader' may only be "
                            "specified with update command\n");
            return CMD_LINE_ERR;
        }

        if (cmdopts.smc_ok && (strcmp(ca->cmd_name, "check") != 0)) {
            error_msg_start("'smcok' may only be specified with "
                            "check command\n");
            return CMD_LINE_ERR;
        }

        return (*ca->cmd_func)((void *)&cmdopts, &option_index,
                               argc, argv);
    }
}

static int flash_help(struct cmd_actions *cact)
{
    printf("Usage:\n");

    if (cact == NULL) {
        struct cmd_actions *p;
        printf("%s {--help|-h} - Print this help\n", progname);
        printf("%s {--help|-h} <cmd> - "
               "Print command specific help\n", progname);

        for (p = actions; p->cmd_name; p++) {
            printf("%s %s - %s\n",
                   progname, p->cmd_options, p->cmd_help);
        }

        return 0;
    }

    printf("%s %s - %s\n", progname, cact->cmd_options, cact->cmd_help);
    return 0;
}

static int get_card_from_cmdline(struct cmdopts *copts)
{
    struct mic_devices_list *md;
    int card;
    int i, d, n;
    char *more;

    if (copts->device == NULL) {
        /* No -d option. */
        if ((mic_get_devices(&md) != E_MIC_SUCCESS) ||
            (mic_get_ndevices(md, &n) != E_MIC_SUCCESS)) {
            error_msg_start("Failed to get cards info: %s: %s\n",
                            mic_get_error_string(), strerror(errno));
            return -1;
        }

        if (n > 1) {
            error_msg_start("%d cards present - "
                            "please specify one of following: ", n);
            for (i = 0; i < n - 1; i++) {
                (void)mic_get_device_at_index(md, i, &d);
                error_msg_cont("%u, ", d);
            }
            (void)mic_get_device_at_index(md, i, &d);
            error_msg_cont("%u\n", d);

            mic_free_devices(md);
            return -1;
        }
        (void)mic_get_device_at_index(md, 0, &card);
        mic_free_devices(md);
    } else {
        /* Use the specified device. */
        card = get_numl(copts->device, &more);
        if (*more || errno) {
            error_msg_start("Bad device id '%s'\n", copts->device);
            return -1;
        }
    }

    return card;
}

static void set_signals(sig_t hdlr)
{
    static int sig[] = { SIGHUP,  SIGINT, SIGQUIT, SIGABRT, SIGPIPE,
                         SIGTERM, SIGTSTP };
    static int n_signals = sizeof(sig) / sizeof(int);
    int i;

    for (i = 0; i < n_signals; i++)
        (void)signal(sig[i], hdlr);
}

static int read_flash_file(const char *fname, void **rbuf, size_t *size)
{
    int fd;
    struct stat sbuf;
    void *buf;
    int ret;

    if ((fd = open(fname, O_RDONLY)) < 0) {
        error_msg_start("%s: %s\n", fname, strerror(errno));
        return FILE_ERR;
    }

    if (fstat(fd, &sbuf) < 0) {
        error_msg_start("%s: %s\n", fname, strerror(errno));
        close(fd);
        return FILE_ERR;
    }

    if ((buf = valloc((size_t)sbuf.st_size)) == NULL) {
        error_msg_start("malloc: %s\n", strerror(ENOMEM));
        close(fd);
        return MEM_ERR;
    }

    if ((ret = read(fd, buf, sbuf.st_size)) != sbuf.st_size) {
        error_msg_start("%s: %s\n", fname, ret < 0 ? strerror(errno) :
                        "Failed to read file");
        free(buf);
        close(fd);
        return FILE_ERR;
    }

    close(fd);
    *rbuf = buf;
    *size = sbuf.st_size;

    return 0;
}

static int exit_maint_mode(struct mic_device *device, int ignore_no_reset)
{
    if (cmdopts.test_mode_flag || (!ignore_no_reset && cmdopts.no_reset))
        return E_MIC_SUCCESS;

    return mic_leave_maint_mode(device);
}

static int set_maint_mode(struct mic_device *device)
{
    uint32_t interval;

    if (cmdopts.test_mode_flag || cmdopts.maint_mode) {
        int maint_mode;

        if (mic_in_maint_mode(device, &maint_mode) == E_MIC_SUCCESS) {
            if (maint_mode)
                return 0;
        }
    }

    if (mic_enter_maint_mode(device) != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to switch to maintenance mode: "
                        "%s: %s\n", mic_get_device_name(device),
                        errno == EPERM ? "Please use micctrl command "
                        "to move card to ready state" :
                        mic_get_error_string(),
                        strerror(errno));
        return -1;
    }

    interval = cmdopts.line_mode_flag ? 0 : MAINT_MODE_POLL_INTERVAL;
    if (poll_maint_mode(device, interval, MODE_CHANGE_TIMEOUT) != 0) {
        if (exit_maint_mode(device, 1) != E_MIC_SUCCESS) {
            error_msg_start("%s: Failed to reset: %s: %s\n",
                            mic_get_device_name(device),
                            mic_get_error_string(),
                            strerror(errno));
        } else {
            interval = cmdopts.line_mode_flag ? 0 :
                       READY_MODE_POLL_INTERVAL;
            (void)poll_ready_state(device,
                                   READY_MODE_POLL_INTERVAL, 
                                   MODE_CHANGE_TIMEOUT);
        }
        return -1;
    }

    return 0;
}

static int flash_update(void *in, int *index, int argc, char *argv[])
{
    struct cmdopts *copts = (struct cmdopts *)in;
    int selected_card;
    void *fbuf;
    struct mic_device *device;
    struct mic_flash_op *desc = NULL;
    int ret = 0, flash_image;
    size_t fsize;
    uint32_t interval;

    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of update command\n");
        return CMD_LINE_ERR;
    }

    if (copts->file == NULL) {
        error_msg_start("Missing '{--file|-f} <file>' arg\n");
        return CMD_LINE_ERR;
    }

    if ((selected_card = get_card_from_cmdline(copts)) < 0)
        return CARD_NUM_ERR;

    if ((ret = read_flash_file(copts->file, &fbuf, &fsize)) != 0)
        return ret;

    ret = 0;
    if (mic_open_device(&device, selected_card) != E_MIC_SUCCESS) {
        error_msg_start("Failed to open card '%d': %s: %s\n",
                        selected_card, mic_get_error_string(),
                        strerror(errno));
        free(fbuf);
        return errno == ENOENT ?  NODEV_ERR : DEVOPEN_ERR;
    }

    set_signals(SIG_IGN);

    if (set_maint_mode(device) < 0) {
        set_signals(SIG_DFL);
        mic_close_device(device);
        free(fbuf);
        return MODE_ERR;
    }

    if ((ret = verify_image(copts->file, device, fbuf, fsize,
                            &flash_image, cmdopts.force_flag)) != 0) {
        if (ret < 0) {
            ret = -ret;
        } else if (ret == IMAGE_MISMATCH) {
            error_msg_start("%s: %s: Invalid image: Device and "
                            "Subsystem ID verification failed\n",
                            mic_get_device_name(
                                device), copts->file);
        }
        goto reset_card;
    }

    if (flash_image && cmdopts.smc_bootloader) {
        error_msg_start("%s: 'smcbootloader' option may not be "
                        "specified with a flash image\n",
                        mic_get_device_name(device));
        ret = CMD_LINE_ERR;
        goto reset_card;
    }

    if (mic_flash_update_start(device, fbuf, fsize, &desc)
        != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to start flash update: %s: %s\n",
                        mic_get_device_name(device),
                        mic_get_error_string(), strerror(errno));
        ret = OTHER_ERR;
        goto reset_card;
    }

    interval = cmdopts.line_mode_flag ? 0 : FLASH_READ_POLL_INTERVAL;
    if ((ret = poll_flash_op(device, desc, interval,
                             FLASH_UPDATE_TIMEOUT, 1, flash_image)) != 0) {
        ret = (ret < 0) ? OTHER_ERR : FLASH_UPDATED;
        (void)mic_flash_update_done(desc);
        goto reset_card;
    }

    if (mic_flash_update_done(desc) != E_MIC_SUCCESS) {
        error_msg_start("%s: An error was seen after successful flash "
                        "update %s: %s\n", mic_get_device_name(device),
                        mic_get_error_string(), strerror(errno));
        ret = OTHER_ERR;
        goto reset_card;
    }

reset_card:
    free(fbuf);

    if (exit_maint_mode(device, ret) != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to reset: %s: %s\n",
                        mic_get_device_name(device),
                        mic_get_error_string(),
                        strerror(errno));
        ret = OTHER_ERR;
    } else {
        interval = cmdopts.line_mode_flag ? 0 :
                   READY_MODE_POLL_INTERVAL;
        (void)poll_ready_state(device, interval, MODE_CHANGE_TIMEOUT);
    }

    set_signals(SIG_DFL);
    mic_close_device(device);
    return ret;
}

static int flash_version(void *in, int *index, int argc, char *argv[])
{
    int selected_card;
    struct cmdopts *copts = (struct cmdopts *)in;
    void *buf = NULL;
    struct mic_device *device;
    char flash[NAME_MAX];
    int ret;
    size_t size;
    uint32_t interval;

    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of version command\n");
        return CMD_LINE_ERR;
    }

    if ((copts->device != NULL) && (copts->file != NULL)) {
        error_msg_start(
            "Only one of device or file options must be "
            "specified with version command\n");
        return CMD_LINE_ERR;
    }

    if ((copts->file != NULL) && (copts->maint_mode != 0)) {
        error_msg_start(
            "'maint' may not be specified with file option "
            "in version command\n");
        return CMD_LINE_ERR;
    }

    if ((copts->file != NULL) && (copts->no_reset != 0)) {
        error_msg_start(
            "'noreset' may not be specified with file option "
            "in version command\n");
        return CMD_LINE_ERR;
    }

    if (copts->file != NULL) {
        /* Read version from the file */
        if ((ret = read_flash_file(copts->file, &buf, &size)) != 0)
            return ret;

        if ((ret = flash_file_version(copts->file, buf,
                                      flash, sizeof(flash))) != 0) {
            free(buf);
            return ret;
        }
        free(buf);
        printf("Version: %s\n", flash);
        return 0;
    }

    selected_card = get_card_from_cmdline(copts);

    if (selected_card < 0)
        return CARD_NUM_ERR;

    if (mic_open_device(&device, selected_card) != 0) {
        error_msg_start("Failed to open card '%d': %s: %s\n",
                        selected_card, mic_get_error_string(),
                        strerror(errno));
        return errno == ENOENT ?  NODEV_ERR : DEVOPEN_ERR;
    }

    set_signals(SIG_IGN);

    if (set_maint_mode(device) != 0) {
        mic_close_device(device);
        set_signals(SIG_DFL);
        return MODE_ERR;
    }

    buf = NULL;
    ret = 0;
    if ((ret = read_flash_device_maint_mode(device, &buf, &size)) != 0) {
        ret = 6;
        goto reset_card;
    }

    if (mic_flash_version(device, buf, flash, sizeof(flash))
        != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to retrieve version: "
                        "%s: %s\n", mic_get_device_name(device),
                        mic_get_error_string(), strerror(errno));
        ret = 4;
        goto reset_card;
    }

    if (!cmdopts.line_mode_flag)
        printf("%s: Version: %s\n", mic_get_device_name(device), flash);
    else
        printf("Version: %s\n", flash);

reset_card:
    if (exit_maint_mode(device, ret) != E_MIC_SUCCESS) {
        error_msg_start(
            "%s: Failed to reset: %s: %s\n",
            mic_get_device_name(device),
            mic_get_error_string(),
            strerror(errno));
        ret = 5;
    } else {
        interval = cmdopts.line_mode_flag ? 0 :
                   READY_MODE_POLL_INTERVAL;
        (void)poll_ready_state(device, interval, MODE_CHANGE_TIMEOUT);
    }
    (void)mic_close_device(device);
    if (buf != NULL)
        free(buf);
    set_signals(SIG_DFL);
    return ret;
}

static int flash_read(void *in, int *index, int argc, char *argv[])
{
    struct cmdopts *copts = (struct cmdopts *)in;
    int selected_card;
    int fd = -1;
    struct mic_device *device;
    int ret = 0;
    struct mic_flash_op *desc = NULL;
    void *buf = NULL, *outbuf = NULL;
    size_t size, outbuf_size;
    uint32_t interval;

    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of read command\n");
        return CMD_LINE_ERR;
    }

    if (copts->file == NULL) {
        error_msg_start("Missing '{--file|-f} <file>' arg\n");
        return CMD_LINE_ERR;
    }

    if ((selected_card = get_card_from_cmdline(copts)) < 0)
        return CARD_NUM_ERR;

    if ((fd = open(copts->file, O_RDWR | O_TRUNC | O_CREAT, 0644)) < 0) {
        error_msg_start("%s: %s\n", copts->file, strerror(errno));
        return FILE_ERR;
    }

    if (mic_open_device(&device, selected_card) != 0) {
        error_msg_start("Failed to open card '%d': %s: %s\n",
                        selected_card, mic_get_error_string(),
                        strerror(errno));
        close(fd);
        return errno == ENOENT ?  NODEV_ERR : DEVOPEN_ERR;
    }

    set_signals(SIG_IGN);

    if (set_maint_mode(device) != 0) {
        close(fd);
        mic_close_device(device);
        return MODE_ERR;
    }

    if (mic_flash_size(device, &size) != E_MIC_SUCCESS) {
        ret = 5;
        error_msg_start(
            "%s: Failed to read flash header information: "
            "%s: %s\n", mic_get_device_name(device),
            mic_get_error_string(), strerror(errno));
        goto reset_card;
    }

    buf = valloc(size);
    if (buf == NULL) {
        error_msg_start("%s: malloc: %s\n", mic_get_device_name(device),
                        strerror(errno));
        ret = 3;
        goto reset_card;
    }

    if (mic_flash_read_start(device, buf, size, &desc) != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to start flash read: %s: %s\n",
                        mic_get_device_name(device),
                        mic_get_error_string(), strerror(errno));
        ret = 5;
        goto reset_card;
    }

    interval = cmdopts.line_mode_flag ? 0 : FLASH_READ_POLL_INTERVAL;
    if (poll_flash_op(device, desc, interval,
                      FLASH_READ_TIMEOUT, 0, 0) != 0) {
        ret = 5;
        goto reset_card;
    }

    if (mic_flash_read_done(desc) != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to read flash: %s: %s\n",
                        mic_get_device_name(device),
                        mic_get_error_string(), strerror(errno));
        ret = 5;
        desc = NULL;
        goto reset_card;
    }
    desc = NULL;

    if ((ret = flash_to_file_image(device, buf, size, &outbuf,
                                   &outbuf_size)) != 0)
        goto reset_card;

    if (write(fd, outbuf, outbuf_size) != (int)outbuf_size) {
        error_msg_start("%s: %s: write: %s\n",
                        mic_get_device_name(device),
                        copts->file, strerror(errno));
        ret = 4;
        goto reset_card;
    }

reset_card:
    close(fd);

    if (desc != NULL)
        (void)mic_flash_read_done(desc);

    if (buf != NULL)
        free(buf);

    if (outbuf != NULL)
        free(outbuf);

    if (exit_maint_mode(device, ret) != E_MIC_SUCCESS) {
        error_msg_start(
            "%s: Failed to reset: %s: %s\n",
            mic_get_device_name(device),
            mic_get_error_string(),
            strerror(errno));
        ret = 5;
    } else {
        interval = cmdopts.line_mode_flag ? 0 :
                   READY_MODE_POLL_INTERVAL;
        (void)poll_ready_state(device, interval, MODE_CHANGE_TIMEOUT);
    }

    set_signals(SIG_DFL);
    (void)mic_close_device(device);
    return ret;
}

static int flash_device(void *in, int *index, int argc, char *argv[])
{
    struct cmdopts *copts = (struct cmdopts *)in;
    int selected_card;
    struct mic_device *device;
    int ret = 0;
    char str[NAME_MAX];
    size_t size = sizeof(str);
    uint32_t interval;

    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of device command\n");
        return CMD_LINE_ERR;
    }

    if (copts->file != NULL) {
        error_msg_start("'{--file|-f} <file>' may not be used with "
                        "device command\n");
        return CMD_LINE_ERR;
    }

    if ((selected_card = get_card_from_cmdline(copts)) < 0)
        return CARD_NUM_ERR;

    if (mic_open_device(&device, selected_card) != 0) {
        error_msg_start("Failed to open card '%d': %s: %s\n",
                        selected_card, mic_get_error_string(),
                        strerror(errno));
        return errno == ENOENT ?  NODEV_ERR : DEVOPEN_ERR;
    }

    set_signals(SIG_IGN);

    if (set_maint_mode(device) != 0) {
        set_signals(SIG_DFL);
        mic_close_device(device);
        return MODE_ERR;
    }

    if (mic_get_flash_vendor_device(device, str, &size) != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to retrieve flash vendor info: "
                        "%s: %s\n", mic_get_device_name(device),
                        mic_get_error_string(), strerror(errno));
        ret = 5;
        goto reset_card;
    }

    if (!cmdopts.line_mode_flag)
        printf("%s: Flash: %s\n", mic_get_device_name(device), str);
    else
        printf("Flash: %s\n", str);

reset_card:
    if (exit_maint_mode(device, ret) != E_MIC_SUCCESS) {
        error_msg_start(
            "%s: Failed to reset: %s: %s\n",
            mic_get_device_name(device),
            mic_get_error_string(),
            strerror(errno));
        ret = 6;
    } else {
        interval = cmdopts.line_mode_flag ? 0 :
                   READY_MODE_POLL_INTERVAL;
        (void)poll_ready_state(device, interval, MODE_CHANGE_TIMEOUT);
    }

    set_signals(SIG_DFL);
    (void)mic_close_device(device);

    return ret;
}

static int flash_check(void *in, int *index, int argc, char *argv[])
{
    struct cmdopts *copts = (struct cmdopts *)in;
    int selected_card;
    struct mic_device *device;
    size_t size;
    int ret, flash_image;
    void *buf;

    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of image check "
                        "command\n");
        return CMD_LINE_ERR;
    }

    if (copts->file == NULL) {
        error_msg_start("Missing '{--file|-f} <file>' arg\n");
        return CMD_LINE_ERR;
    }

    if ((selected_card = get_card_from_cmdline(copts)) < 0)
        return CARD_NUM_ERR;

    if ((ret = read_flash_file(copts->file, &buf, &size)) != 0)
        return ret;

    if (mic_open_device(&device, selected_card) != 0) {
        error_msg_start("Failed to open card '%d': %s: %s\n",
                        selected_card, mic_get_error_string(),
                        strerror(errno));
        free(buf);
        return errno == ENOENT ?  NODEV_ERR : DEVOPEN_ERR;
    }

    if ((ret = verify_image(copts->file, device, buf, size,
                            &flash_image, 1)) != 0) {
        if (ret == IMAGE_MISMATCH) {
            if (cmdopts.line_mode_flag) {
                printf("Image check: Invalid image\n");
            } else {
                printf("%s: %s: Invalid image\n",
                       mic_get_device_name(device), copts->file);
            }
        } else if (ret < 0) {
            ret = -ret;
        }
        free(buf);
        mic_close_device(device);
        return ret;
    }
    free(buf);

    if (cmdopts.smc_ok)
        flash_image = 1;
    ret = flash_image ? 0 : 1;
    if (cmdopts.line_mode_flag) {
        printf("Image check: %s\n", flash_image ? "Valid image" :
               "Invalid image");
    } else {
        printf("%s: %s: %s\n",
               mic_get_device_name(device), copts->file,
               flash_image ? "Valid image" : "Invalid image");
    }

    (void)mic_close_device(device);
    return ret;
}
