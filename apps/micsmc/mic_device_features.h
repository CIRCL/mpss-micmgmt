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

#ifndef MIC_DEVICE_FEATURES_H_
#define MIC_DEVICE_FEATURES_H_

#include <cstdlib>
#include <cassert>

#include <string>
using std::string;

#include <map>
using std::map;

#include <memory>
using std::shared_ptr;

#include <miclib.h>
#include "micmgmt_exception.h"

class mic_device_features {
public:
    mic_device_features();
    virtual ~mic_device_features();

    enum mic_feature {
        THERMAL_INFO,
        CORES_INFO,
        POWER_UTIL_INFO,
        POWER_LIMIT_INFO,
        MEM_UTIL_INFO,
        THROTTLE_INFO,
        PCI_CONFIG_INFO,
        VERSION_INFO,
        PROC_INFO,
        CORE_UTIL_INFO,
        MEM_INFO,
        PM_CONFIG_INFO,
        TURBO_INFO
    };

    static shared_ptr<struct mic_devices_list> get_devices();
    static int get_number_of_devices(
            shared_ptr<struct mic_devices_list> &devices);
    static unsigned int get_device_at_index(
            shared_ptr<struct mic_devices_list> &devices,
            int index);

protected:
    class mic_feature_base {
    public:
        mic_feature_base(struct mic_device **mdh, const char *desc) :
            mdh_(mdh), description_(desc)
        {
        }

        virtual void update() = 0;

    protected:
        struct mic_device **mdh_;
        string description_;
    };

    template <class mic_info_type>
    class mic_info_obj : public mic_feature_base {
    public:
        typedef int (*alloc_info_func)(mic_info_type **);
        typedef int (*update_info_func)(struct mic_device *, mic_info_type *);
        typedef int (*get_info_func)(struct mic_device *, mic_info_type **);
        typedef int (*free_info_func)(mic_info_type *);

        mic_info_obj(struct mic_device **mdh,
                     const char *desc,
                     get_info_func get_info,
                     free_info_func free_info) :
            mic_feature_base(mdh, desc)
        {
            obj_ = NULL;
            alloc_info_ = NULL;
            update_info_ = NULL;
            get_info_ = get_info;
            free_info_ = free_info;
        }

        mic_info_obj(struct mic_device **mdh,
                     const char *desc,
                     alloc_info_func alloc_info,
                     update_info_func update_info,
                     free_info_func free_info) :
            mic_feature_base(mdh, desc)
        {
            obj_ = NULL;
            alloc_info_ = alloc_info;
            update_info_ = update_info;
            get_info_ = NULL;
            free_info_ = free_info;
        }

        ~mic_info_obj()
        {
            if (obj_ && free_info_)
                free_info_(obj_);
        }

        template <class data_type>
        data_type get_data(int (*func)(mic_info_type *, data_type *),
                           const char *desc = "")
        {
            if (!obj_)
                update();

            data_type data;
            errno = 0;
            int ret = func(obj_, &data);
            if (ret != E_MIC_SUCCESS)
                throw micmgmt_exception(desc, ret);

            return data;
        }

        string get_str_data(int (*func)(mic_info_type *, char *, size_t *),
                            const char *desc = "")
        {
            if (!obj_)
                update();

            size_t size = 0;
            errno = 0;
            int ret = func(obj_, NULL, &size);
            if (ret != E_MIC_SUCCESS)
                throw micmgmt_exception(desc, ret);

            shared_ptr<char> buffer(new char[size]);
            errno = 0;
            ret = func(obj_, buffer.get(), &size);
            if (ret != E_MIC_SUCCESS)
                throw micmgmt_exception(desc, ret);

            return string(buffer.get());
        }

        template <class data_type>
        void get_array_data(int (*func)(mic_info_type *, data_type *),
                            data_type *array, const char *desc = "")
        {
            errno = 0;
            int ret = func(obj_, array);

            if (ret != E_MIC_SUCCESS)
                throw micmgmt_exception(desc, ret);
        }

        virtual void update()
        {
            assert((alloc_info_ && update_info_) || get_info_);
            assert(free_info_);
            assert(*mdh_);

            int ret;

            if (obj_)
                free_info_(obj_);

            if (alloc_info_) {
                errno = 0;
                ret = alloc_info_(&obj_);
                if (ret != E_MIC_SUCCESS) {
                    obj_ = NULL;
                    throw micmgmt_exception(description_, ret);
                }
                errno = 0;
                ret = update_info_(*mdh_, obj_);
                if (ret != E_MIC_SUCCESS)
                    throw micmgmt_exception(description_, ret);
            } else {
                errno = 0;
                ret = get_info_(*mdh_, &obj_);
                if (ret != E_MIC_SUCCESS) {
                    obj_ = NULL;
                    throw micmgmt_exception(description_, ret);
                }
            }
        }

    private:
        mic_info_type *obj_;
        alloc_info_func alloc_info_;
        update_info_func update_info_;
        get_info_func get_info_;
        free_info_func free_info_;
    };

    template <class data_type>
    data_type get_device_data(
        int (*func)(struct mic_device *, data_type *),
        const char *desc = "")
    {
        assert(mdh_);

        data_type data;
        errno = 0;
        int ret = func(mdh_, &data);
        if (ret != E_MIC_SUCCESS)
            throw micmgmt_exception(desc, ret);

        return data;
    }

    string get_device_str_data(int (*func)(struct mic_device *, char *,
                                           size_t *), const char *desc = "");
    string get_device_attrib(const char *attrib);
    int get_device_attrib_as_int(const char *attrib);
    void do_device_operation(int (*func)(struct mic_device *), const
                             char *desc = "");

    template <class data_type>
    void set_device_data(
        int (*func)(struct mic_device *, data_type), data_type data,
        const char *desc = "")
    {
        errno = 0;
        int ret = func(mdh_, data);

        if (ret != E_MIC_SUCCESS)
            throw micmgmt_exception(desc, ret);
    }

    template <class data_type>
    void set_device_data(
        int (*func)(struct mic_device *, data_type *), data_type *data,
        const char *desc = "")
    {
        errno = 0;
        int ret = func(mdh_, data);

        if (ret != E_MIC_SUCCESS)
            throw micmgmt_exception(desc, ret);
    }

    struct mic_device *mdh_;
    mic_info_obj<struct mic_thermal_info> thermal_info_;
    mic_info_obj<struct mic_cores_info> cores_info_;
    mic_info_obj<struct mic_power_util_info> power_util_info_;
    mic_info_obj<struct mic_power_limit> power_limit_info_;
    mic_info_obj<struct mic_memory_util_info> mem_util_info_;
    mic_info_obj<struct mic_throttle_state_info> throttle_info_;
    mic_info_obj<struct mic_pci_config> pci_config_info_;
    mic_info_obj<struct mic_version_info> version_info_;
    mic_info_obj<struct mic_processor_info> proc_info_;
    mic_info_obj<struct mic_core_util> core_util_info_;
    mic_info_obj<struct mic_device_mem> mem_info_;
    mic_info_obj<struct mic_uos_pm_config> pm_config_info_;
    mic_info_obj<struct mic_turbo_info> turbo_info_;

    typedef map<mic_feature, mic_feature_base *> feature_map;
    feature_map features_;
};

#endif /* MIC_DEVICE_FEATURES_H_ */
