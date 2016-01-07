#!/bin/bash

########################################################################
# License (LGPL):
########################################################################
# Intel MIC Platform Software Stack (MPSS)
#
# Copyright 2010 2014 Intel Corporation All Rights Reserved.
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, version 2.1.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA.
#
# Disclaimer: The codes contained in these modules may be specific to
# the Intel Software Development Platform codenamed: Knights Ferry, and
# the Intel product codenamed: Knights Corner, and are not backward
# compatible with other Intel products. Additionally, Intel will NOT
# support the codes or instruction set in future products.
#
# Intel offers no warranty of any kind regarding the code. This code is
# licensed on an "AS IS" basis and Intel is not obligated to provide
# any support, assistance, installation, training, or other services of
# any kind. Intel is also not obligated to provide any updates,
# enhancements or extensions. Intel specifically disclaims any warranty
# of merchantability, non-infringement, fitness for any particular
# purpose, and any other warranty.
#
# Further, Intel disclaims all liability of any kind, including but not
# limited to liability for infringement of any proprietary rights,
# relating to the use of the code, even if Intel is notified of the
# possibility of such liability. Except as expressly stated in an Intel
# license agreement provided with this code and agreed upon with Intel,
# no license, express or implied, by estoppel or otherwise, to any
# intellectual property rights is granted herein.
########################################################################

