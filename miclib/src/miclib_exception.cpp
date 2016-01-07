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

/// \file mic_exception.h

#include <string.h>
#include "miclib_exception.h"
#include "miclib_int.h"

const int mic_exception::ERROR_STR_MAX = 256;
__thread char mic_exception::mic_error_str[mic_exception::ERROR_STR_MAX] =
        { '\0' };

const int mic_exception::MAX_RAS_ERRNO = 10;
__thread int mic_exception::mic_ras_errno = 0;

const char *mic_exception::MIC_RAS_ERRORS[] = {
        "Invalid command/operation",
        "Invalid length for operation",
        "Invalid parameter for operation",
        "Invalid data block",
        "Permission denied",
        "Out of memory",
        "SMC communication error",
        "No valid value to report",
        "Unsupported feature/operation",
        "Parameter out of range"};

mic_exception::mic_exception(mic_error_code internal_err, std::string err_msg,
                             int sys_error_code, int ras_errno) throw()
    : _error_msg(err_msg), _mic_errno(internal_err)
{
#ifdef DEBUG
    void *stack_buf[100];
    int stack_size = 0;
#endif

     // Set errno number if we passed in a value different than 0.
    if (sys_error_code != 0)
        errno = sys_error_code;

    _sys_errno = errno;

    // Set the RAS errno if we passed in a value different than 0.
    if (ras_errno != 0)
        mic_ras_errno = ras_errno;

    _ras_errno = mic_ras_errno;

    strncpy(mic_error_str, _error_msg.c_str(), ERROR_STR_MAX - 1);
    mic_error_str[ERROR_STR_MAX - 1] = '\0';
#ifdef DEBUG
    stack_size = backtrace(stack_buf, 100);
    backtrace_symbols_fd(stack_buf, stack_size, fileno(stderr));
#endif
}

mic_exception::~mic_exception() throw()
{
}

error_t mic_exception::get_sys_errno() const throw()
{
    return _sys_errno;
}

int mic_exception::get_mic_errno() const throw()
{
    return _mic_errno;
}

int mic_exception::get_ras_errno() const throw()
{
    return _ras_errno;
}

const char *mic_exception::ras_strerror(int ras_errno)
{
    if (ras_errno < 1 || ras_errno > MAX_RAS_ERRNO)
        return NULL;

    return MIC_RAS_ERRORS[ras_errno - 1];
}

std::string mic_exception::get_error_msg() const throw()
{
    return _error_msg;
}

const char *mic_exception::ts_get_error_msg()
{
    /*
     * The following string is unit-tested at Tests/miclib/miclib_test.h,
     * if you need to change the string, be sure to do the same at that
     * location.
     */
    if (mic_error_str[0] == '\0')
        strcpy(mic_error_str, "No error registered");

    return mic_error_str;
}

void mic_exception::ts_clear_error_msg()
{
    mic_error_str[0] = '\0';
}

int mic_exception::ts_get_ras_errno()
{
    return mic_ras_errno;
}

void mic_exception::ts_clear_ras_errno()
{
    mic_ras_errno = 0;
}
