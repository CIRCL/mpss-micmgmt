# Copyright 2012 Intel Corporation.
#
# This file is subject to the Intel Sample Source Code License. A copy
# of the Intel Sample Source Code License is included.

import micmgmt

if __name__ == '__main__':
    # Get number of devices:
    device_count = micmgmt.mic_get_ndevices()
    
    # Iterate over cards in system
    for device in range(device_count):
        #Open device
        mic = micmgmt.MicDevice(device)
        
        # Get version information
        print "Flash version for card %d: %s" % (device, mic.mic_get_flash_version())
        print "Coprocessor OS version for card %d: %s" % (device, mic.mic_get_uos_version())
        
