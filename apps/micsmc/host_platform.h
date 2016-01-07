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

#ifndef HOST_PLATFORM_H_
#define HOST_PLATFORM_H_

#ifdef _WIN32

#ifdef MPSS_VERSION_WIN
#define MY_VERSION    MPSS_VERSION_WIN
#else
#define MY_VERSION    "0.0.0.0"
#endif

#else

#ifdef MPSS_VERSION
#define MY_VERSION    MPSS_VERSION
#else
#define MY_VERSION    "0.0.0.0"
#endif

#endif

#include <string>
using std::string;

#include <memory>
using std::shared_ptr;

#include "mic_device_class.h"

namespace host_platform {

class mpss_utils {
public:
    mpss_utils();
    virtual ~mpss_utils();

    virtual string get_mic_driver_version() = 0;
    virtual void set_mic_pm_config(mic_device_class &device,
                                   bool cpufreq_enabled, bool corec6_enabled,
                                   bool pc3_enabled, bool pc6_enabled) = 0;
    virtual const char *get_micsmc_doc_path() = 0;

    static mpss_utils *get_instance();

private:
    mpss_utils(const mpss_utils &);
    const mpss_utils &operator=(const mpss_utils &);
};

class os_utils {
public:
    os_utils();
    virtual ~os_utils();

    virtual void sleep_sec(unsigned int sec) = 0;
    virtual void sleep_ms(unsigned int ms) = 0;

    virtual void thread_exit() = 0;

    static os_utils *get_instance();

private:
    os_utils(const os_utils &);
    const os_utils &operator=(const os_utils &);
};

typedef void *(*thread_fn)(void *);

class thread {
public:
    thread();
    virtual ~thread();

    virtual void join() = 0;

    static shared_ptr<thread> new_thread(thread_fn fn, void *param);

protected:
    virtual void start(thread_fn fn, void *param) = 0;

private:
    thread(const thread &);
    const thread &operator=(const thread &);
};

class mutex {
public:
    mutex();
    virtual ~mutex();

    virtual void lock() = 0;
    virtual bool try_lock() = 0;
    virtual void unlock() = 0;

    static shared_ptr<mutex> new_mutex();

private:
    mutex(const mutex &);
    const mutex &operator=(const mutex &);
};

class condition_var {
public:
    condition_var();
    virtual ~condition_var();

    virtual void wait(mutex *mtx) = 0;
    virtual void signal_one() = 0;
    virtual void signal_all() = 0;

    static shared_ptr<condition_var> new_condition_var();

private:
    condition_var(const condition_var &);
    const condition_var &operator=(const condition_var &);
};

}

#endif
