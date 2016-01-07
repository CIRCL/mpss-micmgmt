# Copyright 2010-2013 Intel Corporation.
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, version 2.1.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# Disclaimer: The codes contained in these modules may be specific
# to the Intel Software Development Platform codenamed Knights Ferry,
# and the Intel product codenamed Knights Corner, and are not backward
# compatible with other Intel products. Additionally, Intel will NOT
# support the codes or instruction set in future products.
#
# Intel offers no warranty of any kind regarding the code. This code is
# licensed on an "AS IS" basis and Intel is not obligated to provide
# any support, assistance, installation, training, or other services
# of any kind. Intel is also not obligated to provide any updates,
# enhancements or extensions. Intel specifically disclaims any warranty
# of merchantability, non-infringement, fitness for any particular
# purpose, and any other warranty.
#
# Further, Intel disclaims all liability of any kind, including but
# not limited to liability for infringement of any proprietary rights,
# relating to the use of the code, even if Intel is notified of the
# possibility of such liability. Except as expressly stated in an Intel
# license agreement provided with this code and agreed upon with Intel,
# no license, express or implied, by estoppel or otherwise, to any
# intellectual property rights is granted herein.

"""Bindings for Intel(R) Xeon Phi(TM) Coprocessor API Lite

This module provides bindings for the libmic library in an
object oriented, pythonic way. The memory allocation/deallocation
for the opaque structures is invisible to the user, as they
are handled as *private* class members. All the communication
is done via ctypes.

Accessing/setting card values is as easy as instantiating a mic_device
object and calling the available methods:
    >>> from micmgmt import *
    >>> # Open card 0, get die temp.
    >>> mic0 = MicDevice(0)
    >>> die_temp = mic0.mic_get_die_temp()
    >>> # Open card 1, get fan RPMs
    >>> mic1 = MicDevice(1)
    >>> fan_rpms = mic1.mic_get_fan_rpm()
    >>> # Exit Python. Objects destroyed. Memory freed.
    >>> # No memory hassles for Python programmers.

Classes:
    MicDevice

Functions:
    mic_get_ndevices -> int
"""

import ctypes
import inspect
from . _micdevice import MicDevice
from . _miccommon import E_MIC_SUCCESS, \
                         MicException, \
                         MIC_ERRCODES, \
                         MICMGMT_LIBRARY

def mic_get_ndevices():
    """Returns the number of active cards"""
    mic = ctypes.cdll.LoadLibrary(MICMGMT_LIBRARY)
    count = ctypes.c_int()
    device_list = ctypes.POINTER(ctypes.c_void_p)()
    ret_code = mic.mic_get_devices(ctypes.byref(device_list))
    if ret_code != E_MIC_SUCCESS:
        raise MicException("mic_get_devices() failed: %d : %s" %
                           (ret_code, MIC_ERRCODES[ret_code]))
    ret_code = mic.mic_get_ndevices(device_list, ctypes.byref(count))
    if ret_code != E_MIC_SUCCESS:
        raise MicException("mic_get_ndevices() failed: %d : %s" %
                           (ret_code, MIC_ERRCODES[ret_code]))
    ret_code = mic.mic_free_devices(device_list)
    if ret_code != E_MIC_SUCCESS:
        raise MicException("mic_free_devices() failed: %d : %s" %
                           (ret_code, MIC_ERRCODES[ret_code]))
    return count.value
