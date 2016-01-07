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

#include "micsmc_exception.h"

#include <sstream>
using std::ostringstream;

micsmc_exception::micsmc_exception(const string &err_msg) :
    err_msg_(err_msg)
{
}

micsmc_exception::micsmc_exception(const exception &ex) :
    err_msg_(ex.what())
{
}

micsmc_exception::micsmc_exception(const string &err_msg,
                                   const exception &ex) :
    err_msg_(err_msg),
    sec_err_msg_(ex.what())
{
}

micsmc_exception::~micsmc_exception() throw()
{
}

string micsmc_exception::get_composite_err_msg() const
{
    ostringstream comp_err_msg;

    comp_err_msg << err_msg_;
    if (!sec_err_msg_.empty())
        comp_err_msg << ": " << sec_err_msg_;

    return comp_err_msg.str();
}

const char *micsmc_exception::what() const throw()
{
    return get_composite_err_msg().c_str();
}
