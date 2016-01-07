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

/*
 * mpssflash - A simple wrapper that can be used to perform flash operations on
 * multiple cards. It makes use of flash1 that operates only on a single card.
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
#include <libgen.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <wait.h>
#include <regex.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>
#include "helper.h"
#include <miclib.h>

extern int flash1_start(int, char *[]);

#define ASSERT            assert

#define ARG_USED(arg)    while ((void)arg, 0)

#define TEST_OPTION       (1)
#define SMC_BOOTLOADER    (2)

static struct option options[] = {
    { "device",        1, NULL, 'd'            },
    { "file",          1, NULL, 'f'            },
    { "read",          1, NULL, 'r'            },
    { "help",          0, NULL, 'h'            },
    { "force",         0, NULL, 'F'            },
    { "verbose",       0, NULL, 'v'            },
    { "test",          0, NULL, TEST_OPTION    },
    { "smcbootloader", 0, NULL, SMC_BOOTLOADER },
    { NULL,            0, 0,    0              }
};

struct flashopts {
    int   help_flag;
    int   device_flag;
    int   force_flag;
    int   file_flag;
    int   line_mode_flag;
    int   test_mode_flag;
    int   verbose;
    int   smc_bootloader;
    char *device;
    char *file;
} flashopts;

typedef int (exec_func (int, char *[]));
typedef int (init_cmd_func (int *, int, char *[]));
typedef char ** (build_cmd_func (int, char *, int *));
struct proc_state;
typedef void * (setup_cmd_func (void *, int));
typedef void (done_cmd_func (void *));

struct miccmd_actions;
typedef int (miccmd_func (struct miccmd_actions *, int *, int, char *[]));

static miccmd_func common_flash_cmd;

static build_cmd_func build_update_args;
static build_cmd_func build_version_args;
static build_cmd_func build_read_args;
static build_cmd_func build_check_args;
static build_cmd_func build_device_args;

static init_cmd_func init_update;
static init_cmd_func init_version;
static init_cmd_func init_read;
static init_cmd_func init_check;
static init_cmd_func init_device;

static setup_cmd_func get_update_image;
static done_cmd_func free_update_image;

struct miccmd_actions {
    char *          cmd_name;
    miccmd_func *   cmd_func;
    init_cmd_func * init_func;
    build_cmd_func *build_func;
    setup_cmd_func *setup_func;
    done_cmd_func * done_func;
    char *          cmd_options;
    char *          cmd_help;
};

static struct miccmd_actions actions[] = {
    {
        "update", common_flash_cmd, init_update,
        build_update_args, get_update_image, free_update_image,
        "[{--device|-d} {<device>|<device-list>|all}] "
        "[{--file|-f} <file>] "
        "[{--verbose|-v}] "
        "[--smcbootloader] "
        "[{--force|-F}] update",
        "update flash image using specified file"
    },
    {
        "version", common_flash_cmd, init_version,
        build_version_args, NULL, NULL,
        "[{--device|-d} {<device>|<device-list>|all}] "
        "{--file|-f} <file> version",
        "read version from flash or given file"
    },
    {
        "read", common_flash_cmd, init_read,
        build_read_args, NULL, NULL,
        "[{--device|-d} <device>] "
        "{--file|-f} <file> read",
        "read contents of flash into the specified file"
    },
    {
        "device", common_flash_cmd, init_device,
        build_device_args, NULL, NULL,
        "[{--device|-d} {<device>|<device-list>|all}] device",
        "get vendor and device type information"
    },
    {
        "check", common_flash_cmd, init_check,
        build_check_args, NULL, NULL,
        "[{--device|-d} {<device>|<device-list>|all}] "
        "{--file|-f} <file> check",
        "check if specified file is compatible with the card"
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

struct proc_state {
    int n_procs;
    struct {
        int   devnum;
        pid_t pid;
    }   proc_i[1];
};

static char *image_path;
struct proc_state *procinfo;
extern char *smc_bl;
extern char *flash_dir;

static char *devnum_to_str(int devnum)
{
    static char devstr[sizeof(int) * 2 + 3];     /* '0', 'x' followed by
                                                  * hexa and null-terminator */

    snprintf(devstr, sizeof(devstr), "0x%x", devnum);
    return devstr;
}

static pid_t exec_cmd(exec_func *entry, int argc, char *argv[])
{
    pid_t pid;

    fflush(stdout);
    fflush(stderr);
    if ((pid = fork()) == 0)
        exit((*entry)(argc, argv));

    return pid;
}

