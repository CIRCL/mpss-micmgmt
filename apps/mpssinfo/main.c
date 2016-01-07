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
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <libgen.h>
#include <getopt.h>
#else
#ifdef WINDOWS
#include "os_getopt.h"
#endif
#endif
#include <miclib.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#include "main.h"
#include "mic.h"
#include "help.h"
#include "helper.h"

#ifdef WINDOWS

#ifdef MPSS_VERSION_WIN
#define MY_VERSION MPSS_VERSION_WIN
#else
#define MY_VERSION "0.0.0.0"
#endif

#else

#ifdef MPSS_VERSION
#define MY_VERSION MPSS_VERSION
#else
#define MY_VERSION "0.0.0.0"
#endif

#endif

#ifndef MPSS_VERSION
#define MPSS_VERSION    "0.0.0.0"
#endif
#define MIC_COPYRIGHT \
    "Copyright (c) 2015, Intel Corporation."
extern int host_info();
extern int is_current_user_valid();

/* global variables */
int posix = 1;
#ifdef WINDOWS
static struct option long_options[6] =
{
    /* Help request */
    { "help",     optional_argument, NULL, 'h' },
    /* Display tool version */
    { "versions", no_argument,       NULL, 'v' },
    /* List devices */
    { "list",     no_argument,       NULL, 'l' },
    /* Show specific device only */
    { "device",   required_argument, NULL, 'd' },
    /* Specify a group (requires -d option) */
    { "group",    required_argument, NULL, 'g' },
    { NULL,       0,                 NULL, 0   }
};



#endif
enum e_action_list {
    e_version,
    e_host,
    e_list,
    e_device,
    e_group,
    e_help
};

struct args_list {
    enum e_action_list func;
    char *             device;
    char *             group;
    char *             nogui;
    char *             help;
    int                group_id;
    int                dev_id;
    int                version_arg_cnt;
    int                host_arg_cnt;
    int                list_arg_cnt;
    int                device_arg_cnt;
    int                group_arg_cnt;
    int                help_arg_cnt;
};

int show_version(struct args_list *actions)
{
    if (actions->func == e_version) {
        log_msg_start("VERSION: %s\n", MY_VERSION);
        log_msg_start("%s\n\n", MIC_COPYRIGHT);
    }
    return 0;
}

int show_device(struct args_list *actions)
{
    int ret = 0;

    ret = host_info();
    if (actions->device_arg_cnt == 0)
        ret = show_all(actions->dev_id, actions->group_id);
    else
        ret = mic_info(actions->dev_id, actions->group_id);
    return ret;
}

int check_group(char *group, struct args_list *actions)
{
    if (!strncasecmp(group, GRP_ALL_STR, strlen(group)))
        actions->group_id = GRP_ALL;
    else if (!strncasecmp(group, GRP_VER_STR, strlen(group)))
        actions->group_id = GRP_VER;
    else if (!strncasecmp(group, GRP_BOARD_STR, strlen(group)))
        actions->group_id = GRP_BOARD;
    else if (!strncasecmp(group, GRP_CORES_STR, strlen(group)))
        actions->group_id = GRP_CORES;
    else if (!strncasecmp(group, GRP_THER_STR, strlen(group)))
        actions->group_id = GRP_THER;
    else if (!strncasecmp(group, GRP_GDDR_STR, strlen(group)))
        actions->group_id = GRP_GDDR;
    else
        return -1;
    return 0;
}

