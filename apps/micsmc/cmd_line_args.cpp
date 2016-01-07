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

#include "cmd_line_args.h"
using apps::utils::cmd_line_args;

#include <sstream>
using std::istringstream;
using std::ostringstream;

using std::getline;

cmd_line_args::opt_args::opt_args() :
    duplicate(false),
    error_code(OPT_OK),
    processed(false)
{
}

size_t cmd_line_args::opt_args::get_num_args() const
{
    return args.size();
}

cmd_line_args::cmd_line_args()
{
}

cmd_line_args::~cmd_line_args()
{
    // No action needed.
}

void cmd_line_args::parse_args(int argc, char *argv[])
{
    // i = 1: ignore command
    for (int i = 1; i < argc; i++) {
        // Gather some information about the argument...
        shared_ptr<opt_args> oa_ptr(new opt_args);
        oa_ptr->option.assign(argv[i]);

        // Is this an option?
        if (argv[i][0] == '-') {
            ostringstream text;
            // Get all the args corresponding to this option
            for (i += 1; i < argc && argv[i][0] != '-'; i++) {
                string temp_arg(argv[i]);

                text << temp_arg << " ";
                split(temp_arg, ',', true, &oa_ptr->args);
            }
            i--;
            oa_ptr->text = text.str();
        }

        if (direct_args_.find(oa_ptr->option) == direct_args_.end()) {
            direct_args_[oa_ptr->option] = oa_ptr;
            ordered_args_.push_back(oa_ptr);
        } else {
            direct_args_[oa_ptr->option]->duplicate = true;
        }
    }
}

size_t cmd_line_args::get_num_args() const
{
    return ordered_args_.size();
}

cmd_line_args::opt_args *cmd_line_args::get_opt_args(size_t index)
{
    return ordered_args_.at(index).get();
}

cmd_line_args::opt_args *cmd_line_args::get_opt_args(const string &name)
{
    direct_args_it it = direct_args_.find(name);

    if (it == direct_args_.end())
        return NULL;

    return it->second.get();
}

void cmd_line_args::split(const string &str, char delim, bool ignore_ws,
                          vector<string> *tokens)
{
    istringstream iss(str);
    string tok;

    while (getline(iss, tok, delim))
        if (!tok.empty() || !ignore_ws)
            tokens->push_back(tok);
}
