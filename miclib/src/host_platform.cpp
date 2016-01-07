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

#include <stdlib.h>
#include <new>
#include <algorithm>

#include "host_platform.h"
#include "ut_instr_event.h"

using namespace std;

namespace pciconfig {
const size_t VENDOR_ID_OFFS = 0;
const size_t VENDOR_ID_SIZE = 2;
const size_t DEVICE_ID_OFFS = 2;
const size_t DEVICE_ID_SIZE = 2;
const size_t REVISION_ID_OFFS = 8;
const size_t REVISION_ID_SIZE = 1;
const size_t SUBSYSTEM_ID_OFFS = 0x2e;
const size_t SUBSYSTEM_ID_SIZE = 2;
const size_t PCI_LINK_STATS_OFFS = 0x5e;
const size_t PCI_LINK_STATS_SIZE = 2;
const size_t PCI_CTRL_CAP_OFFS = 0x54;
const size_t PCI_CTRL_CAP_SIZE = 2;
};

/* states and modes */
const char *host_platform::STATE_ONLINE = "online";
const char *host_platform::MODE_FLASH = "flash";
const char *host_platform::MODE_ELF = "elf";
const char *host_platform::STATE_READY = "ready";
const char *host_platform::MODE_MAINT_CMD =
    "boot:flash:/usr/share/mpss/boot/rasmm-kernel.from-eeprom.elf";
const char *host_platform::MODE_MAINT_CMD_AB_STEP =
    "boot:flash:/usr/share/mpss/boot/rasmm-kernel.knightscorner-ab.elf";
const char *host_platform::MODE_MAINT_CMD_C_STEP =
    "boot:flash:/usr/share/mpss/boot/rasmm-kernel.knightscorner-c.elf";
const char *host_platform::MODE_READY_CMD = "reset";


const char *host_platform::STATE = "state";
const char *host_platform::MODE = "mode";
const char *host_platform::KSYSFS_DEVICE_PREFIX = "mic";
const char *host_platform::KHOST_DRIVER_PATH = "/dev/mic/ctrl";
const char *host_platform::PCI_PREFIX = "pci_";
const char *host_platform::SYSFS_STATE_CHANGE = "state";
const char *host_platform::POST_CODE = "post_code";
const char *host_platform::FLASH_UPDATE = "flash_update";
const char *host_platform::ENABLE_VMM_UPDATE = "839909667";     /* 0x32100123 */
const char *host_platform::FAIL_SAFE_OFFS = "fail_safe_offset";
const char *host_platform::MEM_SIZE = "memsize";
const char *host_platform::PROCESSOR_MODEL = "model";
const char *host_platform::PROCESSOR_EXT_MODEL = "extended_model";
const char *host_platform::PROCESSOR_TYPE = "processor";
const char *host_platform::PROCESSOR_FAMILY = "family_data";
const char *host_platform::PROCESSOR_EXT_FAMILY = "extended_family";
const char *host_platform::PROCESSOR_STEPPING = "stepping";
const char *host_platform::PROCESSOR_STEPPING_DATA = "stepping_data";
const char *host_platform::PROCESSOR_SUBSTEPPING_DATA = "substepping_data";
const char *host_platform::SI_SKU = "sku";

std::string host_platform::SYSFS_DEVICE_PATH = "/sys/class/mic";
const std::string host_platform::MIC_MODULE = "/sys/module/mic";
const std::string host_platform::SYSFS_SCIF_PATH = "/sys/class/mic/scif";
const std::string host_platform::KNC_IDENTIFIER = "x100";

static const int SCIF_LISTEN_PORT = 100;

host_platform::host_platform(uint32_t devid, bool init_scif)
    : _devid(devid),
    _scif_ep(0),
    _scif_inited(false),
    _scif_errno(0),
    _scif_mic_errno(0)

{
    if (pthread_mutex_init(&_mutex, NULL) != 0)
        throw mic_exception(E_MIC_SYSTEM, "pthread_mutex_init");

    (void) init_scif;
}

host_platform::~host_platform()
{
    if (_scif_inited)
        scif_close(_scif_ep);

    // Following may fail if, e.g.,  the device was still in use by
    // another thread. Consider this to be an application error and
    // ignore the failure.
    if (pthread_mutex_destroy(&_mutex) == 0) {
    }
    ;
}