static int get_num(char *buf)
{
    char *more;
    long l;

    l = get_numl(buf, &more);
    if (*more || errno)
        return -1;

    return (int)l;
}

int run_cmd(exec_func *func, int argc, char *argv[], int quiet)
{
    pid_t pid;
    int status, fd;

    fflush(stdout);
    fflush(stderr);
    if ((pid = fork()) == 0) {
        if (quiet) {
            if ((fd = open("/dev/null", O_RDWR)) < 0) {
                fprintf(stderr, "open: /dev/null: %s\n",
                        strerror(errno));
                return -1;
            }
            if ((dup2(fd, 1) < 0) || (dup2(fd, 2) < 0)) {
                fprintf(stderr, "dup2: /dev/null: %s\n",
                        strerror(errno));
                return -1;
            }
        }
        exit((*func)(argc, argv));
    }

    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return 1;
}

static int flash2_start(int argc, char *argv[])
{
    int file_opt_index = 3;
    int device_opt_index = 1;
    int i, ret, devnum;
    char *bl_argv[argc + 2];

    int get_num(char *);
    char *image;


    assert(argc >= 6);
    assert(strcmp(argv[file_opt_index], "--file") == 0);
    file_opt_index++;
    image = argv[file_opt_index];

    assert(strcmp(argv[device_opt_index], "--device") == 0);
    device_opt_index++;
    devnum = get_num(argv[device_opt_index]);

    if (check_smc_bootloader_image(image_path, devnum) < 0)
        return 1;

    for (i = 0; i < (argc - 1); i++)
        bl_argv[i] = argv[i];
    bl_argv[i++] = "--smcbootloader";
    bl_argv[i++] = "update";
    bl_argv[file_opt_index] = smc_bl;

    printf("SMC boot-loader image: %s\n", smc_bl);
    if ((ret = run_cmd(flash1_start, i, bl_argv, 0)) != 0)
        return ret;

    i--;
    bl_argv[file_opt_index] = image;
    bl_argv[i - 1] = "update";
    if ((ret = run_cmd(flash1_start, i, bl_argv, 0)) != 0)
        return ret;

    return 0;
}

static int start_cmd(struct miccmd_actions *ca, void *arg, int *index, int argc,
                     char *argv[])
{
    int i, n, n_done, nargs, ret;
    void *desc = NULL;
    pid_t pid;
    char **args;
    exec_func *func;

    n = procinfo->n_procs;
    n_done = 0;
    if ((ret = ca->init_func(index, argc, argv)) != 0)
        return n_done;

    for (i = 0; i < n; i++) {
        if (ca->setup_func != NULL) {
            if ((desc = ca->setup_func(arg, i)) == NULL)
                continue;
        }

        /* Build cmd args for the low level flash command */
        if ((args = ca->build_func(procinfo->proc_i[i].devnum,
                                   desc !=
                                   NULL ? desc : (void *)(flashopts.
                                                          file),
                                   &nargs))
            == NULL)
            continue;

        func = flashopts.smc_bootloader ? flash2_start : flash1_start;
        pid = exec_cmd(func, nargs, args);

        free(args);
        if (ca->done_func != NULL)
            ca->done_func(desc);

        if (pid == (pid_t)-1) {
            procinfo->proc_i[i].pid = 0;
            continue;
        } else {
            procinfo->proc_i[i].pid = pid;
        }

        n_done++;
    }

    return n_done;
}

static struct proc_state *get_devices_list(char *arg, int device_flag)
{
    struct mic_devices_list *md = NULL;
    int n;
    int size;
    char *p, *q, *more, delim;
    struct proc_state *procs;
    int i, d;