########################################################################
# Description:
#    A tool for gathering system and card data after a customer failure.
########################################################################
# Current Version:
MYVER=1.5.0
########################################################################
# CHANGELOG (Script version only, not related to software version):
#   Version 1.5.0   *Added support for systemctl for SLES12 and RHEL7.
#                   *Removed References to "Yocto", and replaced them
#                   with MPSS3X.
#                   *Added support for SLES12 and RHEL7 firewall service.
#                   *Fixed "service micras status" capture over running
#                   the output due to a change in the start script.
#                   *Added gateway capture to help with debug.
#   Version 1.4.0   Bug Fixes:
#                   *Refactored script to re-use code and eliminate many 
#                   hard coded paths
#                   *mic*.conf "HostName" parameter capitalization changed. 
#                   Made the script ignore case on the M.
#                   *Changed card ping threshold to two seconds to allow time
#                   for card to wake up.
#                   *Changed script to use /bin/bash instead of /bin/sh.
#                   +Added check for existence of mpss config before starting.
#   Version 1.3.4   Bug Fixes:
#                   +Added ability to check for custom MPSS config directory 
#                   specified by /etc/sysconfig/mpss.conf.
#   Version 1.3.3 - Bug Fixes:
#                   *Fixed a culture error that caused bad folder names.
#   Version 1.3.2 - Bug Fixes:
#                   +Added MPSS3X compatibility
#                   *Fixed typos in header comments
#                   +Added filename mappings to header comments.
#                   *Fixed a filename bug in the crash dump feature.
#   Version 1.3.1 - Bug Fixes and Added Features:
#                   *Fixed a mic device to PCI Address error.
#   Version 1.3.0 - More Review Updates:
#                   *Rewrote some things to be smaller and more precise
#                   +Added ssh key checking for all discovered card 
#                    users if possible.
#                   *Fixed many potential issues (hardening)
#                   +Added feature to include the vmlinux and System.map
#                    when card crash dumps are requested.
#                   +Added a comparison of lspci count to SW count of
#                    cards based on conf files.
#                   +Added a general ping (ICMP) test.
#                   +Added a feature to reduce possible hangs; limit
#                    network activities to ONLY cards that respond to
#                    ping (ICMP).
#                   *Rearranged to put increasingly dangerous tests at
#                    the bottom.
#                   *Fixed bug that occured when the 'mic' module is not
#                    loaded into memory.
#                   +Added a check for OFED status.
#                   +Added a copy of the SELinux config file is present
#                   +Added recording firewall status if it can be found
#                   +Added NetworkManager status if present
#                   +Added a feature for Gold Update 3 micN to PCI 
#                    address mapping
#                   *Changed the output of RPM packages to be sorted
#   Version 1.2.0 - Review Updates:
#                   +Added lspci -Dtv output to the tar file.
#                   *Changed compression in TAR to gz instead of BZ2 for
#                    WinZip compatibility (files are still Linux 
#                    formatted).
#                   *Changed file extension form .out to .txt also for
#                    Windows (however the files are still Linux
#                    formatted).
#                   *Removed incorrect branding.
#                   *Renamed output folder from "debug_info" to 
#                    "mpss_debug_info".
#                   *Renamed the script to match what customers were
#                    already getting from marketing.
#   Version 1.1.5 - *Replaced getting only the sysfs post code with
#                    saving ALL of sysfs.
#                   +Added Retrieving /proc/elog from each online card.
#                   *Renamed the script as the old name had issues on
#                    RHEL 6.3.  No reason was found.
#                   +Added a feature to remove any captured file edit
#                    backups (*~ files) before packaging.
#                   +Added capturing the rasd log file and micras
#                    status.
#   Version 1.1.4 - Moved to new a new git repo for future inclusion in
#                   the shipping software.
#   Version 1.1.3 - Minor updates
#                   +Added standard -h and --help options
#                   *Changed the ssh hostname to a function for future
#                    changes (right now assumes "micN" for the hostname)
#   Version 1.1.2 - MAJOR CHANGES
#                   *Fixed bug that assumes you needed debugfs to get
#                    the cards post code.
#                   +Added parsing of config files to get actual paths
#                    to software folders.
#   Version 1.1.1 - MAJOR CHANGES
#                   +Added optional option for including card crash
#                    dumps.
#                   +Added dynamic SCRIPT_NAME variable for future name
#                    changes.
#                   +Changed the general output filename to:
#                        ${SCRIPT_NAME}.out
#                   +Added capture of the ifcfg-* files for the host
#                    networking.
#                   +Added checking for ICC and getting its version if
#                    present in the PATH variable.
#                   +Rearranged the data collection order to put more
#                    problematic data gathering near the bottom.
#                   +Made the screen and general output log more
#                    verbose.
#   Version 1.1.0 - MAJOR CHANGES
#                   +Added a folder to the tar file contents so files
#                    are contained.
#                   +Added the option --include_crash_dumps option so
#                    that crash dumps must now be specified and are not
#                    automatic now, you must ask for them using the
#                    option.
#   Version 1.0.5 - +Added more verbose lspci output.
#                   +Minor comment corrections.
#   Version 1.0.4 - +Added a copy of /etc/modprobe.d/mic.conf and added
#                    this CHANGELOG information.
#   Version 1.0.3 - +Changed the BIOS information to include host CPU 
#                    and host memory information.  Renamed the output
#                    file accordingly.
#   Versions 1.0 to 1.0.2 - Initial development and debug
#
# CURRENT FEATURES:
#   Capures the following information:
#     Host:
#       * BIOS information (dmidecode.txt)
#       * LSPCI information (coproc_only_lspci.txt, full_lspci.txt,
#                            tree_lspci.txt)
#       * Linux Kernell Information (uname.txt and os_release_info.txt)
#       * Network configuration information (route -n, host-ifcfg-* and ifconfig.txt)
#       * Installed Package Information (rpm_packages.txt)
#       * Execution environment information (shell_env.txt)
#       * Processes running (processes.txt)
#       * Modules loaded (modules.txt)
#       * dmesg dump (host_dmesg.txt)
#       * /var/log/messages copy
#       * icc version if present in PATH for root (icc_version.txt)
#       * records selinux state (selinuxselinux_config.txt)
#       * records firewall status (firewall.txt)
#       * records NetworkManager's status if present
#     Software Stack: 
#       * module load state (micdebug.sh.txt)
#       * mpss service load state (micdebug.sh.txt)
#       * micras service load state (micdebug.sh.txt)
#       * card states (sysfs_mic*.txt or micdebug.sh.txt)
#       * miccheck for all online cards (miccheck.txt)
#       * mpssinfo for all online cards (mpssinfo.txt)
#       * dmesg buffers for all cards (mic*_dmesg.txt)
#       * all configuration found in /etc/sysconfig/mic/* or /etc/mpss/*
#                                      (original_filenames.txt or conf_d_*.conf)
#       * Configurations specified in /etc/sysconfig/mpss.conf
#       * /etc/modprobe.d/mic.conf copy (modprobe.d.mic.conf)
#       * all files under /opt/intel/mic/filesystem (filesystem_ls.txt)
#       * /var/log/mpssd copy (mpssd)
#       * /var/log/micras.log copy if present (micras.log)
#       * ssh key folder listings for root and logged in user if different
#                                                            (ssh_keys_list.txt)
#       * diff of keys in /opt/intel/mic/filesystem and user+root accounts
#                                                              (micdebug.sh.txt)
#       * all of sysfs for each card is dumped (sysfs_mic*.txt)
#       * OFED service states if installed (ofed.txt)
#       * ping (ICMP) test for cards marked as online (micdebug.sh.txt)
#       * capture micN to PCIe address mappings (mic_to_pci_map.txt)
#     Card: 
#       * check for running COI daemons on online cards (coi_daemons.txt)
#       * capture /proc/elog from each online card (elog_mic*.txt)
#       * card core dumps + vmlinux if requested (mic*-vmcore-*.gz)
#
# KNOWN UNFIXED BUGS:
#   * None
#
# FEATURE REQUEST LIST:
#   + None
#######################################################################
# There will be PATH redundency here on some systems that do different sudo
# configurations or special administrative configurations.
MPSS_INSTALL_FOLDER=/opt/intel/mic
export PATH="${PATH}:/usr/sbin:/sbin:/usr/bin:/usr/local/bin:/bin:${MPSS_INSTALL_FOLDER}/bin:${MPSS_INSTALL_FOLDER}/bin/MicDiagnostic"

# Global Values
EXT="txt"
SCRIPT_NAME="${0##*/}"
SCRIPT_BASENAME="${SCRIPT_NAME%%\.*}"
GENERAL_OUTPUT="${SCRIPT_NAME}.${EXT}"
REAL_USER="${SUDO_USER}"
MPSS3X_DETECTED=true
[ -n "${REAL_USER}" ] || REAL_USER="${USER}"
SCRIPT_START_TIME=`date -u "+%Y%m%d_%H%M%Sutc"`
FOLDER_NAME="${SCRIPT_BASENAME}_${SCRIPT_START_TIME}"
TAR_FILE="${FOLDER_NAME}.tgz"
# Detect the mpss config directory and assign it to the variable "MIC_CONFIG_DIR".
if [ -e "/usr/bin/${SCRIPT_NAME}" ]; then
  if [ -f /etc/sysconfig/mpss.conf ]; then
    MIC_CONFIG_DIR=`grep /etc/sysconfig/mpss.conf -e ^MPSS_CONFIGDIR= | awk -F"=" '{print $2}'`
  else
    if [ -f /etc/mpss/default.conf ]; then
      MIC_CONFIG_DIR="/etc/mpss"
    else
      if [ -f /etc/sysconfig/mic/default.conf ]; then
        MIC_CONFIG_DIR="/etc/sysconfig/mic"
        MPSS3X_DETECTED=false
      else
        echo "Error:  MPSS config not found.  Run \"micctrl --initdefaults\" first."
        exit 1
      fi
    fi
  fi