void host_platform::get_devices(struct mic_devices_list *device_list,
                                uint16_t n_allocated)
{
    ASSERT(device_list != NULL && "list of devices cannot be null");
    ASSERT(n_allocated != 0 && "number of alloc'ed devices cannot be 0");
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    uint16_t count = 0;
    std::string sysfs_path = get_sysfs_base_path();

    if ((dir = opendir(MIC_MODULE.c_str())) == NULL)
        throw mic_exception(E_MIC_DRIVER_NOT_LOADED, "host driver is not loaded");
    else if (closedir(dir) < 0)
        throw mic_exception(E_MIC_SYSTEM, "closedir: " + sysfs_path);

    if ((dir = opendir(sysfs_path.c_str())) == NULL)
        throw mic_exception(E_MIC_DRIVER_INIT, "host driver is not initialized");

    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        string dirname = entry->d_name;
        size_t idx;
        int temp;

        idx = dirname.find(KSYSFS_DEVICE_PREFIX, 0);

        if (idx != 0)
            continue;

        idx += strlen(KSYSFS_DEVICE_PREFIX);

        std::stringstream ss(dirname.substr(idx));

        /* If true, we found, for example, /sys/class/mic/mice. */
        if ((ss >> temp).fail()) {
            throw mic_exception(E_MIC_STACK, sysfs_path +
                      ": malformed entry: " + dirname, ECANCELED);
        }

        if (count < n_allocated)
            device_list->devices[count] = temp;

        count++;
    }

    /* readdir returned NULL - make sure that it's an EOF condition,
     * and not an error.
     */
    if (errno) {
        int save = errno;

        (void)closedir(dir);
        errno = save;
        throw mic_exception(E_MIC_SYSTEM, "readdir: " + sysfs_path,
                            save);
    }

    if (closedir(dir) < 0)
        throw mic_exception(E_MIC_SYSTEM, "closedir: " + sysfs_path);

    sort(device_list->devices,
         device_list->devices + (count < n_allocated ? count : n_allocated));
    device_list->n_devices = count;
}

std::string host_platform::get_sysfs_device_path() const
{
    std::stringstream sstm;

    sstm << get_sysfs_base_path() << "/mic" << _devid;
    return sstm.str();
}

std::string host_platform::get_sysfs_attr_path(string const &attr) const
{
    std::stringstream sstm;

    sstm << get_sysfs_device_path() << "/" << attr;
    return sstm.str();
}

std::string host_platform::get_sysfs_comp_path(string const &component,
                                               string const &attr)
{
    std::stringstream sstm;

    sstm << get_sysfs_base_path() << "/" << component << "/" << attr;
    return sstm.str();
}

void host_platform::get_device_type(uint32_t &device_type)
{
    string device_type_str = get_device_property("family");

    if (device_type_str == KNC_IDENTIFIER) {
        device_type = KNC_ID;
    } else {
        string filepath = get_sysfs_attr_path("family");
        throw mic_exception(E_MIC_UNSUPPORTED_DEV, filepath +
                            ": " + device_type_str.c_str() +
                            ": not supported", ECANCELED);
    }
}

void host_platform::read_postcode_property(string const &filepath, string &strval)
{
    unsigned int wait_count = 0;


    //After the card reboot there is a small duration(~0.5 sec),
    //during this time, the sysfs read for postcode returns with error "EAGAIN"


    for(; wait_count < WAIT_COUNT_EAGAIN; ++wait_count) {
         try {
             read_property(filepath, strval);
             break;
         } catch (mic_exception const& e) {
             if (e.get_sys_errno() == EAGAIN)
                 usleep(WAIT_TIME_EAGAIN);
             else
                 throw e;
         }
     }

     if (wait_count >= WAIT_COUNT_EAGAIN)
         throw mic_exception(E_MIC_STACK, "postcode read timeout: " + filepath);


}


void host_platform::read_property(string const &filepath, string &strval)
{
    int fd, ret;
    char buf[NAME_MAX];
    error_t sys_errno;


    if ((fd = open(filepath.c_str(), O_RDONLY)) < 0)
        throw mic_exception(E_MIC_STACK, "open: " + filepath);

    if ((ret = read(fd, buf, sizeof(buf))) < 0) {
            sys_errno = errno;
            close(fd);
            throw mic_exception(E_MIC_STACK, "read: " + filepath, sys_errno);
    }

    if (close(fd) < 0)
        throw mic_exception(E_MIC_STACK, "close: " + filepath);

    strval = string(buf, ret);
}

