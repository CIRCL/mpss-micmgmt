micmgmt.py: MicMgmt Python Bindings
===================================

    micmgmt.py is a Python module that provides bindings for the MicMgmt SDK library.
    This module is exported for our external customers or cluster ISVs to aid in
    developing custom cluster functionality to programmatically access and
    control some of the hardware registers and parameters of the MIC family of cards.
    Using micmgmt.py allows operations such as monitoring voltage and
    temperature sensor readings on the card, controlling related thresholds and limits,
    or getting information about cores and memory.

1. Introduction
---------------

    Internally, micmgmt.py loads libmicmgmt.so, the shared library for MicMgmt SDK.
    libmicmgmt.so is in turn dependent on libscif.so. Both of these libraries are
    included in our RPM package. Please make sure you have both of them installed.
    
    micmgmt.py is an easy-to-use module, shipping only two classes:
        -MicDevice: This class will channel all communications to specified
            Intel(R) Xeon Phi(TM) Coprocessor.
          
        -MicException: Base exception class to raise exceptions specific to MicMgmt SDK.

2. Installation
---------------

    For installing micmgmt.py to standard python installation location
    please follow the instructions below:

    export PYTHONDONTWRITEBYTECODE=1
    cd /opt/intel/mic/mgmt/sdk/examples
    python setup.py build
    python setup.py install
    
  
2. Tutorial
-----------

    Writing code using micmgmt.py is very simple. The first step is creating an instance
    of MicDevice. The constructor for this class expects an integer specifying the
    card number to initialize.
    
-----------------------------------------------------------------------------------------------
        >>> import micmgmt
        >>> mic = micmgmt.MicDevice(0) # Open device mic0
    
-----------------------------------------------------------------------------------------------
    Simple as that. Object 'mic' is now able to communicate with mic0 card. From there,
    you can retrieve values using simple method calls. More on that later.

    If a multi-card setup is present, you may want to query information from more than
    one device. This can be done using the function 'mic_get_ndevices', which returns
    the number of Intel(R) Xeon Phi(TM) Coprocessor devices present. Please note that
    this is a module-level function, rather than a class method.
    
-----------------------------------------------------------------------------------------------
        >>> import micmgmt
        >>> device_count = micmgmt.mic_get_ndevices()
        >>> for device in range(device_count):
        >>>     #Get some values

-----------------------------------------------------------------------------------------------
        
    To retrieve values from the card, you will have to call MicDevice's methods*.
        
-----------------------------------------------------------------------------------------------
        >>> import micmgmt
        >>> device_count = micmgmt.mic_get_ndevices()
        >>> for device in range(device_count):
        >>>     mic = micmgmt.MicDevice(device)
        >>>     card_name = mic.mic_get_device_name()
        >>>     die_temp = mic.mic_get_die_temp()
        >>>     print "Die temp for card %s = %d" % (card_name, die_temp)
    
-----------------------------------------------------------------------------------------------

    * See section "Important note on variable values"


2.1 Important note on variable values
-------------------------------------

    Some of the values available via MicDevice objects are, by nature, subject to
    change. Take fan RPMs, for example. Any two readings at a given time may differ from
    one another. Such changing values are to be refreshed by calling update methods:

-----------------------------------------------------------------------------------------------
        - mic_update_thermal_info
        - mic_update_core_util
        - mic_update_memory_util
        - mic_update_power_util

-----------------------------------------------------------------------------------------------
    
    As long as an update method isn't called, subsequent calls for any function returning
    variable values will report the same result:
    
-----------------------------------------------------------------------------------------------
        >>> import micmgmt
        >>> mic = micmgmt.MicDevice(0)
        >>> print mic.mic_get_available_memory_size()
        5654260
        >>> print mic.mic_get_available_memory_size()
        5654260
        >>> print mic.mic_get_available_memory_size() #Previous calls report the same value
        5654260
        >>> mic.mic_update_memory_util() #Call the update method
        >>> print mic.mic_get_available_memory_size() #After update:
        5654136
        >>> #Value changed

-----------------------------------------------------------------------------------------------
    
Example on refreshing values:
-----------------------------

-----------------------------------------------------------------------------------------------
    
        >>> import micmgmt
        >>> mic = micmgmt.MicDevice(0)
        >>> mem1 = mic.mic_get_available_memory_size() # Get sample 1
        >>> mic.mic_update_memory_util() # Update memory utilization values
        >>> mem2 = mic.mic_get_available_memory_size() #Get sample 2
        >>> print "Memory util delta = %d" % (mem2 - mem1)
        Memory util delta = -124

-----------------------------------------------------------------------------------------------
    
    For a more detailed explanation on how to refresh values, refer to the standard Python
    documentation by calling 'help(micmgmt)' after having imported the module. The documentation
    for each update method includes a list of refreshable values.

3. More information
--------------------

    For further information on micmgmt.py, such as a full list of available methods and
    functionalities, please take a look at the standard documentation:
-----------------------------------------------------------------------------------------------
    
        >>> import micmgmt
        >>> help(micmgmt)
-----------------------------------------------------------------------------------------------
    
    If you wish to see more examples, go ahead and review the following:
-----------------------------------------------------------------------------------------------
        - /opt/intel/mic/mgmt/sdk/examples/src/examples.py
        - /opt/intel/mic/mgmt/sdk/examples/src/example_version.py

-----------------------------------------------------------------------------------------------

