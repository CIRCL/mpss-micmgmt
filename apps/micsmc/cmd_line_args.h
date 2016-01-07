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

#ifndef CMD_LINE_ARGS_H_
#define CMD_LINE_ARGS_H_

#include <cstddef>

#include <map>
using std::map;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <memory>
using std::shared_ptr;

#include <stdexcept>
using std::out_of_range;

namespace apps {
namespace utils {
class cmd_line_args {
public:
    enum opt_error_code {
        OPT_OK,
        OPT_UNRECOGNIZED_OPT,
        OPT_UNRECOGNIZED_ARG,
        OPT_INCORRECT_NUM_ARGS,
        OPT_INCORRECT_ARG_TYPE,
        OPT_INCORRECT_ARG_FORMAT
    };

    struct opt_args {
        string         option;
        string         text;
        vector<string> args;
        bool           duplicate;
        opt_error_code error_code;
        bool           processed;

        size_t         get_num_args() const;

        opt_args();
    };

    cmd_line_args();
    virtual ~cmd_line_args();

    void parse_args(int argc, char *argv[]);
    size_t get_num_args() const;

    opt_args *get_opt_args(size_t index);
    opt_args *get_opt_args(const string &name);

private:
    void split(const string &str, char delim, bool ignore_ws,
               vector<string> *tokens);

    map<string, shared_ptr<opt_args> > direct_args_;
    vector<shared_ptr<opt_args> > ordered_args_;

    typedef map<string, shared_ptr<opt_args> >::const_iterator direct_args_it;
};
}
}

#endif /* CMD_LINE_ARGS_H_ */
