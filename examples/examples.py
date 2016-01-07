# Copyright 2012 Intel Corporation.
#
# This file is subject to the Intel Sample Source Code License. A copy
# of the Intel Sample Source Code License is included.

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

def do_memory_exmaples(mic):
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
        do_memory_exmaples(mic)
