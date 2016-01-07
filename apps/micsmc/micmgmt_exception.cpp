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

#include "micmgmt_exception.h"

#include <cstring>

#include <sstream>
using std::ostringstream;

using std::getline;

micmgmt_exception::micmgmt_exception(const string &err_msg,
                                     int mic_errno,
                                     int sys_errno) :
    err_msg_(err_msg),
    mic_errno_(mic_errno),
    sys_errno_(sys_errno)
{
    if (mic_errno_ != E_MIC_SUCCESS)
        mic_err_msg_ = mic_get_error_string();

    if (sys_errno_ || (sys_errno_ = errno))
        sys_err_msg_ = strerror(sys_errno_);
}

micmgmt_exception::~micmgmt_exception() throw()
{
}

const char *micmgmt_exception::what() const throw()
{
    return get_composite_err_msg().c_str();
}

string micmgmt_exception::get_composite_err_msg() const
{
    ostringstream comp_err_msg;

    comp_err_msg << err_msg_;
    if (!mic_err_msg_.empty())
        comp_err_msg << ": " << mic_err_msg_;

    if (!sys_err_msg_.empty())
        comp_err_msg << ": " << sys_err_msg_;

    return comp_err_msg.str();
}

int micmgmt_exception::get_mic_err_code() const
{
    return mic_errno_;
}

int micmgmt_exception::get_sys_err_code() const
{
    return sys_errno_;
}
