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

#ifndef MICSMC_CLI_H_
#define MICSMC_CLI_H_

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <memory>
using std::shared_ptr;

#include "cmd_line_args.h"
using apps::utils::cmd_line_args;

#include "cmd_config.h"
using apps::utils::cmd_config;

#include "host_platform.h"
#include "mic_device_class.h"

#define MIC_COPYRIGHT \
    "Copyright (c) 2015, Intel Corporation."

#define DEFAULT_TIMEOUT    100

namespace apps {
class micsmc_cli {
public:
    micsmc_cli(int argc, char *argv[]);
    virtual ~micsmc_cli();

    int run();

private:
    // Command options
    void perform_option(const cmd_line_args::opt_args &opt);
    void perform_all(const cmd_line_args::opt_args &opt);
    void perform_cores_opt(const cmd_line_args::opt_args &opt);
    void perform_cores_opt(const vector<unsigned int> &device_nums,
                           bool devices_verified);
    void perform_freq_opt(const cmd_line_args::opt_args &opt);
    void perform_freq_opt(const vector<unsigned int> &device_nums,
                          bool devices_verified);
    void perform_info_opt(const cmd_line_args::opt_args &opt);
    void perform_info_opt(const vector<unsigned int> &device_nums,
                          bool devices_verified);
    void perform_lost_opt();
    void perform_online_opt();
    void perform_offline_opt();
    void perform_mem_opt(const cmd_line_args::opt_args &opt);
    void perform_mem_opt(const vector<unsigned int> &device_nums,
                         bool devices_verified);
    void perform_temp_opt(const cmd_line_args::opt_args &opt);
    void perform_temp_opt(const vector<unsigned int> &device_nums,
                          bool devices_verified);
    void perform_ecc_opt(const cmd_line_args::opt_args &opt);
    void perform_ecc_status(vector<string>::const_iterator begin,
                            vector<string>::const_iterator end);
    void perform_ecc_set(bool enabled, vector<string>::const_iterator begin,
                         vector<string>::const_iterator end);
    void perform_turbo_opt(const cmd_line_args::opt_args &opt);
    void perform_turbo_status(vector<string>::const_iterator begin,
                              vector<string>::const_iterator end);
    void perform_turbo_set(bool enabled, vector<string>::const_iterator begin,
                           vector<string>::const_iterator end);
    void perform_led_opt(const cmd_line_args::opt_args &opt);
    void perform_led_status(vector<string>::const_iterator begin,
                            vector<string>::const_iterator end);
    void perform_led_set(bool enabled, vector<string>::const_iterator begin,
                         vector<string>::const_iterator end);
    void perform_pthrottle_opt(const cmd_line_args::opt_args &opt);
    void perform_tthrottle_opt(const cmd_line_args::opt_args &opt);
    void perform_pwrenable_opt(const cmd_line_args::opt_args &opt);
    void perform_pwrstatus_opt(const cmd_line_args::opt_args &opt);
    void perform_timeout_opt(const cmd_line_args::opt_args &opt);
    void perform_help_opt(const cmd_line_args::opt_args &opt);
    void show_about();
    void perform_version_opt();

    // Error message handling
    enum msg_type {
        MSG_INFO,
        MSG_WARNING,
        MSG_ERROR
    };

    void info_msg(const char *format, ...);
    void info_msg_dev(const char *dev_name, const char *format, ...);
    void warning_msg(bool error, const char *format, ...);
    void warning_msg_dev(const char *dev_name, bool error,
                         const char *format, ...);
    void error_msg(const char *format, ...);
    void error_msg_dev(const char *dev_name, const char *format, ...);
    void status_msg_dev(const char *dev_name, msg_type type,
                        const char *msg);

    // Helper functions
    mic_device_class *get_device(unsigned int dev_num);
    bool get_device_nums(vector<string>::const_iterator begin,
                         vector<string>::const_iterator end,
                         vector<unsigned int> * device_nums,
                         const char *opt);
    bool get_avail_device_nums(vector<unsigned int> *device_nums);
    static void set_device_name(string *dev_name, unsigned int dev_num);
    bool validate_device_is_online(const string &dev_name,
                                   mic_device_class &device);

