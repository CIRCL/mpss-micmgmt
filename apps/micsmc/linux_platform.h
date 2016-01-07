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

#ifndef LINUX_PLATFORM_H_
#define LINUX_PLATFORM_H_

#ifdef __linux__

#include "host_platform.h"

#include <pthread.h>

#include <string>
using std::string;

#include "mic_device_class.h"

namespace host_platform {

class linux_mpss_utils: public mpss_utils {
public:
    linux_mpss_utils();
    virtual ~linux_mpss_utils();

    virtual string get_mic_driver_version();
    virtual void set_mic_pm_config(mic_device_class &device,
                                   bool cpufreq_enabled, bool corec6_enabled,
                                   bool pc3_enabled, bool pc6_enabled);
    virtual const char *get_micsmc_doc_path();

    string get_conf_file_path(mic_device_class &device);

    static const char *DRIVER_VER_ENTRY_PATH;
    static const char *MIC_CONFIG_PATH;
    static const char *MICSMC_DOC_PATH;

private:
    string construct_pm_conf_line(bool cpufreq_enabled, bool corec6_enabled,
                                  bool pc3_enabled, bool pc6_enabled);
};

class linux_os_utils: public os_utils {
public:
    linux_os_utils();
    virtual ~linux_os_utils();

    virtual void sleep_sec(unsigned int sec);
    virtual void sleep_ms(unsigned int ms);

    virtual void thread_exit();
};

class linux_thread: public thread {
public:
    linux_thread();
    virtual ~linux_thread();

    virtual void join();

private:
    virtual void start(thread_fn fn, void *param);

    pthread_t thread_;
};

class linux_cond_var: public condition_var {
public:
    linux_cond_var();
    virtual ~linux_cond_var();

    virtual void wait(mutex *mtx);
    virtual void signal_one();
    virtual void signal_all();

private:
    pthread_cond_t cond_;
};

class linux_mutex: public mutex {
public:
    linux_mutex();
    virtual ~linux_mutex();

    virtual void lock();
    virtual bool try_lock();
    virtual void unlock();

private:
    pthread_mutex_t mutex_;

    friend void linux_cond_var::wait(mutex *mtx);
};

}

#endif

#endif