    if ((arg == NULL) || !strcmp(arg, "all")) {
        if (!device_flag) {
            n = 1;
        } else if ((mic_get_devices(&md) != E_MIC_SUCCESS) ||
                   (mic_get_ndevices(md, &n) != E_MIC_SUCCESS)) {
            error_msg_start("Failed to get cards info: %s: %s\n",
                            mic_get_error_string(), strerror(errno));
            return NULL;
        }

        UT_INSTRUMENT_EVENT("MPSSFLASH_DEVICES_LIST_0", n = 2);
        UT_INSTRUMENT_EVENT("MPSSFLASH_DEVICES_LIST_1", n = 1);
        if (n > 1) {
            UT_INSTRUMENT_EVENT("MPSSFLASH_DEVICES_LIST_0", n = 1);
            if (arg == NULL) {
                error_msg_start(
                    "%d cards present - "
                    "please specify one of following: ",
                    n);
                for (i = 0; i < n - 1; i++) {
                    (void)mic_get_device_at_index(md, i, &d);
                    error_msg_cont("%d, ", d);
                }
                (void)mic_get_device_at_index(md, i, &d);
                error_msg_cont("%d\n", d);

                mic_free_devices(md);
                return NULL;
            }
        }
        procs = (struct proc_state *)calloc(1,
                                            sizeof(struct proc_state) +
                                            (n -
                                             1) *
                                            sizeof(procs->proc_i[0]));
        UT_INSTRUMENT_EVENT("MPSSFLASH_DEVICES_LIST_1",
                            (free(procs), procs = NULL));

        if (procs == NULL) {
            error_msg_start("Out of memory\n");
            mic_free_devices(md);
            return NULL;
        }
        procs->n_procs = n;

        if (device_flag) {
            for (i = 0; i < n; i++) {
                (void)mic_get_device_at_index(
                    md, i,
                    &procs->proc_i[i]
                    .devnum);
            }
            mic_free_devices(md);
        } else {
            procs->proc_i[0].devnum = -1;
        }
    } else {
        size = strlen(arg) + 1;
        q = p = malloc(size);

        UT_INSTRUMENT_EVENT("MPSSFLASH_DEVICES_LIST_2",
                            (free(p), p = q = NULL));

        if (p == NULL) {
            error_msg_start("Out of memory\n");
            return NULL;
        }
        strncpy(p, arg, size);
        p[size - 1] = '\0';

        n = 0;
        delim = ',';
        while (strsep(&p, &delim) != NULL)
            n++;
        free(q);

        procs = (struct proc_state *)calloc(1,
                                            sizeof(struct proc_state) +
                                            (n -
                                             1) *
                                            sizeof(procs->proc_i[0]));

        UT_INSTRUMENT_EVENT("MPSSFLASH_DEVICES_LIST_3",
                            (free(procs), procs = NULL));

        if (procs == NULL) {
            error_msg_start("Out of memory\n");
            return NULL;
        }
        procs->n_procs = n;
        p = arg;
        i = 0;
        while (strsep(&arg, &delim) != NULL) {
            procs->proc_i[i].devnum = get_numl(p, &more);
            if (*more || errno) {
                free(procs);
                error_msg_start("Error in devices list: "
                                "'%s'\n", p);
                return NULL;
            }
            i++;
            p = arg;
        }
    }

    return procs;
}

static char file_opt[] = "--file";
static char device_opt[] = "--device";
static char test_opt[] = "--test";

static int init_update(int *index, int argc, char *argv[])
{
    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of update command\n");
        return 1;
    }

    return 0;
}

static int check_image(int proc_slot, char *file, int devnum, int smc_ok)
{
    char *check_args[] = {
        NULL,
        "--device",
        "0",
        "--file",
        file,
        "check",
        NULL
    };
    int check_nargs = sizeof(check_args) / sizeof(char *) - 1;
    int status;

    ARG_USED(proc_slot);

    /*
     * Using magic numbers below. Make sure that the arg[1] is '--device'
     * and arg[3] is '--file', so can substitute device number into arg[2]
     * and filename into arg[4].
     */
    ASSERT(strcmp(check_args[1], "--device") == 0);
    ASSERT(strcmp(check_args[3], "--file") == 0);

    check_args[0] = progname;
    if (smc_ok) {
        check_args[check_nargs - 1] = "--smcok";
        check_args[check_nargs++] = "check";
    }

    check_args[2] = devnum_to_str(devnum);
    status = run_cmd(flash1_start, check_nargs, check_args, 1);

    return status;
}

static char *find_image(int proc_slot, char *path)
{
    char *p;
    struct dirent **namelist;
    int n;
    int ret, devnum;

    devnum = procinfo->proc_i[proc_slot].devnum;

    n = scandir(path, &namelist, 0, alphasort);
    if (n < 0) {
        fprintf(stderr, "mic%d: %s: scandir: %s\n",
                devnum, path, strerror(errno));
        return NULL;
    }

    /*
     * We check files in reverse order because we want to use .rom.smc,
     * in case one is present, instead of .rom.
     */
    while (n--) {
        p = malloc(strlen(path) + strlen(namelist[n]->d_name) + 2);
        if (p == NULL) {
            fprintf(stderr, "mic%d: Out of memory\n", devnum);
            return NULL;
        }
        sprintf(p, "%s/%s", path, namelist[n]->d_name);

        free(namelist[n]);
        if ((ret = check_image(proc_slot, p, devnum, 0)) < 0) {
            free(p);
            while (n--)
                free(namelist[n]);
            free(namelist);
            return NULL;
        } else if (ret == 0) {
            while (n--)
                free(namelist[n]);
            free(namelist);
            return p;
        }

        free(p);
    }

    free(namelist);
    return NULL;
}