void host_platform::read_property_token(string const &filepath,
                                        string &token, string &strval)
{
    FILE *fd = NULL;
    char buf[NAME_MAX];
    bool found = false;

    if ((fd = fopen(filepath.c_str(), "r")) == NULL)
        throw mic_exception(E_MIC_STACK, "fopen: " + filepath);

    while (fgets(buf, sizeof(buf), fd) != NULL) {
        if (strncmp(buf, token.c_str(), token.length()) == 0) {
            found = true;
            break;
        }
    }

    if (found == false) {
        fclose(fd);
        throw mic_exception(E_MIC_STACK,
                            "read: " + filepath + ":" + token);
    }

    if (fclose(fd) < 0)
        throw mic_exception(E_MIC_STACK, "close: " + filepath);

    strval = string(buf, strlen(buf) + 1);
}

std::string host_platform::get_device_property(string const &property)
{
    size_t idx;

    if (property.empty()) {
        throw mic_exception(E_MIC_INTERNAL, property +
                            ": null property",
                            ECANCELED);
    }

    std::string output;
    if(property==host_platform::POST_CODE) {
        read_postcode_property(get_sysfs_attr_path(property), output);
    }
    else {
        read_property(get_sysfs_attr_path(property), output);
    }

    idx = output.find("\n", 0);

    if (idx != string::npos)
        output = output.substr(0, idx);

    return output;
}

void host_platform::set_device_property(string const &property,
                                        string const &value) const
{
    int fd;
    int retval = -1;

    if (property.empty())
        throw mic_exception(E_MIC_INTERNAL, "property null", ECANCELED);

    string filepath = get_sysfs_attr_path(property);

    if ((fd = open(filepath.c_str(), O_RDWR)) < 0)
        throw mic_exception(E_MIC_STACK, "open: " + filepath);

    if ((retval = write(fd, value.c_str(), value.size())) < 0) {
        close(fd);
        throw mic_exception(E_MIC_STACK, "write: " + filepath);
    }

    if (close(fd) < 0)
        throw mic_exception(E_MIC_STACK, "close: " + filepath);
}

