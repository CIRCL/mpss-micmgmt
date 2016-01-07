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

#include <new>
#include <string>
#include <stdlib.h>
#include "mic_device.h"
#include "knc_device.h"
#include "host_platform.h"
#include "miclib_exception.h"
#include "miclib_int.h"
#include "miclib.h"
#include <stdlib.h>

const static int N_DEVICES = 8;

const char *mic_get_error_string()
{
    return mic_exception::ts_get_error_msg();
}

int mic_clear_error_string()
{
    try {
        mic_exception::ts_clear_error_msg();
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_get_ras_errno()
{
    return mic_exception::ts_get_ras_errno();
}

const char *mic_get_ras_error_string(int ras_errno)
{
    return mic_exception::ras_strerror(ras_errno);
}

int mic_get_devices(struct mic_devices_list **d)
{
    ASSERT(d != NULL);
    *d = NULL;
    try {
        int n;

        *d = (struct mic_devices_list *)
             malloc(sizeof(struct mic_devices_list) +
                    (N_DEVICES - 1) * sizeof((*d)->devices[0]));
        if (*d == NULL)
            throw mic_exception(E_MIC_NOMEM, "malloc", ENOMEM);

        host_platform::get_devices(*d, N_DEVICES);
        if ((n = (*d)->n_devices) > N_DEVICES) {
            free((void *)*d);
            *d = (struct mic_devices_list *)
                 malloc(sizeof(struct mic_devices_list) +
                        (n - 1) * sizeof((*d)->devices[0]));
            if (*d == NULL) {
                throw mic_exception(E_MIC_NOMEM,
                                    "malloc", ENOMEM);
            }
            host_platform::get_devices(*d, n);

            ASSERT((*d)->n_devices == n);

            if ((*d)->n_devices != n) {
                free((void *)*d);
                throw mic_exception(E_MIC_INTERNAL,
                                    "Internal error", EINVAL);
            }
        }
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        if (*d != NULL)
            free((void *)*d);
        return e.get_mic_errno();
    }
    catch (...) {
        if (*d != NULL)
            free((void *)*d);
        ASSERT(0);
        return E_MIC_INTERNAL;
    }
}

int mic_free_devices(struct mic_devices_list *d)
{
    ASSERT(d != NULL);
    free((void *)d);
    return E_MIC_SUCCESS;
}

int mic_get_ndevices(struct mic_devices_list *d, int *ndevices)
{
    ASSERT((d != NULL) && (ndevices != NULL));
    *ndevices = d->n_devices;
    return E_MIC_SUCCESS;
}

int mic_get_device_at_index(struct mic_devices_list *d, int index, int *device)
{
    ASSERT((d != NULL) && (device != NULL));
    try {
        /*
         * This function is used to unit test the error handling at
         * Tests/miclib/miclib_test.h, if you need to change anything
         * in the if below, be sure to do the same at that location.
         *
         * The string below doesn't provide any extra information -
         * we should remove it.
         */
        if (index < 0 || index >= d->n_devices) {
            throw mic_exception(E_MIC_RANGE,
                                "Incorrect device number", ERANGE);
        }

        *device = d->devices[index];
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

const char *mic_get_device_name(struct mic_device *mdh)
{
    ASSERT(mdh != NULL);
    try {
			return mdh->get_device_name();
		} catch (mic_exception const &e) {
			e.get_mic_errno();
			return (const char *)NULL;
		}
		catch (...) {
			return (const char *)NULL;
		}
}

int mic_open_device(struct mic_device **device, uint32_t device_num)
{
    ASSERT(device != NULL);

    try {
        *device = NULL;
        uint32_t device_type;
        host_platform hp(device_num, false);         // we don't need scif conn
        hp.get_device_type(device_type);

        if (device_type == KNC_ID) {
            *device = new knc_device(device_num);
        } else {
            /* Only KNC is supported for now */
            std::stringstream ss;

            ss << "Unsupported device id: '0x" << std::hex <<
                device_type << "'";

            throw mic_exception(E_MIC_UNSUPPORTED_DEV,
                                ss.str(), ECANCELED);
        }

        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        if (*device != NULL)
            delete *device;

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*device != NULL)
            delete *device;

        return E_MIC_NOMEM;
    } catch (...) {
        if (*device != NULL)
            delete *device;

        return E_MIC_INTERNAL;
    }
}

int mic_close_device(struct mic_device *device)
{
    ASSERT(device != NULL);
    delete device;
    return E_MIC_SUCCESS;
}

int mic_get_device_type(struct mic_device *device, uint32_t *device_type)
{
    ASSERT((device != NULL) && (device_type != NULL));
    try {
        device->get_device_type(*device_type);
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_flash_update_start(struct mic_device *mdh, void *buffer, size_t bufsize,
                           struct mic_flash_op **desc)
{
    ASSERT((mdh != NULL) && (buffer != NULL) && (desc != NULL));

    try {
        struct host_flash_op *hdesc;
        *desc = NULL;
        *desc = new (struct mic_flash_op);
        hdesc = mdh->flash_update_start(buffer, bufsize);
        (*desc)->mdh = mdh;
        (*desc)->h_desc = hdesc;

        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (*desc != NULL)
            delete(*desc);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*desc != NULL)
            delete(*desc);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*desc != NULL)
            delete(*desc);

        return E_MIC_INTERNAL;
    }
}

int mic_flash_update_done(struct mic_flash_op *desc)
{
    ASSERT(desc != NULL);
    int remove_desc = true;

    try {
        desc->mdh->flash_update_done(desc->h_desc);
        remove_desc = false;
        delete(desc);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (remove_desc)
            delete(desc);
        return e.get_mic_errno();
    } catch (...) {
        if (remove_desc)
            delete(desc);
        return E_MIC_INTERNAL;
    }
}

int mic_flash_read_start(struct mic_device *mdh, void *buffer,
                         size_t bufsize, struct mic_flash_op **desc)
{
    ASSERT((mdh != NULL) && (buffer != NULL) && (desc != NULL));
    struct host_flash_op *hdesc;

    try {
        *desc = NULL;
        *desc = new (struct mic_flash_op);
        hdesc = mdh->flash_read_start(buffer, bufsize);
        (*desc)->mdh = mdh;
        (*desc)->h_desc = hdesc;
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        if (*desc != NULL)
            delete(*desc);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*desc != NULL)
            delete(*desc);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*desc != NULL)
            delete(*desc);

        return E_MIC_INTERNAL;
    }
}

int mic_flash_read_done(struct mic_flash_op *desc)
{
    int remove_desc = true;

    ASSERT(desc != NULL);
    try {
        desc->mdh->flash_read_done(desc->h_desc);
        remove_desc = false;
        delete(desc);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (remove_desc)
            delete(desc);
        return e.get_mic_errno();
    } catch (...) {
        if (remove_desc)
            delete(desc);
        return E_MIC_INTERNAL;
    }
}

int mic_set_ecc_mode_start(struct mic_device *mdh, uint16_t ecc_enabled,
                           struct mic_flash_op **desc)
{
    ASSERT((mdh != NULL) && (desc != NULL));

    try {
        struct host_flash_op *hdesc = NULL;
        *desc = new struct mic_flash_op;
        hdesc = mdh->set_ecc_mode_start(ecc_enabled);
        (*desc)->mdh = mdh;
        (*desc)->h_desc = hdesc;

        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (*desc != NULL)
            delete *desc;

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*desc != NULL)
            delete *desc;

        return E_MIC_NOMEM;
    } catch (...) {
        if (*desc != NULL)
            delete *desc;

        return E_MIC_INTERNAL;
    }
}

