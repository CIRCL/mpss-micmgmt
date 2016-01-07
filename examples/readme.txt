MicMgmtAPI Guide (A.K.A MicAccessAPI Guide)
===========================================

MicMgmt sdk library is bundled as part of *micmgmt* binaries and it is 
built from the open source package intel-mic-micmgmt-2.1.xxxx-src.rpm. 
This package is built for our external customers or cluster ISVs to aid in 
developing custom cluster functionality to programmatically access and
control some of the hardware registers and parameters of the MIC 
family of cards. Some of the available APIs allow such operations as to
monitor the voltage and temperature sensor readings on the card, 
control related thresholds and limits, or get information about cores 
and memory. 
Note: The micmgmt sdk is a re-write of MicAccessAPI sdk package, which was
released in intel-mic-sysmgmt-devel-2.1.xxxx.rpm package in previous 
releases. MicAccessAPI sdk library will not be supported from June 2013.
All the standard functionalities in MicAccessAPI will be supported 
in the micmgmt sdk library.  

Introduction
------------
micmgmt sdk library ('libmicmgmt.so') is a C/C++ library that allows 
programmatic access to internal control and status registers of the MIC 
cards. It also allows communication with other agents, such as 
System Management Controller if present on the card. The library is in 
turn dependent on 'libscif.so' that's required in order to be able to 
connect and communicate with the kernel components of our software stack. 
Please note that 'libscif.so' is installed as part of our 'rpm' packages.


Getting Started
---------------

Please make sure that you have a C compiler installed. This library has 
been tested with 'gcc version 4.4.4 20100726 (Red Hat 4.4.4-13)' but 
should work with other versions of C compiler too. In order to use the 
sdk  install intel-mic-mgmt-2.1.xxxx.rpm, then change directory to
'/opt/intel/mic/mgmt/sdk/exampes/' and run 'make' to compile the 
example source code files.

Writing Your Own Programs with MicMgmt
-------------------------------------------

Initial steps 
~~~~~~~~~~~~~~

Step 1: Discover all the mic cards in the system using the following APIs 
	mic_get_devices() and mic_get_ndevices().
	
	example:
	struct mic_devices_list *mdl;
	ncards = 0;
	mic_get_devices(&mdl); //list of all the mic devices
	mic_get_ndevices(mdl, &ncards); //number of mic devices found
	mic_free_devices(mdl); // to free the device list

Step 2: Identify your mic device using the mic_get_device_at_index() API
	
	example:
	index = 0;
	card;
	for (index=0; index <ncards; index++)
	{
		mic_get_device_at_index(mdl, index, &card);   
		printf ("mic%d\n", card); *** mic0, mic1, mic2,...
	}

Step 3: Choose a individual mic device to talk to using mic_open_device()
	API call.

	example:
	card = 0;
 	struct mic_device *mdh;
	mic_open_device(&mdh, card);
	....call apis using mdh
	....
	mic_close_device(mdh);

Step 4: Call an API on the specific mic device from the library.

	example:
	card = 0;
 	struct mic_device *mdh;
	struct mic_version_info *version;
	char uos_version[NAME_MAX];
	char flash_version[NAME_MAX];
	size_t size = NAME_MAX;

	mic_open_device(&mdh, card);
	mic_get_version_info(mdh, &version);

	/* Coprocessor OS version example */
	mic_get_uos_version(version, uos_version, &size);
	printf("Coprocessor OS Version :%s\n", uos_version);

	/* Flash version example */
	mic_get_flash_version(version, flash_version, &size);
	printf("Flash Version :%s\n", flash_version);

	mic_free_version_info(version); **** release the memory allocated
					     for version information
	mic_close_device(mdh);

-------------------------------------------------------------------------------


Example Makefile
----------------
The included makefile 'examples/Makefile', whose contents are listed below, may be
used to compile this program.

Please make sure that libmicmgmt.so is in the PATH before executing the test.
If the library was not installed to /usr/lib or usr/lib64 consider moving it
there manually or adding it to LD_LIBRARY_PATH.