static void
*get_update_image(void *arg, int proc_slot)
{
    char *path = (char *)arg;
    char *image_file = NULL;
    struct stat sbuf;
    int len, devnum;
    struct mic_device *mdh;

    devnum = procinfo->proc_i[proc_slot].devnum;
    /*
     * Make sure that the device exists.
     */
    if (mic_open_device(&mdh, devnum) != E_MIC_SUCCESS) {
        fprintf(stderr, "failed to open mic'%d': %s: %s\n",
                devnum, mic_get_error_string(),
                strerror(errno));
        return NULL;
    }
    (void)mic_close_device(mdh);

    if (path == NULL) {
        /* Print this message only once */
        if (proc_slot == 0) {
            printf("No image path specified - "
                   "Searching: %s\n", flash_dir);
        }
        image_path = flash_dir;
        path = flash_dir;
    }

    if (stat(path, &sbuf) != 0) {
        fprintf(stderr, "mic%d: %s: %s\n", devnum, path,
                strerror(errno));
        return NULL;
    }

    if (!S_ISDIR(sbuf.st_mode)) {
        if (!S_ISREG(sbuf.st_mode)) {
            fprintf(stderr, "mic%d: %s: not a regular file\n",
                    devnum, path);
            return NULL;
        }

        if (check_image(proc_slot, path, devnum, 1) == 0) {
            len = strlen(path) + 1;
            image_file = malloc(len);
            if (image_file == NULL) {
                fprintf(stderr, "mic%d: Out of memory\n",
                        devnum);
                return NULL;
            }
            strncpy(image_file, path, len);
            image_file[len - 1] = '\0';
        }

        image_path = dirname(path);
    } else {
        image_file = find_image(proc_slot, path);
        image_path = path;
    }

    if (image_file == NULL)
        fprintf(stderr, "mic%d: No valid image found\n", devnum);
    else
        printf("mic%d: Flash image: %s\n", devnum, image_file);

    return (void *)image_file;
}

static void free_update_image(void *arg)
{
    free(arg);
}

static char **build_update_args(int device, char *image, int *nargs)
{
    static char oldimage_opt[] = "--oldimage";
    static char verbose_opt[] = "-v";
    static char update_cmd[] = "update";
    static char **update_args;
    int argnum;

    /*
     * 'flash1 --device <dev> --file <file> [--oldimage] [--test]
     *  update'
     */
    *nargs = 6;
    if (flashopts.force_flag != 0)
        (*nargs)++;
    if (flashopts.test_mode_flag)
        (*nargs)++;
    if (flashopts.verbose)
        (*nargs)++;

    update_args = (char **)malloc(sizeof(char *) * *nargs);
    if (update_args == NULL) {
        error_msg_start("Out of memory\n");
        return NULL;
    }

    update_args[0] = progname;
    update_args[1] = device_opt;
    update_args[2] = devnum_to_str(device);
    update_args[3] = file_opt;
    update_args[4] = image;
    argnum = 5;
    if (flashopts.force_flag != 0)
        update_args[argnum++] = oldimage_opt;
    if (flashopts.test_mode_flag) {
        update_args[argnum++] = test_opt;;
    }
    if (flashopts.verbose) {
        update_args[argnum++] = verbose_opt;;
    }
    update_args[argnum] = update_cmd;

    return update_args;
}

static int check_for_update_options(void)
{
    if (flashopts.force_flag) {
        error_msg_start("'{--force | -f}': Invalid option\n");
        return -1;
    }

    return 0;
}

static int init_version(int *index, int argc, char *argv[])
{
    int ret;

    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of version command\n");
        return 1;
    }

    if ((ret = check_for_update_options()) < 0)
        return ret;

    if ((flashopts.device != NULL) && flashopts.file != NULL) {
        error_msg_start("Only one of device or image-file "
                        "may be specified\n");
        return -11;
    }

    return 0;
}