int mic_set_ecc_mode_done(struct mic_flash_op *desc)
{
    ASSERT(desc != NULL);
    int remove_desc = true;

    try {
        desc->mdh->set_ecc_mode_done(desc->h_desc);
        remove_desc = false;
        delete desc;
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (remove_desc)
            delete desc;
        return e.get_mic_errno();
    } catch (...) {
        if (remove_desc)
            delete desc;
        return E_MIC_INTERNAL;
    }
}

int mic_flash_size(struct mic_device *mdh, size_t *size)
{
    ASSERT((mdh != NULL) && (size != NULL));
    try {
        mdh->flash_size(*size);
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_flash_active_offs(struct mic_device *mdh, off_t *active)
{
    ASSERT(active != NULL);
    try {
        mdh->flash_active_offs(*active);
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_flash_version(struct mic_device *mdh, void *buf,
                      char *str, size_t strsize)
{
    off_t offs;

    ASSERT((mdh != NULL) && (buf != NULL));

    try {
        mdh->flash_version_offs(offs);
        if ((str != NULL) && (strsize > 0)) {
            strncpy(str, ((const char *)buf) + offs, strsize);
            str[strsize - 1] = '\0';
        }
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_get_flash_status_info(struct mic_flash_op *desc,
                              struct mic_flash_status_info **status_desc)
{
    ASSERT((desc != NULL) && (status_desc != NULL));

    try {
        *status_desc = NULL;
        *status_desc = new mic_flash_status_info;
        desc->mdh->flash_get_status(desc->h_desc, *status_desc);
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        if (*status_desc != NULL)
            delete(*status_desc);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*status_desc != NULL)
            delete(*status_desc);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*status_desc != NULL)
            delete(*status_desc);

        return E_MIC_INTERNAL;
    }
}

int mic_free_flash_status_info(struct mic_flash_status_info *status)
{
    ASSERT(status != NULL);
    delete(status);
    return E_MIC_SUCCESS;
}

int mic_get_progress(struct mic_flash_status_info *status, uint32_t *progress)
{
    ASSERT((status != NULL) && (progress != NULL));

    *progress = status->complete;
    return E_MIC_SUCCESS;
}

int mic_get_status(struct mic_flash_status_info *status, int *fstatus)
{
    ASSERT((status != NULL) && (fstatus != NULL));

    *fstatus = status->flash_status;
    return E_MIC_SUCCESS;
}

int mic_get_ext_status(struct mic_flash_status_info *status, int *ext_status)
{
    ASSERT((status != NULL) && (ext_status != NULL));

    *ext_status = status->ext_status;
    return E_MIC_SUCCESS;
}

int mic_get_flash_vendor_device(struct mic_device *device, char *buf,
                                size_t *size)
{
    ASSERT((device != NULL) && (size != NULL));

    try {
        std::string vendor = device->get_flash_vendor_device();

        if ((*size) > 0 && buf != NULL) {
            strncpy(buf, vendor.c_str(), *size);
            buf[(*size) - 1] = '\0';
        }

        if (*size < vendor.length() + 1)
            *size = vendor.length() + 1;

        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_enter_maint_mode(struct mic_device *mdh)
{
    assert(mdh != NULL && "mic_device struct ptr null");

    try {
        mdh->set_mode(MIC_MODE_MAINT);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_in_maint_mode(struct mic_device *mdh, int *is_maint)
{
    assert(mdh != NULL && "mic_device struct ptr null");
    assert(is_maint != NULL && "is_maint ptr null");

    try {
        mdh->in_maint_mode(is_maint);

        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_leave_maint_mode(struct mic_device *mdh)
{
    ASSERT(mdh != NULL);
    try {
        mdh->set_mode(MIC_MODE_READY);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_in_ready_state(struct mic_device *mdh, int *is_ready)
{
    ASSERT((mdh != NULL) && (is_ready != NULL));
    try {
        mdh->in_ready_state(is_ready);
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_get_post_code(struct mic_device *mdh, char *postcode, size_t *size)
{
    ASSERT((mdh != NULL) && (size != NULL));
    try {
        std::string poststr;

        mdh->get_post_code(poststr);

        if (*size > 0 && postcode != NULL)
            strncpy(postcode, poststr.c_str(), *size);

        if (*size < poststr.length() + 1)
            *size = poststr.length() + 1;

        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_get_pci_config(struct mic_device *mdh, struct mic_pci_config **conf)
{
    assert(mdh != NULL && "mic_device struct ptr null");
    assert(conf != NULL && "mic_pci_config struct ptr to ptr null");

    try {
        *conf = NULL;
        *conf = new mic_pci_config();
        mdh->get_pci_config(**conf);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (*conf != NULL)
            mic_free_pci_config(*conf);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*conf != NULL)
            mic_free_pci_config(*conf);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*conf != NULL)
            mic_free_pci_config(*conf);

        return E_MIC_INTERNAL;
    }
}

int mic_get_pci_domain_id(struct mic_pci_config *conf,
                          uint16_t *domain)
{
    ASSERT((conf != NULL) && (domain != NULL));
    if (conf->domain_info_implemented==1)
    {
        *domain = conf->domain;
        return E_MIC_SUCCESS;
    }
    else
    {
        return E_MIC_NOT_IMPLEMENTED;
    }
}

int mic_get_vendor_id(struct mic_pci_config *conf, uint16_t *id)
{
    ASSERT((conf != NULL) && (id != NULL));
    *id = conf->vendor_id;
    return E_MIC_SUCCESS;
}

int mic_get_device_id(struct mic_pci_config *conf, uint16_t *id)
{
    ASSERT((conf != NULL) && (id != NULL));
    *id = conf->device_id;
    return E_MIC_SUCCESS;
}

int mic_get_revision_id(struct mic_pci_config *conf, uint8_t *id)
{
    ASSERT((conf != NULL) && (id != NULL));
    *id = conf->revision_id;
    return E_MIC_SUCCESS;
}

int mic_get_subsystem_id(struct mic_pci_config *conf, uint16_t *id)
{
    ASSERT((conf != NULL) && (id != NULL));
    *id = conf->subsystem_id;
    return E_MIC_SUCCESS;
}

int mic_get_bus_number(struct mic_pci_config *conf, uint16_t *bus_no)
{
    ASSERT((conf != NULL) && (bus_no != NULL));
    *bus_no = conf->bus_no;
    return E_MIC_SUCCESS;
}

int mic_get_device_number(struct mic_pci_config *conf, uint16_t *device_no)
{
    ASSERT((conf != NULL) && (device_no != NULL));
    *device_no = conf->device_no;
    return E_MIC_SUCCESS;
}

int mic_get_link_speed(struct mic_pci_config *conf, char *speed, size_t *size)
{
    assert(conf != NULL && "mic_pci_config struct ptr null");
    assert(size != NULL && "size ptr null");

    std::string speedstr;

    if (conf->access_violation)
        return E_MIC_ACCESS;

    switch (conf->link_speed) {
    case 1:
        speedstr = "2.5 GT/s";
        break;
    case 2:
        speedstr = "5 GT/s";
        break;
    default:
        speedstr = "Unknown";
    }
    if ((speed != NULL) && (*size > 0)) {
        strncpy(speed, speedstr.c_str(), *size);
        speed[*size - 1] = '\0';
    }
    if (*size < speedstr.length() + 1)
        *size = speedstr.length() + 1;
    return E_MIC_SUCCESS;
}

int mic_get_link_width(struct mic_pci_config *conf, uint32_t *width)
{
    ASSERT((conf != NULL) && (width != NULL));
    if (conf->access_violation)
        return E_MIC_ACCESS;
    *width = conf->link_width;
    return E_MIC_SUCCESS;
}

int mic_get_max_readreq(struct mic_pci_config *conf, uint32_t *readreq)
{
    ASSERT((conf != NULL) && (readreq != NULL));
    if (conf->access_violation)
        return E_MIC_ACCESS;
    *readreq = conf->read_req_size;
    return E_MIC_SUCCESS;
}

int mic_get_max_payload(struct mic_pci_config *conf, uint32_t *payload)
{
    ASSERT((conf != NULL) && (payload != NULL));
    if (conf->access_violation)
        return E_MIC_ACCESS;
    *payload = conf->payload_size;
    return E_MIC_SUCCESS;
}

int mic_get_pci_class_code(struct mic_pci_config *conf,
                           char *class_code, size_t *size)
{
    assert(conf != NULL && "mic_pci_config struct ptr null");
    assert(size != NULL && "size ptr null");
    size_t length = strlen(conf->class_code) + 1;

    if ((class_code != NULL) && (*size > 0)) {
        strncpy(class_code, conf->class_code, *size);
        class_code[*size - 1] = '\0';
    }

    if (*size < length)
        *size = length;

    return E_MIC_SUCCESS;
}

int mic_free_pci_config(struct mic_pci_config *conf)
{
    ASSERT(conf != NULL);
    delete(conf);
    return E_MIC_SUCCESS;
}

int mic_get_memory_info(struct mic_device *mdh, struct mic_device_mem **mem)
{
    assert(mdh != NULL && "mic_device struct ptr null");
    assert(mem != NULL && "mic_device_mem struct ptr to ptr null");

    try {
        *mem = NULL;
        *mem = new struct mic_device_mem ();
        mdh->get_memory_info(*mem);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (*mem != NULL)
            mic_free_memory_info(*mem);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*mem != NULL)
            mic_free_memory_info(*mem);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*mem != NULL)
            mic_free_memory_info(*mem);

        return E_MIC_INTERNAL;
    }
}

int mic_get_memory_vendor(struct mic_device_mem *mem,
                          char *vendor, size_t *bufsize)
{
    ASSERT((mem != NULL) && (bufsize != NULL));
    if ((vendor != NULL) && (*bufsize > 0)) {
        strncpy(vendor, mem->vendor_name, *bufsize);
        vendor[*bufsize - 1] = '\0';
    }
    if (*bufsize < strlen(mem->vendor_name) + 1)
        *bufsize = strlen(mem->vendor_name) + 1;
    return E_MIC_SUCCESS;
}

int mic_get_memory_revision(struct mic_device_mem *mem, uint32_t *revision)
{
    ASSERT((mem != NULL) && (revision != NULL));
    *revision = mem->revision;
    return E_MIC_SUCCESS;
}

int mic_get_memory_density(struct mic_device_mem *mem, uint32_t *density)
{
    ASSERT((mem != NULL) && (density != NULL));
    *density = mem->density;
    return E_MIC_SUCCESS;
}

int mic_get_memory_size(struct mic_device_mem *mem, uint32_t *size)
{
    ASSERT((mem != NULL) && (size != NULL));
    *size = mem->size;
    return E_MIC_SUCCESS;
}

int mic_get_memory_speed(struct mic_device_mem *mem, uint32_t *speed)
{
    ASSERT((mem != NULL) && (speed != NULL));
    *speed = mem->speed;
    return E_MIC_SUCCESS;
}

int mic_get_memory_type(struct mic_device_mem *mem, char *type,
                        size_t *bufsize)
{
    ASSERT((mem != NULL) && (bufsize != NULL));

    if ((type != NULL) && (*bufsize > 0)) {
        strncpy(type, mem->memory_type, *bufsize);
        type[(*bufsize - 1)] = '\0';
    }
    if (*bufsize < (strlen(mem->memory_type) + 1))
        *bufsize = strlen(mem->memory_type) + 1;
    return E_MIC_SUCCESS;
}

int mic_get_memory_frequency(struct mic_device_mem *mem, uint32_t *frequency)
{
    ASSERT((mem != NULL) && (frequency != NULL));
    *frequency = mem->freq;
    return E_MIC_SUCCESS;
}

int mic_get_memory_voltage(struct mic_device_mem *mem, uint32_t *voltage)
{
    ASSERT((mem != NULL) && (voltage != NULL));
    *voltage = mem->volt;
    return E_MIC_SUCCESS;
}

int mic_get_ecc_mode(struct mic_device_mem *mem, uint16_t *ecc)
{
    ASSERT((mem != NULL) && (ecc != NULL));
    *ecc = mem->ecc;
    return E_MIC_SUCCESS;
}

int mic_free_memory_info(struct mic_device_mem *mem)
{
    ASSERT(mem != NULL);
    delete(mem);
    return E_MIC_SUCCESS;
}

int mic_get_processor_info(struct mic_device *mdh,
                           struct mic_processor_info **processor)
{
    ASSERT((mdh != NULL) && (processor != NULL));

    try {
        *processor = NULL;
        *processor = new struct mic_processor_info ();
        mdh->get_processor_info(*processor);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (*processor != NULL)
            mic_free_processor_info(*processor);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*processor != NULL)
            mic_free_processor_info(*processor);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*processor != NULL)
            mic_free_processor_info(*processor);

        return E_MIC_INTERNAL;
    }
}

int mic_get_processor_model(struct mic_processor_info *processor,
                            uint16_t *model, uint16_t *model_ext)
{
    ASSERT((processor != NULL) && (model != NULL) && (model_ext != NULL));
    *model = processor->model;
    *model_ext = processor->model_ext;
    return E_MIC_SUCCESS;
}

int mic_get_processor_family(struct mic_processor_info *processor,
                             uint16_t *family, uint16_t *family_ext)
{
    ASSERT((processor != NULL) && (family != NULL) && (family_ext != NULL));
    *family = processor->family;
    *family_ext = processor->family_ext;
    return E_MIC_SUCCESS;
}

int mic_get_processor_type(struct mic_processor_info *processor,
                           uint16_t *type)
{
    ASSERT((processor != NULL) && (type != NULL));
    *type = processor->type;
    return E_MIC_SUCCESS;
}

int mic_get_processor_steppingid(struct mic_processor_info *processor,
                                 uint32_t *id)
{
    ASSERT((processor != NULL) && (id != NULL));
    *id = processor->stepping_data << 4;
    *id |= processor->substepping_data;
    return E_MIC_SUCCESS;
}

int mic_get_processor_stepping(struct mic_processor_info *processor,
                               char *stepping, size_t *size)
{
    ASSERT((processor != NULL) && (size != NULL));
    if ((stepping != NULL) && (*size > 0)) {
		strncpy(stepping, processor->stepping, *size);
		stepping[*size-1] = '\0';
	}

    if (*size < strlen(processor->stepping) + 1)
        *size = strlen(processor->stepping) + 1;

	return E_MIC_SUCCESS;
}

int mic_free_processor_info(struct mic_processor_info *processor)
{
    ASSERT(processor != NULL);
    delete(processor);
    return E_MIC_SUCCESS;
}

int mic_get_cores_info(struct mic_device *mdh, struct mic_cores_info **cores)
{
    ASSERT((mdh != NULL) && (cores != NULL));

    try {
        *cores = NULL;
        *cores = new struct mic_cores_info ();
        mdh->get_cores_info(*cores);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (*cores != NULL)
            mic_free_cores_info(*cores);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*cores != NULL)
            mic_free_cores_info(*cores);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*cores != NULL)
            mic_free_cores_info(*cores);

        return E_MIC_INTERNAL;
    }
}

int mic_get_cores_count(struct mic_cores_info *cores, uint32_t *num_cores)
{
    ASSERT((cores != NULL) && (num_cores != NULL));
    *num_cores = cores->num_cores;
    return E_MIC_SUCCESS;
}

int mic_get_cores_voltage(struct mic_cores_info *cores, uint32_t *voltage)
{
    ASSERT((cores != NULL) && (voltage != NULL));
    *voltage = cores->voltage;
    return E_MIC_SUCCESS;
}

int mic_get_cores_frequency(struct mic_cores_info *cores, uint32_t *frequency)
{
    ASSERT((cores != NULL) && (frequency != NULL));
    *frequency = cores->frequency;
    return E_MIC_SUCCESS;
}

int mic_free_cores_info(struct mic_cores_info *cores)
{
    ASSERT(cores != NULL);
    delete(cores);
    return E_MIC_SUCCESS;
}

/* thermal info */
int mic_get_thermal_info(struct mic_device *mdh,
                         struct mic_thermal_info **thermal)
{
    ASSERT((mdh != NULL) && (thermal != NULL));

    try {
        *thermal = NULL;
        *thermal = new struct mic_thermal_info;
        mdh->get_thermal_info(*thermal);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (*thermal != NULL)
            mic_free_thermal_info(*thermal);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*thermal != NULL)
            mic_free_thermal_info(*thermal);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*thermal != NULL)
            mic_free_thermal_info(*thermal);

        return E_MIC_INTERNAL;
    }
}

int mic_get_fsc_status(struct mic_thermal_info *thermal, uint32_t *fsc_status)
{
    ASSERT((thermal != NULL) && (fsc_status != NULL));
    *fsc_status = thermal->fsc_status;
    return E_MIC_SUCCESS;
}

int mic_get_fan_rpm(struct mic_thermal_info *thermal, uint32_t *fan_rpm)
{
    ASSERT((thermal != NULL) && (fan_rpm != NULL));
    *fan_rpm = thermal->fan_rpm;
    return E_MIC_SUCCESS;
}

int mic_get_fan_pwm(struct mic_thermal_info *thermal, uint32_t *fan_pwm)
{
    ASSERT((thermal != NULL) && (fan_pwm != NULL));
    *fan_pwm = thermal->fan_pwm;
    return E_MIC_SUCCESS;
}

int mic_get_die_temp(struct mic_thermal_info *thermal, uint32_t *temp)
{
    ASSERT((thermal != NULL) && (temp != NULL));
    *temp = thermal->temp.die.cur;
    return E_MIC_SUCCESS;
}

int mic_is_die_temp_valid(struct mic_thermal_info *thermal, int *valid)
{
    ASSERT((thermal != NULL) && (valid != NULL));
    *valid = (thermal->temp.die.c_val &
              host_platform::SMC_SENSOR_UNAVAILABLE) !=
             host_platform::SMC_SENSOR_UNAVAILABLE;
    return E_MIC_SUCCESS;
}

int mic_get_gddr_temp(struct mic_thermal_info *thermal, uint16_t *temp)
{
    ASSERT((thermal != NULL) && (temp != NULL));
    *temp = thermal->temp.gddr.cur;
    return E_MIC_SUCCESS;
}

int mic_is_gddr_temp_valid(struct mic_thermal_info *thermal, int *valid)
{
    ASSERT((thermal != NULL) && (valid != NULL));
    *valid = (thermal->temp.gddr.c_val &
              host_platform::SMC_SENSOR_UNAVAILABLE) !=
             host_platform::SMC_SENSOR_UNAVAILABLE;
    return E_MIC_SUCCESS;
}

int mic_get_fanin_temp(struct mic_thermal_info *thermal, uint16_t *temp)
{
    ASSERT((thermal != NULL) && (temp != NULL));
    *temp = thermal->temp.fin.cur;
    return E_MIC_SUCCESS;
}

int mic_is_fanin_temp_valid(struct mic_thermal_info *thermal, int *valid)
{
    ASSERT((thermal != NULL) && (valid != NULL));
    *valid = (thermal->temp.fin.c_val &
              host_platform::SMC_SENSOR_UNAVAILABLE) !=
             host_platform::SMC_SENSOR_UNAVAILABLE;
    return E_MIC_SUCCESS;
}

int mic_get_fanout_temp(struct mic_thermal_info *thermal, uint16_t *temp)
{
    ASSERT((thermal != NULL) && (temp != NULL));
    *temp = thermal->temp.fout.cur;
    return E_MIC_SUCCESS;
}

int mic_is_fanout_temp_valid(struct mic_thermal_info *thermal, int *valid)
{
    ASSERT((thermal != NULL) && (valid != NULL));
    *valid = (thermal->temp.fout.c_val &
              host_platform::SMC_SENSOR_UNAVAILABLE) !=
             host_platform::SMC_SENSOR_UNAVAILABLE;
    return E_MIC_SUCCESS;
}

int mic_get_vccp_temp(struct mic_thermal_info *thermal, uint16_t *temp)
{
    ASSERT((thermal != NULL) && (temp != NULL));
    *temp = thermal->temp.vccp.cur;
    return E_MIC_SUCCESS;
}

int mic_is_vccp_temp_valid(struct mic_thermal_info *thermal, int *valid)
{
    ASSERT((thermal != NULL) && (valid != NULL));
    *valid = (thermal->temp.vccp.c_val &
              host_platform::SMC_SENSOR_UNAVAILABLE) !=
             host_platform::SMC_SENSOR_UNAVAILABLE;
    return E_MIC_SUCCESS;
}

int mic_get_vddg_temp(struct mic_thermal_info *thermal, uint16_t *temp)
{
    ASSERT((thermal != NULL) && (temp != NULL));
    *temp = thermal->temp.vddg.cur;
    return E_MIC_SUCCESS;
}

int mic_is_vddg_temp_valid(struct mic_thermal_info *thermal, int *valid)
{
    ASSERT((thermal != NULL) && (valid != NULL));
    *valid = (thermal->temp.vddg.c_val &
              host_platform::SMC_SENSOR_UNAVAILABLE) !=
             host_platform::SMC_SENSOR_UNAVAILABLE;
    return E_MIC_SUCCESS;
}

int mic_get_vddq_temp(struct mic_thermal_info *thermal, uint16_t *temp)
{
    ASSERT((thermal != NULL) && (temp != NULL));
    *temp = thermal->temp.vddq.cur;
    return E_MIC_SUCCESS;
}

int mic_is_vddq_temp_valid(struct mic_thermal_info *thermal, int *valid)
{
    ASSERT((thermal != NULL) && (valid != NULL));
    *valid = (thermal->temp.vddq.c_val &
              host_platform::SMC_SENSOR_UNAVAILABLE) !=
             host_platform::SMC_SENSOR_UNAVAILABLE;
    return E_MIC_SUCCESS;
}

int mic_get_smc_fwversion(struct mic_thermal_info *thermal,
                          char *fwversion, size_t *size)
{
    std::stringstream sstr;
    std::string str;

    ASSERT((thermal != NULL) && (size != NULL));
    sstr << std::dec << thermal->smc_version.bits.major << "." <<
        std::dec << thermal->smc_version.bits.minor << "." <<
        thermal->smc_version.bits.buildno;
    str = sstr.str();
    if ((fwversion != NULL) && (*size > 0)) {
        strncpy(fwversion, str.c_str(), *size);
        fwversion[(*size - 1)] = '\0';
    }
    if (*size < str.length() + 1)
        *size = str.length() + 1;
    return E_MIC_SUCCESS;
}

int mic_get_smc_hwrevision(struct mic_thermal_info *thermal,
                           char *hwrevision, size_t *size)
{
    union smc_hw_rev rev;
    std::stringstream sstr;

    ASSERT((thermal != NULL) && (size != NULL));
    rev.value = thermal->smc_revision.value;
    switch (rev.bits.board_type) {
    case 0:
        sstr << "MPI";
        break;
    case 1:
        sstr << "CRB";
        break;
    case 2:
        sstr << "DFF";
        break;
    case 3:
        sstr << "Product";
        break;
    default:
        sstr << "unknown";
        break;
    }
    sstr << " ";
    switch (rev.bits.power) {
    case 0:
        sstr << "300W";
        break;
    case 1:
        switch (rev.bits.board_type) {
        case 0:
        case 1:
        case 3:
            sstr << "225W";
            break;
        case 2:
            sstr << "245W";
            break;
        }
        break;
    }
    sstr << " ";
    switch (rev.bits.hsink_type) {
    case 0:
        sstr << "Active";
        break;
    case 1:
        sstr << "Passive";
        break;
    }
    sstr << " ";
    switch (rev.bits.mem_cfg) {
    case 0:
        sstr << "NCS";
        break;
    case 1:
        sstr << "CS";
        break;
    }

    if ((hwrevision != NULL) && (*size > 0)) {
        strncpy(hwrevision, sstr.str().c_str(), *size);
        hwrevision[*size - 1] = '\0';
    }
    if (*size < sstr.str().length() + 1)
        *size = sstr.str().length() + 1;
    return E_MIC_SUCCESS;
}

int mic_is_smc_boot_loader_ver_supported(struct mic_thermal_info *thermal,
                                         int *supported)
{
    ASSERT((thermal != NULL) && (supported != NULL));
    *supported = thermal->smc_boot_loader_ver_supported;

    return E_MIC_SUCCESS;
}

int mic_get_smc_boot_loader_ver(struct mic_thermal_info *thermal,
                                char *ver,
                                size_t *size)
{
    std::stringstream sstr;
    std::string str;

    ASSERT((thermal != NULL) && (size != NULL));
    sstr << std::dec << thermal->smc_boot_loader.bits.major << "." <<
        std::dec << thermal->smc_boot_loader.bits.minor << "." <<
        thermal->smc_boot_loader.bits.buildno;
    str = sstr.str();

    if ((ver != NULL) && (*size > 0)) {
        strncpy(ver, str.c_str(), *size);
        ver[(*size - 1)] = '\0';
    }
    if (*size < str.length() + 1)
        *size = str.length() + 1;

    return E_MIC_SUCCESS;
}

int mic_free_thermal_info(struct mic_thermal_info *thermal)
{
    ASSERT(thermal != NULL);
    delete(thermal);
    return E_MIC_SUCCESS;
}

/* version info */
int mic_get_version_info(struct mic_device *mdh,
                         struct mic_version_info **version)
{
    ASSERT((mdh != NULL) && (version != NULL));

    try {
        *version = NULL;
        *version = new struct mic_version_info ();
        mdh->get_version_info(*version);
    } catch (mic_exception const &e) {
        if (*version != NULL)
            mic_free_version_info(*version);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*version != NULL)
            mic_free_version_info(*version);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*version != NULL)
            mic_free_version_info(*version);

        return E_MIC_INTERNAL;
    }

    return E_MIC_SUCCESS;
}

int mic_get_uos_version(struct mic_version_info *version,
                        char *uos, size_t *size)
{
    std::string ver_str;
    std::string uos_str;
    size_t start = 0;
    size_t end = 0;

    ASSERT((version != NULL) && (size != NULL));
    try {
        ver_str = version->vers.uos;
        start = ver_str.find(": ");
        start += 2;
        end = ver_str.find(" (build");
        if (start && (end > start)) {
            uos_str = ver_str.substr(start, (end - start));
            if ((uos != NULL) && (*size > 0)) {
                strncpy(uos, uos_str.c_str(), *size);
                uos[*size - 1] = '\0';
            }
            if (*size < (uos_str.length() + 1))
                *size = uos_str.length() + 1;
        }
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
    return E_MIC_SUCCESS;
}

int mic_get_flash_version(struct mic_version_info *version,
                          char *flash, size_t *size)
{
    std::string ver_str;
    std::string flash_str;
    int start = 0;
    int end = 0;

    ASSERT((version != NULL) && (size != NULL));
    try {
        ver_str = version->vers.fboot1;
        start = ver_str.find(": ");
        start += 2;
        end = ver_str.find(" (build");
        if (start && (end > start)) {
            flash_str = ver_str.substr(start, (end - start));
            if ((flash != NULL) && (*size > 0)) {
                strncpy(flash, flash_str.c_str(), *size);
                flash[*size - 1] = '\0';
            }
            if (*size < (flash_str.length() + 1))
                *size = flash_str.length() + 1;
        }
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
    return E_MIC_SUCCESS;
}

int mic_get_fsc_strap(struct mic_version_info *version,
                      char *strap, size_t *size)
{
    ASSERT((version != NULL) && (size != NULL));
    const std::string ver_str = "14 MHz";
    try {
        if ((strap != NULL) && (*size > 0)) {
            strncpy(strap, ver_str.c_str(), *size);
            strap[*size - 1] = '\0';
        }
        if (*size < (ver_str.length() + 1))
            *size = ver_str.length() + 1;
    }
    catch (mic_exception const &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
    return E_MIC_SUCCESS;
}

int mic_free_version_info(struct mic_version_info *version)
{
    ASSERT(version != NULL);
    delete(version);
    return E_MIC_SUCCESS;
}

int mic_get_silicon_sku(struct mic_device *mdh, char *sku, size_t *size)
{
    ASSERT((mdh != NULL) && (size != NULL));

    try {
        mdh->get_silicon_sku(sku, size);
    } catch (mic_exception const &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }

    return E_MIC_SUCCESS;
}

int mic_get_serial_number(struct mic_device *mdh, char *serial, size_t *size)
{
    ASSERT((mdh != NULL) && (size != NULL));

    try {
        mdh->get_serial_number(serial, size);
    } catch (mic_exception const &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }

    return E_MIC_SUCCESS;
}

/* power utilization info */
int mic_get_power_utilization_info(struct mic_device *mdh,
                                   struct mic_power_util_info **power)
{
    ASSERT((mdh != NULL) && (power != NULL));
    try {
        *power = NULL;
        *power = new struct mic_power_util_info ();
        mdh->get_power_utilization_info(*power);
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        if (*power != NULL)
            mic_free_power_utilization_info(*power);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*power != NULL)
            mic_free_power_utilization_info(*power);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*power != NULL)
            mic_free_power_utilization_info(*power);

        return E_MIC_INTERNAL;
    }
}

int mic_get_total_power_readings_w0(struct mic_power_util_info *power,
                                    uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.tot0.prr;
    return E_MIC_SUCCESS;
}

int mic_get_total_power_sensor_sts_w0(struct mic_power_util_info *power,
                                      uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.tot0.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_total_power_readings_w1(struct mic_power_util_info *power,
                                    uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.tot1.prr;
    return E_MIC_SUCCESS;
}

int mic_get_total_power_sensor_sts_w1(struct mic_power_util_info *power,
                                      uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.tot1.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_inst_power_readings(struct mic_power_util_info *power,
                                uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.inst.prr;
    return E_MIC_SUCCESS;
}

int mic_get_inst_power_sensor_sts(struct mic_power_util_info *power,
                                  uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.inst.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_max_inst_power_readings(struct mic_power_util_info *power,
                                    uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.imax.prr;
    return E_MIC_SUCCESS;
}

int mic_get_max_inst_power_sensor_sts(struct mic_power_util_info *power,
                                      uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.imax.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_pcie_power_readings(struct mic_power_util_info *power,
                                uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.pcie.prr;
    return E_MIC_SUCCESS;
}

int mic_get_pcie_power_sensor_sts(struct mic_power_util_info *power,
                                  uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.pcie.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_c2x3_power_readings(struct mic_power_util_info *power,
                                uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.c2x3.prr;
    return E_MIC_SUCCESS;
}

int mic_get_c2x3_power_sensor_sts(struct mic_power_util_info *power,
                                  uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.c2x3.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_c2x4_power_readings(struct mic_power_util_info *power,
                                uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.c2x4.prr;
    return E_MIC_SUCCESS;
}

int mic_get_c2x4_power_sensor_sts(struct mic_power_util_info *power,
                                  uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.c2x4.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_vccp_power_readings(struct mic_power_util_info *power,
                                uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.vccp.pwr;
    return E_MIC_SUCCESS;
}

int mic_get_vccp_power_sensor_sts(struct mic_power_util_info *power,
                                  uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.vccp.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_vccp_current_readings(struct mic_power_util_info *power,
                                  uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.vccp.cur;
    return E_MIC_SUCCESS;
}

int mic_get_vccp_current_sensor_sts(struct mic_power_util_info *power,
                                    uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.vccp.c_val;
    return E_MIC_SUCCESS;
}

int mic_get_vccp_voltage_readings(struct mic_power_util_info *power,
                                  uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.vccp.volt;
    return E_MIC_SUCCESS;
}

int mic_get_vccp_voltage_sensor_sts(struct mic_power_util_info *power,
                                    uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.vccp.v_val;
    return E_MIC_SUCCESS;
}

int mic_get_vddg_power_readings(struct mic_power_util_info *power,
                                uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.vddg.pwr;
    return E_MIC_SUCCESS;
}

int mic_get_vddg_power_sensor_sts(struct mic_power_util_info *power,
                                  uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.vddg.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_vddg_current_readings(struct mic_power_util_info *power,
                                  uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.vddg.cur;
    return E_MIC_SUCCESS;
}

int mic_get_vddg_current_sensor_sts(struct mic_power_util_info *power,
                                    uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.vddg.c_val;
    return E_MIC_SUCCESS;
}

int mic_get_vddg_voltage_readings(struct mic_power_util_info *power,
                                  uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.vddg.volt;
    return E_MIC_SUCCESS;
}

int mic_get_vddg_voltage_sensor_sts(struct mic_power_util_info *power,
                                    uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.vddg.v_val;
    return E_MIC_SUCCESS;
}

int mic_get_vddq_power_readings(struct mic_power_util_info *power,
                                uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.vddq.pwr;
    return E_MIC_SUCCESS;
}

int mic_get_vddq_power_sensor_sts(struct mic_power_util_info *power,
                                  uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.vddq.p_val;
    return E_MIC_SUCCESS;
}

int mic_get_vddq_current_readings(struct mic_power_util_info *power,
                                  uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.vddq.cur;
    return E_MIC_SUCCESS;
}

int mic_get_vddq_current_sensor_sts(struct mic_power_util_info *power,
                                    uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.vddq.c_val;
    return E_MIC_SUCCESS;
}

int mic_get_vddq_voltage_readings(struct mic_power_util_info *power,
                                  uint32_t *pwr)
{
    ASSERT((power != NULL) && (pwr != NULL));
    *pwr = power->pwr.vddq.volt;
    return E_MIC_SUCCESS;
}

int mic_get_vddq_voltage_sensor_sts(struct mic_power_util_info *power,
                                    uint32_t *sts)
{
    ASSERT((power != NULL) && (sts != NULL));
    *sts = power->pwr.vddq.v_val;
    return E_MIC_SUCCESS;
}

int mic_free_power_utilization_info(struct mic_power_util_info *power)
{
    ASSERT(power != NULL);
    delete(power);
    return E_MIC_SUCCESS;
}

/* power limits */
int mic_get_power_limit(struct mic_device *mdh,
                        struct mic_power_limit **limit)
{
    ASSERT((mdh != NULL) && (limit != NULL));
    try {
        *limit = NULL;
        *limit = new struct mic_power_limit ();
        mdh->get_power_limit(*limit);
    } catch (mic_exception const &e) {
        if (*limit != NULL)
            mic_free_power_limit(*limit);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*limit != NULL)
            mic_free_power_limit(*limit);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*limit != NULL)
            mic_free_power_limit(*limit);

        return E_MIC_INTERNAL;
    }

    return E_MIC_SUCCESS;
}

int mic_get_power_phys_limit(struct mic_power_limit *limit,
                             uint32_t *phys_lim)
{
    ASSERT((limit != NULL) && (phys_lim != NULL));
    *phys_lim = limit->phys;
    return E_MIC_SUCCESS;
}

int mic_get_power_hmrk(struct mic_power_limit *limit,
                       uint32_t *hmrk)
{
    ASSERT((limit != NULL) && (hmrk != NULL));
    *hmrk = limit->hmrk;
    return E_MIC_SUCCESS;
}

int mic_get_power_lmrk(struct mic_power_limit *limit,
                       uint32_t *lmrk)
{
    ASSERT((limit != NULL) && (lmrk != NULL));
    *lmrk = limit->lmrk;
    return E_MIC_SUCCESS;
}

int mic_get_time_window0(struct mic_power_limit *limit,
                         uint32_t *time_window)
{
        ASSERT((limit != NULL) && (time_window != NULL));
        *time_window = limit->time_win0;
        return E_MIC_SUCCESS;
}

int mic_get_time_window1(struct mic_power_limit *limit,
                         uint32_t *time_window)
{
        ASSERT((limit != NULL) && (time_window != NULL));
        *time_window = limit->time_win1;
        return E_MIC_SUCCESS;
}

int mic_free_power_limit(struct mic_power_limit *limit)
{
    ASSERT(limit != NULL);
    delete limit;
    return E_MIC_SUCCESS;
}

int mic_set_power_limit0(struct mic_device *mdh,
                         uint32_t power, uint32_t time_window)
{
        ASSERT(mdh != NULL);

        try {
                mdh->set_power_limit0(power, time_window);

                return E_MIC_SUCCESS;
        } catch (mic_exception const &e) {
                return e.get_mic_errno();
        } catch (...) {
                return E_MIC_INTERNAL;
        }
}

int mic_set_power_limit1(struct mic_device *mdh,
                         uint32_t power, uint32_t time_window)
{
        ASSERT(mdh != NULL);

        try {
                mdh->set_power_limit1(power, time_window);

                return E_MIC_SUCCESS;
        } catch (mic_exception const &e) {
                return e.get_mic_errno();
        } catch (...) {
                return E_MIC_INTERNAL;
        }
}

int mic_get_memory_utilization_info(struct mic_device *mdh,
                                    struct mic_memory_util_info **memory)
{
    ASSERT((mdh != NULL) && (memory != NULL));
    try {
        *memory = NULL;
        *memory = new struct mic_memory_util_info ();
        mdh->get_memory_utilization_info(*memory);
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e) {
        if (*memory != NULL)
            mic_free_memory_utilization_info(*memory);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*memory != NULL)
            mic_free_memory_utilization_info(*memory);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*memory != NULL)
            mic_free_memory_utilization_info(*memory);

        return E_MIC_INTERNAL;
    }
}

int mic_get_total_memory_size(struct mic_memory_util_info *memory,
                              uint32_t *total_size)
{
    ASSERT((memory != NULL) && (total_size != NULL));
    *total_size = memory->mem.total;
    return E_MIC_SUCCESS;
}

int mic_get_available_memory_size(struct mic_memory_util_info *memory,
                                  uint32_t *avail_size)
{
    ASSERT((memory != NULL) && (avail_size != NULL));
    *avail_size = memory->mem.free;
    return E_MIC_SUCCESS;
}

int mic_get_memory_buffers_size(struct mic_memory_util_info *memory,
                                uint32_t *bufs)
{
    ASSERT((memory != NULL) && (bufs != NULL));
    *bufs = memory->mem.bufs;
    return E_MIC_SUCCESS;
}

int mic_free_memory_utilization_info(struct mic_memory_util_info *memory)
{
    ASSERT(memory != NULL);
    delete(memory);
    return E_MIC_SUCCESS;
}

int mic_alloc_core_util(struct mic_core_util **cutil)
{
    assert(cutil != NULL && "mic_core_util struct ptr to ptr null");

    try {
        *cutil = NULL;
        *cutil = new struct mic_core_util;
    } catch (mic_exception const &e) {
        if (*cutil != NULL)
            mic_free_core_util(*cutil);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        return E_MIC_NOMEM;
    } catch (...) {
        if (*cutil != NULL)
            mic_free_core_util(*cutil);

        return E_MIC_INTERNAL;
    }

    return E_MIC_SUCCESS;
}

int mic_update_core_util(struct mic_device *mdh, struct mic_core_util *cutil)
{
    ASSERT((mdh != NULL) && (cutil != NULL));

    try {
        mdh->get_core_util(cutil);
    } catch (mic_exception const &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }

    return E_MIC_SUCCESS;
}

int mic_get_idle_counters(struct mic_core_util *cutil, uint64_t *idle_counters)
{
    ASSERT((cutil != NULL) && (idle_counters != NULL));

    uint16_t num_cores = cutil->c_util.core;

    for (uint16_t core_idx = 0; core_idx < num_cores; ++core_idx)
        idle_counters[core_idx] = cutil->c_util.cpu[core_idx].idle;

    return E_MIC_SUCCESS;
}

int mic_get_nice_counters(struct mic_core_util *cutil, uint64_t *nice_counters)
{
    ASSERT((cutil != NULL) && (nice_counters != NULL));

    uint16_t num_cores = cutil->c_util.core;

    for (uint16_t core_idx = 0; core_idx < num_cores; ++core_idx)
        nice_counters[core_idx] = cutil->c_util.cpu[core_idx].nice;

    return E_MIC_SUCCESS;
}

int mic_get_sys_counters(struct mic_core_util *cutil, uint64_t *sys_counters)
{
    ASSERT((cutil != NULL) && (sys_counters != NULL));

    uint16_t num_cores = cutil->c_util.core;

    for (uint16_t core_idx = 0; core_idx < num_cores; ++core_idx)
        sys_counters[core_idx] = cutil->c_util.cpu[core_idx].sys;

    return E_MIC_SUCCESS;
}

int mic_get_user_counters(struct mic_core_util *cutil, uint64_t *user_counters)
{
    ASSERT((cutil != NULL) && (user_counters != NULL));

    uint16_t num_cores = cutil->c_util.core;

    for (uint16_t core_idx = 0; core_idx < num_cores; ++core_idx)
        user_counters[core_idx] = cutil->c_util.cpu[core_idx].user;

    return E_MIC_SUCCESS;
}

int mic_get_idle_sum(struct mic_core_util *cutil, uint64_t *idle_sum)
{
    ASSERT((cutil != NULL) && (idle_sum != NULL));

    *idle_sum = cutil->c_util.sum.idle;

    return E_MIC_SUCCESS;
}

int mic_get_sys_sum(struct mic_core_util *cutil, uint64_t *sys_sum)
{
    ASSERT((cutil != NULL) && (sys_sum != NULL));

    *sys_sum = cutil->c_util.sum.sys;

    return E_MIC_SUCCESS;
}

int mic_get_nice_sum(struct mic_core_util *cutil, uint64_t *nice_sum)
{
    ASSERT((cutil != NULL) && (nice_sum != NULL));

    *nice_sum = cutil->c_util.sum.nice;

    return E_MIC_SUCCESS;
}

int mic_get_user_sum(struct mic_core_util *cutil, uint64_t *user_sum)
{
    ASSERT((cutil != NULL) && (user_sum != NULL));

    *user_sum = cutil->c_util.sum.user;

    return E_MIC_SUCCESS;
}

int mic_get_jiffy_counter(struct mic_core_util *cutil, uint64_t *jiffy)
{
    ASSERT((cutil != NULL) && (jiffy != NULL));

    *jiffy = cutil->c_util.jif;

    return E_MIC_SUCCESS;
}

int mic_get_num_cores(struct mic_core_util *cutil, uint16_t *num_cores)
{
    ASSERT((cutil != NULL) && (num_cores != NULL));

    *num_cores = cutil->c_util.core;

    return E_MIC_SUCCESS;
}

int mic_get_threads_core(struct mic_core_util *cutil, uint16_t *threads_core)
{
    ASSERT((cutil != NULL) && (threads_core != NULL));

    *threads_core = cutil->c_util.thr;

    return E_MIC_SUCCESS;
}

int mic_get_tick_count(struct mic_core_util *cutil,
                       uint32_t *tick_count)
{
    ASSERT((cutil != NULL) && (tick_count != NULL));

    *tick_count = cutil->c_util.tck;

    return E_MIC_SUCCESS;
}

int mic_free_core_util(struct mic_core_util *cutil)
{
    ASSERT(cutil != NULL);
    delete(cutil);
    return E_MIC_SUCCESS;
}

int mic_get_led_alert(struct mic_device *mdh, uint32_t *led_alert)
{
    ASSERT((mdh != NULL) && (led_alert != NULL));
    try {
        mdh->get_led_alert(led_alert);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_set_led_alert(struct mic_device *mdh, uint32_t *value)
{
    ASSERT((mdh != NULL) && (value != NULL));
    try {
        mdh->set_led_alert(value);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_get_turbo_state_info(struct mic_device *mdh,
                             struct mic_turbo_info **turbo)
{
    ASSERT((mdh != NULL) && (turbo != NULL));

    try {
        *turbo = NULL;
        *turbo = new struct mic_turbo_info ();
        mdh->get_turbo_state_info(*turbo);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        if (*turbo != NULL)
            mic_free_turbo_info(*turbo);

        return e.get_mic_errno();
    } catch (std::bad_alloc const &e) {
        if (*turbo != NULL)
            mic_free_turbo_info(*turbo);

        return E_MIC_NOMEM;
    } catch (...) {
        if (*turbo != NULL)
            mic_free_turbo_info(*turbo);

        return E_MIC_INTERNAL;
    }
}

int mic_get_turbo_state_valid(struct mic_turbo_info *turbo, uint32_t *valid)
{
    ASSERT((turbo != NULL) && (valid != NULL));

    *valid = turbo->trbo.avail;

    return E_MIC_SUCCESS;
}

int mic_get_turbo_state(struct mic_turbo_info *turbo, uint32_t *state)
{
    ASSERT((turbo != NULL) && (state != NULL));

    *state = turbo->trbo.state;

    return E_MIC_SUCCESS;
}

int mic_get_turbo_mode(struct mic_turbo_info *turbo, uint32_t *mode)
{
    ASSERT((turbo != NULL) && (mode != NULL));
    *mode = turbo->trbo.set;
    return E_MIC_SUCCESS;
}

int mic_set_turbo_mode(struct mic_device *mdh, uint32_t *state)
{
    ASSERT((mdh != NULL) && (state != NULL));
    try
    {
        mdh->set_turbo_mode(state);
        return E_MIC_SUCCESS;
    }
    catch (mic_exception const &e)
    {
        return e.get_mic_errno();
    }
    catch (...)
    {
        return E_MIC_INTERNAL;
    }
}

int mic_free_turbo_info(struct mic_turbo_info *turbo)
{
    ASSERT(turbo != NULL);
    delete(turbo);
    return E_MIC_SUCCESS;
}

/* throttle state info */
int mic_get_throttle_state_info(struct mic_device *mdh,
                                struct mic_throttle_state_info **ttl_state)
{
    ASSERT((mdh != NULL) && (ttl_state != NULL));

    try {
        *ttl_state = NULL;
        *ttl_state = new struct mic_throttle_state_info;
        mdh->get_throttle_state_info(*ttl_state);
        return E_MIC_SUCCESS;
    } catch (const mic_exception &e) {
        if (*ttl_state != NULL)
            mic_free_throttle_state_info(*ttl_state);

        return e.get_mic_errno();
    } catch (const std::bad_alloc &e) {
        return E_MIC_NOMEM;
    } catch (...) {
        if (*ttl_state != NULL)
            mic_free_throttle_state_info(*ttl_state);

        return E_MIC_INTERNAL;
    }
}

int mic_get_thermal_ttl_active(struct mic_throttle_state_info *ttl_state,
                               int *active)
{
    ASSERT((ttl_state != NULL) && (active != NULL));

    *active = ttl_state->ttl_state.thermal.active;

    return E_MIC_SUCCESS;
}

int mic_get_thermal_ttl_current_len(struct mic_throttle_state_info *ttl_state,
                                    uint32_t *current)
{
    ASSERT((ttl_state != NULL) && (current != NULL));

    *current = ttl_state->ttl_state.thermal.since;

    return E_MIC_SUCCESS;
}

int mic_get_thermal_ttl_count(struct mic_throttle_state_info *ttl_state,
                              uint32_t *count)
{
    ASSERT((ttl_state != NULL) && (count != NULL));

    *count = ttl_state->ttl_state.thermal.count;

    return E_MIC_SUCCESS;
}

int mic_get_thermal_ttl_time(struct mic_throttle_state_info *ttl_state,
                             uint32_t *time)
{
    ASSERT((ttl_state != NULL) && (time != NULL));

    *time = ttl_state->ttl_state.thermal.time;

    return E_MIC_SUCCESS;
}

int mic_get_power_ttl_active(struct mic_throttle_state_info *ttl_state,
                             int *active)
{
    ASSERT((ttl_state != NULL) && (active != NULL));

    *active = ttl_state->ttl_state.power.active;

    return E_MIC_SUCCESS;
}

int mic_get_power_ttl_current_len(struct mic_throttle_state_info *ttl_state,
                                  uint32_t *current)
{
    ASSERT((ttl_state != NULL) && (current != NULL));

    *current = ttl_state->ttl_state.power.since;

    return E_MIC_SUCCESS;
}

int mic_get_power_ttl_count(struct mic_throttle_state_info *ttl_state,
                            uint32_t *count)
{
    ASSERT((ttl_state != NULL) && (count != NULL));

    *count = ttl_state->ttl_state.power.count;

    return E_MIC_SUCCESS;
}

int mic_get_power_ttl_time(struct mic_throttle_state_info *ttl_state,
                           uint32_t *time)
{
    ASSERT((ttl_state != NULL) && (time != NULL));

    *time = ttl_state->ttl_state.power.time;

    return E_MIC_SUCCESS;
}

int mic_free_throttle_state_info(struct mic_throttle_state_info *ttl_state)
{
    ASSERT(ttl_state != NULL);
    delete ttl_state;
    return E_MIC_SUCCESS;
}

int mic_get_uuid(struct mic_device *mdh, uint8_t *uuid, size_t *size)
{
    ASSERT((mdh != NULL) && (uuid != NULL) && (size != NULL));
    try {
        mdh->get_uuid(uuid, size);
        return E_MIC_SUCCESS;
    } catch (mic_exception const &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }
}

/* device properties */
int mic_get_sysfs_attribute(struct mic_device *mdh,
                            const char *attr,
                            char *value,
                            size_t *size)
{
    ASSERT((mdh != NULL) && (attr != NULL) && (size != NULL));

    try {
        std::string attr_value =
            mdh->get_sysfs_attribute(std::string(attr));

        if (*size > 0 && value != NULL)
            strncpy(value, attr_value.c_str(), *size);

        if (*size < attr_value.length() + 1)
            *size = attr_value.length() + 1;

        return E_MIC_SUCCESS;
    }
    catch (const mic_exception &e) {
        return e.get_mic_errno();
    }
    catch (...) {
        return E_MIC_INTERNAL;
    }
}

int mic_is_ras_avail(struct mic_device *mdh, int *ras_avail)
{
    ASSERT((mdh != NULL) && (ras_avail != NULL));

    int avail;
    try{
        avail = mdh->is_ras_avail();
    } catch (const mic_exception &e) {
        return e.get_mic_errno();
    } catch (...) {
        return E_MIC_INTERNAL;
    }
    *ras_avail = avail;
    return E_MIC_SUCCESS;
}

/* uos power management config */
int mic_get_uos_pm_config(struct mic_device *mdh,
                          struct mic_uos_pm_config **pm_config)
{
    ASSERT((mdh != NULL) && (pm_config != NULL));

    try {
        *pm_config = NULL;
        *pm_config = new struct mic_uos_pm_config;
        mdh->get_uos_pm_config(*pm_config);
        return E_MIC_SUCCESS;
    } catch (const mic_exception &e) {
        if (*pm_config != NULL)
            mic_free_uos_pm_config(*pm_config);

        return e.get_mic_errno();
    } catch (const std::bad_alloc &e) {
        return E_MIC_NOMEM;
    } catch (...) {
        if (*pm_config != NULL)
            mic_free_uos_pm_config(*pm_config);

        return E_MIC_INTERNAL;
    }
}

int mic_get_cpufreq_mode(struct mic_uos_pm_config *pm_config, int *mode)
{
    ASSERT((pm_config != NULL) && (mode != NULL));

    *mode = pm_config->pm_config.bits.cpufreq;

    return E_MIC_SUCCESS;
}

int mic_get_corec6_mode(struct mic_uos_pm_config *pm_config, int *mode)
{
    ASSERT((pm_config != NULL) && (mode != NULL));

    *mode = pm_config->pm_config.bits.corec6;

    return E_MIC_SUCCESS;
}

int mic_get_pc3_mode(struct mic_uos_pm_config *pm_config, int *mode)
{
    ASSERT((pm_config != NULL) && (mode != NULL));

    *mode = pm_config->pm_config.bits.pc3;

    return E_MIC_SUCCESS;
}

int mic_get_pc6_mode(struct mic_uos_pm_config *pm_config, int *mode)
{
    ASSERT((pm_config != NULL) && (mode != NULL));

    *mode = pm_config->pm_config.bits.pc6;

    return E_MIC_SUCCESS;
}

int mic_free_uos_pm_config(struct mic_uos_pm_config *pm_config)
{
    ASSERT(pm_config != NULL);
    delete pm_config;
    return E_MIC_SUCCESS;
}

/* smc config */
int mic_get_smc_persistence_flag(struct mic_device *mdh,
                                             int *persist_flag)
{
        ASSERT((mdh != NULL) && (persist_flag != NULL));
        try {
                mdh->get_smc_persistence_flag(persist_flag);
                return E_MIC_SUCCESS;
        } catch (mic_exception const &e) {
                return e.get_mic_errno();
        } catch (...) {
                return E_MIC_INTERNAL;
        }
}

int mic_set_smc_persistence_flag(struct mic_device *mdh,
                                             int persist_flag)
{
        ASSERT(mdh != NULL);
        try {
                mdh->set_smc_persistence_flag(persist_flag);
                return E_MIC_SUCCESS;
        } catch (mic_exception const &e) {
                return e.get_mic_errno();
        } catch (...) {
                return E_MIC_INTERNAL;
        }
}