else
  echo "Error:  Script must be run from /usr/bin/ directory"
  exit 1
fi

# Detect if OS uses systemctl.
if [ -a "/usr/bin/systemctl" ]; then
  SYSCTL=1
else
  SYSCTL=''
fi

  general_out() {
  [ ! -f $GENERAL_OUTPUT ] && echo "'$SCRIPT_NAME' Version $MYVER" | tee $GENERAL_OUTPUT
  echo $1 | tee -a $GENERAL_OUTPUT
}

# Find a place for the temporary folder if possible...
TEMP_FOLDER=/tmp
if [ -d "$TEMP_FOLDER" -a -r "$TEMP_FOLDER" -a -w "$TEMP_FOLDER" -a -x "$TEMP_FOLDER" ]; then
  echo -n
else
  echo "Warning: ${SCRIPT_NAME}: '${TEMP_FOLDER}' is missing or doesn't have traditional permissions!"
  echo "Attempting to use current folder..."
  TEMP_FOLDER="."
  if [ -d "$TEMP_FOLDER" -a -r "$TEMP_FOLDER" -a -w "$TEMP_FOLDER" -a -x "$TEMP_FOLDER" ]; then
    echo -n
  else
    echo "Warning: ${SCRIPT_NAME}: '${TEMP_FOLDER}' doesn't have required permissions!"
    home_folder="$(su $REAL_USER -c 'echo $HOME')"
    if [ "$(pwd)" != "$home_folder" ]; then
      echo "Attempting to use ${REAL_USER}'s home folder..."
      TEMP_FOLDER="$home_folder"
      if [ -d "$TEMP_FOLDER" -a -r "$TEMP_FOLDER" -a -w "$TEMP_FOLDER" -a -x "$TEMP_FOLDER" ]; then
        echo -n
      else
        echo "Warning: ${SCRIPT_NAME}: '${TEMP_FOLDER}' doesn't have required permissions!"
        fatal_exit "Was unable to determine a place to put temporary files!"
      fi
    else
      fatal_exit "Current directory is ${REAL_USER}'s home folder; was unable to determine a place to put temporary files!"
    fi
  fi
fi

# Common Functions...
string_list_append() {
  if [ -n "$1" ]; then
    echo "$1 $2"
  else
    echo "$2"
  fi
}

string_list_has() {
  for i in "$1"; do
    [ "$i" = "$2" ] && return 0
  done
  return 1
}

fatal_exit() {
  if [ -z "$1" ]; then
    echo "FATAL ERROR: ${SCRIPT_NAME}: Unknown error!"
  else
    echo "FATAL ERROR: ${SCRIPT_NAME}: $1"
  fi
  shift
  local arg=""
  for arg in $*; do
    echo "     $arg"
  done
  exit 255
}

usage_exit() {
  echo "Usage: sudo ./$SCRIPT_NAME [--include_crash_dumps | -h | --help]"
  echo "   *** This script requires passwordless ssh to cards for the cards for"
  echo "       the root account (if different) or some features will fail."
  echo "   *** If the host crashes while running this script data"
  echo "       collected will be in the following folder:"
  echo "          ${TEMP_FOLDER}/${FOLDER_NAME}"
  echo "       Please zip the folder and send it and the host crash"
  echo "       dump to Intel.  Remember it may be a security risk to leave the"
  echo "       folder after you have sent the information, other users can see"
  echo "       it's contents!  We recommend deleting it or adjusting it's"
  echo "       permissions according to your administrators guidelines."
  echo "       Thanks."
  
  exit $1
}

get_crash_dump_path() {
  local basedir=`awk '/^CrashDumpDir/ {print $2}' $MIC_CONFIG_DIR/default.conf`
  echo "${basedir%%/}"
}

get_mic_filesystem_path() {
  awk '/^MicDir/ { print $2 }' $MIC_CONFIG_DIR/mic${1}.conf
}

get_mic_filelist_filename() {
  awk '/^MicDir/ { print $3 }' $MIC_CONFIG_DIR/mic${1}.conf
}

get_host_or_ip_by_mic_number() {
  local host=`awk '/^Host[Nn]ame/ { print $2 }' "$MIC_CONFIG_DIR/mic${1}.conf" | tr -d \"`
  [ "$host" != "dhcp" ] || host="`hostname`-mic${1}"
  echo "$host"
}

ping_test() {
  local card=`get_host_or_ip_by_mic_number $1`
  ping -c 1 -W 2 $card >/dev/null 2>&1
  return $?
}

generate_ping_list() {
  local list=""
  local delim="$1"
  shift
  for p in $*; do
    ping_test "${p}"
    if [ $? -eq 0 ]; then
      [ -n "$list" ] && list="${list}${delim}"
      list="${list}${p}"
    fi
  done
  echo "${list}"
}

test_folder_read() {
  test -d "$1" -a -x "$1" -a -r "$1"
  return $?
}

test_file_read() {
  test -r "$1"
  return $?
}

# $1 = string list in form "0:state 1:state..."; $2 index of card to get state from.
get_card_state_from_list() {
  local card=""
  for card in $1; do
    if [ "${card%%:*}" = "$2" ]; then
      echo "${card##*:}"
      return 0
    fi
  done
  return 1
}

get_list_of_card_users() {
  local passwd="$(get_mic_filesystem_path $1)/etc/passwd"
  sed 's/:.*$//' $passwd  | grep -v -e sshd -e micuser
}