Example: export set LD_LIBRARY_PATH="/opt/intel/mic/micmgmt/sdk/lib".

-------------------------------------------------------------------------------
LIBPATH += -L../miclib/libs

EXTRA_LDFLAGS = $(LDFLAGS) $(LIBPATH) -lscif -lmicmgmt

INCDIR=-I../include -I../miclib/include -I.

MAIN_SRCS=examples.c example_version.c

HEADERS=

MAIN_OBJS=$(MAIN_SRCS:.c=.o)

MAIN_EXEC=$(MAIN_SRCS:.c=)

-------------------------------------------------------------------------------


Example Code
------------
Example code provided with *micmgmt* binaries is a good starting powhen
first writing programs that use the 'micmgmt' library. A very simple example 
code is shown below to get the number of MIC cards in the machine and to get
the device name.

-------------------------------------------------------------------------------

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <miclib.h>

main(void)
{
	int ncards, card_num, card;
	struct mic_devices_list *mdl;
	struct mic_device *mdh;
	uint32_t device_type;
	if (mic_get_devices(&mdl) != E_MIC_SUCCESS) {
		fprintf(stderr, "Failed to get cards list: %s: %s\n",
			mic_get_error_string(), strerror(errno));
		return 1;
	}

	if (mic_get_ndevices(mdl, &ncards) != E_MIC_SUCCESS) {
		fprintf(stderr, "Failed to get number of cards: %s: %s\n",
			mic_get_error_string(), strerror(errno));
		(void)mic_free_devices(mdl);
		return 2;
	}
	printf("\tNumber of cards : %d\n", ncards);
	if (ncards == 0) {
		fprintf(stderr, "No MIC card found\n");
		(void)mic_free_devices(mdl);
		return 3;
	}

	/* Get card at index 0 */
	card_num = 0;
	if (mic_get_device_at_index(mdl, card_num, &card) != E_MIC_SUCCESS) {
		fprintf(stderr, "Failed to get card at index %d: %s: %s\n",
			card_num, mic_get_error_string(), strerror(errno));
		mic_free_devices(mdl);
		return 4;
	}

	(void)mic_free_devices(mdl);
	if (mic_open_device(&mdh, card) != E_MIC_SUCCESS) {
		fprintf(stderr, "Failed to open card %d: %s: %s\n",
			card_num, mic_get_error_string(), strerror(errno));
		return 5;
	}

	if (mic_get_device_type(mdh, &device_type) != E_MIC_SUCCESS) {
		fprintf(stderr, "%s: Failed to get device type: %s: %s\n",
			mic_get_device_name(mdh),
			mic_get_error_string(), strerror(errno));
		(void)mic_close_device(mdh);
		return 6;
	}

	if (device_type != KNC_ID) {
		fprintf(stderr, "Unknown device Type: %u\n", device_type);
		(void)mic_close_device(mdh);
		return 7;
	}
	printf("	Found KNC device '%s'\n", mic_get_device_name(mdh));

	(void)mic_close_device(mdh);
	return 0;
}

-------------------------------------------------------------------------------
Note that the above calls will initialize and open the device. We can call 
several other APIs to get specific information by using this open device.
We will start with a simple example. In the example below, we have populateda data structure for PCIe metrics.
-------------------------------------------------------------------------------

get_pcie_info(struct mic_device *mdh)
{
	struct mic_pci_config *pcfg;
	/* PCI config examples */
	printf("PCI Config Examples : \n\n");
	if (mic_get_pci_config(mdh, &pcfg) != E_MIC_SUCCESS) 
	{
		fprintf(stderr, "%s: Failed to get PCI configuration "
			"information: %s: %s\n", mic_get_device_name(mdh),
			mic_get_error_string(), strerror(errno));
		return 1;
	}

	(void)mic_free_pci_config(pcfg);
	return 0;
}

-------------------------------------------------------------------------------
Once we get the pci_config by making a call to "mic_get_pci_config", we
can derive more PCIe characteristics by calling the corresponding APIs that 
retrieve specific information. The following sections will contain code 
snippets to return specific device information. 