static char **build_version_args(int device, char *image, int *nargs)
{
    char **version_args;
    static char version_cmd[] = "version";
    int argnum;

    /*
     * flash1 --file <file> version
     * flash1 --device <device> version
     */
    *nargs = 4;

    if (flashopts.test_mode_flag)
        (*nargs)++;

    version_args = (char **)malloc(sizeof(char *) * *nargs);
    if (version_args == NULL) {
        error_msg_start("Out of memory\n");
        return NULL;
    }

    version_args[0] = progname;
    if (image != NULL) {
        version_args[1] = file_opt;
        version_args[2] = image;
    } else {
        version_args[1] = device_opt;
        version_args[2] = devnum_to_str(device);
    }
    argnum = 3;

    if (flashopts.test_mode_flag) {
        version_args[argnum++] = test_opt;;
    }
    version_args[argnum] = version_cmd;

    return version_args;
}

static int init_read(int *index, int argc, char *argv[])
{
    int ret;

    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of read command\n");
        return 1;
    }

    if ((ret = check_for_update_options()) < 0)
        return ret;

    if (flashopts.file == NULL) {
        error_msg_start("Please specify a filename to save image\n");
        return 1;
    }

    if (procinfo->n_procs > 1) {
        error_msg_start("Only one device may be specified "
                        "for reading\n");
        return 1;
    }

    return 0;
}

static char **build_read_args(int device, char *image, int *nargs)
{
    char **read_args;
    static char read_cmd[] = "read";
    int argnum;

    if(device < 0)
    {
        device  = 0;
    }

    /*
     * flash1 --file <file> --device <device> read
     */
    *nargs = 6;
    if (flashopts.test_mode_flag)
        (*nargs)++;

    read_args = (char **)malloc(sizeof(char *) * *nargs);
    if (read_args == NULL) {
        error_msg_start("Out of memory\n");
        return NULL;
    }

    read_args[0] = progname;
    read_args[1] = file_opt;
    read_args[2] = image;
    read_args[3] = device_opt;
    read_args[4] = devnum_to_str(device);
    argnum = 5;
    if (flashopts.test_mode_flag) {
        read_args[argnum++] = test_opt;;
    }
    read_args[argnum] = read_cmd;

    return read_args;
}

static int init_check(int *index, int argc, char *argv[])
{
    int ret;

    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of check command\n");
        return 1;
    }

    if ((ret = check_for_update_options()) < 0)
        return ret;


    if (flashopts.file == NULL) {
        error_msg_start("Please specify a filename\n");
        return 1;
    }

    return 0;
}

static char **build_check_args(int device, char *image, int *nargs)
{
    char **check_args;
    static char check_cmd[] = "check";
    int argnum;

    if(device < 0)
    {
        device  = 0;
    }

    /*
     * flash1 --file <file> --device <device> check
     */
    *nargs = 6;
    if (flashopts.test_mode_flag)
        (*nargs)++;

    check_args = (char **)malloc(sizeof(char *) * *nargs);
    if (check_args == NULL) {
        error_msg_start("Out of memory\n");
        return NULL;
    }

    check_args[0] = progname;
    check_args[1] = file_opt;
    check_args[2] = image;
    check_args[3] = device_opt;
    check_args[4] = devnum_to_str(device);
    argnum = 5;
    if (flashopts.test_mode_flag) {
        check_args[argnum++] = test_opt;;
    }
    check_args[argnum] = check_cmd;

    return check_args;
}

static int init_device(int *index, int argc, char *argv[])
{
    int ret;

    ARG_USED(argv);

    if (*index < argc) {
        error_msg_start("Extra args at the end of device command\n");
        return 1;
    }

    if ((ret = check_for_update_options()) < 0)
        return ret;


    if (flashopts.file != NULL) {
        error_msg_start("<file-name> not needed\n");
        return 1;
    }

    return 0;
}

static char **build_device_args(int device, char *image, int *nargs)
{
    char **device_args;
    static char device_cmd[] = "device";
    int argnum;

    ARG_USED(image);

    /*
     * flash1 --device <device> device
     */
    *nargs = 4;
    if (flashopts.test_mode_flag)
        (*nargs)++;

    device_args = (char **)malloc(sizeof(char *) * *nargs);
    if (device_args == NULL) {
        error_msg_start("Out of memory\n");
        return NULL;
    }

    device_args[0] = progname;
    device_args[1] = device_opt;
    device_args[2] = devnum_to_str(device);
    argnum = 3;
    if (flashopts.test_mode_flag) {
        device_args[argnum++] = test_opt;;
    }
    device_args[argnum] = device_cmd;

    return device_args;
}