get_host_user_folder() {
  grep "^${1}:" /etc/passwd | cut -d: -f6
}

get_card_user_folder() { # $1 = card; $2 = user
  if [ "$2" = "root" ]; then
    echo "$(get_mic_filesystem_path ${1})/${2}"
  else
    echo "$(get_mic_filesystem_path ${1})/home/${2}"
  fi
}

get_mic_pcie_addr_by_number() {
  echo `udevadm info --query=path --name="/dev/mic${1}" | cut -d '/' -f 5`
}

get_safe_card_count() {
  # Less accurate but needed for Gold, Gold Update 1, Gold Update 2
  if [ "$MPSS3X_DETECTED" == "false" ]; then
    try1=`ls -1 $MIC_CONFIG_DIR/mic*.conf | wc -l`
  fi
  # More accurate for Gold Update 3 and forward
  try2=`ls -d1 /dev/mic* | grep -v "/dev/mic$" | wc -l`
  
  if [ $try2 -eq 0 ]; then
    echo $try1
  else
    echo $try2
  fi
}

# Verify it was run as root!
if [ "$USER" != "root" ]; then
  echo "FATAL ERROR: ${SCRIPT_NAME}: This script *MUST* be run as root, try using sudo!"
  usage_exit 255
fi

# Parse command line and set vars or exit...
INC_DUMPS=0
for arg in $*; do
  case $arg in
    -h | --help)
      usage_exit 0 ;;
    --include_crash_dumps)
      INC_DUMPS=1 ;;
    *)
      fatal_exit "Unknown parameter '$arg' on the command line!"
  esac
done

# Manage folder for script output...
pushd "$TEMP_FOLDER" >/dev/null
[ $? -ne 0 ] && fatal_exit "Was unable to change the the '${TEMP_FOLDER}'!"
[ -d "$FOLDER_NAME" ] && rm -rf "$FOLDER_NAME"
if [ -d "$FOLDER_NAME" ]; then
  fatal_exit "Was unable to cleanup existing folder '${TEMP_FOLDER}/${FOLDER_NAME}'!"
fi
mkdir "$FOLDER_NAME"
[ $? -ne 0 ] && fatal_exit "Unable to create folder '${FOLDER_NAME}' in '${TEMP_FOLDER}'!"
chmod 0700 "$FOLDER_NAME"
[ $? -ne 0 ] && fatal_exit "Unable to set permissions on '${TEMP_FOLDER}/${FOLDER_NAME}'!"
cd "$FOLDER_NAME"
[ $? -ne 0 ] && fatal_exit "Unable to change to folder '${TEMP_FOLDER}/${FOLDER_NAME}'!"

########################################################################
# Capture static data before anything possibly destructive is called...
general_out "${SCRIPT_NAME}: Using folder '${TEMP_FOLDER}' for temporary data storage..."
general_out "Saving the host dmesg output..."
dmesg >"host_dmesg.${EXT}"
general_out "Copying messages log, mpssd log, '/etc/modprobe.d/mic.conf'..."
cp /var/log/messages "messages.${EXT}"
cp /var/log/mpssd "mpssd.${EXT}"
if [ -f "/var/log/micras.log" ]; then
  cp /var/log/micras.log "micras.log.${EXT}"
fi
cp /etc/modprobe.d/mic.conf "modprobe.d.mic.conf.${EXT}"
general_out "Saving shell environment..."
set >"shell_env.${EXT}"
general_out "Copying distribution release information..."
cat /etc/*release >"os_release_info.${EXT}"

# capture SELinux config file if present...
general_out "Copying '/etc/selinux/config' if present..."
[ -d "/etc/selinux" ] && [ -f "/etc/selinux/config" ] && cp /etc/selinux/config "selinux_config.${EXT}"

# Get network config scripts for host if found...
general_out "Gathering host network data (ifconfig and /etc/sysconfig/network*/ifcgf*.conf)..."
ifconfig >"ifconfig.${EXT}"
if [ -d "/etc/sysconfig/network-scripts" ]; then
  for cfg in `ls /etc/sysconfig/network-scripts/ifcfg*`; do
    cp "${cfg}" "host-${cfg##*/}.${EXT}"
  done
elif [ -d "/etc/sysconfig/network" ]; then
  for cfg in `ls /etc/sysconfig/network/ifcfg*`; do
    cp "${cfg}" "host-${cfg##*/}.${EXT}"
  done
fi

# capture host gateway and route information.
if [ -e "/sbin/route" ]; then
  general_out "Capturing gateway information with 'route -n'..."
  route -n >"route.${EXT}"
fi

# Get uname info...
general_out "Saving 'uname -a' informations..."
uname -a >"uname.${EXT}"

# Get ICC info if present...
ICC_LOC=`which icc 2>/dev/null`
if [ -z "${ICC_LOC}" ]; then
  ICC_LOC="/opt/intel/bin/icc"
fi
if [ -x "${ICC_LOC}" ]; then
  general_out "Found ICC, recording version information (icc -V)..."
  "${ICC_LOC}" -V 2>&1 | head -1 >"icc_version.${EXT}"
else
  general_out "Unable to find ICC!; This doesn't mean it not installed, just not found!"
fi

# Get RPM Packages List...
general_out "Saving RPM package list (rpm -qa)..."
rpm -qa | sort >"rpm_packages.${EXT}"

# Get detailed process list...
general_out "Saving list of running processes (ps -aef)..."
ps -aef >"processes.${EXT}"

