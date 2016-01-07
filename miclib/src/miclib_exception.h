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
/// \brief This file contains C++ exception class definitions.

#ifndef MICLIB_SRC_MICLIB_EXCEPTION_H_
#define MICLIB_SRC_MICLIB_EXCEPTION_H_

#include <errno.h>           // For errno
#include <stdint.h>
#ifdef __linux__
#include <execinfo.h>
#endif
#include <string>               // For string
#include <exception>            // For exception class
#include <iostream>
#include "miclib_int.h"
#include "miclib.h"
#ifndef __linux__
#define __thread    __declspec(thread)
#define error_t     int
#endif

/// \brief mic_exception class derived from std::exception class.
class mic_exception : public std::exception
{
public:
    /// \brief Construct a mic_exception object with a explanatory message.
    /// \param internal_err The internal error number.
    /// \param err_msg  Where the exception is coming from.
    /// \param errno_num The errno code to set.
    /// \param ras_errno The RAS error number to set.
    mic_exception(mic_error_code internal_err, std::string err_msg,
                  int sys_error_code = 0, int ras_errno = 0) throw();

    /// \brief mic_exception destructor
    ~mic_exception() throw();

    /// \brief  Get the exception message string.
    /// \return Exception message string.
    // const char *what() const throw();

    /// \brief Get the errno we previously set.
    /// \return exception error code.
    int get_sys_errno() const throw();

    /// \brief Get the miclib error code.
    /// \return miclib error code.
    int get_mic_errno() const throw();

    /// \brief Get the RAS module error number.
    int get_ras_errno() const throw();

    /// \brief Get the faulting object
    /// \return error object
    std::string get_error_msg() const throw();

    static const char *ts_get_error_msg();
    static void ts_clear_error_msg();

    static int ts_get_ras_errno();
    static const char *ras_strerror(int ras_errno);
    static void ts_clear_ras_errno();

private:

    static const int ERROR_STR_MAX;
    // Don't use this variable directly, use ts_get_error_msg
    static __thread char mic_error_str[];

    // Don't use this variable directly, use ts_get_ras_errno
    static __thread int mic_ras_errno;

    static const int MAX_RAS_ERRNO;
    static const char *MIC_RAS_ERRORS[];

    error_t _sys_errno;             // System Error code
    std::string _error_msg;         // Where the error is coming from
    mic_error_code _mic_errno;      // MIC lib specific  error code
    int _ras_errno;                 // RAS error number
};

#endif /* MICLIB_SRC_MICLIB_EXCEPTION_H_ */