static int process_common_opts(struct miccmd_actions *ca)
{
    int cmd_version = !strcmp(ca->cmd_name, "version");
    int cmd_update  = !strcmp(ca->cmd_name, "update");
    int cmd_check   = !strcmp(ca->cmd_name, "check");
    //device_flag is set to 1, get_devices_list() will look for all the installed MIC devices.
    //device_flag is set to 0, get_devices_list() will not search any MIC devices.
    //   Command     Image      device_flag
    //   -------     ------     -----------
    //   check       YES/NO        SET
    //   version      YES        NOT SET
    //   version      NO           SET
    //   update      YES/NO        SET
    int device_flag = ((cmd_version && !flashopts.file) || cmd_check || cmd_update);

    if ((procinfo = get_devices_list(flashopts.device, device_flag)) == NULL)
        return errno == ENOMEM ? -3 : -1;
    return 0;
}

static void free_common(void)
{
    if (procinfo != NULL)
        free(procinfo);
}

static int common_flash_cmd(struct miccmd_actions *ca, int *index, int argc,
                            char *argv[])
{
    int ret, status;
    int n_done;

    if ((ret = process_common_opts(ca)) < 0)
        return -ret;

    /* Start operation for each device in the specified list. */
    n_done = start_cmd(ca, flashopts.file, index, argc, argv);

    if (n_done == 0) {
        free_common();
        return 1;
    }

    ret = 0;
    while (waitpid(-1, &status, 0) > 0) {
        if ((ret == 0) && WIFEXITED(status))
            ret = WEXITSTATUS(status);
    }

    free_common();
    return ret;
}

struct miccmd_actions *parse_cmd(struct miccmd_actions *cactions, int *index,
                                 int argc, char *argv[])
{
    int option_list = *index;
    struct miccmd_actions *p;

    if (option_list >= argc)
        return NULL;

    for (p = cactions; p->cmd_name; p++) {
        if (!strcmp(argv[option_list], p->cmd_name)) {
            *index = option_list + 1;
            return p;
        }
    }

    return NULL;
}

static int flash_help(struct miccmd_actions *cact)
{
    printf("%s\n\n", MIC_COPYRIGHT);
    printf("Usage:\n");

    if (cact == NULL) {
        struct miccmd_actions *p;
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

int main(int argc, char *argv[])
{
    int c, option_index = 0;
    struct miccmd_actions *ca;

    progname = basename(argv[0]);

    for (;; ) {
        c = getopt_long(argc, argv, "d:f:Fhmv", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case TEST_OPTION:
            /* Test mode */
            flashopts.test_mode_flag++;
            break;

        case SMC_BOOTLOADER:
            flashopts.smc_bootloader++;
            break;

        case 'd':
            flashopts.device_flag++;
            flashopts.device = optarg;
            break;

        case 'f':
            flashopts.file_flag++;
            flashopts.file = optarg;
            break;

        case 'h':
            flashopts.help_flag++;
            break;

        case 'F':
            flashopts.force_flag++;
            break;

        case 'v':
            flashopts.verbose++;
            break;

        default:
            error_msg_start("Unrecognized option: %c\n", c);
        /* FALL THRU */
        case '?':
            return 1;
        }
    }

    if (flashopts.device_flag > 1) {
        error_msg_start("Multiple '{--device|-d} <device>' "
                        "options\n");
        return 1;
    }

    if (flashopts.file_flag > 1) {
        error_msg_start("Multiple '{--file|-f} <file>' options\n");
        return 1;
    }

    if (flashopts.help_flag > 1) {
        error_msg_start("Multiple '{--help|-h}' options\n");
        return 1;
    }

    if (flashopts.force_flag > 1) {
        error_msg_start("Multiple '{--force|-f}' options\n");
        return 1;
    }

    if (flashopts.help_flag && (flashopts.device_flag ||
                                flashopts.file_flag)) {
        error_msg_start("Device or file must not be specified with "
                        "the help command\n");
        return 1;
    }

    if (optind >= argc) {
        if (flashopts.help_flag)
            return flash_help(NULL);

        error_msg_start("No action specified\n");
        return 1;
    }

    option_index = optind;

    if ((ca = parse_cmd(actions, &option_index, argc, argv)) == NULL) {
        error_msg_start("Unrecognized action - '%s'\n",
                        argv[option_index]);
        return 1;
    }

    if (flashopts.help_flag)
        return flash_help(ca);
    return (*ca->cmd_func)(ca, &option_index, argc, argv);
}