void host_platform::get_pci_config(struct mic_pci_config &pci_config)
{
    std::string dirname;
    string config_info, comp;
    string pci_str;
    uint16_t control_cap = 0;
    uint16_t link_stat = 0;
    uint32_t dindx = 0, sindx = 0, eindx = 0;
    string token;

    pci_config.access_violation = 0;
    dirname = get_sysfs_device_path();
    dirname += "/device";
    pci_config.domain_info_implemented = 1;
    token = "PCI_CLASS";
    read_property_token(dirname + "/uevent", token, comp);
    if (comp.length() > 0) {
        /* get busno, deviceno */
        sindx = comp.find("=");
        if (sindx > 0) {
            pci_str = comp.substr(++sindx);
            size_t terminator = pci_str.find_first_of(" \t\n\f\v\r");
            if(terminator != std::string::npos)
                pci_str.erase(terminator);
            strncpy(pci_config.class_code, pci_str.c_str(), NAME_MAX);
            pci_config.class_code[NAME_MAX] = '\0';
        }
    }
    token = "PCI_SLOT_NAME";
    read_property_token(dirname + "/uevent", token, comp);
    /* get domain, busno, deviceno */
    dindx = 0; sindx = 0; eindx = 0;
    dindx = comp.find("=");
    sindx = comp.find(":", ++dindx);
    eindx = comp.find(":", ++sindx);
    if (sindx > dindx) {
        pci_str = comp.substr(dindx, (sindx - dindx));
        errno = 0;
        pci_config.domain = strtol(pci_str.c_str(), NULL, 16);
        if (errno) {
            throw mic_exception(E_MIC_SYSTEM,
                                comp + ": domain not found",
                                EINVAL);
        }
    }
    if (eindx > sindx) {
        pci_str = comp.substr(sindx, (eindx - sindx));
        errno = 0;
        pci_config.bus_no = strtol(pci_str.c_str(), NULL, 16);
        if (errno) {
            throw mic_exception(E_MIC_SYSTEM,
                                comp + ": bus number not found",
                                EINVAL);
        }
        /* device_no */
        sindx = eindx + 1;
        eindx = comp.find(".");
        if (eindx > sindx) {
            pci_str = comp.substr(sindx, (eindx - sindx));
            errno = 0;
            pci_config.device_no =
                strtol(pci_str.c_str(), NULL, 16);
            if (errno) {
                throw mic_exception(
                          E_MIC_SYSTEM,
                          comp +
                          ": device number not found",
                          EINVAL);
            }
        }
    }
    read_property(dirname + "/config", config_info);
    pci_config.vendor_id = *(reinterpret_cast < const uint16_t *>
                             (&config_info.c_str()[pciconfig::
                                                   VENDOR_ID_OFFS]));
    pci_config.device_id = *(reinterpret_cast < const uint16_t *>
                             (&config_info.c_str()[pciconfig::
                                                   DEVICE_ID_OFFS]));
    pci_config.revision_id = *(reinterpret_cast <const uint8_t *>
                               (&config_info.c_str()[pciconfig::
                                                     REVISION_ID_OFFS]));
    pci_config.subsystem_id = *(reinterpret_cast <const uint16_t *>
                                (&config_info.c_str()[pciconfig::
                                                      SUBSYSTEM_ID_OFFS]));

    if (config_info.length() >= pciconfig::PCI_LINK_STATS_OFFS + 2) {
        link_stat = *(reinterpret_cast <const uint16_t *>
                      (&config_info.
                       c_str()[pciconfig::PCI_LINK_STATS_OFFS]));
        pci_config.link_speed = link_stat & 0xf;
        link_stat = *(reinterpret_cast <const uint16_t *>(&config_info.
                                                          c_str()[
                                                              pciconfig
                                                              ::
                                                              PCI_LINK_STATS_OFFS
                                                          ]));
        pci_config.link_width = ((link_stat >> 4) & 0x3f);
    }
    if (config_info.length() >= pciconfig::PCI_CTRL_CAP_OFFS + 2) {
        /* max payload size */
        control_cap = *(reinterpret_cast <const uint16_t *>
                        (&config_info.c_str()[pciconfig::
                                              PCI_CTRL_CAP_OFFS]));
        control_cap &= 0xe0;
        pci_config.payload_size = 128 * (1 << (control_cap >> 5));

        /* Read req size */
        control_cap = *(reinterpret_cast <const uint16_t *>
                        (&config_info.c_str()[pciconfig::
                                              PCI_CTRL_CAP_OFFS]));
        control_cap &= 0x7000;
        pci_config.read_req_size = 128 * (1 << (control_cap >> 12));
    }
    if (!link_stat || !control_cap)
        pci_config.access_violation = 1;
}

int host_platform::scif_open_conn()
{
    scif_epd_t ep = -1;
    struct scif_portID peer = { 0, 0 };
    int port = -1;

    ep = scif_open();
    UT_INSTRUMENT_EVENT("MICLIB_HOST_PLATFORM_SCIF_CONN_0", ep = -1);
    if (ep < 0) {
        UT_INSTRUMENT_EVENT("MICLIB_HOST_PLATFORM_SCIF_CONN_0",
                            (scif_close(ep), errno = EIO));
        throw mic_exception(E_MIC_SCIF_ERROR, "scif_open failed", errno);
    }

    if (geteuid() == 0) {
        uint16_t t;

        for (t = SCIF_ADMIN_PORT_END - 1; t > 0; t--) {
            port = scif_bind(ep, t);
            if (port == t)
                break;
            ASSERT(port < 0);
            ASSERT(errno == EINVAL);
        }
    } else {
        port = scif_bind(ep, 0);
        if (port < 0) {
            (void)scif_close(ep);
            throw mic_exception(E_MIC_SCIF_ERROR, "scif_bind failed", errno);
        }
    }

    peer.node = _devid + 1;
    peer.port = SCIF_LISTEN_PORT;

    if (scif_connect(ep, &peer) < 0) {
        error_t saved_errno = errno;
        (void)scif_close(ep);

        if (saved_errno == ECONNREFUSED)
            throw mic_exception(E_MIC_SCIF_ERROR, "scif connection refused",
                                saved_errno);
        else
            throw mic_exception(E_MIC_SCIF_ERROR, "scif_connect failed",
                                saved_errno);
    }

    _scif_inited = true;
    _scif_ep = ep;

    return 0;
}