# Get module list...
general_out "Saving 'lsmod' output for the host..."
lsmod >"modules.${EXT}"

# Get BIOS information...
general_out "Saving BIOS information (dmidecode)..."
dmidecode >"dmidecode_info.${EXT}"

# Get firewall state if found...
if [ -n "$SYSCTL" ] && [ -a "/lib/systemd/system/iptables.service" ]; then
  general_out "'iptables.service' firewall service found.  Recording current status..."
  systemctl status iptables.service >"firewall.${EXT}"
fi # RHEL7 has IPtables and a Firewall service.
if [ -n "$SYSCTL" ] && [ -a "/lib/systemd/system/firewalld.service" ]; then
  general_out "'firewalld.service' firewall service found. Recording current status..."
  systemctl status firewalld.service >>"firewall.${EXT}"
  elif [ -n "$SYSCTL" ] && [ -a "/usr/lib/systemd/system/SuSEfirewall2.service" ]; then
    general_out "'SuSEfirewall2.service' found.  Recording current status..."
    systemctl status SuSEfirewall2.service >"firewall.${EXT}"
  elif [ -x "/etc/init.d/SuSEfirewall2_setup" ]; then
    general_out "SuSE firewall service found. Recording current status..."
    service SuSEfirewall2_setup status >"firewall.${EXT}"
  elif [ -x "/etc/init.d/iptables" ]; then
    general_out "'iptables' firewall service found. Recording current status..."
    service iptables status >"firewall.${EXT}"
  else
    general_out "Firewall service not found!"
fi

# Get NetworkManager state if found...
if [ -a "/lib/systemd/system/NetworkManager.service" ]; then
  general_out "Detected NetworkManager.service, getting its status..."
  systemctl status NetworkManager.service >"NetworkManagerStatus.${EXT}"
elif [ -x "/etc/init.d/NetworkManager" ]; then
  general_out "Detected NetworkManager, getting its status..."
  service NetworkManager status >"NetworkManagerStatus.${EXT}"
else
  general_out "NetworkManager service not found!"
fi

########################################################################
# Get prerequisites for running the more dangerous commands.
COPROC_SW_COUNT=`get_safe_card_count`
card_states=""
COPROC_ONLINE_COUNT=0
COPROC_ONLINE_CARDS=""
COPROC_OFFLINE_CARDS=""
COPROC_ALL_CARDS=""

general_out "Gathering debug data..."
MODULE=`lsmod | awk '{print $1}' | grep -w mic`
if [ -z "$MODULE" ]; then
  general_out "Warning: The 'mic' kernel module is not loaded; results will be greatly reduced."
  SERVICE=""
  RASSERVICE=""
else
  if [ -n "$SYSCTL" ]; then
    general_out "Running 'systemctl status mpss.service'..."
    SERVICE=$(systemctl status mpss.service | awk 'FNR == 3 {print $2}')
  else
    general_out "Running 'service mpss status'..."
    SERVICE=$(service mpss status | awk -- '{print $NF}')
  fi
  general_out "The 'mpss' service is ${SERVICE}."
  if [ -n "$SYSCTL" ]; then
    general_out "Running 'systemctl status micras.service'..."
    RASSERVICE=$(systemctl status micras.service | awk 'FNR == 3 {print $2}')
  else
    general_out "Running 'service micras status'..."
    RASSERVICE=$(service micras status | awk -- '{print $NF}')
  fi
  general_out "The 'micras' service is ${RASSERVICE}."
fi

if [ -z "$MODULE" ]; then
  minus1=`expr $COPROC_SW_COUNT - 1`
  COPROC_OFFLINE_CARDS="`seq 0 $minus1`"
  COPROC_ALL_CARDS="$COPROC_OFFLINE_CARDS"
else
  for j in `seq $COPROC_SW_COUNT`; do
    i=`expr $j - 1`
    COPROC_ALL_CARDS=`string_list_append "${COPROC_ALL_CARDS}" "$i"`
    general_out "Running 'micctrl -s mic$i'..."
    # Might be dangerous but needed to continue:
    state=`micctrl -s "mic$i" | awk -- '{print $2'}`
    card_states=`string_list_append "${card_states}" "${i}:${state}"`
    if [ "${state}" = "online" ]; then
      COPROC_ONLINE_COUNT=`expr $COPROC_ONLINE_COUNT + 1`
      COPROC_ONLINE_CARDS=`string_list_append "${COPROC_ONLINE_CARDS}" "$i"`
    else
      COPROC_OFFLINE_CARDS=`string_list_append "${COPROC_OFFLINE_CARDS}" "$i"`
    fi
  done
fi

# Get configuration information...
  general_out "Gathering software configuration data..."
  general_out "  Copying $MIC_CONFIG_DIR/default.conf..."
	cp $MIC_CONFIG_DIR/default.conf "default.conf.${EXT}"
	general_out "  Copying $MIC_CONFIG_DIR/conf.d/*.conf files..."
  for f in $MIC_CONFIG_DIR/conf.d/*.conf; do
	  cp "$f" "conf_d_${f##*/}.${EXT}" 2>/dev/null
	done
	for i in $COPROC_ALL_CARDS; do
	  general_out "  For 'mic${i}'..."
	  general_out "    Listing recursively the coprocessor filesystem folder (ls -lARh)..."
	  micdir=`get_mic_filesystem_path $i`
	  ls -lARh $micdir >"mic${i}_filesystem_ls.${EXT}"
	  general_out "    Copying filelist file (using cp)..."
	  fn=`get_mic_filelist_filename $i`
	  cp "${fn}" "${fn##*/}.${EXT}"
	  general_out "    Copying all passwd, group and hosts files..."
	  cp "${micdir}/etc/passwd" "mic${i}_passwd.${EXT}"
	  cp "${micdir}/etc/group" "mic${i}_group.${EXT}"
	  cp "${micdir}/etc/hosts" "mic${i}_hosts.${EXT}"
	  general_out "    Copying $MIC_CONFIG_DIR/mic${i}.conf file (using cp)..."
	  cp "$MIC_CONFIG_DIR/mic${i}.conf" "mic${i}.conf.${EXT}"
	done