Memory Info Example
-------------------
Header File for the API: miclib.h

This snippet will return the "Total memory available on the device".

Data Structure Used: mic_device_mem

-------------------------------------------------------------------------------

do_memory_examples(struct mic_device *mdh)
{
	struct mic_device_mem *minfo;
	uint32_t msize;

	/* Memory device examples */
	printf("Memory API Examples : \n\n");
	if (mic_get_memory_info(mdh, &minfo) != E_MIC_SUCCESS) {
		fprintf(stderr, "%s: Failed to get memory "
			"information: %s: %s\n", mic_get_device_name(mdh),
			mic_get_error_string(), strerror(errno));
		return 1;
	}
	if (mic_get_memory_size(minfo, &msize) != E_MIC_SUCCESS) {
		fprintf(stderr, "%s: Failed to get memory size: "
			"%s: %s\n", mic_get_device_name(mdh),
			mic_get_error_string(), strerror(errno));
		(void)mic_free_memory_info(minfo);
		return 2;
	}
	printf("	Memory size: %u KBytes\n", msize);
	(void)mic_free_memory_info(minfo);
	return 0;
}

-------------------------------------------------------------------------------
More APIs can be called within the same function before the call to free "minfo".
More examples are available on the examples.c source file.

Die Temperature Readings API Example
------------------------------------

Header File for the API: miclib.h

This snippet will return the "Die Temperature" of the MIC device.

Data Structure Used: mic_device_mem


-------------------------------------------------------------------------------

do_thermal_examples(struct mic_device *mdh)
{
	struct mic_thermal_info *tinfo;
	uint32_t dtemp;
	
	if (mic_get_thermal_info(mdh, &tinfo) != E_MIC_SUCCESS) {
		fprintf(stderr, "%s: Failed to get thermal information: %s: %s\n", 
				mic_get_device_name(mdh),
				mic_get_error_string(), 
				strerror(errno));
		return 1;
	}
	if (mic_get_die_temp(tinfo, &dtemp) != E_MIC_SUCCESS) 
	{
		fprintf(stderr, "%s: Failed to get die temperature : %s: %s\n",
				mic_get_device_name(mdh),
				mic_get_error_string(),
				strerror(errno));
		(void)mic_free_thermal_info(tinfo);
		return 5;
	}
	printf("	Die Temperature : %uC\n", dtemp); 
	(void)mic_free_thermal_info(tinfo);
	return 0;
}

-------------------------------------------------------------------------------
More APIs can be called within the same function before the call to free "tinfo".
More examples are available on the examples.c source file.

Power consumption Examples
---------------------------

Header File for the API: miclib.h

This snippet will return the all the core, uncore, and memory subsystem rail power.

Data Structure Used: mic_power_util_info

-------------------------------------------------------------------------------

do_power_examples(struct mic_device *mdh)
{
	struct mic_power_util_info *pinfo;
	uint32_t pwr;
	
	/* Power utilization examples */
	printf("Power utilization Examples : \n\n");
	if (mic_get_power_utilization_info(mdh, &pinfo) != E_MIC_SUCCESS) {
		fprintf(stderr, "%s: Failed to get power utilization information: %s: %s\n", 
				mic_get_device_name(mdh),
				mic_get_error_string(), 
				strerror(errno));
		return 1;
	}

	if(mic_get_vccp_power_readings(pinfo, &pwr) != E_MIC_SUCCESS)
	{
		fprintf(stderr, "%s: Failed to get vccp(core rail) power readings: %s: %s\n", 
				mic_get_device_name(mdh),
				mic_get_error_string(), 
				strerror(errno));
		(void)mic_free_power_utilization_info(pinfo);
		return 2;
	}
	printf("	Vccp(core rail) power reading : %u watts\n", pwr/1000000);	

	if(mic_get_vddg_power_readings(pinfo, &pwr) != E_MIC_SUCCESS)
	{
		fprintf(stderr, "%s: Failed to get vddg(uncore rail) power readings: %s: %s\n", 
				mic_get_device_name(mdh),
				mic_get_error_string(), 
				strerror(errno));
		(void)mic_free_power_utilization_info(pinfo);
		return 3;
	}
	printf("	Vddg(uncore rail) power reading : %u watts\n", pwr/1000000);

	if(mic_get_vddq_power_readings(pinfo, &pwr) != E_MIC_SUCCESS)
	{
		fprintf(stderr, "%s: Failed to get vddq(Memory subsystem rail) power readings: %s: %s\n", 
				mic_get_device_name(mdh),
				mic_get_error_string(), 
				strerror(errno));
		(void)mic_free_power_utilization_info(pinfo);
		return 4;
	}
	printf("	Vddq(Memory subsystem rail) power reading : %u watts\n", pwr/1000000);

	(void)mic_free_power_utilization_info(pinfo);
	return 0;
}