void host_platform::scif_request(int req_id, void *resp_buf,
                                 size_t resp_size, uint32_t parm)
{
    ASSERT(resp_buf != NULL && "resp_buff cannot be null");
    struct mr_hdr init_buf = { 0, 0, 0, 0, 0 };
    struct mr_hdr ack_buf = { 0, 0, 0, 0, 0 };
    int ret;
    int n;
    void *msg;

    /* Serialize SCIF requests. */
    if (pthread_mutex_lock(&_mutex) != 0)
        throw mic_exception(E_MIC_SYSTEM, "pthread_mutex_lock");

    mic_exception::ts_clear_ras_errno();

    try {
        std::stringstream ss;

        if (!_scif_inited) {
            scif_open_conn();
        }

        init_buf.cmd = req_id;
        init_buf.len = 0;
        init_buf.parm = parm;
        init_buf.stamp = 0;
        init_buf.spent = 0;

        /* send the request */
        n = sizeof(init_buf);
        msg = (void *)&init_buf;
        while (n > 0) {
            ret = scif_send(_scif_ep, msg, n, SCIF_SEND_BLOCK);
            UT_INSTRUMENT_EVENT("MICLIB_HOST_PLATFORM_SCIF_REQ_0",
                                (ret = -1, errno = EIO));
            /* Re-establish the connection if the
             *  connection was reset by peer (ECONNRESET) */
            if (ret < 0 && errno == ECONNRESET) {
                try {
                    scif_open_conn();
                    ret = scif_send(_scif_ep, msg, n, SCIF_SEND_BLOCK);
                } catch (mic_exception const &e) {
                    ret = -1;
                }
            }
            if (ret < 0) {
                ss << "scif_send: cmd 0x" << std::hex
                   << req_id << ": Len 0x" << std::hex <<
                    sizeof(init_buf);
                throw mic_exception(E_MIC_SCIF_ERROR, ss.str());
            }
            msg = (void *)((char *)msg + ret);
            n -= ret;
        }

        UT_INSTRUMENT_EVENT("MICLIB_HOST_PLATFORM_SCIF_REQ_1", n = -1);
        if (n < 0) {
            throw mic_exception(E_MIC_SCIF_ERROR, "scif_send: "
                                "Internal error", EPROTO);
        }

        /* receive response of request. e.g. was it a valid request? */
        n = sizeof(ack_buf);
        msg = (void *)&ack_buf;
        while (n > 0) {
            ret = scif_recv(_scif_ep, msg, n, SCIF_RECV_BLOCK);
            UT_INSTRUMENT_EVENT("MICLIB_HOST_PLATFORM_SCIF_REQ_2",
                                (ret = -1, errno = EIO));
            if (ret < 0) {
                ss << "scif_recv: cmd 0x" << std::hex <<
                    req_id << ": Len 0x" << std::hex <<
                    sizeof(init_buf);

                throw mic_exception(E_MIC_SCIF_ERROR, ss.str());
            }
            msg = (void *)((char *)msg + ret);
            n -= ret;
        }

        UT_INSTRUMENT_EVENT("MICLIB_HOST_PLATFORM_SCIF_REQ_3", n = -1);
        if (n < 0) {
            throw mic_exception(E_MIC_SCIF_ERROR, "scif_recv "
                                "(header): Internal error", EPROTO);
        }

        if (ack_buf.cmd & MR_ERROR) {
            if ((ack_buf.cmd & MR_OP_MASK) != req_id) {
                ss << "scif_recv: Unexpected opcode 0x" <<
                    std::hex <<
                    (ack_buf.cmd & MR_OP_MASK) <<
                    ": Expected 0x" << std::hex << req_id;
                throw mic_exception(E_MIC_SCIF_ERROR,
                                    ss.str(), EPROTO);
            }

            if (ack_buf.len == sizeof(struct mr_err)) {
                struct mr_err me;

                n = sizeof(me);
                msg = (void *)&me;
                while (n > 0) {
                    ret = scif_recv(_scif_ep, msg, n,
                                    SCIF_RECV_BLOCK);
                    if (ret < 0) {
                        ss << "scif_recv: cmd 0x" <<
                            std::hex << req_id <<
                            ": Failed error record: "
                           << "Len 0x" << std::hex <<
                            sizeof(me);
                        throw mic_exception(
                                  E_MIC_SCIF_ERROR,
                                  ss.str());
                    }
                    msg = (void *)((char *)msg + ret);
                    n -= ret;
                }
                ss << "RAS: cmd 0x" << std::hex
                   << req_id << ": Error 0x" << std::hex << me.err
                   << ": " << mic_exception::ras_strerror(me.err);
                throw mic_exception(E_MIC_RAS_ERROR, ss.str(), 0, me.err);
            } else {
                ss << "scif_recv: cmd 0x" << std::hex
                   << req_id << ": Unknown error: Len 0x"
                   << std::hex << req_id;

                throw mic_exception(E_MIC_SCIF_ERROR, ss.str(),
                                    EPROTO);
            }
        }

        if (ack_buf.len != resp_size) {
            ss << "scif_recv: cmd 0x" << std::hex << req_id <<
                ": Response payload len 0x" << std::hex <<
                ack_buf.len << ": Expected 0x" << std::hex << resp_size;

            throw mic_exception(E_MIC_SCIF_ERROR, ss.str(),
                                EPROTO);
        }

        /* get the actual data (resp_buf) of the performed request */
        n = resp_size;
        msg = resp_buf;
        while (n > 0) {
            ret = scif_recv(_scif_ep, resp_buf, resp_size,
                            SCIF_RECV_BLOCK);
            if (ret < 0) {
                ss << "scif_recv: cmd 0x" << std::hex <<
                    req_id << ": Response failed";
                throw mic_exception(E_MIC_SCIF_ERROR, ss.str());
            }

            msg = (void *)((char *)msg + ret);
            n -= ret;
        }

        if (n < 0) {
            throw mic_exception(E_MIC_SCIF_ERROR, "scif_recv "
                                "(data): Internal error", EPROTO);
        }
    } catch (mic_exception const &e) {
        if (pthread_mutex_unlock(&_mutex) == 0) {
        }
        ;

        if (e.get_mic_errno() == E_MIC_SCIF_ERROR) {
            /* Reset SCIF connection. */
            if (_scif_inited)
                (void)scif_close(_scif_ep);
            _scif_inited = false;
        }

        throw e;
    }

    if (pthread_mutex_unlock(&_mutex) == 0) {
    }
    ;
}