# Verifying ssh keys exist and are correctly copied to the mic filesystem...
general_out "Checking and validating ssh keys (using ls and diff)..."
rm -f "ssh_keys_list.${EXT}"
host_reported=""
for i in $COPROC_ALL_CARDS; do
  card_users=`get_list_of_card_users $i`
  for u in $card_users; do
    host_folder=`get_host_user_folder $u`
    card_folder=`get_card_user_folder $i $u`
    if [ -z "$host_folder" ]; then
      general_out "Warning: On card 'mic${i}'; user '$u' has no host account, skipping this user."
      echo "Warning: On card 'mic${i}'; user '$u' has no host account, skipping this user." >>"ssh_keys_list.${EXT}"
    else
      if ! test_folder_read "${host_folder}/.ssh"; then
        string_list_has "$host_reported" "$u"
        if [ $? = 1 ]; then
          echo "PROBLEM: On host user '$u' has no '${host_folder}/.ssh' folder or permission is denied!" >>"ssh_keys_list.${EXT}"
          general_out "PROBLEM: On host user '$u' has no '${host_folder}/.ssh' folder or permission is denied!"
          host_reported=`string_list_append "$host_reported" "$u"`
        fi
      elif ! test_folder_read "${card_folder}/.ssh"; then
        echo "PROBLEM: On card 'mic${i}'; user '$u' has no '${card_folder}/.ssh' folder or permission is denied!" >>"ssh_keys_list.${EXT}"
        general_out "PROBLEM: On card 'mic${i}'; user '$u' has no '${card_folder}/.ssh' folder or permission is denied!"
      else
        echo "User='$u'; For Card='mic${i}'..." >>ssh_keys_list.${EXT}
        echo "################################################################################" >>"ssh_keys_list.${EXT}"
        ls -lAh ${host_folder}/.ssh/id_* >>"ssh_keys_list.${EXT}" 2>&1
        echo "################################################################################" >>"ssh_keys_list.${EXT}"
        ls -lAh ${card_folder}/.ssh/id_* >>"ssh_keys_list.${EXT}" 2>&1
        echo "################################################################################" >>"ssh_keys_list.${EXT}"
        fails=0
        for key in ${host_folder}/.ssh/id_*; do
          if [ -f "${card_folder}/.ssh/${key##*/}" ]; then
            diff "$key" "${card_folder}/.ssh/${key##*/}" >/dev/null 2>/dev/null
            if [ $? -ne 0 ]; then
              echo "PROBLEM: ssh keyfile '${key}' doesn't match '${card_folder}/.ssh/${key##*/}'!!!" >>"ssh_keys_list.${EXT}"
              general_out "PROBLEM: ssh keyfile '${key}' doesn't match '${card_folder}/.ssh/${key##*/}'!!!"
              fails=`expr $fails + 1`
            fi
          else
            echo "PROBLEM: ssh keyfile '${key}' is missing or permission is denied from the mic folder '${card_folder}/.ssh'!!!" >>"ssh_keys_list.${EXT}"
            general_out "PROBLEM: ssh keyfile '${key}' is missing from the mic folder '${card_folder}/.ssh'!!!"
            fails=`expr $fails + 1`
          fi
        done
        [ $fails -eq 0 ] && echo "User ${u}'s keyfiles are all correct for 'mic${i}'." >>"ssh_keys_list.${EXT}"
      fi
    fi
  done
done

# Card crash dumps if requested...
if [ "$INC_DUMPS" = "1" ]; then
  general_out "Finding card crash dumps (this may take a while)..."
  mic_options=`grep "^options" /etc/modprobe.d/mic.conf | grep "crash_dump=1"`
  crash=false
  need_vmlinux=false
  [ -n "$mic_options" ] && crash=true
  [ $crash = true ] && general_out "Card crash dumps are enabled."
  [ $crash = false ] && general_out "Card crash dumps are NOT enabled."
  if [ $crash = true ]; then
    CRASH_PATH=`get_crash_dump_path`
    if [ -d "${CRASH_PATH}" ]; then
      for cardno in $COPROC_ALL_CARDS; do
        if [ -d "${CRASH_PATH}/mic${cardno}" ]; then
          general_out "     Card mic${cardno} has a crash dump folder."
          last_core=`ls -1t ${CRASH_PATH}/mic${cardno}/vmcore-*.gz | head -1`
          if [ "$last_core" != "" ]; then
            general_out "     Found vmcore file for card mic${cardno}."
            echo -n "   Copying core for card mic${cardno}...this could take a little while..."
            cp $last_core ./mic${cardno}-${last_core}
            need_vmlinux=true
            echo "Done."
          else
            general_out "     No vmcore files found for card mic${cardno}."
          fi
        else
          general_out "     Card mic${cardno} has no crash dump."
        fi
      done
    else
      general_out "     No card crash dumps detected (parent folder missing)."
    fi
  fi
  if [ $need_vmlinux = true ]; then
    general_out "Copying 'vmlinux' and 'System.map' for use with the crash dumps."
    cp /lib/firmware/mic/vmlinux .
    cp /lib/firmware/mic/System.map .
  fi
