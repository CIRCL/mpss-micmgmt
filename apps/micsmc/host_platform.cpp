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

#include "host_platform.h"

#ifdef __linux__
#include "linux_platform.h"
#elif defined _WIN32
#include "windows_platform.h"
#endif

using namespace host_platform;

mpss_utils::mpss_utils()
{
}

mpss_utils::~mpss_utils()
{
}

mpss_utils *mpss_utils::get_instance()
{
#ifdef __linux__
    static linux_mpss_utils mpss;
#elif defined _WIN32
    static windows_mpss_utils mpss;
#else
    // Force compilation error: unsupported platform
#endif

    return &mpss;
}

os_utils::os_utils()
{
}

os_utils::~os_utils()
{
}

os_utils *os_utils::get_instance()
{
#ifdef __linux__
    static linux_os_utils os;
#elif defined _WIN32
    static windows_os_utils os;
#else
    // Force compilation error: unsupported platform
#endif

    return &os;
}

thread::thread()
{
}

thread::~thread()
{
}

shared_ptr<thread> thread::new_thread(thread_fn fn, void *param)
{
#ifdef __linux__
    shared_ptr<thread> thd(new linux_thread());
#elif defined _WIN32
    shared_ptr<thread> thd(new windows_thread());
#else
    // Force compilation error: unsupported platform
#endif

    thd->start(fn, param);
    return thd;
}

mutex::mutex()
{
}

mutex::~mutex()
{
}

shared_ptr<mutex> mutex::new_mutex()
{
#ifdef __linux__
    return shared_ptr<mutex>(new linux_mutex());
#elif defined _WIN32
    return shared_ptr<mutex>(new windows_mutex());
#else
    // Force compilation error: unsupported platform
#endif
}

condition_var::condition_var()
{
}

condition_var::~condition_var()
{
}

shared_ptr<condition_var> condition_var::new_condition_var()
{
#ifdef __linux__
    return shared_ptr<condition_var>(new linux_cond_var());
#elif defined _WIN32
    return shared_ptr<condition_var>(new windows_cond_var());
#else
    // Force compilation error: unsupported platform
#endif
}