int dispatch(struct args_list *actions)
{
    int ret = 0;
    time_t rawtime;
    int valid_user; /* 1 = true; 0 = false */

    valid_user = is_current_user_valid();

    if (actions->version_arg_cnt > 1 || actions->host_arg_cnt > 1
        || actions->list_arg_cnt > 1 || actions->device_arg_cnt > 1
        || actions->group_arg_cnt > 1 || actions->help_arg_cnt > 1) {
        error_msg_start("Options are specified more than once \n\n");
        actions->func = e_help;
        ret = 1;
        return ret;
    }
    if (actions->version_arg_cnt && (actions->host_arg_cnt
                                     || actions->list_arg_cnt
                                     || actions->device_arg_cnt
                                     || actions->group_arg_cnt
                                     || actions->help_arg_cnt)) {
        error_msg_start("Invalid combination of options \n\n");
        actions->func = e_help;
        ret = 1;
        return ret;
    }
    if (actions->list_arg_cnt && (actions->host_arg_cnt
                                  || actions->version_arg_cnt
                                  || actions->device_arg_cnt
                                  || actions->group_arg_cnt
                                  || actions->help_arg_cnt)) {
        error_msg_start("Invalid combination of options \n\n");
        actions->func = e_help;
        ret = 1;
        return ret;
    }
    if (actions->help_arg_cnt && (actions->host_arg_cnt
                                  || actions->version_arg_cnt
                                  || actions->device_arg_cnt
                                  || actions->group_arg_cnt
                                  || actions->list_arg_cnt)) {
        error_msg_start("Invalid combination of options \n\n");
        actions->func = e_help;
        ret = 1;
        return ret;
    }
    time(&rawtime);
    if (strncasecmp(progname, "micinfo", strlen("micinfo")) == 0)
        log_msg_start("MicInfo Utility Log\n");
    else
        log_msg_start("MpssInfo Utility Log\n");
    log_msg_start("Created %s\n", ctime(&rawtime));

    switch (actions->func) {
    case e_help:
        log_msg_start("%s\n", MIC_COPYRIGHT);
        show_help(actions->help);
        break;
    case e_host:
        ret = host_info();
        break;
    case e_list:
        if(valid_user) {
            ret = show_list();
        } else {
            error_msg_start("The current user is not authorized to execute this utility\n\n");
            ret = 1;
        }
        break;
    case e_device:
    case e_group:
        if(valid_user) {
            ret = show_device(actions);
        } else {
            error_msg_start("The current user is not authorized to execute this utility\n\n");
            ret = 1;
        }
        break;
    case e_version:
        ret = show_version(actions);
        break;
    default:
        break;
    }
    return ret;
}