else
  general_out "Crash dumps were not requested (use the '--include_crash_dumps' option)..."
fi

# Get mic device PCIe mappings
if [ -c "/dev/mic0" ]; then
  general_out "Capturing micN to PCIe addresses..."
  rm -f "mic_to_pci_map.${EXT}"
  for n in $COPROC_ALL_CARDS; do
    pci=`get_mic_pcie_addr_by_number "${n}"`
    echo "/dev/mic${n} ==> PCIe: ${pci}" >>"mic_to_pci_map.${EXT}"
  done
fi

########################################################################
# Tests requiring that the 'mpss' service is "running" or "active":
if [ "$SERVICE" = "running" ] || [ "$SERVICE" = "active" ]; then
# Getting the cards sysfs entries...
  for card in $COPROC_ALL_CARDS; do
    rm -f "sysfs_mic${card}.${EXT}"
    general_out "Saving host sysfs for card mic${card}..."
    for file in /sys/class/mic/mic${card}/*; do
      [ -f "$file" ] && echo "${file##*/}='`cat ${file}`'" >>"sysfs_mic${card}.${EXT}"
    done
  done
# Run a ping test of online cards to see which are not responding...
  general_out "Testing the online cards network with ping..."
  for card in $COPROC_ONLINE_CARDS; do
    ping_test "$card"
    if [ $? -ne 0 ]; then
      general_out "PROBLEM: Card mic${card} is 'online' but NOT reponding to a ping (ICMP)!"
    fi
  done

# Get General active coprocessor software information...
  general_out "Gathering active coprocessor state data (using miccheck and mpssinfo)..."
  if [ "$COPROC_SW_COUNT" -eq "$COPROC_ONLINE_COUNT" ]; then
	RUN_ALL=1
	general_out "All $COPROC_ONLINE_COUNT cards are online."
  elif [ "$COPROC_ONLINE_COUNT" -eq 0 ]; then
    general_out "None of the cards are online."
  else
    general_out "Not all cards are online ($COPROC_ONLINE_COUNT online out of $COPROC_SW_COUNT):"
    for i in $COPROC_OFFLINE_CARDS; do
       state=`get_card_state_from_list "${card_states}" "${i}"`
       general_out "     mic${i} is in the '${state}' state"
    done
  fi
 
 rm -f "miccheck.${EXT}"
  if [ -n "$COPROC_ONLINE_CARDS" ]; then
    if [ "$RUN_ALL" -eq 1 ]; then
      general_out "Running miccheck on all devices..."
      echo "Running miccheck on all devices..." >>"miccheck.${EXT}"
      miccheck -v >>"miccheck.${EXT}"
    else
      general_out "Running miccheck on device list '${COPROC_ONLINE_CARDS}'..."
      echo "Running miccheck on device list '${COPROC_ONLINE_CARDS}'..." >>"miccheck.${EXT}"
      for micnum in $COPROC_ONLINE_CARDS; do
        miccheck -vd $micnum >>"miccheck.${EXT}"
      done
    fi
  else
    general_out "No online cards '${COPROC_ONLINE_CARDS}' responded to a ping test!"
    echo "No online cards '${COPROC_ONLINE_CARDS}' responded to a ping test!" >>"miccheck.${EXT}"
	fi
  mpssinfo >"mpssinfo.${EXT}"

# Gathering card dmesg logs...
  MOUNTED=`mount | awk '{if($5=="debugfs") { print $3 } }'`
  if [ -z "$MOUNTED" ]; then
    folder="${TEMP_FOLDER}/debug`date +%s`"
    general_out "Mounting '${folder}' (mount -t debugfs none ${folder})..."
    mkdir "$folder"
    if [ $? -ne 0 ]; then
      general_out "Could not create a temporary folder to mount the 'debugfs'!"
      folder=""
    else
      mount -t debugfs none "$folder"
      if [ $? -ne 0 ]; then
        general_out "Could not mount the 'debugfs' at '${folder}'!"
        folder=""
      fi
    fi
  else
    folder="$MOUNTED"
  fi
  if [ -n "$folder" ]; then
    for card in $COPROC_ALL_CARDS; do
      general_out "Saving card dmesg log files (using 'cat -v ${folder}/mic_debug/mic${card}/log_buf')..."
      cat -v "${folder}/mic_debug/mic${card}/log_buf" | sed 's/\^@//g' >"mic${card}_dmesg.${EXT}"
    done
  fi
  if [ -z "$MOUNTED" ]; then
    general_out "Unmounting '${folder}' (umount ${folder})..."
    umount "$folder"
    rmdir "$folder"
  fi 

# Checkin COI Daemon on the online cards...
  general_out "Checking on COI daemon running on the cards (WARNING: uses ssh as root)..."
  COI_FILE="coi_daemons.${EXT}"
  rm -f "$COI_FILE"
  responding=`generate_ping_list " " $COPROC_ONLINE_CARDS`
  for card in $responding; do
    netname=`get_host_or_ip_by_mic_number ${card}`
    ssh -q -o "BatchMode yes" -o "StrictHostKeyChecking no" "${netname}" "echo -n"
    if [ $? -eq 0 ]; then
      coi_state=`ssh -q -o "BatchMode yes" -o "StrictHostKeyChecking no" ${netname} "ps -ae | grep coi_daemon | grep -v -e grep"`
      if [ -z "$coi_state" ]; then
        echo "Card 'mic${card}': COI Daemon NOT RUNNING" | tee -a "$COI_FILE"
      else
        echo "Card 'mic${card}': COI Daemon Running" | tee -a "$COI_FILE"
      fi
    else
      echo "ERROR: Could not ssh to online card at '${netname}' as root!" | tee -a "$COI_FILE"
    fi
  done