-------------------------------------------------------------------------------

More APIs can be called within the same function before the call to free "pinfo".
More examples are available on the examples.c source file.

Coprocessor OS Version Retrieving API Example
------------------------------------

Header File for the API: miclib.h

This snippet will return the all version information.


-------------------------------------------------------------------------------
Data Structure Used: mic_version_info
main()
{
	struct mic_devices_list *dev_list=NULL;
	struct mic_device *mdh = NULL;
	struct mic_version_info *version=NULL;
	ndevices=0;
	indx=0;
	card=0;
	ret = 0;
	char str[NAME_MAX];
	size_t size = NAME_MAX;

	if( mic_get_devices(&dev_list) != E_MIC_SUCCESS )
	{
		error_msg_start("No devices found : %s: %s\n",
			 mic_get_error_string(), strerror(errno));
		return -1;
	}

	if( mic_get_ndevices(dev_list, &ndevices) != E_MIC_SUCCESS )
	{
		error_msg_start("Failed to get devices : %s: %s\n",
			mic_get_error_string(), strerror(errno));
		return -1;
	}


	for (indx=0;indx < ndevices; indx++)
	{

		if (mic_get_device_at_index(dev_list, card, &indx) != E_MIC_SUCCESS)
		{
			error_msg_start("Failed to get card at index %d: %s: %s\n",
			indx, mic_get_error_string(), strerror(errno));
			continue;
		}

		ret=mic_open_device(&mdh, card);
		if( ret != E_MIC_SUCCESS )
		{
			error_msg_start("Failed to open device '%d': %s: %s\n",
			card, mic_get_error_string(), strerror(errno));
			continue;
		}
		ret = mic_get_version_info(mdh, &version);
		if( ret != E_MIC_SUCCESS )
		{
			error_msg_start("Failed to get version info: %s: %s\n",
				mic_get_error_string(), strerror(errno));
			mic_close_device(mdh);
			continue;
		}
       		log_msg_start("\n\tVersion\n"); 
		mic_get_flash_version(version, str,&size);
       		log_msg_start("\t\tFlash Version \t\t\t: %s\n", str); 
		mic_get_uos_version(version, str,&size);
       		log_msg_start("\t\tCoprocessor OS Version \t\t\t: %s\n", str); 
		mic_free_version_info(version);
		mic_get_silicon_sku(mdh, str, &size);
		log_msg_start("\t\tSKU \t\t\t\t: %s\n", str);
		mic_get_serial_number(mdh, str, &size);
		log_msg_start("\t\tSerial Number \t\t\t: %s\n", str);
		mic_get_flash_vendor_device(mdh, str, &size);
		log_msg_start("\t\tVendor device \t\t\t: %s\n", str);
		mic_close_device(mdh);
	}
	mic_free_devices(dev_list);
	return 0;
}

-----------------------------------------------------------------------------------

Data Structures list
--------------------

Following are the list of some Data Structures used in micmgmt library.
Please look up micmgmt documentation for details.

-----------------------------------------------------------------------------------