host_flash_op *host_platform::flash_init_fd(void *buf, size_t size,
                                            MIC_FLASH_CMD_TYPE op)
{
    return flash_init_fd(buf, size, op, O_RDONLY);
}

host_flash_op *host_platform::flash_init_fd(void *buf, size_t size,
                                            MIC_FLASH_CMD_TYPE op,
                                            int open_flags)
{
    int fd = -1;
    struct host_flash_op *desc;

    try {
        fd = open(KHOST_DRIVER_PATH, open_flags);

        if (fd < 0) {
            string error_msg = KHOST_DRIVER_PATH;
            throw mic_exception(E_MIC_SYSTEM, "open: " + error_msg);
        }

        desc = new host_flash_op();

        desc->op = op;
        desc->fd = fd;
        desc->buf = buf;
        desc->bufsize = size;

        return desc;
    } catch (std::exception const &e) {
        close(fd);
        throw e;
    }
}

void host_platform::flash_close_fd(struct host_flash_op *desc)
{
    /* if we ever modify this to check retval of close,
     * we will need to improve some unit tests */
    close(desc->fd);
    delete(desc);
}

struct host_flash_op *host_platform::flash_read_start(void *buf, size_t size)
{
    struct host_flash_op *desc = NULL;

    try {
        desc = flash_init_fd(buf, size, FLASH_READ);
        flash_ioctl(desc);

        return desc;
    } catch (mic_exception const &e) {
        if (desc != NULL)
            flash_close_fd(desc);
        throw e;
    }
}