int process_args(int argc, char *argv[])
{
    char *p = NULL;
    int indx = 0;
    struct args_list actions;
    int error_cnt = 0;
    char *endptr = NULL;

    actions.version_arg_cnt = 0;
    actions.host_arg_cnt = 0;
    actions.list_arg_cnt = 0;
    actions.device_arg_cnt = 0;
    actions.group_arg_cnt = 0;
    actions.help_arg_cnt = 0;
    actions.func = e_device;
    actions.dev_id = -1;
    actions.group_id = 0;
    actions.help = NULL;

    for (indx = 1; indx < argc; indx++) {
        p = (char *)argv[indx];
        if (*p == '-') {
            p++;

            switch (*p) {
            case 'l':
            case 'L':
                if (strncasecmp(p, "listDevices",
                                strlen(p)) != 0) {
                    error_msg_start(
                        "Invalid Argument %s\n\n", p);
                    actions.func = e_help;
                    error_cnt = 1;
                } else {
                    actions.func = e_list;
                    actions.list_arg_cnt++;
                }
                break;
            case 'v':
            case 'V':
                if (strncasecmp(p, "versions",
                                strlen(p)) != 0) {
                    error_msg_start(
                        "Invalid Argument %s\n\n", p);
                    actions.func = e_help;
                    error_cnt = 1;
                } else {
                    actions.func = e_version;
                    actions.version_arg_cnt++;
                }
                break;
            case 'd':
            case 'D':
                if (strncasecmp(p, "deviceInfo",
                                strlen(p)) != 0) {
                    error_msg_start(
                        "Invalid Argument %s\n\n", p);
                    actions.func = e_help;
                    error_cnt = 1;
                } else {
                    actions.func = e_device;
                    actions.device_arg_cnt++;
                    indx++;
                    if (argv[indx] == NULL) {
                        error_msg_start
                        (
                            "Missing required argument for option: -device\n\n");
                        actions.func = e_help;
                        error_cnt = 1;
                    } else {
                        if (strcasecmp(argv[indx],
                                       "all")
                            ==
                            0) {
                            actions.device_arg_cnt
                                = 0;
                        } else {
                            errno = 0;
                            actions.dev_id = strtol(
                                argv[indx],
                                &endptr, 10);
                            if (errno || actions.dev_id < 0) {
                                error_msg_start
                                (
                                    "Invalid device %s\n\n",
                                    argv[
                                        indx
                                    ]);
                                actions.func =
                                    e_help;
                                error_cnt = 1;
                            }
                            if (endptr ==
                                argv[indx] ||
                                *endptr !=
                                '\0') {
                                error_msg_start(
                                    "Invalid device %s\n\n",
                                    argv
                                    [indx]);
                                actions.func =
                                    e_help;
                                error_cnt = 1;
                            }
                        }
                    }
                }
                break;
            case 'g':
            case 'G':
                if (strncasecmp(p, "group", strlen(p)) != 0) {
                    error_msg_start(
                        "Invalid Argument %s\n\n", p);
                    actions.func = e_help;
                    error_cnt = 1;
                } else {
                    actions.func = e_group;
                    actions.group_arg_cnt++;
                    indx++;
                    if (argv[indx] == NULL) {
                        error_msg_start
                        (
                            "Missing required argument for option: -group\n\n");
                        actions.func = e_help;
                        error_cnt = 1;
                    } else if (check_group(argv[indx],
                                           &actions) !=
                               0) {
                        error_msg_start
                        (
                            "Invalid group name %s\n\n",
                            argv[indx]);
                        actions.func = e_help;
                        error_cnt = 1;
                    }
                }
                break;
            case 'h':
            case 'H':
                if (strncasecmp(p, "help", strlen(p)) != 0) {
                    error_msg_start(
                        "Invalid Argument %s\n\n", p);
                    actions.func = e_help;
                    error_cnt = 1;
                } else {
                    actions.func = e_help;
                    actions.help = NULL;
                    actions.help_arg_cnt++;
                    indx++;
                    if (argv[indx] != NULL
                        && (!strncasecmp((char *)argv[indx],
                                         (char *)
                                         "listDevices",
                                         strlen(argv[indx]))
                            || !strncasecmp((char *)
                                            argv[indx],
                                            (char *)
                                            "deviceInfo",
                                            strlen(argv
                                                   [indx]))
                            || !strncasecmp((char *)
                                            argv[indx],
                                            (char *)
                                            "versions",
                                            strlen(argv[
                                                       indx
                                                   ]))))
                        actions.help =
                            (char *)argv[indx];
                }
                break;
            default:
                error_msg_start("Unknown option %s\n\n",
                                argv[indx]);
                actions.func = e_help;
                error_cnt = 1;
            }
        } else {
            error_msg_start("Unknown option %s\n\n",
                            argv[indx]);
            actions.func = e_help;
            error_cnt = 1;
        }
    }
    if (error_cnt)
        return 1;

    return dispatch(&actions);
}

