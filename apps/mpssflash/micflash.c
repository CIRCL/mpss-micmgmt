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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <wait.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <libgen.h>
#include <regex.h>
#include "helper.h"

#define ASSERT    assert

#define LOG_STATUS(format, args ...)         { \
        if (log_fp != NULL) {                  \
            time_t t;                          \
            char *str;                         \
            (void)time(&t);                    \
            str = ctime(&t);                   \
            str[strlen(str) - 1] = '\0';       \
            fprintf(log_fp, "%s: ", str);      \
            fprintf(log_fp, format, ## args);  \
            fflush(log_fp);                    \
        }                                      \
}

#define ERROR_MSG_START(format, args ...)    { \
        LOG_STATUS(format, ## args);           \
        error_msg_start(format, ## args);      \
        fflush(stderr);                        \
}

#define PRINT(format, args ...)              { \
        if (!silent) {                         \
            printf(format, ## args);           \
            fflush(stdout);                    \
        }                                      \
}

#define PRINT_ERR(format, args ...)          { \
        fprintf(stderr, format, ## args);      \
        fflush(stderr);                        \
}

#define PRINT_LOG_ERR(format, args ...)      { \
        LOG_STATUS(format, ## args);           \
        PRINT_ERR(format, ## args);            \
}

#ifndef MPSS_VERSION
static const char *tool_version = "<unknown>";
#else
static const char *tool_version = MPSS_VERSION;
#endif
typedef int (output_func (FILE *, const char *, ...));

typedef int (exec_func (int, char *[]));
typedef int (init_cmd_func (void));
typedef char ** (build_cmd_func (int, char *, int *));
struct proc_state;
typedef void * (setup_cmd_func (void *, int));
typedef void (done_cmd_func (void *));

typedef int (miccmd_func (build_cmd_func *, setup_cmd_func *, done_cmd_func *,
                          init_cmd_func *));

static miccmd_func common_flash_cmd;
static miccmd_func do_help_cmd;
static miccmd_func do_tool_version;

struct opt_actions;
typedef int (opt_func (int *, int, char *[], struct opt_actions *));

struct opt_actions {
    char *    option;
    opt_func *func;
    intptr_t  arg;
};

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

static int proc_opt(int *, int, char *[], struct opt_actions *);
static int proc_opt_arg(int *, int, char *[], struct opt_actions *);
static int proc_cmd(int *, int, char *[], struct opt_actions *);
static int proc_simple_cmd(int *, int, char *[], struct opt_actions *);

static int nodowngrade = 0;
static int silent = 0;
static char *logfile = NULL;
static char *device_arg = NULL;
static int noreboot = 0;
static char *image_path = NULL;
static int opt_rotating_bar = 0;
static int smc_bootloader = 0;
static int verbose = 0;
static int test_arg = 0;
static int cmd_update = 0;
static int cmd_version = 0;
static int cmd_save = 0;
static int cmd_compat = 0;
static int cmd_devinfo = 0;
static int cmd_info = 0;
static int cmd_help = 0;
static int cmd_tool_version = 0;

static struct opt_actions oa[] = {
    { "-nodowngrade",   proc_opt,        (intptr_t)&nodowngrade    },
    { "-silent",        proc_opt,        (intptr_t)&silent         },
    { "-smcbootloader", proc_opt,        (intptr_t)&smc_bootloader },
    { "-noreboot",      proc_opt,        (intptr_t)&noreboot       },
    { "-device",        proc_opt_arg,    (intptr_t)&device_arg     },
    { "-d",             proc_opt_arg,    (intptr_t)&device_arg     },
    { "-log",           proc_opt_arg,    (intptr_t)&logfile        },
    { "-vv",            proc_opt,
      (intptr_t)&opt_rotating_bar },
    { "-v",             proc_opt,        (intptr_t)&verbose        },
    { "-test",          proc_opt,        (intptr_t)&test_arg       },
    { "-update",        proc_cmd,        (intptr_t)&cmd_update     },
    { "-getversion",    proc_cmd,        (intptr_t)&cmd_version    },
    { "-save",          proc_cmd,        (intptr_t)&cmd_save       },
    { "-compatible",    proc_cmd,        (intptr_t)&cmd_compat     },
    { "-devinfo",       proc_cmd,        (intptr_t)&cmd_devinfo    },
    { "-info",          proc_cmd,        (intptr_t)&cmd_info       },
    { "-help",          proc_simple_cmd, (intptr_t)&cmd_help       },
    { "-version",       proc_simple_cmd,
      (intptr_t)&cmd_tool_version },
    { NULL,             NULL,            (intptr_t)NULL            }
};

static build_cmd_func build_update_args;
static build_cmd_func build_version_args;
static build_cmd_func build_save_args;
static build_cmd_func build_compat_args;
static build_cmd_func build_devinfo_args;

static setup_cmd_func get_update_image;
static done_cmd_func free_update_image;

static init_cmd_func init_update;
static init_cmd_func init_version;
static init_cmd_func init_save;
static init_cmd_func init_compat;
static init_cmd_func init_devinfo;

#define UPDATE_USAGE                                          \
    "-update [<path>] [{-device|-d} <device_id>] [-v] [-vv] " \
    "[-nodowngrade] [-noreboot] [-silent] [-log <logfile>] [-smcbootloader]"

#define UPDATE_HELP                                                        \
    "Updates the flash on specified Intel(R) Xeon Phi(TM) Co-processor\n"  \
    "using given image.\n\n"                                               \
    ""                                                                     \
    "<path>: If <path> refers to a regular file then it's used as an\n"    \
    "image file. If a directory path is specified then the program\n"      \
    "looks for a compatible image inside that directory. If <path> is\n"   \
    "not given then /usr/share/mpss/flash is searched.\n\n"                \
    ""                                                                     \
    "-v: Show verbose status messages.\n\n"                                \
    ""                                                                     \
    "-vv: Display progress with a rotating-bar progress monitor.\n\n"      \
    ""                                                                     \
    "-nodowngrade: Do not allow downgrade of the flash software.\n\n"      \
    ""                                                                     \
    "-noreboot: Do not display reboot message at the end of successful\n"  \
    "update.\n\n"                                                          \
    ""                                                                     \
    "-smcbootloader: Update SMC boot-loader before performing the flash\n" \
    "operation.\n\n"

#define GETVERSION_USAGE                                           \
    "-getversion {[<file>]|[{-device|-d} <device_id>]} [-silent] " \
    "[-log <logfile>]"

#define GETVERSION_HELP                                           \
    "Get version of flash image in the specified file, or from\n" \
    "the specified device.\n\n"                                   \
    ""                                                            \
    "<file>: Name of the image file.\n\n"

#define SAVE_USAGE \
    "-save <file> [{-device|-d} <device_id>] [-silent] [-log <logfile>]"

#define SAVE_HELP                                                     \
    "Save flash image from given device into the specified file.\n\n" \
    ""                                                                \
    "<file>: Name of the file where image should be saved.\n\n"

#define COMPAT_USAGE                                           \
    "-compatible <file> [{-device|-d} <device_id>] [-silent] " \
    "[-log <logfile>]"
#define COMPAT_HELP                                                       \
    "Check if specified flash image is compatible with given device.\n\n" \
    "<file>: Name of the image file to check for compatibility.\n\n"

#define DEVINFO_USAGE \
    "-devinfo [{-device|-d} <device_id>] [-silent] [-log <logfile>]"

#define DEVINFO_HELP \
    "Get flash device vendor information\n\n"

#define HELP_USAGE      \
    "-help [command]\n" \
    "command: -update|-getversion|-save|-devinfo|-version"

#define HELP_ON_HELP                                               \
    "Displays help for all commands if invoked without command.\n" \
    "Otherwise displays command specific help\n\n"

#define COMMON_HELP                                                       \
    "Following options are common between various micflash commands:\n\n" \
    ""                                                                    \
    "-device <device_id>: <device_id> gives the numeric ID of the\n"      \
    "device on which the specified operation must be performed. If\n"     \
    "this option is not specified then <device_id> of 0 is implied\n"     \
    "on a single-card system, and error is reported on multi-card\n"      \
    "machines.\n\n"                                                       \
    ""                                                                    \
    "A list of devices may be specified using comma separated numeric\n"  \
    "values, e.g. '-device 0,3,0x5'.\n\n"                                 \
    ""                                                                    \
    "<device_id> of 'all' implies all available devices.\n\n"             \
    ""                                                                    \
    "-silent: Suppress status messages while operation is in progress.\n" \
    "Errors are still reported.\n\n"                                      \
    ""                                                                    \
    "-log <logfile>: <logfile> specifies the name of the file to log\n"   \
    "operation status into.\n\n\n\n"

static struct miccmd_actions cmds_l[] = {
    {
        "-update", common_flash_cmd, init_update,
        build_update_args, get_update_image, free_update_image,
        UPDATE_USAGE,
        UPDATE_HELP
    },
    {
        "-getversion", common_flash_cmd, init_version,
        build_version_args, NULL, NULL,
        GETVERSION_USAGE,
        GETVERSION_HELP
    },
    {
        "-save", common_flash_cmd, init_save,
        build_save_args, NULL, NULL,
        SAVE_USAGE,
        SAVE_HELP
    },
    {
        "-compatible", common_flash_cmd, init_compat,
        build_compat_args, NULL, NULL,
        COMPAT_USAGE,
        COMPAT_HELP
    },
    {
        "-devinfo", common_flash_cmd, init_devinfo,
        build_devinfo_args, NULL, NULL,
        DEVINFO_USAGE,
        DEVINFO_HELP
    },
    {
        "-version", do_tool_version, NULL,
        NULL, NULL, NULL,
        "-version",
        "Print tool version.\n\n"
    },
    {
        "-help", do_help_cmd, NULL,
        NULL, NULL, NULL,
        HELP_USAGE,
        HELP_ON_HELP
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

#define INIT_STATE      ('I')
#define READ_STATE      ('R')
#define UPDATE_STATE    ('U')
#define SMC_STATE       ('S')
#define SMC_BL_STATE    ('B')
#define RESET_STATE     ('P')
#define EXIT_STATE      ('E')
#define EXEC_STATE      ('X')

struct proc_state {
    int n_procs;
    struct {
        int    devnum;
        pid_t  pid;
        FILE * fpout;
        FILE * fperr;

        int    state;
        int    flash_read_status;
        int    flash_update_status;
        int    smc_update_status;
        int    smc_bl_update_status;

        int    percent_read;
        int    percent_update;
        int    percent_smc_update;
        int    percent_smc_bl_update;

#define MAX_OUT    (4096)
        char   reset_status[MAX_OUT];

        char   output[MAX_OUT];
        int    exit_status;

        char   stat_msg[MAX_OUT];

        time_t update_at;
    } proc_i[1];
};

struct proc_state *procinfo;

#define SET_STATUS(i, format, args ...)    {                                      \
        snprintf(procinfo->proc_i[(i)].stat_msg,                                  \
                 sizeof(procinfo->proc_i[(i)].stat_msg), format,                  \
                 ## args);                                                        \
        if ((verbose || opt_rotating_bar)) {                                      \
             PRINT_LOG_ERR("%s\n", procinfo->proc_i[(i)].stat_msg);               \
        } else {                                                                  \
             ERROR_MSG_START("%s\n", procinfo->proc_i[(i)].stat_msg);             \
        }                                                                         \
}

static struct miccmd_actions *selected_cmd;

FILE *log_fp = NULL;

extern char *flash_dir;
extern char *smc_bl;

extern int flash1_start(int, char *[]);

static int ic_strcmp(const char *s1, const char *s2)
{
    return strcasecmp(s1, s2);
}

static char *get_arg(int argc, char *argv[], int *index)
{
    if (*index >= argc)
        return NULL;

    return argv[(*index)++];
}

static int proc_simple_cmd(int *index, int argc, char *argv[],
                           struct opt_actions *oa)
{
    ARG_USED(index);
    ARG_USED(argc);
    ARG_USED(argv);

    (*((int *)oa->arg))++;
    return 0;
}

static int proc_cmd(int *index, int argc, char *argv[], struct opt_actions *oa)
{
    struct miccmd_actions *c;

    ARG_USED(index);
    ARG_USED(argc);
    ARG_USED(argv);

    if (selected_cmd != NULL) {
        ERROR_MSG_START("Multiple commands: %s, %s\n",
                        selected_cmd->cmd_name, oa->option);
        return CMD_LINE_ERR;
    }

    for (c = cmds_l; c->cmd_name != NULL; c++) {
        if (!ic_strcmp(c->cmd_name, oa->option))
            break;
    }

    if (c->cmd_name == NULL) {
        fprintf(stderr, "Action for '%s' not found: Internal error\n",
                oa->option);
        return OTHER_ERR;
    }

    selected_cmd = c;
    (*((int *)oa->arg))++;

    return 0;
}

static int proc_opt(int *index, int argc, char *argv[], struct opt_actions *oa)
{
    ARG_USED(index);
    ARG_USED(argc);
    ARG_USED(argv);

    (*((int *)oa->arg))++;
    if (*((int *)oa->arg) > 1) {
        ERROR_MSG_START("Multiple options: '%s'\n", oa->option);
        return -1;
    }

    return 0;
}

static int proc_opt_arg(int *index, int argc, char *argv[],
                        struct opt_actions *oa)
{
    char *opt = *((char **)oa->arg);
    char *optarg;

    if (opt != NULL) {
        ERROR_MSG_START("Multiple options: '%s'\n", oa->option);
        return -1;
    }

    optarg = get_arg(argc, argv, index);
    if (optarg == NULL) {
        ERROR_MSG_START("%s: Missing argument\n", oa->option);
        return -1;
    }

    *(char **)oa->arg = optarg;
    return 0;
}

static int get_options(int argc, char *argv[], int *index,
                       struct opt_actions *oa)
{
    char *arg;
    int ret;
    struct opt_actions *a;

    if ((arg = get_arg(argc, argv, index)) == NULL) {
        /* No options */
        return 0;
    }

    while (arg != NULL) {
        for (a = oa; a->option != NULL; a++) {
            if (!ic_strcmp(a->option, arg))
                break;
        }

        if (a->option == NULL) {
            if (arg[0] == '-') {
                ERROR_MSG_START("Unknown option: '%s'\n", arg);
                return -1;
            }
            if (image_path == NULL) {
                image_path = arg;
            } else {
                ERROR_MSG_START("Multiple image-files: "
                                "%s, %s\n", image_path, arg);
                return -1;
            }
        } else {
            ret = a->func(index, argc, argv, a);
            if (ret)
                return ret;
        }

        arg = get_arg(argc, argv, index);
    }

    return 0;
}

static pid_t popen2(exec_func *entry, int argc, char *argv[], FILE **fpout,
                    FILE **fperr)
{
    pid_t pid;
    int fdout[2], fderr[2];

    if (pipe(fdout) < 0) {
        ERROR_MSG_START("pipe: %s\n", strerror(errno));
        return (pid_t)-1;
    }

    if (pipe(fderr) < 0) {
        close(fdout[0]);
        close(fdout[1]);
        ERROR_MSG_START("pipe: %s\n", strerror(errno));
        return (pid_t)-1;
    }

    fflush(stdout);
    fflush(stderr);
    if ((pid = fork()) == 0) {
        if ((dup2(fdout[1], 1) < 0) || (dup2(fderr[1], 2) < 0)) {
            ERROR_MSG_START("dup2: %s\n", strerror(errno));
            close(fdout[0]);
            close(fdout[1]);
            close(fderr[0]);
            close(fderr[1]);
            exit(1);
        }
        close(fdout[0]);
        close(fderr[0]);
        exit((*entry)(argc, argv));
    } else {
        close(fdout[1]);
        close(fderr[1]);

        *fpout = fdopen(fdout[0], "r");
        *fperr = fdopen(fderr[0], "r");
        if ((*fpout == NULL) || (*fperr == NULL)) {
            ERROR_MSG_START("fdopen: %s\n", strerror(errno));
            waitpid(pid, NULL, 0);
            if (*fpout != NULL) {
                fclose(*fpout);
                close(fderr[0]);
            }

            if (*fperr != NULL) {
                fclose(*fperr);
                close(fdout[0]);
            }

            return (pid_t)-1;
        }
    }

    return pid;
}

static int poll_std_out_err(pid_t pid, FILE *fpout, FILE *fperr,
                            output_func *parse_out,
                            output_func *parse_err)
{
    int status;
    char buf[PIPE_BUF];

    for (;; ) {
        if (fgets(buf, sizeof(buf), fpout) != NULL) {
            if (parse_out != NULL)
                (void)(*parse_out)(stdout, buf);
        }

        if (fgets(buf, sizeof(buf), fperr) != NULL) {
            if (parse_err != NULL)
                (void)(*parse_err)(stderr, buf);
        }

        if (feof(fpout) && feof(fperr))
            break;
    }

    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return -1;
}

static char *devnum_to_str(int devnum)
{
    static char devstr[sizeof(int) * 2 + 3];     /* '0', 'x' followed by
                                                  * hexa and null-terminator */

    snprintf(devstr, sizeof(devstr), "0x%x", devnum);
    return devstr;
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
    FILE *fpout, *fperr;
    pid_t pid;
    int ret;

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
    pid = popen2(flash1_start, check_nargs, check_args, &fpout, &fperr);
    if (pid == (pid_t)-1)
        return -1;

    ret = poll_std_out_err(pid, fpout, fperr, NULL, NULL);

    return ret;
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
        SET_STATUS(proc_slot, "mic%d: %s: scandir: %s\n",
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
            SET_STATUS(proc_slot, "mic%d: Out of memory\n", devnum);
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

static void clear_screen(void)
{
    system("clear");
}

static void show_all_errors(void)
{
    int i;
    char buf[PIPE_BUF];

    for (i = 0; i < procinfo->n_procs; i++) {
        if ((procinfo->proc_i[i].pid == 0))
            continue;
        if (fgets(buf, sizeof(buf),
                  procinfo->proc_i[i].fperr) != NULL) {
            PRINT_ERR("\n");
            PRINT_LOG_ERR("%s", buf);
            while (fgets
                       (buf, sizeof(buf),
                       procinfo->proc_i[i].fperr)
                   != NULL)
                PRINT_LOG_ERR("%s", buf);
            break;
        }
    }
}

int run_cmd(exec_func *func, int argc, char *argv[])
{
    pid_t pid;
    int status;

    fflush(stdout);
    fflush(stderr);
    if ((pid = fork()) == 0)
        exit((*func)(argc, argv));

    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return 1;
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

static int flash2_start(int argc, char *argv[])
{
    int file_opt_index = 3;
    int device_opt_index = 1;
    int i, ret, devnum;
    char *bl_argv[argc + 2];

    int get_num(char *);
    char *image;


    ASSERT(argc >= 7);
    ASSERT(strcmp(argv[file_opt_index], "--file") == 0);
    file_opt_index++;
    image = argv[file_opt_index];

    /*
     * 'flash1 --device <dev> --file <file> --line [--oldimage] [--test]
     *  --smcbootloader update' &&
     * 'flash1 --device <dev> --file <file> --line [--oldimage] [--test]
     *  update'
     */
    ASSERT(strcmp(argv[device_opt_index], "--device") == 0);
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
    if ((ret = run_cmd(flash1_start, i, bl_argv)) != 0)
        return ret;

    i--;
    bl_argv[file_opt_index] = image;
    bl_argv[i - 1] = "update";
    if ((ret = run_cmd(flash1_start, i, bl_argv)) != 0)
        return ret;

    return 0;
}

static int rotating_bar(void)
{
    static int bar_num = 0;
    static char bar_chars[] = { '/', '-', '\\' };
    int i, done;
    struct timespec sreq;

    bar_num++;
    if (bar_num == (sizeof(bar_chars) / sizeof(char)))
        bar_num = 0;
    clear_screen();
    printf("%c\n", bar_chars[bar_num]);

    done = 0;
    for (i = 0; i < procinfo->n_procs; i++) {
        if (procinfo->proc_i[i].pid == 0) {
            printf("%s\n\n", procinfo->proc_i[i].stat_msg);
            done++;
            continue;
        }

        printf("mic%d: Read Flash: %d%%\n",
               procinfo->proc_i[i].devnum,
               procinfo->proc_i[i].percent_read);

        if (cmd_update) {
            printf("mic%d: Update Flash: %d%%\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_update);

            printf("mic%d: Update SMC: %d%%\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_smc_update);

            printf("mic%d: Update SMC boot-loader: %d%%\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_smc_bl_update);
        }

        if (procinfo->proc_i[i].output[0] != '\0') {
            printf("mic%d: %s\n", procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].output);
        }

        if (procinfo->proc_i[i].state == RESET_STATE) {
            printf("mic%d: Transitioning to ready state: "
                   "POST code: %s\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].reset_status);
        }

        if (procinfo->proc_i[i].stat_msg[0] != '\0') {
            printf("mic%d: Status: %s\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].stat_msg);
        }

        if (procinfo->proc_i[i].state == EXIT_STATE) {
            printf("mic%d: Done: Exit Status: %d\n\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].exit_status);
            done++;
        } else {
            printf("\n\n");
        }
    }

    if (done == procinfo->n_procs) {
        show_all_errors();
        return 1;
    }
#define ROTATING_BAR_DELAY    ((long)5000000000)
    sreq.tv_sec = 0;
    sreq.tv_nsec = ROTATING_BAR_DELAY;
    (void)nanosleep(&sreq, NULL);

    return 0;
}

static void log_error(int i)
{
    char buf[PIPE_BUF];

    while (fgets(buf, sizeof(buf), procinfo->proc_i[i].fperr) != NULL)
        PRINT_LOG_ERR("%s", buf);
}

static int ascii_update(void)
{
    int i, done = 0;
    static time_t last_update = 0;
    time_t t;

    t = time(NULL);
    if (t - last_update < 4)
        return 0;
    last_update = t;
    for (i = 0; i < procinfo->n_procs; i++) {
        if (procinfo->proc_i[i].pid == 0) {
            if (procinfo->proc_i[i].stat_msg[0] != '\0') {
                printf("mic%d: Operation not started: %s\n",
                       procinfo->proc_i[i].devnum,
                       procinfo->proc_i[i].stat_msg);
                procinfo->proc_i[i].stat_msg[0] = '\0';
            }
            done++;
            continue;
        }

        if (procinfo->proc_i[i].output[0] != '\0') {
            printf("mic%d: %s\n", procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].output);
            procinfo->proc_i[i].output[0] = '\0';
        }

        if (procinfo->proc_i[i].stat_msg[0] != '\0') {
            procinfo->proc_i[i].stat_msg[0] = '\0';
            continue;
        }

        if (procinfo->proc_i[i].state == READ_STATE) {
            printf("mic%d: Reading flash\n",
                   procinfo->proc_i[i].devnum);
        }

        if (procinfo->proc_i[i].state == UPDATE_STATE) {
            printf("mic%d: Updating flash: %d%%\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_update);
        }

        if (procinfo->proc_i[i].state == SMC_STATE) {
            printf("mic%d: Updating SMC: %d%%\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_smc_update);
        }

        if (procinfo->proc_i[i].state == SMC_BL_STATE) {
            printf("mic%d: Updating SMC boot-loader: %d%%\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_smc_bl_update);
        }

        if (procinfo->proc_i[i].state == RESET_STATE) {
            printf("mic%d: Transitioning to ready state: "
                   "POST code: %s\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].reset_status);
        }

        if (procinfo->proc_i[i].state == EXIT_STATE) {
            if (procinfo->proc_i[i].exit_status == 0) {
                printf("mic%d: Done",
                       procinfo->proc_i[i].devnum);
            } else {
                printf("mic%d: Error "
                       "(Exit status: %d)",
                       procinfo->proc_i[i].devnum,
                       procinfo->proc_i[i].exit_status);
                log_error(i);
            }

            if (procinfo->proc_i[i].smc_bl_update_status < 0)
                printf(": SMC boot-loader update failed");
            else if (procinfo->proc_i[i].percent_smc_bl_update ==
                     100)
                printf(": SMC boot-loader update successful");
            if (procinfo->proc_i[i].flash_update_status < 0)
                printf(": Flash update failed");
            else if (procinfo->proc_i[i].percent_update == 100)
                printf(": Flash update successful");
            if (procinfo->proc_i[i].smc_update_status < 0)
                printf(": SMC update failed");
            else if (procinfo->proc_i[i].percent_smc_update ==
                     100)
                printf(": SMC update successful");
            printf("\n");
            done++;
        }
    }
    printf("\n");

    return procinfo->n_procs == done;
}

static int default_update(void)
{
    int i;
    int done;

    done = 0;
    for (i = 0; i < procinfo->n_procs; i++) {
        if (procinfo->proc_i[i].pid == 0) {
            done++;
            continue;
        }

        log_error(i);

        if (procinfo->proc_i[i].state == EXIT_STATE)
            done++;
    }

    return procinfo->n_procs == done;
}

static int show_progress(void)
{
    if (procinfo->proc_i[0].devnum == -1) {
        procinfo->proc_i[0].exit_status =
            poll_std_out_err(procinfo->proc_i[0].pid,
                             procinfo->proc_i[0].fpout,
                             procinfo->proc_i[0].fperr, fprintf,
                             fprintf);
        return 1;
    }

    if (opt_rotating_bar)
        return rotating_bar();

    if (verbose)
        return ascii_update();

    return default_update();
}

static void set_error_status(int i, char *buf)
{
    SET_STATUS(i, "%s", buf);

    switch (procinfo->proc_i[i].state) {
    case READ_STATE:
        procinfo->proc_i[i].flash_read_status = -1;
        LOG_STATUS("mic%d: Flash read: %d%% was completed\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_read);
        break;

    case UPDATE_STATE:
        procinfo->proc_i[i].flash_update_status = -1;
        LOG_STATUS("mic%d: Flash update: %d%% was completed\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_update);
        break;

    case SMC_STATE:
        procinfo->proc_i[i].smc_update_status = -1;
        LOG_STATUS("mic%d: SMC update: %d%% was completed\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_smc_update);
        break;

    case SMC_BL_STATE:
        procinfo->proc_i[i].smc_bl_update_status = -1;
        LOG_STATUS("mic%d: SMC boot-loader update: "
                   "%d%% was completed\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].percent_smc_bl_update);
        break;
    }
}

static void do_update_status(int i, char *buf)
{
    char *p;

    p = strchr(buf, '\n');
    if (p != NULL)
        *p = '\0';
    if (p == buf)     /* null line */
        return;

    switch (procinfo->proc_i[i].state) {
    case READ_STATE:
        if ((procinfo->proc_i[i].percent_read = get_num(buf)) < 0) {
            SET_STATUS(i, "Internal error: state: %d: buf: %s\n",
                       procinfo->proc_i[i].state, buf);
            procinfo->proc_i[i].flash_read_status = -1;
        }
        break;

    case UPDATE_STATE:
        if ((procinfo->proc_i[i].percent_update = get_num(buf)) < 0) {
            SET_STATUS(i, "Internal error: state: %d: "
                       "buf: %s\n", procinfo->proc_i[i].state,
                       buf);
            procinfo->proc_i[i].flash_update_status = -1;
        }
        break;

    case SMC_STATE:
        if ((procinfo->proc_i[i].percent_smc_update =
                 get_num(buf)) < 0) {
            SET_STATUS(i, "Internal error: state: %d: "
                       "buf: %s\n", procinfo->proc_i[i].state,
                       buf);
            procinfo->proc_i[i].smc_update_status = -1;
        }
        break;

    case SMC_BL_STATE:
        if ((procinfo->proc_i[i].percent_smc_bl_update =
                 get_num(buf)) < 0) {
            SET_STATUS(i, "Internal error: state: %d: "
                       "buf: %s\n", procinfo->proc_i[i].state,
                       buf);
            procinfo->proc_i[i].smc_bl_update_status = -1;
        }
        break;

    case RESET_STATE:
        strncpy(procinfo->proc_i[i].reset_status, buf,
                (MAX_OUT - 1));
        procinfo->proc_i[i].reset_status[MAX_OUT - 1] = '\0';
        p = strchr(procinfo->proc_i[i].reset_status, '\n');
        if (p != NULL)
            *p = '\0';
        break;

    case INIT_STATE:
    case EXIT_STATE:
        break;

    default:
        SET_STATUS(i, "Internal error: state: %d: "
                   "buf: %s\n", procinfo->proc_i[i].state,
                   buf);
        break;
    }
}

static void show_done(int i)
{
    UT_INSTRUMENT_EVENT("MIC_FLASH_SHOW_DONE_0",
                        procinfo->proc_i[i].state = 1023);
    switch (procinfo->proc_i[i].state) {
    case READ_STATE:
        if (procinfo->proc_i[i].flash_read_status < 0)
            break;
        LOG_STATUS("mic%d: Read done\n", procinfo->proc_i[i].devnum);
        if (!(silent || opt_rotating_bar || verbose)) {
            printf("mic%d: Read done\n",
                   procinfo->proc_i[i].devnum);
        }
        break;

    case UPDATE_STATE:
        if (procinfo->proc_i[i].flash_update_status < 0)
            break;
        LOG_STATUS("mic%d: Flash update done\n",
                   procinfo->proc_i[i].devnum);
        if (!(silent || opt_rotating_bar || verbose)) {
            printf("mic%d: Flash update done\n",
                   procinfo->proc_i[i].devnum);
        }
        break;

    case SMC_STATE:
        if (procinfo->proc_i[i].smc_update_status < 0)
            break;
        LOG_STATUS("mic%d: SMC update done\n",
                   procinfo->proc_i[i].devnum);
        if (!(silent || opt_rotating_bar || verbose)) {
            printf("mic%d: SMC update done\n",
                   procinfo->proc_i[i].devnum);
        }
        break;

    case SMC_BL_STATE:
        if (procinfo->proc_i[i].smc_bl_update_status < 0)
            break;
        LOG_STATUS("mic%d: SMC boot-loader update done\n",
                   procinfo->proc_i[i].devnum);
        if (!(silent || opt_rotating_bar || verbose)) {
            printf("mic%d: SMC boot-loader update done\n",
                   procinfo->proc_i[i].devnum);
        }
        break;

    case EXEC_STATE:
    case EXIT_STATE:
    case RESET_STATE:
    case INIT_STATE:
        break;

    default:
        LOG_STATUS("mic%d: Invalid state: %d\n",
                   procinfo->proc_i[i].devnum,
                   procinfo->proc_i[i].state);
        if (!(opt_rotating_bar || verbose)) {
            PRINT_ERR("mic%d: Invalid state: %d\n",
                      procinfo->proc_i[i].devnum,
                      procinfo->proc_i[i].state);
        }
        break;
    }
}

static void set_state(int i)
{
    char buf[PIPE_BUF], *p;
    int status;
    static char str_flash_read[] = "Flash read";
    static int flash_read_len = sizeof(str_flash_read) - 1;
    static char str_flash_update[] = "Flash update";
    static int flash_update_len = sizeof(str_flash_update) - 1;
    static char str_smc_update[] = "SMC update";
    static int smc_update_len = sizeof(str_smc_update) - 1;
    static char str_smc_bl_image[] = "SMC boot-loader image";
    static int smc_bl_image_len = sizeof(str_smc_bl_image) - 1;
    static char str_smc_bl_update[] = "SMC boot-loader update";
    static int smc_bl_update_len = sizeof(str_smc_bl_update) - 1;
    static char str_reset[] = "Resetting";
    static int reset_len = sizeof(str_reset) - 1;
    static char str_version[] = "Version";
    static int version_len = sizeof(str_version) - 1;
    static char str_devinfo[] = "Flash:";
    static int devinfo_len = sizeof(str_devinfo) - 1;
    static char str_compat[] = "Image check";
    static int compat_len = sizeof(str_compat) - 1;
    static char str_error[] = "ERROR: ";
    static int error_len = sizeof(str_error) - 1;
    int l;

    while (fgets(buf, sizeof(buf), procinfo->proc_i[i].fpout) != NULL) {
        /* It's possible to get partial line because O_NONBLOCK
         * is set.
         */
        while ((p = strchr(buf, '\n')) == NULL) {
            l = strlen(buf);
            p = buf + l;
            ASSERT((sizeof(buf) - l) > 0);
            while (fgets(p, sizeof(buf) - l,
                         procinfo->proc_i[i].fpout) == NULL) {
                if (feof(procinfo->proc_i[i].fpout)) {
                    *p = '\n';
                    break;
                }
            }
        }
        if (!strncmp(buf, str_error, error_len)) {
            set_error_status(i, buf + error_len);
            continue;
        }
        if (!strncmp(buf, str_flash_read, flash_read_len)) {
            show_done(i);
            LOG_STATUS("mic%d: Flash read started...\n",
                       procinfo->proc_i[i].devnum);
            if (!(silent || opt_rotating_bar || verbose)) {
                printf("mic%d: Flash read started\n",
                       procinfo->proc_i[i].devnum);
            }
            procinfo->proc_i[i].state = READ_STATE;
        } else if (!strncmp(buf, str_flash_update, flash_update_len)) {
            show_done(i);
            LOG_STATUS("mic%d: Flash update started...\n",
                       procinfo->proc_i[i].devnum);
            if (!(silent || opt_rotating_bar || verbose)) {
                printf("mic%d: Flash update started\n",
                       procinfo->proc_i[i].devnum);
            }
            procinfo->proc_i[i].state = UPDATE_STATE;
        } else if (!strncmp(buf, str_smc_bl_image,
                            smc_bl_image_len)) {
            LOG_STATUS("mic%d: %s",
                       procinfo->proc_i[i].devnum, buf);
            if (!(silent || opt_rotating_bar || verbose)) {
                printf("mic%d: %s",
                       procinfo->proc_i[i].devnum, buf);
            }
            strncpy(procinfo->proc_i[i].output, buf,
                    sizeof(procinfo->proc_i[i].output));
            procinfo->proc_i[i].output[
                sizeof(procinfo->proc_i[i].output) - 1] = '\0';
        } else if (!strncmp(buf, str_smc_bl_update,
                            smc_bl_update_len)) {
            show_done(i);
            LOG_STATUS("mic%d: SMC boot-loader update started...\n",
                       procinfo->proc_i[i].devnum);
            if (!(silent || opt_rotating_bar || verbose)) {
                printf("mic%d: SMC boot-loader update "
                       "started\n", procinfo->proc_i[i].devnum);
            }
            procinfo->proc_i[i].state = SMC_BL_STATE;
        } else if (!strncmp(buf, str_smc_update, smc_update_len)) {
            show_done(i);
            LOG_STATUS("mic%d: SMC update started...\n",
                       procinfo->proc_i[i].devnum);
            if (!(silent || opt_rotating_bar || verbose)) {
                printf("mic%d: SMC update started\n",
                       procinfo->proc_i[i].devnum);
            }
            procinfo->proc_i[i].state = SMC_STATE;
        } else if (!strncmp(buf, str_reset, reset_len)) {
            show_done(i);
            LOG_STATUS("mic%d: Transitioning to ready state...\n",
                       procinfo->proc_i[i].devnum);
            if (!(silent || opt_rotating_bar || verbose)) {
                printf("mic%d: Transitioning to ready state\n",
                       procinfo->proc_i[i].devnum);
            }
            procinfo->proc_i[i].state = RESET_STATE;
        } else if (!strncmp(buf, str_version, version_len) ||
                   !strncmp(buf, str_devinfo, devinfo_len) ||
                   !strncmp(buf, str_compat, compat_len)) {
            show_done(i);
            LOG_STATUS("mic%d: %s\n", procinfo->proc_i[i].devnum,
                       buf);
            p = strchr(buf, '\n');
            UT_INSTRUMENT_EVENT("MIC_FLASH_SET_STATE_0", p = NULL);
            if (p != NULL)
                *p = '\0';
            if (!(verbose || opt_rotating_bar)) {
                printf("mic%d: %s\n",
                       procinfo->proc_i[i].devnum, buf);
            }
            strncpy(procinfo->proc_i[i].output, buf,
                    sizeof(procinfo->proc_i[i].output));
            procinfo->proc_i[i].output[
                sizeof(procinfo->proc_i[i].output) - 1] = '\0';
            procinfo->proc_i[i].state = EXEC_STATE;
        } else {
            do_update_status(i, buf);
        }
    }
    if (feof(procinfo->proc_i[i].fpout)) {
        show_done(i);
        procinfo->proc_i[i].state = EXIT_STATE;
        waitpid(procinfo->proc_i[i].pid, &status, 0);
        procinfo->proc_i[i].exit_status = WEXITSTATUS(status);
        LOG_STATUS("mic%d: Exit status: %d\n",
                   procinfo->proc_i[i].devnum, WEXITSTATUS(status));
    }
}

static char file_opt[] = "--file";
static char device_opt[] = "--device";
static char line_opt[] = "--line";
static char test_opt[] = "--test";

static int init_update(void)
{
    if (silent && verbose) {
        silent = 0;
        ERROR_MSG_START("-silent may not be specified with -v\n");
        return 1;
    }

    if (silent && opt_rotating_bar) {
        silent = 0;
        ERROR_MSG_START("-silent may not be specified with " "-vv\n");
        return 1;
    }

    if (verbose && opt_rotating_bar) {
        ERROR_MSG_START("-v may not be specified with -vv\n");
        return 1;
    }

    if (smc_bootloader && nodowngrade) {
        ERROR_MSG_START("-smcbootloader may not be specified with "
                        "-nodowngrade\n");
        return 1;
    }

    return 0;
}

static void *get_update_image(void *arg, int proc_slot)
{
    char *path = arg;
    char *image_file = NULL;
    char modifiable_path_var[MAX_OUT];
    struct stat sbuf;
    int len, devnum;
    struct mic_device *mdh;

    devnum = procinfo->proc_i[proc_slot].devnum;
    /*
     * Make sure that the device exists.
     */
    if (mic_open_device(&mdh, devnum) != E_MIC_SUCCESS) {
        SET_STATUS(proc_slot, "failed to open mic'%d': %s: %s\n",
                   devnum, mic_get_error_string(),
                   strerror(errno));
        return NULL;
    }
    (void)mic_close_device(mdh);

    if (path == NULL) {
        /* Print this message only once */
        if (proc_slot == 0) {
            PRINT("No image path specified - "
                  "Searching: %s\n", flash_dir);
        }
        image_path = flash_dir;
        path = flash_dir;
    }

    if (stat(path, &sbuf) != 0) {
        SET_STATUS(proc_slot, "mic%d: %s: %s\n", devnum, path,
                   strerror(errno));
        return NULL;
    }

    if (!S_ISDIR(sbuf.st_mode)) {
        if (!S_ISREG(sbuf.st_mode)) {
            SET_STATUS(proc_slot, "mic%d: %s: not a regular file\n",
                       devnum, path);
            return NULL;
        }

        if (check_image(proc_slot, path, devnum, 1) == 0) {
            len = strlen(path) + 1;
            image_file = malloc(len);
            if (image_file == NULL) {
                SET_STATUS(proc_slot, "mic%d: Out of memory\n",
                           devnum);
                return NULL;
            }
            strncpy(image_file, path, len);
            image_file[len - 1] = '\0';
        }

        //dirname() is known to modify the buffer passed as parameter.
        //In a multi-card invocation, the first call to get_update_image()
        //will modify the path which causes the subsequent invocation of
        //get_update_image() to use modified path which will lead to
        //incorrect behavior.
        //To counter this situation, a temporary buffer is used to pass
        //as parameter to dirname().
        strncpy(modifiable_path_var, arg, strlen(arg));
        image_path = dirname(modifiable_path_var);
    } else {
        image_file = find_image(proc_slot, path);
        image_path = path;
    }

    if (image_file == NULL) {
        SET_STATUS(proc_slot, "mic%d: No valid image found\n", devnum);
        return NULL;
    }

    LOG_STATUS("mic%u: Flash image: %s\n", devnum, image_file);
    PRINT("mic%u: Flash image: %s\n", devnum, image_file);

    return (void *)image_file;
}

static void free_update_image(void *arg)
{
    free(arg);
}

static char **build_update_args(int proc_slot, char *image, int *nargs)
{
    static char oldimage_opt[] = "--oldimage";
    static char update_cmd[] = "update";
    static char **update_args;
    int argnum;
    int devnum;

    devnum = procinfo->proc_i[proc_slot].devnum;

    /*
     * 'flash1 --device <dev> --file <file> --line [--oldimage] [--test]
     *  update'
     */
    *nargs = 7;
    if (nodowngrade == 0)
        (*nargs)++;
    if (test_arg)
        (*nargs)++;

    update_args = (char **)malloc(sizeof(char *) * *nargs);
    if (update_args == NULL) {
        SET_STATUS(proc_slot, "mic%d: Out of memory\n", devnum);
        return NULL;
    }

    update_args[0] = progname;
    update_args[1] = device_opt;
    update_args[2] = devnum_to_str(devnum);
    update_args[3] = file_opt;
    update_args[4] = image;
    update_args[5] = line_opt;
    argnum = 6;
    if (nodowngrade == 0)
        update_args[argnum++] = oldimage_opt;
    if (test_arg) {
        update_args[argnum++] = test_opt;;
    }
    update_args[argnum] = update_cmd;

    return update_args;
}

static int check_for_update_options(void)
{
    if (nodowngrade) {
        ERROR_MSG_START("'nodowngrade' may be specified with "
                        "update command only\n");
        return -1;
    }

    if (noreboot) {
        ERROR_MSG_START("'noreboot' may be specified only with "
                        "update command only\n");
        return -1;
    }

    if (smc_bootloader) {
        ERROR_MSG_START("'smcbootloader' may be specified only with "
                        "update command only\n");
        return -1;
    }

    return 0;
}

static int init_version(void)
{
    int ret;

    if ((ret = check_for_update_options()) < 0)
        return ret;

    if ((device_arg != NULL) && image_path != NULL) {
        ERROR_MSG_START("Only one of -device or image-file "
                        "may be specified\n");
        return -11;
    }

    return 0;
}

static char **build_version_args(int proc_slot, char *image, int *nargs)
{
    char **version_args;
    static char version_cmd[] = "version";
    int argnum;
    int devnum;

    devnum = procinfo->proc_i[proc_slot].devnum;

    /*
     * flash1 --file <file> version
     * flash1 --device <device> --line version
     */
    *nargs = image == NULL ? 5 : 4;

    if (test_arg)
        (*nargs)++;

    version_args = (char **)malloc(sizeof(char *) * *nargs);
    if (version_args == NULL) {
        SET_STATUS(proc_slot, "mic%d: Out of memory\n", devnum);
        return NULL;
    }

    version_args[0] = progname;
    if (image != NULL) {
        version_args[1] = file_opt;
        version_args[2] = image;
        argnum = 3;
    } else {
        version_args[1] = device_opt;
        version_args[2] = devnum_to_str(devnum);
        version_args[3] = line_opt;
        argnum = 4;
    }

    if (test_arg) {
        version_args[argnum++] = test_opt;;
    }
    version_args[argnum] = version_cmd;

    return version_args;
}

static int init_save(void)
{
    int ret;

    if ((ret = check_for_update_options()) < 0)
        return ret;

    if (image_path == NULL) {
        ERROR_MSG_START("Please specify a filename to save image\n");
        return 1;
    }

    if (procinfo->n_procs > 1) {
        ERROR_MSG_START("Only one device may be specified "
                        "for reading\n");
        return 1;
    }

    return 0;
}

static char **build_save_args(int proc_slot, char *image, int *nargs)
{
    char **save_args;
    static char save_cmd[] = "read";
    int argnum;
    int devnum;

    devnum = procinfo->proc_i[proc_slot].devnum;

    /*
     * flash1 --file <file> --device <device> --line save
     */
    *nargs = 7;
    if (test_arg)
        (*nargs)++;

    save_args = (char **)malloc(sizeof(char *) * *nargs);
    if (save_args == NULL) {
        SET_STATUS(proc_slot, "mic%d: Out of memory\n", devnum);
        return NULL;
    }

    save_args[0] = progname;
    save_args[1] = file_opt;
    save_args[2] = image;
    save_args[3] = device_opt;
    save_args[4] = devnum_to_str(devnum);
    save_args[5] = line_opt;
    argnum = 6;
    if (test_arg) {
        save_args[argnum++] = test_opt;;
    }
    save_args[argnum] = save_cmd;

    return save_args;
}

static int init_compat(void)
{
    int ret;

    if ((ret = check_for_update_options()) < 0)
        return ret;

    if (image_path == NULL) {
        ERROR_MSG_START("Please specify a filename\n");
        return 1;
    }

    return 0;
}

static char **build_compat_args(int proc_slot, char *image, int *nargs)
{
    char **compat_args;
    static char compat_cmd[] = "check";
    int argnum;
    int devnum;

    devnum = procinfo->proc_i[proc_slot].devnum;

    /*
     * flash1 --file <file> --device <device> --line check
     */
    *nargs = 7;
    if (test_arg)
        (*nargs)++;

    compat_args = (char **)malloc(sizeof(char *) * *nargs);
    if (compat_args == NULL) {
        SET_STATUS(proc_slot, "mic%d: Out of memory\n", devnum);
        return NULL;
    }

    compat_args[0] = progname;
    compat_args[1] = file_opt;
    compat_args[2] = image;
    compat_args[3] = device_opt;
    compat_args[4] = devnum_to_str(devnum);
    compat_args[5] = line_opt;
    argnum = 6;
    if (test_arg) {
        compat_args[argnum++] = test_opt;;
    }
    compat_args[argnum] = compat_cmd;

    return compat_args;
}

static int init_devinfo(void)
{
    int ret;

    if ((ret = check_for_update_options()) < 0)
        return ret;

    if (image_path != NULL) {
        ERROR_MSG_START("<file-name> not needed\n");
        return 1;
    }

    return 0;
}

static char **build_devinfo_args(int proc_slot, char *image, int *nargs)
{
    char **devinfo_args;
    static char devinfo_cmd[] = "device";
    int argnum;

    ARG_USED(image);
    int devnum;

    devnum = procinfo->proc_i[proc_slot].devnum;

    /*
     * flash1 --device <device> --line device
     */
    *nargs = 5;
    if (test_arg)
        (*nargs)++;

    devinfo_args = (char **)malloc(sizeof(char *) * *nargs);
    if (devinfo_args == NULL) {
        SET_STATUS(proc_slot, "mic%d: Out of memory\n", devnum);
        return NULL;
    }

    devinfo_args[0] = progname;
    devinfo_args[1] = device_opt;
    devinfo_args[2] = devnum_to_str(devnum);
    devinfo_args[3] = line_opt;
    argnum = 4;
    if (test_arg) {
        devinfo_args[argnum++] = test_opt;;
    }
    devinfo_args[argnum] = devinfo_cmd;

    return devinfo_args;
}

static struct proc_state *get_devices_list(char *arg, int device_flag)
{
    struct mic_devices_list *md = NULL;
    int n;
    char *p, *q, *more, delim[] = ",";
    struct proc_state *procs;
    int i, d;
    int len;

    if ((arg == NULL) || !ic_strcmp(arg, "all")) {
        if (!device_flag) {
            n = 1;
        } else if ((mic_get_devices(&md) != E_MIC_SUCCESS) ||
                   (mic_get_ndevices(md, &n) != E_MIC_SUCCESS)) {
            ERROR_MSG_START("Failed to get cards info: %s: %s\n",
                            mic_get_error_string(),
                            strerror(errno));
            return NULL;
        }

        UT_INSTRUMENT_EVENT("MIC_FLASH_DEVICES_LIST_0", n = 2);
        UT_INSTRUMENT_EVENT("MIC_FLASH_DEVICES_LIST_1", n = 1);
        if (n > 1) {
            UT_INSTRUMENT_EVENT("MIC_FLASH_DEVICES_LIST_0", n = 1);
            if (arg == NULL) {
                ERROR_MSG_START(
                    "%d cards present - "
                    "please specify one of following: ",
                    n);
                for (i = 0; i < n - 1; i++) {
                    (void)mic_get_device_at_index(md, i,
                                                  &d);
                    error_msg_cont("%d, ", d);
                }
                (void)mic_get_device_at_index(md, i, &d);
                error_msg_cont("%d\n", d);

                mic_free_devices(md);
                return NULL;
            }
        }
        procs =
            (struct proc_state *)calloc(
                1,
                sizeof(struct proc_state) +
                (n - 1) * sizeof(procs->proc_i[0]));

        UT_INSTRUMENT_EVENT("MIC_FLASH_DEVICES_LIST_1",
                            (free(procs), procs = NULL));

        if (procs == NULL) {
            ERROR_MSG_START("Out of memory\n");
            mic_free_devices(md);
            return NULL;
        }
        procs->n_procs = n;

        if (device_flag) {
            for (i = 0; i < n; i++)
                (void)mic_get_device_at_index(
                    md, i, &procs->proc_i[i].devnum);
            mic_free_devices(md);
        } else {
            procs->proc_i[0].devnum = -1;
        }
    } else {
        len = strlen(arg);
        q = p = malloc(len + 1);

        UT_INSTRUMENT_EVENT("MIC_FLASH_DEVICES_LIST_2",
                            (free(p), p = q = NULL));

        if (p == NULL) {
            ERROR_MSG_START("Out of memory\n");
            return NULL;
        }
        strncpy(p, arg, len + 1);
        p[len] = '\0';

        n = 0;
        while (strsep(&p, delim) != NULL)
            n++;
        free(q);

        procs = (struct proc_state *)calloc(1,
                                            sizeof(struct proc_state) +
                                            (n -
                                             1) *
                                            sizeof(procs->proc_i[0]));

        UT_INSTRUMENT_EVENT("MIC_FLASH_DEVICES_LIST_3",
                            (free(procs), procs = NULL));

        if (procs == NULL) {
            ERROR_MSG_START("Out of memory\n");
            return NULL;
        }
        procs->n_procs = n;
        p = arg;
        i = 0;
        while (strsep(&arg, delim) != NULL) {
            procs->proc_i[i].devnum = get_numl(p, &more);
            if (*more || errno) {
                free(procs);
                ERROR_MSG_START("Error in devices list: "
                                "'%s'\n", p);
                return NULL;
            }
            i++;
            p = arg;
        }
    }

    return procs;
}

static void set_noblock(void)
{
    int i;

    for (i = 0; i < procinfo->n_procs; i++) {
        int fd, flags;

        if (procinfo->proc_i[i].pid == 0)
            continue;

        fd = fileno(procinfo->proc_i[i].fpout);
        flags = fcntl(fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(fd, F_SETFL, flags);

        fd = fileno(procinfo->proc_i[i].fperr);
        flags = fcntl(fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(fd, F_SETFL, flags);
    }
}

static int process_common_opts(void)
{
    if (logfile != NULL) {
        if ((log_fp = fopen(logfile, "w")) == NULL) {
            ERROR_MSG_START("%s: open: %s\n", logfile,
                            strerror(errno));
            return -FILE_ERR;
        }
        LOG_STATUS("%s: Tool version: %s\n", progname, tool_version);
    }

    if ((procinfo =
             get_devices_list(device_arg,
                              (cmd_version &&
                               (image_path !=
                                NULL)) ? 0 : 1)) == NULL) {
        if (log_fp != NULL)
            fclose(log_fp);
        return errno == ENOMEM ? -MEM_ERR : -CARD_NUM_ERR;
    }

    return 0;
}

static void free_common(void)
{
    if (log_fp != NULL) {
        fclose(log_fp);
        log_fp = NULL;
    }

    if (procinfo != NULL)
        free(procinfo);
}

static int start_cmd(build_cmd_func *build_func, setup_cmd_func *setup_func,
                     void *arg,
                     done_cmd_func *done_func,
                     init_cmd_func *init_func)
{
    int i, n, n_done, nargs, ret;
    void *desc = NULL;
    pid_t pid;
    char **args;
    FILE *fpout, *fperr;
    exec_func *func;

    n = procinfo->n_procs;
    n_done = 0;
    if ((ret = init_func()) != 0)
        return n_done;

    for (i = 0; i < n; i++) {
        if (setup_func != NULL) {
            if ((desc = setup_func(arg, i)) == NULL)
                continue;
        }

        /* Build cmd args for the low level flash command */
        if ((args = build_func(i,
                               desc != NULL ? desc : (void *)image_path,
                               &nargs)) == NULL)
            continue;

        func = smc_bootloader ? flash2_start : flash1_start;
        pid = popen2(func, nargs, args, &fpout, &fperr);

        free(args);
        if (done_func != NULL)
            done_func(desc);

        if (pid == (pid_t)-1) {
            procinfo->proc_i[i].pid = 0;
            continue;
        } else {
            procinfo->proc_i[i].pid = pid;
        }
        procinfo->proc_i[i].fpout = fpout;
        procinfo->proc_i[i].fperr = fperr;
        procinfo->proc_i[i].state = INIT_STATE;

        n_done++;
    }

    return n_done;
}

static int do_tool_version(build_cmd_func *build_func,
                           setup_cmd_func *setup_func,
                           done_cmd_func *done_func,
                           init_cmd_func *init_func)
{
    ARG_USED(build_func);
    ARG_USED(setup_func);
    ARG_USED(done_func);
    ARG_USED(init_func);

    printf("%s\n\n", MIC_COPYRIGHT);
    printf("%s: Tool version: %s\n", progname, tool_version);
    return 0;
}

static void common_options_help(void)
{
    printf(COMMON_HELP);
}

static int do_help_cmd(build_cmd_func *build_func, setup_cmd_func *setup_func,
                       done_cmd_func *done_func, init_cmd_func *init_func)
{
    ARG_USED(build_func);
    ARG_USED(setup_func);
    ARG_USED(done_func);
    ARG_USED(init_func);
    
	printf("%s\n\n", MIC_COPYRIGHT);

    if (nodowngrade || silent || device_arg || noreboot || image_path ||
        opt_rotating_bar || verbose || test_arg) {
        ERROR_MSG_START("Usage: %s [cmd] -help\n", progname);
        return -1;
    }

    printf("Usage:\n\n");

    if (selected_cmd == NULL) {
        struct miccmd_actions *p;

        common_options_help();

        for (p = cmds_l; p->cmd_name; p++) {
            printf("%s %s\n\n%s\n", progname, p->cmd_options,
                   p->cmd_help);
        }

        return 0;
    }

    if (strcmp(selected_cmd->cmd_name, "-help") &&
        strcmp(selected_cmd->cmd_name, "-version"))
        common_options_help();
    printf("%s %s\n\n%s\n", progname, selected_cmd->cmd_options,
           selected_cmd->cmd_help);
    return 0;
}

static void show_errors(void)
{
    int i;

    for (i = 0; i < procinfo->n_procs; i++)
        if (procinfo->proc_i[i].stat_msg[0] != '\0')
            PRINT_LOG_ERR("%s\n\n", procinfo->proc_i[i].stat_msg);
}

static int common_flash_cmd(build_cmd_func *build_func,
                            setup_cmd_func *setup_func,
                            done_cmd_func *done_func,
                            init_cmd_func *init_func)
{
    int i, ret;
    int n_done;

    if ((ret = process_common_opts()) < 0)
        return -ret;

    /* Start operation for each device in the specified list. */
    n_done = start_cmd(build_func, setup_func, image_path,
                       done_func, init_func);

    if (n_done == 0) {
        if (verbose || opt_rotating_bar)
            show_errors();
        free_common();
        return OTHER_ERR;
    }

    /* Read standard output from each process. First make
     * each read unblocking.
     */
    set_noblock();

    /*
     * Read standard output of each process that's still running.
     */
    do {
        for (i = 0; i < procinfo->n_procs; i++) {
            if ((procinfo->proc_i[i].pid == 0) ||
                (procinfo->proc_i[i].state == EXIT_STATE)) {
                /* Update never started for this device
                 * or the process has exited.
                 */
                continue;
            }

            set_state(i);
        }
    } while (show_progress() == 0);
    PRINT("\n");

    /*
     * Log any errors.
     */
    ret = 0;
    n_done = 0;
    for (i = 0; i < procinfo->n_procs; i++) {
        if (procinfo->proc_i[i].pid == 0) {
            ret = 1;
            continue;
        }

        if (ret == 0)
            ret = procinfo->proc_i[i].exit_status;

        if ((procinfo->proc_i[i].exit_status == 0) ||
            (procinfo->proc_i[i].exit_status == FLASH_UPDATED))
            n_done++;
    }

    if (cmd_update && n_done && !noreboot) {
        PRINT("Please restart host for flash "
              "changes to take effect\n");
    }

    free_common();
    return ret;
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

int main(int argc, char *argv[])
{
    int arg_index = 0;
    int index = 1;
    int ret;

    progname = basename(get_arg(argc, argv, &arg_index));

    if ((ret = get_options(argc, argv, &index, oa)) != 0)
        return ret;

    if (cmd_help)
        return do_help_cmd(NULL, NULL, NULL, NULL);

    if (cmd_tool_version)
        return do_tool_version(NULL, NULL, NULL, NULL);

    if (selected_cmd == NULL) {
        ERROR_MSG_START("No command specified\n");
        return 1;
    }

    set_signals(SIG_IGN);

    ret = (*selected_cmd->cmd_func)(selected_cmd->build_func,
                                    selected_cmd->setup_func,
                                    selected_cmd->done_func,
                                    selected_cmd->init_func);

    set_signals(SIG_DFL);

    return ret;
}