void host_platform::flash_read_done(struct host_flash_op *desc)
{
    bool remove_desc = true;

    bzero(desc->buf, desc->bufsize);     // To keep valgrind happy.
    try {
        flash_ioctl(desc->fd, desc->buf, desc->bufsize,
                    FLASH_READ_DATA);
        remove_desc = false;
        flash_close_fd(desc);
    } catch (mic_exception const &e) {
        if (remove_desc)
            flash_close_fd(desc);
        throw e;
    }
}

std::string host_platform::get_sysfs_attribute(std::string const &attr)
{
    return get_device_property(attr);
}

int host_platform::is_ras_avail()
{
    int ret = 0;

    if (!_scif_inited) {
        if (pthread_mutex_lock(&_mutex) != 0)
            throw mic_exception(E_MIC_SYSTEM, "pthread_mutex_lock");

        try {
            scif_open_conn();
        } catch (mic_exception const &e) {
            _scif_inited = false;

            if (pthread_mutex_unlock(&_mutex) == 0) {
            }

            int err = e.get_sys_errno();
            if (err == ENODEV || err == ECONNREFUSED)
                ret = -1;
            else
                throw;
        }

        if (pthread_mutex_unlock(&_mutex) == 0) {
        }
    }

    if (_scif_inited) {
        try {
            struct mr_rsp_pver pver;
            scif_request(host_platform::PVER_CMD,
                         &pver, host_platform::PVER_SIZE);
        } catch (...) {
            ret = -1;
        }
    }
    return ret == 0;
}

struct host_flash_op *host_platform::flash_update_start(void *buf, size_t size)
{
    struct host_flash_op *desc = NULL;
    bool update_enabled = false;

    try {
        desc = flash_init_fd(buf, size, FLASH_WRITE);

        set_device_property(FLASH_UPDATE, ENABLE_VMM_UPDATE);
        update_enabled = true;
        flash_ioctl(desc);

        return desc;
    } catch (mic_exception const &e) {
        if (update_enabled)
            set_device_property(FLASH_UPDATE, "0");
        if (desc != NULL)
            flash_close_fd(desc);
        throw e;
    }
}

void host_platform::flash_update_done(struct host_flash_op *desc)
{
    bool remove_desc = true;

    try {
        set_device_property(FLASH_UPDATE, "0");
        remove_desc = false;
        flash_close_fd(desc);
    } catch (mic_exception const &e) {
        if (remove_desc)
            flash_close_fd(desc);
        throw e;
    }
}

void host_platform::flash_ioctl(int fd, void *buf, size_t size,
                                MIC_FLASH_CMD_TYPE cmd)
{
    struct ctrlioctl_flashcmd fcmd;

    fcmd.brdnum = _devid;
    fcmd.data = buf;
    fcmd.len = size;
    fcmd.type = cmd;

    if (ioctl(fd, IOCTL_FLASHCMD, &fcmd) < 0) {
        string error_msg = KHOST_DRIVER_PATH;
        throw mic_exception(E_MIC_SYSTEM, "ioctl: " + error_msg);
    }
}

void host_platform::flash_ioctl(struct host_flash_op *desc)
{
    flash_ioctl(desc->fd, desc->buf, desc->bufsize, desc->op);
}

void host_platform::flash_get_status(struct host_flash_op *desc,
                                     struct mic_flash_status_info *status)
{
    struct flash_stat fs;

    bzero((void *)&fs, sizeof(fs));
    bzero((void *)status, sizeof(*status));
    flash_ioctl(desc->fd, (void *)&fs, sizeof(fs), FLASH_CMD_STATUS);

    switch (fs.status) {
    case FLASH_IDLE:
        status->flash_status = FLASH_OP_IDLE;
        break;

    case FLASH_CMD_IN_PROGRESS:
        status->flash_status = FLASH_OP_IN_PROGRESS;
        status->complete = fs.percent;
        break;

    case FLASH_CMD_COMPLETED:
        status->flash_status = FLASH_OP_COMPLETED;
        status->complete = 100;
        break;

    case FLASH_CMD_FAILED:
        status->flash_status = FLASH_OP_FAILED;
        break;

    case FLASH_CMD_AUTH_FAILED:
        status->flash_status = FLASH_OP_AUTH_FAILED;
        break;

    case FLASH_SMC_CMD_IN_PROGRESS:
        status->complete = fs.percent;
        status->flash_status = SMC_OP_IN_PROGRESS;
        break;

    case FLASH_SMC_CMD_COMPLETE:
        status->complete = 100;
        status->flash_status = SMC_OP_COMPLETED;
        break;

    case FLASH_SMC_CMD_FAILED:
        status->flash_status = SMC_OP_FAILED;
        break;

    case FLASH_SMC_CMD_AUTH_FAILED:
        status->flash_status = SMC_OP_AUTH_FAILED;
        break;

    case FLASH_CMD_INVALID:
        // status = FLASH_OP_INVALID;
        status->flash_status = FLASH_OP_IN_PROGRESS;
        break;

    default:
        status->flash_status = FLASH_OP_INVALID;
    }