int main(int argc, char *argv[])
{
    int arg;
    const char *opt_string = ":h::lGd:g:";
    int indexptr = 0;
    char *endptr = NULL;
    struct args_list actions;
    int error_cnt = 0;

    actions.version_arg_cnt = 0;
    actions.host_arg_cnt = 0;
    actions.list_arg_cnt = 0;
    actions.device_arg_cnt = 0;
    actions.group_arg_cnt = 0;
    actions.help_arg_cnt = 0;

    progname = (char *)basename(argv[0]);
    actions.func = e_device;
    actions.dev_id = -1;
    actions.group_id = 0;
    actions.help = NULL;
    posix = 1;
#ifdef __linux__
    static int NO_MORE_ARGUMENTS = -1;
    static struct option long_options[] =
    {
        /* Help request */
        { "help",     optional_argument, NULL, 'h' },
        /* Display tool version */
        { "versions", no_argument,       NULL, 'v' },
        /* List devices */
        { "list",     no_argument,       NULL, 'l' },
        /* Show specific device only */
        { "device",   required_argument, NULL, 'd' },
        /* Specify a group (requires -d option) */
        { "group",    required_argument, NULL, 'g' },
        { NULL,       0,                 NULL, 0   }
    };
#endif

    if (strncasecmp(progname, "micinfo",strlen("micinfo")) == 0) {
        posix = 0;
        return process_args(argc, argv);
    }
    while (!error_cnt && (arg = getopt_long(argc, argv, opt_string,
                                            long_options,
                                            &indexptr)) !=
           NO_MORE_ARGUMENTS) {
        if ((optarg == NULL) &&
            (long_options[indexptr].has_arg == required_argument)) {
            error_msg_start(
                "Error when trying to get arguments!\n\n");
            return -1;
        }


        switch (arg) {
        case 'h':
            actions.func = e_help;

            if (optind < argc)
                optarg = argv[optind++];
            if (optarg != NULL
                && (!strncasecmp((char *)optarg, (char *)"list",
                                 strlen(optarg))
                    || !strncasecmp((char *)optarg,
                                    (char *)"device",
                                    strlen(optarg))
                    || !strncasecmp((char *)optarg,
                                    (char *)"versions",
                                    strlen(optarg))))
                actions.help = (char *)optarg;
            actions.help_arg_cnt++;
            break;

        case 'v':
            actions.func = e_version;
            actions.version_arg_cnt++;
            break;

        case 'l':
            actions.func = e_list;
            actions.list_arg_cnt++;
            break;

        case 'd':
            actions.func = e_device;
            actions.device = optarg;
            actions.device_arg_cnt++;
            if (optarg == NULL) {
                error_msg_start
                (
                    "Missing required argument for option: --device\n\n");
                actions.func = e_help;
                error_cnt = 1;
            } else {
                if (strcasecmp(optarg, "all")
                    == 0) {
                    actions.device_arg_cnt = 0;
                } else {
                    errno = 0;
                    actions.dev_id = strtol(optarg, &endptr,
                                            10);
                    if (errno || actions.dev_id < 0) {
                        error_msg_start(
                            "Invalid device %s\n\n",
                            optarg);
                        actions.func = e_help;
                        error_cnt = 1;
                    }
                    if (endptr == optarg || *endptr !=
                        '\0') {
                        error_msg_start(
                            "Invalid device %s\n\n",
                            optarg);
                        actions.func = e_help;
                        error_cnt = 1;
                    }
                }
            }
            break;

        case 'g':
            actions.func = e_group;
            actions.group_arg_cnt++;
            if (optarg == NULL) {
                error_msg_start
                (
                    "Missing required argument for option: --group\n\n");
                actions.func = e_help;
                error_cnt = 1;
            } else if (check_group(optarg, &actions) != 0) {
                error_msg_start("Invalid group name %s\n\n",
                                optarg);
                actions.func = e_help;
                error_cnt = 1;
            }
            break;

        case '?':
            error_msg_start("Unknown option %s\n\n",
                            argv[optind - 1]);
            actions.func = e_help;
            error_cnt = 1;
            break;

        case ':':
            error_msg_start
            (
                "Missing required argument for option: %s \n\n",
                argv[optind - 1]);
            actions.func = e_help;
            error_cnt = 1;
            break;

        default:
            error_msg_start("Unknown option %s\n\n",
                            argv[optind - 1]);
            actions.func = e_help;
            error_cnt = 1;
            break;
        }
    }
    if (optind < argc) {
        error_msg_start("Unknown option %s\n\n",
                        argv[optind]);
        error_cnt = 1;
    }
    if (error_cnt)
        return 1;
    return dispatch(&actions);
}