    static void *ecc_set_op_thread(void *work_area);
    static void *ecc_status_thread(void *work_area);
    static void *ecc_verbose_status_thread(void *work_area);

    // Command configuration
    void configure_micsmc_cmd(cmd_config *config);
    static cmd_line_args::opt_error_code status_cmd_validator(
            const cmd_line_args::opt_args &args,
            string *err_msg);
    static cmd_line_args::opt_error_code pwrenable_validator(
            const cmd_line_args::opt_args &args,
            string *err_msg);

    static const char *PROGRAM_NAME;
    static const char *MICSMC_GUI_PATH;
    static const unsigned int CORE_UTIL_SAMPLE_INTERVAL; // Milliseconds
    static const unsigned int ECC_OP_STATUS_INTERVAL; // Milliseconds
    static const unsigned int ECC_OP_LONG_STATUS_INTERVAL; // Milliseconds
    static const unsigned int  AVG_THROTTLE_SAMPLE_COUNT;
    static const unsigned int  AVG_THROTTLE_SAMPLE_DELAY_MS; // Milliseconds


    // ECC
    enum ecc_op_step {
        ECC_OP_START,
        ECC_OP_ENTER_MAINT_MODE,
        ECC_OP_SET_ECC_MODE,
        ECC_OP_RESET,
        ECC_OP_FINISH
    };

    class ecc_op_master_work_area;

    class ecc_op_work_area {
    public:
        ecc_op_work_area();
        virtual ~ecc_op_work_area();

        void assign_work(unsigned int dev_num, bool enable_ecc,
                         unsigned int timeout);
        
        unsigned int get_dev_num() const;
        bool enable_ecc() const;
        unsigned int get_timeout() const;

        void next_step(ecc_op_step step, const string &info_msg);
        void signal_ecc_op_complete(bool success);
        void finish_with_msg(msg_type type, const string &msg);
        void get_status(ecc_op_step *step, msg_type *type, string *msg);

        micsmc_cli *this_cli_;
        ecc_op_master_work_area *master_;

    private:
        unsigned int dev_num_;
        bool enable_ecc_;
        unsigned int timeout_;

        shared_ptr<host_platform::mutex> status_mtx_;
        ecc_op_step step_;
        msg_type status_msg_type_;
        string status_msg_;
    };

    class ecc_op_master_work_area {
    public:
        ecc_op_master_work_area(unsigned int num_devices,
                                micsmc_cli *this_cli);
        virtual ~ecc_op_master_work_area();

        unsigned int get_num_op_threads() const;

        void signal_ecc_op_complete(bool success);
        void signal_thread_done();

        void lock_op_complete_mtx();
        void wait_for_op_complete();
        unsigned int get_op_complete() const;
        void unlock_op_complete_mtx();

        bool at_least_one_success() const;
        bool all_ops_complete() const;
        bool all_threads_done();
        
        vector<ecc_op_work_area> op_work_areas_;

    private:
        unsigned int op_complete_;
        shared_ptr<host_platform::mutex> op_complete_mtx_;
        shared_ptr<host_platform::condition_var> op_complete_cv_;

        unsigned int op_threads_done_;
        shared_ptr<host_platform::mutex> op_threads_done_mtx_;
        bool at_least_one_success_;
    };

    struct ecc_status_work_area {
        micsmc_cli *this_cli_;
        ecc_op_master_work_area *master_;

        ecc_status_work_area(micsmc_cli *this_cli,
                             ecc_op_master_work_area *master);
    };

    cmd_line_args args_;
    cmd_config config_;
    map<unsigned int, mic_device_ptr> devices_;

    bool verbose_;
    unsigned int timeout_; // Seconds
    int exit_code_;
};
}

#endif /* MICSMC_CLI_H_ */