    status->ext_status = fs.smc_status;
}

uint32_t host_platform::get_flash_vendor()
{
#pragma pack(push, 1)
    struct vendor_dev_info {
        struct version_struct vs;
        uint32_t              vs_vendor_dev;
    } vs;
#pragma pack(pop)
    struct host_flash_op *desc = NULL;

    try {
        desc = flash_init_fd((void *)&vs, sizeof(vs), FLASH_CMD_VERSION);
        flash_ioctl(desc);
        flash_close_fd(desc);
        return vs.vs_vendor_dev;
    } catch (mic_exception const &e) {
        if (desc != NULL)
            flash_close_fd(desc);
        throw e;
    }
}

std::string host_platform::get_sysfs_base_path()
{
    return SYSFS_DEVICE_PATH;
}

void host_platform::set_sysfs_base_path(std::string const &device_path)
{
    SYSFS_DEVICE_PATH = device_path;
}

void host_platform::set_device_mode(int const &mode)
{
    if (mode == MIC_MODE_MAINT) {
        char stepping = get_device_property(PROCESSOR_STEPPING).at(0);
        if (stepping == 'A' || stepping == 'B') {
            /* Stepping IS A or B */
            set_device_property(host_platform::STATE,
                                host_platform::MODE_MAINT_CMD_AB_STEP);
        } else if (stepping == 'C') {
            /* Stepping IS C */
            set_device_property(host_platform::STATE,
                                host_platform::MODE_MAINT_CMD_C_STEP);
        } else {
            /* Any stepping other than A,B and C.
             * This command will load the dummy maintenance mode handler.
             * The loaded dummy maintenance mode handler fails the authentication
             * test, and eventually bootstrap loads the default maintenance mode
             * handler from the flash*/
            set_device_property(host_platform::STATE,
                                host_platform::MODE_MAINT_CMD);
        }
    } else if (mode == MIC_MODE_READY) {
        set_device_property(host_platform::STATE,
                            host_platform::MODE_READY_CMD);

        //read postcode in sysfs after reset to make sure it is initialized
        get_sysfs_attribute(host_platform::POST_CODE);
    } else {
        std::stringstream ss;

        ss << "Mode request: '0x" << std::hex << mode << "'";
        throw mic_exception(E_MIC_INVAL, ss.str(), EINVAL);
    }
}

void host_platform::set_device_mode(int const &mode, const
                                    std::string &elf_path) const
{
    FILE *fptr;
    int pos = 0;

    fptr = fopen((const char *)elf_path.c_str(), "r");
    if (fptr == NULL) {
        // Validate if the elf file exists and is not empty
        std::stringstream ss;

        ss << "Invalid image : " << elf_path.c_str();
        throw mic_exception(E_MIC_INVAL, ss.str(), EINVAL);
    } else {
        // Validate if the file is empty or not
        pos = fseek(fptr, 0, SEEK_END);
        pos = ftell(fptr);
        fclose(fptr);

        if (pos == 0 || pos == -1) {
            std::stringstream ss;
            ss << "Empty image : " << elf_path.c_str();
            throw mic_exception(E_MIC_INVAL, ss.str(), EINVAL);
        }
    }

    if (mode == MIC_MODE_ELF) {
        std::string cmd = "boot:elf:";

        set_device_property(host_platform::STATE, cmd + elf_path);
    } else {
        std::stringstream ss;

        ss << "Mode request: '0x" << std::hex << mode << "'";
        throw mic_exception(E_MIC_INVAL, ss.str(), EINVAL);
    }
}