# Get /proc/elog from running cards...
  general_out "Getting /proc/elog on the cards (WARNING: uses ssh as root)..."
  responding=`generate_ping_list " " $COPROC_ONLINE_CARDS`
  for card in $responding; do
    netname=`get_host_or_ip_by_mic_number ${card}`
    ssh -q -o "BatchMode yes" -o "StrictHostKeyChecking no" "${netname}" "echo -n"
    if [ $? -eq 0 ]; then
      ssh -q -o "BatchMode yes" -o "StrictHostKeyChecking no" "${netname}" "cat /proc/elog" >"elog_mic${card}.${EXT}"
    else
      general_out "ERROR: Could not ssh to online card at '${netname}' as root!"
    fi
  done

# Checking for ofed installation and status
  rm -f "ofed.${EXT}"
  if [ -x /etc/init.d/ofed-mic ]; then
    general_out "Checking for the state of OFED components..."
    if [ -n "$SYSCTL" ]; then
      echo "systemctl status openibd..." >>"ofed.${EXT}"
      systemctl status openibd >>"ofed.${EXT}"
    else
      echo "service openibd status..." >>"ofed.${EXT}"
      service openibd status >>"ofed.${EXT}"
    fi
    if [ -n "$SYSCTL" ]; then
      echo "systemctl status ofed-mic..." >>"ofed.${EXT}"
      systemctl status ofed-mic >>"ofed.${EXT}"
    else
      echo "service ofed-mic status..." >>"ofed.${EXT}"
      service ofed-mic status >>"ofed.${EXT}"
    fi
    if [ -n "$SYSCTL" ]; then
      echo "systemctl status mpxyd..." >>"ofed.${EXT}"
      systemctl status mpxyd >>"ofed.${EXT}"
    else
      echo "service mpxyd status..." >>"ofed.${EXT}"
      service mpxyd status >>"ofed.${EXT}"
    fi
  else
    general_out "'ofed-mic' for  is not installed, skipping that check."
  fi
fi
########################################################################
# Known dangerous commands (deep inspection lspci)...
# Does the SW count equal the HW count?
COPROC_HW_COUNT=`lspci | grep "Co-proc" | grep "Intel" | wc -l`
if [ $COPROC_SW_COUNT -ne $COPROC_HW_COUNT ]; then
  general_out "PROBLEM: The number of '$MIC_CONFIG_DIR/mic*.conf' files does not equal what 'lspci' reports (${COPROC_HW_COUNT})!"
  general_out "         Possible configuration problem!"
fi

# Get detailed lspci listing for coprocessor devices only...
general_out "Getting PCIe addresses for all coprocessors (using lspci)..."
COPROC_PCI_ADDRS=`lspci | grep Co-proc | grep Intel | awk -- 'BEGIN {list=""} {if(list!="") list=list " "; list=list $1} END {print list}'`
general_out "Saving detailed (lspci -vvvv) information for only coprocessors..."
rm -f "coproc_only_lspci.${EXT}"
for coproc in $COPROC_PCI_ADDRS; do
  lspci -vvvvv -s "$coproc"
  echo "#######################################################################"
done >>"coproc_only_lspci.${EXT}"

# Get full lspci device tree listing (usefule for discovering parents of the cards)...
general_out "Saving full lspci -t tree information..."
lspci -Dtv >"tree_lspci.${EXT}"

# Get full detailed lspci listing (last resort analysis; its HUGE)...
general_out "Saving full 'lspci' output for the entire PCIe tree..."
lspci -vvvvv >"full_lspci.${EXT}"

########################################################################
# Package data...
echo "==================================================================="
echo "Packaging TAR file to send to your system support representative..."
echo " * If crash dumps were enabled and present then this will take a long while..."
echo " * This will list the contents of the archive file..."
echo
# Added step to remove any editor backups accidently copied...
rm -f *~

cd ..
tar_ok=-1
if [ ! -f $TAR_FILE ]; then
  tar -czvf $TAR_FILE $FOLDER_NAME
  tar_ok=$?
fi

# Cleanup...
if [ $tar_ok -eq 0 ]; then
  echo "Cleaning up..."
  rm -rf "$FOLDER_NAME"
  popd  >/dev/null
else
  echo "FATAL ERROR: ${SCRIPT_NAME}: The compressed tar file '${TAR_FILE}' was NOT created!"
  echo "     Are you out of disk space in '${TEMP_FOLDER}'?"
  echo "     Have permissions to '${TEMP_FOLDER}' changed for 'root'?"
  echo "     For security reasons, you will have to re-run this script!"
  echo "Cleaning up..."
  rm -rf "$FOLDER_NAME"
  popd  >/dev/null
  exit -1
fi
location="./${TAR_FILE}"
if [ "$TEMP_FOLDER" != "$(pwd)" ]; then
   mv ${TEMP_FOLDER}/${TAR_FILE} .
fi   
if [ ! -f "$location" ]; then
   location="${TEMP_FOLDER}/${TAR_FILE}"
fi
pgid=$(su $REAL_USER -c 'id -gn')
chmod 0400 $location
chown $REAL_USER:$pgid $location

echo
echo "*** Please send the file '${location}'"
echo "to your support representitive for analysis, Thanks."
echo
echo "*** If the host crashed please also compress and send the host's"
echo "    crash dump file and the System.map file, Thanks."
exit 0
