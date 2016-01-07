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

#ifdef __linux__

#include "linux_platform.h"
using namespace host_platform;

#include <cstring>
#include <unistd.h>

#include <fstream>
using std::ifstream;
using std::ofstream;

#include <sstream>
using std::ostringstream;

#include <iostream>
using std::endl;

#include <vector>
using std::vector;

#include <stdexcept>
using std::runtime_error;

#include <system_error>
using std::system_error;
using std::error_code;
using std::system_category;

const char *linux_mpss_utils::DRIVER_VER_ENTRY_PATH =
        "/sys/class/mic/ctrl/version";
const char *linux_mpss_utils::MIC_CONFIG_PATH = "/etc/mpss/";
const char *linux_mpss_utils::MICSMC_DOC_PATH = "/usr/share/doc/micmgmt/";

static inline void on_sys_error_throw(int error, const string &msg)
{
    if (error) {
        string error_msg = msg + ": " + strerror(error);
        throw system_error(error_code(error, system_category()), error_msg);
    }
}

linux_mpss_utils::linux_mpss_utils()
{
}

linux_mpss_utils::~linux_mpss_utils()
{
}

string linux_mpss_utils::get_mic_driver_version()
{
    ifstream version_file(DRIVER_VER_ENTRY_PATH);
    string version;

    if (!version_file.is_open()) {
        version_file.close();
        throw runtime_error(string("unable to open: ") + DRIVER_VER_ENTRY_PATH);
    }

    getline(version_file, version);
    if (!version_file.good()) {
        version_file.close();
        throw runtime_error(string("unable to read: ") + DRIVER_VER_ENTRY_PATH);
    }
    version_file.close();

    return version;
}

void linux_mpss_utils::set_mic_pm_config(mic_device_class &device,
                                         bool cpufreq_enabled, bool
                                         corec6_enabled, bool pc3_enabled,
                                         bool pc6_enabled)
{
    ifstream mic_conf_in;
    ofstream mic_conf_out;
    string path = get_conf_file_path(device);

    string pm_line = construct_pm_conf_line(cpufreq_enabled, corec6_enabled,
                                            pc3_enabled, pc6_enabled);
    int pm_line_at = -1;

    vector<string> file_content;
    string line;

    // Read configuration file...
    mic_conf_in.open(path.c_str());
    for (int i = 0; mic_conf_in.good(); i++) {
        getline(mic_conf_in, line);
        file_content.push_back(line);
        if (line.find("PowerManagement") != string::npos)
            pm_line_at = i;
    }
    mic_conf_in.close();

    // Set the power management line
    if (pm_line_at != -1)
        file_content.at(pm_line_at) = pm_line;  // Replace PM line
    else
        file_content.push_back(pm_line);        // Not found, so add it

    // Write the configuration file with the new settings...
    mic_conf_out.open(path.c_str());
    if (!mic_conf_out.is_open())
        throw runtime_error("unable to open configuration file: " + path);

    for (vector<string>::const_iterator it = file_content.begin();
         it != file_content.end(); ++it) {
        if (it != file_content.begin())
            mic_conf_out << endl;
        mic_conf_out << *it;
    }
    mic_conf_out.close();
}

const char *linux_mpss_utils::get_micsmc_doc_path()
{
    return MICSMC_DOC_PATH;
}

string linux_mpss_utils::get_conf_file_path(mic_device_class &device)
{
    ostringstream oss;

    oss << MIC_CONFIG_PATH << device.get_device_name() << ".conf";
    return oss.str();
}

string linux_mpss_utils::construct_pm_conf_line(bool cpufreq_enabled,
                                                bool corec6_enabled,
                                                bool pc3_enabled,
                                                bool pc6_enabled)
{
    ostringstream oss;

    oss << "PowerManagement \"";
    oss << (cpufreq_enabled ? "cpufreq_on;" : "cpufreq_off;");
    oss << (corec6_enabled ? "corec6_on;" : "corec6_off;");
    oss << (pc3_enabled ? "pc3_on;" : "pc3_off;");
    oss << (pc6_enabled ? "pc6_on" : "pc6_off");
    oss << "\"";

    return oss.str();
}

linux_os_utils::linux_os_utils()
{
}

linux_os_utils::~linux_os_utils()
{
}

void linux_os_utils::sleep_sec(unsigned int sec)
{
    sleep(sec);
}

void linux_os_utils::sleep_ms(unsigned int ms)
{
    usleep(ms * 1000);
}

void linux_os_utils::thread_exit()
{
    pthread_exit(NULL);
}

linux_thread::linux_thread():
        thread_(0)
{
}

linux_thread::~linux_thread()
{
}

void linux_thread::start(thread_fn fn, void *param)
{
    pthread_attr_t attr;
    int rc;

    rc = pthread_attr_init(&attr);
    on_sys_error_throw(rc, "pthread_attr_init");

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    rc = pthread_create(&thread_, &attr, fn, param);
    on_sys_error_throw(rc, "pthread_create");

    pthread_attr_destroy(&attr);
}

void linux_thread::join()
{
    int rc;
    void *status;

    rc = pthread_join(thread_, &status);
    on_sys_error_throw(rc, "pthread_join");
}

linux_mutex::linux_mutex()
{
    int rc = pthread_mutex_init(&mutex_, NULL);
    on_sys_error_throw(rc, "pthread_mutex_init");
}

linux_mutex::~linux_mutex()
{
    pthread_mutex_destroy(&mutex_);
}

void linux_mutex::lock()
{
    int rc = pthread_mutex_lock(&mutex_);
    on_sys_error_throw(rc, "pthread_mutex_lock");
}

bool linux_mutex::try_lock()
{
    int rc = pthread_mutex_trylock(&mutex_);
    if (rc && rc != EBUSY)
        on_sys_error_throw(rc, "pthread_mutex_trylock");

    return rc != EBUSY;
}

void linux_mutex::unlock()
{
    int rc = pthread_mutex_unlock(&mutex_);
    on_sys_error_throw(rc, "pthread_mutex_unlock");
}

linux_cond_var::linux_cond_var()
{
    int rc = pthread_cond_init(&cond_, NULL);
    on_sys_error_throw(rc, "pthread_cond_init");
}

linux_cond_var::~linux_cond_var()
{
    pthread_cond_destroy(&cond_);
}

void linux_cond_var::wait(mutex *mtx)
{
    linux_mutex *lm = (linux_mutex *)mtx;
    int rc = pthread_cond_wait(&cond_, &lm->mutex_);
    on_sys_error_throw(rc, "pthread_cond_wait");
}

void linux_cond_var::signal_one()
{
    pthread_cond_signal(&cond_);
}

void linux_cond_var::signal_all()
{
    pthread_cond_broadcast(&cond_);
}

#endif