mic_devices_list
mic_device_mem
mic_processor_info
mic_cores_info
smc_hw_rev_bits
smc_hw_rev
smc_fw_ver_bits
smc_fw_ver
smc_boot_loader_ver_bits
smc_boot_loader_ver
mic_version_info
smc_die_temp_bits
smc_die_temp
mic_thermal_info
mic_pci_config
mic_flash_status_info
mic_power_util_info
mic_memory_util_info
mic_core_util
mic_flash_op
mic_turbo_info

-----------------------------------------------------------------------------------

MicAccessSDK API list
---------------------

Following are the list of API in micmgmt library. Please look up the
micmgmt documentation for details.

-----------------------------------------------------------------------------------

/* pci configuration */
mic_get_pci_config
mic_get_bus_number
mic_get_device_number
mic_get_vendor_id
mic_get_device_id
mic_get_revision_id
mic_get_subsystem_id
mic_get_link_speed
mic_get_link_width
mic_get_max_payload
mic_get_max_readreq
mic_free_pci_config
/* Operations host platform */
mic_get_devices
mic_free_devices
mic_get_ndevices
mic_get_device_at_index
/* Open close device and device type */
mic_open_device
mic_close_device
mic_get_device_type
const char *mic_get_device_name
/* thermal info */
mic_get_thermal_info
mic_get_smc_hwrevision
mic_get_smc_fwversion
mic_is_smc_boot_loader_ver_supported
mic_get_smc_boot_loader_ver
mic_get_fsc_status
mic_get_die_temp
mic_get_fan_rpm
mic_get_fan_pwm
mic_free_thermal_info
/* device memory info */
mic_get_memory_info
mic_get_memory_vendor
mic_get_memory_revision
mic_get_memory_density
mic_get_memory_size
mic_get_memory_speed
mic_get_memory_type
mic_get_memory_frequency
mic_get_memory_voltage
mic_get_ecc_mode
mic_free_memory_info
/* processor info */
mic_get_processor_info
mic_get_processor_model
mic_get_processor_family
mic_get_processor_type
mic_get_processor_steppingid
mic_get_processor_stepping
mic_free_processor_info
/* Coprocessor OS core info */
mic_get_cores_info
mic_get_cores_count
mic_get_cores_voltage
mic_get_cores_frequency
mic_free_cores_info
/* version info*/
mic_get_version_info
mic_get_uos_version
mic_get_flash_version
mic_get_fsc_strap
mic_free_version_info
/* silicon SKU */
mic_get_silicon_sku
/* serial number */
mic_get_serial_number
/* power utilization info */
mic_get_power_utilization_info
mic_get_total_power_readings_w0
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
mic_free_power_utilization_info
/* memory utilization */
mic_get_memory_utilization_info
mic_get_total_memory_size
mic_get_available_memory_size
mic_get_memory_buffers_size
mic_free_memory_utilization_info
/* core utilization apis */
mic_alloc_core_util
mic_update_core_util
mic_get_idle_counters
mic_get_nice_counters
mic_get_sys_counters
mic_get_user_counters
mic_get_idle_sum
mic_get_sys_sum
mic_get_nice_sum
mic_get_user_sum
mic_get_jiffy_counter
mic_get_num_cores
mic_get_threads_core
mic_free_core_util
mic_get_led_alert
mic_get_turbo_state_info
mic_get_turbo_state
mic_get_turbo_state_valid
mic_free_turbo_info
/* Error handling */
mic_get_error_string
mic_clear_error_string
/* Maintenance mode start/stop */
mic_enter_maint_mode
mic_leave_maint_mode
mic_in_maint_mode
mic_in_ready_state
mic_get_post_code
/* Flash operations */
mic_flash_size
mic_flash_active_offs
mic_flash_update_start
mic_flash_update_done
mic_flash_read_start
mic_flash_read_done
mic_get_flash_status_info
mic_get_progress
mic_get_status
mic_get_ext_status
mic_free_flash_status_info
mic_flash_version
mic_get_flash_vendor_device


-------------------------------------------------------------------------------


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

-------------------------------------------------------------------------------