4. Examples
------------

    The following example code is taken from /opt/intel/mic/mgmt/sdk/examples/src/examples.py.
    You will find a few functions that receive as a parameter a MicDevice object. From each
    function, several values will be queried from the card. 
    
------------------------------------------------------------------------------------------
        import micmgmt
        
        def do_pci_config_examples(mic):
            try:
                print "\tVendor ID: %s" % hex(mic.mic_get_vendor_id())
            except micmgmt.MicException as e:
                print "\tFailed to get vendor ID: %s: %s" % (mic.mic_get_device_name(), e)
                
            try:
                print "\tDevice ID: %s" % hex(mic.mic_get_device_id())
            except micmgmt.MicException as e:
                print "\tFailed to get device ID: %s: %s" % (mic.mic_get_device_name(), e)
                
            try:
                print "\tRevision ID: %s" % hex(mic.mic_get_revision_id())
            except micmgmt.MicException as e:
                print "\tFailed to get revision ID: %s: %s" % (mic.mic_get_device_name(), e)
                
            try:
                print "\tSubsystem ID: %s" % hex(mic.mic_get_subsystem_id())
            except micmgmt.MicException as e:
                print "\tFailed to get subsystem ID: %s: %s" % (mic.mic_get_device_name(), e)
                
            try:
                print "\tSubsystem ID: %s" % hex(mic.mic_get_subsystem_id())
            except micmgmt.MicException as e:
                print "\tFailed to get subsystem ID: %s: %s" % (mic.mic_get_device_name(), e)
        
            try:
                print "\tLink width: %s" % str(mic.mic_get_link_width())
            except micmgmt.MicException as e:
                print "\tFailed to get link width: %s: %s" % (mic.mic_get_device_name(), e)
            
            try:
                print "\tMax payload: %s" % str(mic.mic_get_max_payload())
            except micmgmt.MicException as e:
                print "\tFailed to get max payload: %s: %s" % (mic.mic_get_device_name(), e)
            
            try:
                print "\tMax readreq: %s" % str(mic.mic_get_max_readreq())
            except micmgmt.MicException as e:
                print "\tFailed to get max readreq: %s: %s" % (mic.mic_get_device_name(), e)
        
        def do_memory_examples(mic):
            try:
                print "\tMemory vendor: %s" % mic.mic_get_memory_vendor()
            except micmgmt.MicException as e:
                print "\tFailed to get memory vendor: %s: %s" % (mic.mic_get_device_name(), e)
                
            try:
                print "\tMemory revision: %s" % str(mic.mic_get_memory_revision())
            except micmgmt.MicException as e:
                print "\tFailed to get memory revision: %s: %s" % (mic.mic_get_device_name(), e)
            
            try:
                print "\tMemory density: %s kb/device" % str(mic.mic_get_memory_density())
            except micmgmt.MicException as e:
                print "\tFailed to get memory density: %s: %s" % (mic.mic_get_device_name(), e)
            
            try:
                print "\tMemory density: %s kB" % str(mic.mic_get_memory_size())
            except micmgmt.MicException as e:
                print "\tFailed to get memory size: %s: %s" % (mic.mic_get_device_name(), e)
            
            try:
                print "\tMemory type: %s" % mic.mic_get_memory_type()
            except micmgmt.MicException as e:
                print "\tFailed to get memory type: %s: %s" % (mic.mic_get_device_name(), e)
            
            try:
                print "\tMemory frequency: %s kHz" % str(mic.mic_get_memory_frequency())
            except micmgmt.MicException as e:
                print "\tFailed to get memory frequency: %s: %s" % (mic.mic_get_device_name(), e)
            
            try:
                print "\tMemory voltage: %s uV" % str(mic.mic_get_memory_voltage())
            except micmgmt.MicException as e:
                print "\tFailed to get memory voltage: %s: %s" % (mic.mic_get_device_name(), e)
            
        if __name__ == '__main__':
            # Get number of devices:
            device_count = micmgmt.mic_get_ndevices()
            
            # Iterate over cards in system
            for device in range(device_count):
                mic = micmgmt.MicDevice(0)
                print "Found KNC device: %s" % mic.mic_get_device_name()
                do_pci_config_examples(mic)
                do_memory_examples(mic)

--------------------------------------------------------------------------------------------

INTEL CORPORATION PROPRIETARY INFORMATION
-----------------------------------------

-------------------------------------------------------------------------------
Copyright 2013-2015 Intel Corporation. All Rights Reserved.

Intel makes no warranty of any kind regarding this code. This code is
licensed on an "AS IS" basis and Intel will not provide any support,
assistance, installation, training,  or other services. Intel may not
provide any updates, enhancements or extensions to this code. Intel
specifically disclaims any warranty of merchantability, non-infringement,
fitness for any particular purpose, or any other warranty. Intel disclaims
all liability, including liability for infringement of any proprietary
rights, relating to use of the code. No license, express or implied, by
estoppel or otherwise, to any intellectual property rights is granted
herein.

Designers must not rely on the absence or characteristics of any features or
instructions marked "reserved" or "undefined". Intel reserves
these for future definition and shall have no responsibility whatsoever for
conflicts or incompatibilities arising from future changes to them.

    Xeon, and Intel Xeon Phi are trademarks of Intel Corporation in the 
    U.S. and/or other countries.

----------------------------------------------------------------------------------------

