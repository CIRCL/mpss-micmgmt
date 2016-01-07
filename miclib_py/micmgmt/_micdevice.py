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

import ctypes
import inspect
import _miccommon
import _micflash

MAX_STRLEN = _miccommon.MAX_STRLEN

class MicDevice():
    """This class will channel all communications to specified
    Intel(R) Xeon Phi(TM) Coprocessor.
    
    Whenever an instance is created, a SCIF connection is established to the device. When developing
    multithreaded scripts/applications, it is recommended to create one MicDevice object per thread.
    If using one MicDevice object in more than one thread is desired, the user will have to provide
    synchronization mechanisms when updating values.
    """
    def __init__(self, card_num):
        """Initializes connection to card
        
        Parameter card_num specifies the number of card to initialize.
        """
        self.mic = ctypes.cdll.LoadLibrary(_miccommon.MICMGMT_LIBRARY)
        #Initialize card card_num
        self.card_num = ctypes.c_int(card_num)
        self.mdh = ctypes.POINTER(ctypes.c_void_p)()
        self.card = ctypes.POINTER(ctypes.c_void_p)()
        self.feature = None
        self.last_func = None
        mdl = ctypes.POINTER(ctypes.c_void_p)()
        ret_code = self.mic.mic_get_devices(ctypes.byref(mdl))
        self._check_success(ret_code)
        ret_code = self.mic.mic_get_device_at_index(mdl, self.card_num.value,
                                                    ctypes.byref(self.card))
        self._check_success(ret_code)
        ret_code = self.mic.mic_free_devices(mdl)
        self._check_success(ret_code)
        ret_code = self.mic.mic_open_device(ctypes.byref(self.mdh), self.card)
        self._check_success(ret_code)
        #All opaque structs are class members. They are initialized
        #to None. When required, struct's memory will be allocated.
        for feature, struct in _miccommon.STRUCTS.iteritems():
            #Don't set devide handle to None
            if struct == "mdh": continue 
            setattr(self, struct, None)
        #Special structs for flash
        self.mic_flash_op = None
        #Attribute to hold flash image
        self.img = None
        self.img_size = None
    
    def __del__(self):
        """Destructor for MicDevice class"""
        #Check if object was correctly initialized
        if not hasattr(self, "mic"):
            return
        
        #Iterate over structs. If a struct was initialized (allocated),
        #free it.
        for feature, struct_str in _miccommon.STRUCTS.iteritems():
            if feature == _miccommon.MDH:
                continue
            struct = getattr(self, struct_str)
            if struct is None:
                continue
            free = getattr(self.mic, _miccommon.FREE_STRUCTS[feature])
            ret_code = free(struct)
            self._check_success(ret_code)
        
        #Free memory for flash structs
        if self.mic_flash_op is not None:
            self.mic.mic_flash_update_done(self.mic_flash_op)
            
        #Call mic_close_device to free mdh.
        ret_code = self.mic.mic_close_device(self.mdh)
        self._check_success(ret_code)
    
    def _check_success(self, ret_code, calling_func=None):
        """Expect E_MIC_SUCCESS from libmic calls. If not, raise exception."""
        if calling_func is None:
            calling_func = inspect.stack()[1][3]
        if ret_code != _miccommon.E_MIC_SUCCESS:
            self.mic.mic_get_error_string.restype = ctypes.c_char_p
            errstring = ctypes.create_string_buffer(self.mic.mic_get_error_string())
            self.mic.mic_clear_error_string()
            # If failed call is an initializer function, set struct to None
            # to avoid a double free.
            if calling_func in _miccommon.INIT_STRUCTS.values():
                feature = _miccommon.FEATURES[calling_func]
                struct = _miccommon.STRUCTS[feature]
                setattr(self, struct, None)
            
            #If failed call is flash method, set to None
            if calling_func in _miccommon.FLASH_APIS:
                if "read" in calling_func:
                    if self.mic_flash_op is not None:
                        self.mic_flash_op = None
                elif "update" in calling_func:
                    if self.mic_flash_op is not None:
                        self.mic_flash_op = None
            
            raise _miccommon.MicException("%s failed: %s : %s" %
                                (calling_func,
                                 _miccommon.MIC_ERRCODES[ret_code],
                                 errstring.value))
    
    def _set_feature(self, calling_func=None):
        """Whenever an API from a certain feature (PCI, FLASH, THERMAL, etc)
        is called, this method allocates memory for the corresponding
        struct, and refreshes struct when appropriate
        """
        if calling_func is None:
            #Useful when checking if a refresh for the struct is needed
            calling_func = inspect.stack()[1][3] 
            
        #If calling function is an *_update_* method,
        # struct values need to be refreshed.
        refresh = ("update" in calling_func)
        
        #Get the feature that the caller function belongs to,
        # then the struct that corresponds to the function,
        # as well as the initializer for that struct
        feature = _miccommon.FEATURES[calling_func]
        struct = getattr(self, _miccommon.STRUCTS[feature])
        #If MDH don't get initializer function. MDH has already been
        # allocated in constructor
        if feature != _miccommon.MDH:
            init_func_str = _miccommon.INIT_STRUCTS[feature]
            init = getattr(self.mic, init_func_str)
        
        #If the struct has not yet been initialized
        if struct is None:
            #Create a pointer, and initialize struct
            setattr(self, _miccommon.STRUCTS[feature],
                    ctypes.POINTER(ctypes.c_void_p)())
            struct = getattr(self, _miccommon.STRUCTS[feature])
            #Special case for core_util
            if feature == _miccommon.COREUTIL:
                ret_code = init(ctypes.byref(struct))
                self._check_success(ret_code)
                ret_code = self.mic.mic_update_core_util(self.mdh, struct)
                self._check_success(ret_code)
            else:
                ret_code = init(self.mdh, ctypes.byref(struct))
                self._check_success(ret_code, init_func_str)
        
        if refresh:
            if feature == _miccommon.COREUTIL:
                ret_code = self.mic.mic_update_core_util(self.mdh, struct)
                self._check_success(ret_code)
            else:
                free_func_str = _miccommon.FREE_STRUCTS[feature]
                free = getattr(self.mic, free_func_str)
                ret_code = free(struct)
                self._check_success(ret_code, free_func_str)
                ret_code = init(self.mdh, ctypes.byref(struct))
                self._check_success(ret_code, init_func_str)
    
    def _get_func(self, func_name=None):
        """Return a tuple (<function pointer>, <struct pointer>)
        based on caller function. This way, each wrapper
        will have access to its corresponding API function
        and struct pointers using only its name.
        """
        if not func_name:
            func_name = inspect.stack()[1][3]
        self._set_feature(func_name)
        func_ptr = getattr(self.mic, func_name)
        feature = _miccommon.FEATURES[func_name]
        struct_ptr = getattr(self, _miccommon.STRUCTS[feature])
        return (func_ptr, struct_ptr)
    
    def _get_flash_status(self):
        if self.mic_flash_op is None:
            #No flash operation
            return _micflash.FLASH_OP_IDLE
        
        status = ctypes.c_int(-1)
        status_info = ctypes.c_long()
        ret_code = self.mic.mic_get_flash_status_info(self.mic_flash_op, ctypes.byref(status_info))
        self._check_success(ret_code)
        ret_code = self.mic.mic_get_status(status_info, ctypes.byref(status))
        self._check_success(ret_code)
        ret_code = self.mic.mic_free_flash_status_info(status_info)
        self._check_success(ret_code)
        return status.value    
    
    ##################################################
    #               PCI Config
    ##################################################
    def mic_get_vendor_id(self):
        """Returns vendor ID in integer representation.
        Vendor ID is usually displayed as a hex value."""
        return _miccommon.mic_generic_func(self, ctypes.c_ushort)

    def mic_get_device_id(self):
        """Returns device ID in integer representation.
        Device ID is usually displayed as a hex value."""
        return _miccommon.mic_generic_func(self, ctypes.c_ushort)
    
    def mic_get_revision_id(self):
        """Returns PCI's revision ID in integer representation.
        Revision ID is usually displayed as a hex value."""
        return _miccommon.mic_generic_func(self, ctypes.c_ubyte)
    
    def mic_get_bus_number(self):
        """Returns PCI bus number. Valid values range from 0 to 255."""
        return _miccommon.mic_generic_func(self, ctypes.c_ushort)
    
    def mic_get_device_number(self):
        """Returns PCI slot number (0 to 31) in memory."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_subsystem_id(self):
        """Returns subsystem ID in integer representation.
        Subsystem ID is usually displayed as a hex value."""
        return _miccommon.mic_generic_func(self, ctypes.c_ushort)
    
    def mic_get_link_width(self):
        """Returns PCI link width."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_max_payload(self):
        """Returns PCI maximum data payload size."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_max_readreq(self):
        """Returns the maximum PCI read request size."""
        return _miccommon.mic_generic_func(self, ctypes.c_long)
    
    def mic_get_link_speed(self):
        """Returns PCI link speed string representation,
        including units (GT/s)."""
        return _miccommon.mic_generic_func(self, str)
    
    def mic_get_pci_domain_id(self):
        """Returns PCI domain id."""
        return _miccommon.mic_generic_func(self, ctypes.c_ushort)
    
    def mic_get_pci_class_code(self):
        """Returns PCI class code string representation."""
        return _miccommon.mic_generic_func(self, str)
    
    """Reserved to support future methods
    def mic_update_pci_config(self):
        self._set_feature()
    """
    
    ##################################################
    #               Thermal info
    ##################################################
    def mic_get_fsc_status(self):
        """Returns Fan Speed Controller status: 0-inactive / 1 active."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_die_temp(self):
        """Returns die temperature in degree Celsius.
        See mic_update_thermal_info().
        
        After calling this method, it is recommended to call mic_is_die_temp_valid
        to verify reading's validity. Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_is_die_temp_valid(self):
        """Returns True if sensor was available at the time of reading;
        False otherwise.
        
        It is recommended to call this method after calling mic_get_die_temp to
        verify reading's validity. Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, bool)
    
    def mic_get_fan_rpm(self):
        """Returns fan's RPMs. See mic_update_thermal_info()."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
        
    def mic_get_fan_pwm(self):
        """Returns fan's PWM value. See mic_update_thermal_info()."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_smc_hwrevision(self):
        """Returns SMC's HW revision."""
        return _miccommon.mic_generic_func(self, str)
    
    def mic_get_smc_fwversion(self):
        """Returns SMC's FW version."""
        return _miccommon.mic_generic_func(self, str)
    
    def mic_is_smc_boot_loader_ver_supported(self):
        """Returns True if boot-loader version is supported by
        the current SMC firmware; False otherwise.
        """
        return _miccommon.mic_generic_func(self, bool)
    
    def mic_get_smc_boot_loader_ver(self):
        """Returns SMC boot-loader version.
        
        Call this method after having verified it's supported,
        using mic_is_smc_boot_loader_ver_supported().
        
        """
        return _miccommon.mic_generic_func(self, str)
    
    def mic_get_gddr_temp(self):
        """Returns GDDR temperature in degree Celsius.
        See mic_update_thermal_info().
        
        After calling this method, it is recommended to call mic_is_gddr_temp_valid
        to verify reading's validity. Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_uint16)
    
    def mic_is_gddr_temp_valid(self):
        """Returns True if sensor was available at the time of reading;
        False otherwise.
        
        It is recommended to call this method after calling mic_get_gddr_temp
        to verify reading's validity. Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, bool)
    
    def mic_get_fanin_temp(self):
        """Returns temperature of incoming air in degree Celsius.
        See mic_update_thermal_info().
        
        After calling this method, it is recommended to call
        mic_is_fanin_temp_valid to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_uint16)
    
    def mic_is_fanin_temp_valid(self):
        """Returns True if sensor was available at the time of reading;
        False otherwise.
        
        It is recommended to call this method after calling
        mic_get_fanin_temp to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, bool)
    
    def mic_get_fanout_temp(self):
        """Returns temperature of outgoing air in degree Celsius.
        See mic_update_thermal_info().
        
        After calling this method, it is recommended to call
        mic_is_fanout_temp_valid to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_uint16)
    
    def mic_is_fanout_temp_valid(self):
        """Returns True if sensor was available at the time of reading;
        False otherwise.
        
        It is recommended to call this method after calling
        mic_get_fanout_temp to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, bool)
    
    def mic_get_vccp_temp(self):
        """Returns the VCCP (core rail) temperature in degree Celsius.
        See mic_update_thermal_info().
        
        It is recommended to call this method after calling
        mic_is_vccp_temp_valid to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_uint16)
    
    def mic_is_vccp_temp_valid(self):
        """Returns True if sensor was available at the time of reading;
        False otherwise.
        
        It is recommended to call this method after calling
        mic_get_vccp_temp to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, bool)
    
    def mic_get_vddg_temp(self):
        """Return the VDDG (uncore rail) temperature in degree Celsius.
        See mic_update_thermal_info().
        
        It is recommended to call this method after calling
        mic_is_vddg_temp_valid to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_uint16)
    
    def mic_is_vddg_temp_valid(self):
        """Returns True if sensor was available at the time of reading;
        False otherwise.
        
        It is recommended to call this method after calling
        mic_get_vddg_temp to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, bool)
    
    def mic_get_vddq_temp(self):
        """Returns the VDDQ (memory subsystem rail) temperature in
        degree Celsius. See mic_update_thermal_info().
        
        It is recommended to call this method after calling
        mic_is_vddq_temp_valid to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_uint16)
    
    def mic_is_vddq_temp_valid(self):
        """Returns True if sensor was available at the time of reading;
        False otherwise.
        
        It is recommended to call this method after calling
        mic_get_vddq_temp to verify reading's validity.
        Invalid readings will return 0.
        
        """
        return _miccommon.mic_generic_func(self, bool)
    
    def mic_update_thermal_info(self):
        """Refreshes values for thermal info methods:
        
        mic_get_fsc_status
        mic_get_die_temp
        mic_get_fan_rpm
        mic_get_fan_pwm
        mic_get_gddr_temp
        mic_get_fanin_temp
        mic_get_fanout_temp
        mic_get_vccp_temp
        mic_get_vddg_temp
        mic_get_vddq_temp
        
        When the above methods are called for the first time, they return current values;
        subsequent calls will display the same result. Call this method to refresh values.
        """
        
        self._set_feature()
    
    ##################################################
    #               Memory info
    ##################################################
    def mic_get_memory_revision(self):
        """Returns memory revision."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_memory_density(self):
        """Returns memory density in Kb."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_memory_size(self):
        """Returns memory size in KB."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_memory_speed(self):
        """Returns memory speed in KT/s."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_memory_frequency(self):
        """Returns memory frequency in one-tenths of GT/s."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_memory_voltage(self):
        """Returns memory voltage in uV."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_ecc_mode(self):
        """Returns Error Correction mode. 0-inactive / 1 active.""" 
        return _miccommon.mic_generic_func(self, ctypes.c_ushort)
    
    def mic_set_ecc_mode(self, bool_ecc):
        """Sets Error Correction mode. Expects boolean value."""
        short_ecc = int(bool_ecc)
        ret_code = self.mic.mic_set_ecc_mode(self.mdh, short_ecc)
        self._check_success(ret_code)

    def mic_get_memory_vendor(self):
        """Returns memory vendor as a string."""
        return _miccommon.mic_generic_func(self, str)
    
    def mic_get_memory_type(self):
        """Returns memory type as a string."""
        return _miccommon.mic_generic_func(self, str)
    
    """Reserved to support future methods.
    def mic_update_memory_info(self):
        self._set_feature()
    """
    
    ##################################################
    #               Processor info
    ##################################################
    def mic_get_processor_model(self):
        """Returns a 2-tuple, being [0] the model ID,
        and [1] the extended model ID."""
        func, struct = self._get_func()
        model = ctypes.c_ushort()
        model_ext = ctypes.c_ushort()
        ret_code = func(struct, ctypes.byref(model), ctypes.byref(model_ext))
        self._check_success(ret_code)
        return model.value, model_ext.value
    
    def mic_get_processor_family(self):
        """Returns a 2-tuple, being [0] the family ID,
        and [1] the extended family ID."""
        func, struct = self._get_func()
        family = ctypes.c_ushort()
        family_ext = ctypes.c_ushort()
        ret_code = func(struct, ctypes.byref(family), ctypes.byref(family_ext))
        self._check_success(ret_code)
        return family.value, family_ext.value
    
    def mic_get_processor_type(self):
        """Returns processor type ID."""
        return _miccommon.mic_generic_func(self, ctypes.c_ushort)
    
    def mic_get_processor_steppingid(self):
        """Returns processor stepping ID."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_processor_stepping(self):
        """Returns processor stepping as a string."""
        return _miccommon.mic_generic_func(self, str)
    
    """Reserved to support future methods.
    def mic_update_processor_info(self):
        self._set_feature()
    """
    
    ##################################################
    #               Cores info
    ##################################################
    def mic_get_cores_count(self):
        """Returns number of cores for the Intel(R) Xeon Phi(TM) Coprocessor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_cores_voltage(self):
        """Returns voltage value for the Intel(R) Xeon Phi(TM) Coprocessor's
        cores in uV."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_cores_frequency(self):
        """Returns frequency value for the Intel(R) Xeon Phi(TM) Coprocessor's
        cores in KHz."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    """Reserve to support future methods.
    def mic_update_cores_info(self):
        self._set_feature()
    """
        
    ##################################################
    #               Version info
    ##################################################
    def mic_get_uos_version(self):
        """Returns Coprocessor OS version as a string."""
        return _miccommon.mic_generic_func(self, str)
    
    def mic_get_flash_version(self):
        """Returns flash version as a string."""
        return _miccommon.mic_generic_func(self, str)
    
    def mic_get_fsc_strap(self):
        """Returns the Fan Speed Controller firmware version as a string."""
        return _miccommon.mic_generic_func(self, str)
    
    """Reserved to support future methods
    def mic_update_version_info(self):
        self._set_feature()
    """
        
    ##################################################
    #               Silicon SKU
    ##################################################
    def mic_get_silicon_sku(self):
        """Returns silicon SKU as a string."""
        return _miccommon.mic_generic_func(self, str)
    
    ##################################################
    #               Serial Number
    ##################################################
    def mic_get_serial_number(self):
        """Returns Intel(R) Xeon Phi(TM) Coprocessor's serial number."""
        return _miccommon.mic_generic_func(self, str)
    
    ##################################################
    #               Power utilization info
    ##################################################
    def mic_get_total_power_readings_w0(self):
        """Returns Window 0 power utilization.
        
        The status of this register is determined by
        mic_get_total_power_sensor_sts_w0().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_total_power_sensor_sts_w0(self):
        """Returns the status of the Window 0 power utilization register."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_total_power_readings_w1(self):
        """Returns Window 1 power utilization.
        
        The status of this sensor is determined by
        mic_get_total_power_sensor_sts_w1().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_total_power_sensor_sts_w1(self):
        """Returns the status of the Window 1 power utilization sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_inst_power_readings(self):
        """Returns instantaneous power utilization.
        
        The status of this sensor is determined by
        mic_get_inst_power_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_inst_power_sensor_sts(self):
        """Returns the status of the instantaneous power utilization sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_get_max_inst_power_readings(self):
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_max_inst_power_sensor_sts(self):
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_pcie_power_readings(self):
        """Returns PCIe power utilization.
        
        The status of this sensor is determined by
        mic_get_pcie_power_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_pcie_power_sensor_sts(self):
        """Returns the status of the PCIe power utilization sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_c2x3_power_readings(self):
        """Returns the value from the 2X3 connector sensor.
        
        The status of this sensor is determined by
        mic_get_c2x3_power_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_c2x3_power_sensor_sts(self):
        """Returns the status of the 2X3 power connector sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_c2x4_power_readings(self):
        """Returns the value from the 2X4 connector sensor.
        
        The status of this sensor is determined by
        mic_get_c2x4_power_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_c2x4_power_sensor_sts(self):
        """Returns the status of the 2X4 power connector sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vccp_power_readings(self):
        """Returns VCCP (core rail) power utilization.
        
        The status of this sensor is determined by
        mic_get_vccp_power_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vccp_power_sensor_sts(self):
        """Returns the status of the VCCP power register."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vccp_current_readings(self):
        """Returns VCCP (core rail) current readings.
        
        The status of this sensor is determined by
        mic_get_vccp_current_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vccp_current_sensor_sts(self):
        """Returns the status of the VCCP current sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vccp_voltage_readings(self):
        """Returns VCCP (core rail) voltage readings.
        
        The status of this sensor is determined by
        mic_get_vccp_voltage_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vccp_voltage_sensor_sts(self):
        """Returns the status of the VCCP voltage sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddg_power_readings(self):
        """Returns VDDG (uncore rail) power utilization.
        
        The status of this sensor is determined by
        mic_get_vddg_power_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddg_power_sensor_sts(self):
        """Returns the status of the VDDG power sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddg_current_readings(self):
        """Returns VDDG (uncore rail) current readings.
        
        The status of this sensor is determined by
        mic_get_vddg_current_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddg_current_sensor_sts(self):
        """Returns the status of the VDDG current sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddg_voltage_readings(self):
        """Returns VDDG (uncore rail) voltage readings.
        
        The status of this sensor is determined by
        mic_get_vddg_voltage_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddg_voltage_sensor_sts(self):
        """Returns the status of the VDDG voltage sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddq_power_readings(self):
        """Returns VDDQ (memory subsystem rail) power utilization.
        
        The status of this sensor is determined by
        mic_get_vddq_power_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddq_power_sensor_sts(self):
        """Returns the status of the VDDQ power sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddq_current_readings(self):
        """Returns VDDQ (memory subsystem rail) current readings.
        
        The status of this sensor is determined by
        mic_get_vddq_current_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddq_current_sensor_sts(self):
        """Returns the status of the VDDQ current sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddq_voltage_readings(self):
        """Returns VDDQ (memory subsystem rail) voltage readings.
        
        The status of this sensor is determined by
        mic_get_vddq_voltage_sensor_sts().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_vddq_voltage_sensor_sts(self):
        """Returns the status of the VDDQ voltage sensor."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)
    
    def mic_update_power_utilization_info(self):
        """Refreshes values for power utilization methods:
    
        mic_get_total_power_sensor_sts_w0
        mic_get_total_power_readings_w1
        mic_get_total_power_sensor_sts_w1
        mic_get_inst_power_readings
        mic_get_inst_power_sensor_sts
        mic_get_max_inst_power_readings
        mic_get_max_inst_power_sensor_sts
        mic_get_pcie_power_readings
        mic_get_pcie_power_sensor_sts
        mic_get_c2x3_power_readings
        mic_get_c2x3_power_sensor_sts
        mic_get_c2x4_power_readings
        mic_get_c2x4_power_sensor_sts
        mic_get_vccp_power_readings
        mic_get_vccp_power_sensor_sts
        mic_get_vccp_current_readings
        mic_get_vccp_current_sensor_sts
        mic_get_vccp_voltage_readings
        mic_get_vccp_voltage_sensor_sts
        mic_get_vddg_power_readings
        mic_get_vddg_power_sensor_sts
        mic_get_vddg_current_readings
        mic_get_vddg_current_sensor_sts
        mic_get_vddg_voltage_readings
        mic_get_vddg_voltage_sensor_sts
        mic_get_vddq_power_readings
        mic_get_vddq_power_sensor_sts
        mic_get_vddq_current_readings
        mic_get_vddq_current_sensor_sts
        mic_get_vddq_voltage_readings
        mic_get_vddq_voltage_sensor_sts        
        
        When the above methods are called for the first time, they return current values;
        subsequent calls will display the same result. Call this method to refresh values.
        
        """
        self._set_feature()
    
    ##################################################
    #               Core utilization
    ##################################################
    def mic_get_idle_counters(self):
        """Returns a list containing the count of idle jiffies for each core.
        
        See mic_update_core_util().
        
        """
        func, struct = self._get_func()
        num_cores = ctypes.c_ushort()
        #Get num_cores
        ret_code = self.mic.mic_get_num_cores(struct, ctypes.byref(num_cores))
        self._check_success(ret_code)
        #Create an array for idle_counters
        idle_counters = (ctypes.c_ulong * num_cores.value)()
        ret_code = func(struct, ctypes.byref(idle_counters))
        self._check_success(ret_code)
        idle_list = list()
        #Append all idle counters to a list, and return it
        for idle in idle_counters:
            idle_list.append(idle)
        return idle_list
    
    def mic_get_nice_counters(self):
        """Returns a list containing the count of jiffies spent running
        at reduced priority (nice) for each core.
        
        See mic_update_core_util().
        
        """
        func, struct = self._get_func()
        num_cores = ctypes.c_ushort()
        ret_code = self.mic.mic_get_num_cores(struct, ctypes.byref(num_cores))
        self._check_success(ret_code)
        nice_counters = (ctypes.c_ulong * num_cores.value)()
        ret_code = func(struct, ctypes.byref(nice_counters))
        self._check_success(ret_code)
        nice_list = list()
        for nice in nice_counters:
            nice_list.append(nice)
        return nice_list
    
    def mic_get_sys_counters(self):
        """Returns a list containing the count of sys (kernel)
        jiffies for each core.
        
        See mic_update_core_util().
        
        """
        func, struct = self._get_func()
        num_cores = ctypes.c_ushort()
        ret_code = self.mic.mic_get_num_cores(struct, ctypes.byref(num_cores))
        self._check_success(ret_code)
        sys_counters = (ctypes.c_ulong * num_cores.value)()
        ret_code = func(struct, ctypes.byref(sys_counters))
        self._check_success(ret_code)
        sys_list = list()
        for sysc in sys_counters:
            sys_list.append(sysc)
        return sys_list
    
    def mic_get_user_counters(self):
        """Returns a list containing the count of user mode jiffies
        for each core.
        
        See mic_update_core_util().
        
        """
        func, struct = self._get_func()
        num_cores = ctypes.c_ushort()
        ret_code = self.mic.mic_get_num_cores(struct, ctypes.byref(num_cores))
        self._check_success(ret_code)
        user_counters = (ctypes.c_ulong * num_cores.value)()
        ret_code = func(struct, ctypes.byref(user_counters))
        self._check_success(ret_code)
        user_list = list()
        for user_v in user_counters:
            user_list.append(user_v)
        return user_list
    
    def mic_get_idle_sum(self):
        """Returns sum of idle time for all the cores.
        
        See mic_update_core_util().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_ulong)
    
    def mic_get_sys_sum(self):
        """Returns sum of time spent on sys mode (kernel)
        for all the cores.
        
        See mic_update_core_util().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_ulong)
    
    def mic_get_nice_sum(self):
        """Returns sum of time spent by processes running at
        reduced priority for all the cores.
        
        See mic_update_core_util().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_ulong)
    
    def mic_get_user_sum(self):
        """Returns sum of time spent on user mode for all the cores.
        
        See mic_update_core_util().
        
        """
        return _miccommon.mic_generic_func(self, ctypes.c_ulong)
    
    def mic_get_jiffy_counter(self):
        """Returns the unit for all the counters. See mic_update_core_util()."""
        return _miccommon.mic_generic_func(self, ctypes.c_ulong)
    
    def mic_get_num_cores(self):
        """Returns number of cores for the Intel(R) Xeon Phi(TM) Coprocessor."""
        return _miccommon.mic_generic_func(self, ctypes.c_ushort)
    
    def mic_get_threads_core(self):
        """Returns number of threads per core."""
        return _miccommon.mic_generic_func(self, ctypes.c_ushort)

    def mic_get_tick_count(self):
        """Returns the tick count."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_update_core_util(self):
        """Refreshes values for core utilization methods:
        
        mic_get_nice_counters
        mic_get_sys_counters
        mic_get_user_counters
        mic_get_idle_sum
        mic_get_sys_sum
        mic_get_nice_sum
        mic_get_user_sum
        mic_get_jiffy_counter

        When the above methods are called for the first time, they return current values;
        subsequent calls will display the same result. Call this method to refresh values.
        
        """
        self._set_feature()
        struct = getattr(self, "mic_core_util")
        ret_code = self.mic.mic_update_core_util(self.mdh, struct)
        self._check_success(ret_code)
        
    ##################################################
    #               Memory Utilization
    ##################################################
    
    def mic_get_total_memory_size(self):
        """Returns total memory size in KB."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)
    
    def mic_get_available_memory_size(self):
        """Returns available memory size in KB. See mic_update_memory_util()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_memory_buffers_size(self):
        """Returns the amount of memory used by kernel buffers in KB.
        See mic_update_memory_util()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)
    
    def mic_update_memory_util(self):
        """Refreshes values for memory utilization methods:
        
        mic_get_available_memory_size
        mic_get_memory_buffers_size

        When the above methods are called for the first time, they return current values;
        subsequent calls will display the same result. Call this method to refresh values.
        
        """
        self._set_feature()
        
    ##################################################
    #               Led Alert
    ##################################################
    def mic_get_led_alert(self):
        """Returns 1 if LED alert is set; 0 otherwise."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_set_led_alert(self, uint_led):
        """Sets LED alert."""
        _miccommon.mic_generic_set(self, uint_led, ctypes.c_uint, True)

    ##################################################
    #               Turbo State
    ##################################################
    def mic_get_turbo_state(self):
        """Returns 1 if turbo active; 0 otherwise."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_turbo_state_valid(self):
        """Returns 1 if turbo available; 0 otherwise."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_turbo_mode(self):
        """Returns 1 if turbo mode enabled; 0 otherwise."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_set_turbo_mode(self, uint_turbo):
        """Sets turbo mode to turbo_mode."""
        _miccommon.mic_generic_set(self, uint_turbo, ctypes.c_uint, True)

    ##################################################
    #               Device info
    ##################################################
    def mic_get_device_type(self):
        """Returns the type of the Intel(R) Xeon Phi(TM) Coprocessor board."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)
    
    def mic_get_device_name(self):
        """Returns device name as a string."""
        self.mic.mic_get_device_name.restype = ctypes.c_char_p
        name = self.mic.mic_get_device_name(self.mdh)
        return name
    
    ##################################################
    #               Maintenance mode
    ##################################################
    def mic_enter_maint_mode(self):
        """Enter maintenance mode.
        
        Device must be in ready state to enter maintenance mode. See micctrl documentation.
        This method operates asynchronously, and returns immediately after calling it.
        To ensure that the switch to maintenance mode has completed successfully, use
        mic_in_maint_mode().
        
        """
        ret_code = self.mic.mic_enter_maint_mode(self.mdh)
        self._check_success(ret_code)
        return ret_code
    
    def mic_leave_maint_mode(self):
        """Leave maintenance mode.
        
        After leaving maintenance mode, reset device. See micctrl documentation.
        This method operates asynchronously, and returns immediately after calling it.
        To ensure that the switch back to ready state has completed successfully, use
        mic_in_ready_state().
        
        """
        ret_code = self.mic.mic_leave_maint_mode(self.mdh)
        self._check_success(ret_code)
        return ret_code
    
    def mic_in_maint_mode(self):
        """Returns 1 if device was successfully set to maintenance mode;
        0 otherwise."""
        mode = ctypes.c_int()
        ret_code = self.mic.mic_in_maint_mode(self.mdh, ctypes.byref(mode))
        self._check_success(ret_code)
        return mode.value
    
    def mic_in_ready_state(self):
        """Returns 1 if device was successfully set to ready state;
        0 otherwise."""
        state = ctypes.c_int()
        ret_code = self.mic.mic_in_ready_state(self.mdh, ctypes.byref(state))
        self._check_success(ret_code)
        return state.value
    
    def mic_get_post_code(self):
        """Returns the POST code in string representation."""
        return _miccommon.mic_generic_func(self, str)

    ##################################################
    #               Device Properties
    ##################################################
    def mic_get_sysfs_attribute(self, attribute):
        buf = ctypes.create_string_buffer(MAX_STRLEN)
        size = ctypes.c_long(MAX_STRLEN)
        ret_code = self.mic.mic_get_sysfs_attribute(self.mdh,
                                             ctypes.create_string_buffer(attribute),
                                             ctypes.byref(buf),
                                             ctypes.byref(size))
        self._check_success(ret_code)
        return buf.value
    
    ##################################################
    #               RAS
    ##################################################
    def mic_is_ras_avail(self):
        """Returns True if RAS daemon is available in the device;
        False otherwise."""
        return _miccommon.mic_generic_func(self, bool)
    
    ##################################################
    #               Power Limit
    ##################################################
    def mic_get_power_phys_limit(self):
        """Returns the physical power limit."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_power_hmrk(self):
        """Returns the high watter mark in Watts (power limit 0/CPU hot)."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)
    
    def mic_get_power_lmrk(self):
        """Returns the low watter mark in Watts (power limit 1/power alert)."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_time_window0(self):
        """Returns time window 0 in msec (corresponding to power limit 0)."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_time_window1(self):
        """Returns time window 1 in msec (corresponding to power limit 1)."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_set_power_limit0(self, uint_power, uint_time):
        """Sets power limit 0."""
        ret_code = self.mic.mic_set_power_limit0(self.mdh,
                                                 ctypes.c_uint(uint_power),
                                                 ctypes.c_uint(uint_time))
        self._check_success(ret_code)

    def mic_set_power_limit1(self, uint_power, uint_time):
        """Sets power limit 1."""
        ret_code = self.mic.mic_set_power_limit1(self.mdh,
                                                 ctypes.c_uint(uint_power),
                                                 ctypes.c_uint(uint_time))
        self._check_success(ret_code)
    
    ##################################################
    #               Throttle States
    ##################################################
    
    def mic_get_thermal_ttl_active(self):
        """Returns the state of thermal throttling.
        See mic_update_ttl_state()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_thermal_ttl_current_len(self):
        """Returns the duration of the current thermal throttling event.
        See mic_update_ttl_state()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_thermal_ttl_count(self):
        """Returns the total count of thermal throttling events.
        See mic_update_ttl_state()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_thermal_ttl_time(self):
        """Returns the total time spent in thermal throttling events.
        See mic_update_ttl_state()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_power_ttl_active(self):
        """Returns the state of power throttling.
        See mic_update_ttl_state()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_power_ttl_current_len(self):
        """Returns the duration of the current power throttling event.
        See mic_update_ttl_state()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_power_ttl_count(self):
        """Returns the total time spent in power throttling events.
        See mic_update_ttl_state()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)

    def mic_get_power_ttl_time(self):
        """Returns the total time spent in power throttling events.
        See mic_update_ttl_state()."""
        return _miccommon.mic_generic_func(self, ctypes.c_uint)
    
    def mic_update_ttl_state(self):
        """Refreshes values for throttling state methods:
        
        mic_get_thermal_ttl_current_len
        mic_get_thermal_ttl_count
        mic_get_thermal_ttl_time
        mic_get_power_ttl_active
        mic_get_power_ttl_current_len
        mic_get_power_ttl_count
        mic_get_power_ttl_time
        
        When the above methods are called for the first time, they return current values;
        subsequent calls will display the same result. Call this method to refresh values.
        
        """
        self._set_feature()
        
    ##################################################
    #      Coprocessor OS Power Management Config
    ##################################################
    def mic_get_cpufreq_mode(self):
        """Returns 1 if the cpufreq (P state) power management feature
        is enabled; 0 otherwise.
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_corec6_mode(self):
        """Returns 1 if the corec6 (Core C6) power management feature
        is enabled; 0 otherwise.
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_pc3_mode(self):
        """Returns 1 if the pc3 (Package C3) power management feature
        is enabled; 0 otherwise.
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_get_pc6_mode(self):
        """Returns 1 if the pc6 (Package C6) power management feature
        is enabled; 0 otherwise.
        """
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_update_uos_pm_config(self):
        """Refreshes values for power management methods:
        
        mic_get_cpufreq_mode
        mic_get_corec6_mode
        mic_get_pc3_mode
        mic_get_pc6_mode
        
        When the above methods are called for the first time, they return current values;
        subsequent calls will display the same result. Call this method to refresh values.
        
        """
        self._set_feature()

    ##################################################
    #                      UUID
    ##################################################
    def mic_get_uuid(self):
        """Returns UUID string representation."""
        return _miccommon.mic_generic_func(self, str)

    ##################################################
    #               Persistence Flag
    ##################################################
    def mic_get_smc_persistence_flag(self):
        """Returns whether the SMC's persistence flag is set."""
        return _miccommon.mic_generic_func(self, ctypes.c_int)

    def mic_set_smc_persistence_flag(self, int_persist):
        """Sets the SMC's persistence flag."""
        _miccommon.mic_generic_set(self, int_persist, ctypes.c_int)
