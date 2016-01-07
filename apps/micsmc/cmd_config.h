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

#ifndef CMD_CONFIG_H_
#define CMD_CONFIG_H_

#include <cstdlib>

#include <memory>
using std::shared_ptr;

#include <map>
using std::map;

#include <vector>
using std::vector;

#include <string>
using std::string;

#include <iostream>
using std::ostream;
using std::cout;
using std::cerr;

#include "cmd_line_args.h"

namespace apps {
namespace utils {
typedef cmd_line_args::opt_error_code (*opt_validator)(
            const cmd_line_args::opt_args &, string *);

class cmd_config {
public:
    struct opt_config {
        string        short_opt;
        string        long_opt;
        string        help;
        unsigned int  min_args;
        unsigned int  max_args;

        opt_validator validator;

        opt_config();
        opt_config(const char *s_opt, const char *l_opt);

private:
        void init();
    };

    cmd_config();
    virtual ~cmd_config();

    bool set_opt(const shared_ptr<opt_config> &opt);
    const opt_config *get_opt(const cmd_line_args::opt_args &opt) const;
    const opt_config *get_opt_by_name(const string &name) const;

    void set_header(const string &header);
    void set_footer(const string &footer);
    void set_error_msg_header(const string &header);
    void set_stop_on_first_error(bool stop);
    void set_silent(bool silent);
    void set_ignore_duplicates(bool ignore);
    void set_on_error_show_opt_help(bool show_help);

    void print_help(ostream &out = cout) const;
    bool process_cmd_line_args(cmd_line_args *args, ostream &out = cerr) const;

    static cmd_line_args::opt_error_code basic_uint_validator(
                const cmd_line_args::opt_args &args,
                string *err_msg);

private:
    string header_;
    string footer_;
    string error_msg_header_;
    bool stop_on_first_error_;
    bool silent_;
    bool ignore_duplicates_;
    bool on_error_show_opt_help_;

    static inline void error(const opt_config *config,
                             cmd_line_args::opt_args *arg,
                             cmd_line_args::opt_error_code error,
                             bool show_help, ostream &out);

    static inline void error(const opt_config *config, bool show_help,
                             ostream &out);

    map<string, shared_ptr<opt_config> > direct_opts_;
    vector<shared_ptr<opt_config> > ordered_opts_;

    typedef map<string, shared_ptr<opt_config> >::const_iterator direct_opts_it;
    typedef vector<shared_ptr<opt_config> >::const_iterator ordered_opts_it;
};
}
}

#endif /* CMD_CONFIG_H_ */
