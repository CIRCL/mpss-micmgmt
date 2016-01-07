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

#include "cmd_config.h"
using apps::utils::cmd_config;
using apps::utils::cmd_line_args;

#include <limits>
using std::numeric_limits;

#include <sstream>
using std::ostringstream;
using std::stringstream;

using std::endl;

cmd_config::opt_config::opt_config()
{
    init();
}

cmd_config::opt_config::opt_config(const char *s_opt, const char *l_opt) :
    short_opt(s_opt),
    long_opt(l_opt)
{
    init();
}

void cmd_config::opt_config::init()
{
    validator = NULL;
    min_args = 0;
    max_args = numeric_limits<unsigned int>::max();
}

cmd_config::cmd_config() :
    stop_on_first_error_(false),
    silent_(false),
    ignore_duplicates_(false),
    on_error_show_opt_help_(false)
{
}

cmd_config::~cmd_config()
{
}

bool cmd_config::set_opt(const shared_ptr<opt_config> &opt)
{
    if (!opt)
        return false;

    bool set = false;
    if (!opt->short_opt.empty()) {
        direct_opts_[opt->short_opt] = opt;
        set = true;
    }
    if (!opt->long_opt.empty()) {
        direct_opts_[opt->long_opt] = opt;
        set = true;
    }

    if (set)
        ordered_opts_.push_back(opt);

    return set;
}

const cmd_config::opt_config *cmd_config::get_opt(
    const cmd_line_args::opt_args &opt) const
{
    direct_opts_it it = direct_opts_.find(opt.option);

    if (it == direct_opts_.end())
        return NULL;

    return it->second.get();
}

const cmd_config::opt_config *cmd_config::get_opt_by_name(
    const string &name) const
{
    string opt = '-' + name;
    direct_opts_it it = direct_opts_.find(opt);

    if (it == direct_opts_.end()) {
        opt.insert(0, 1, '-');
        it = direct_opts_.find(opt);
        if (it == direct_opts_.end())
            return NULL;
    }

    return it->second.get();
}

void cmd_config::set_header(const string &header)
{
    header_ = header;
}

void cmd_config::set_footer(const string &footer)
{
    footer_ = footer;
}

void cmd_config::set_error_msg_header(const string &header)
{
    error_msg_header_ = header;
}

void cmd_config::set_stop_on_first_error(bool stop)
{
    stop_on_first_error_ = stop;
}

void cmd_config::set_silent(bool silent)
{
    silent_ = silent;
}

void cmd_config::set_ignore_duplicates(bool ignore)
{
    ignore_duplicates_ = ignore;
}

void cmd_config::set_on_error_show_opt_help(bool show_help)
{
    on_error_show_opt_help_ = show_help;
}

void cmd_config::print_help(ostream &out) const
{
    if (!header_.empty())
        out << header_ << endl;
    for (ordered_opts_it it = ordered_opts_.begin();
         it != ordered_opts_.end();
         ++it) {
        if (!(*it)->help.empty()) {
            out << (*it)->help;
            out << endl;
        }
    }
    if (!footer_.empty())
        out << footer_ << endl;
}

bool cmd_config::process_cmd_line_args(cmd_line_args *args, ostream &out) const
{
    ostringstream msgs;
    bool success = true;

    for (size_t i = 0; i < args->get_num_args() &&
         (success || !stop_on_first_error_); i++) {
        cmd_line_args::opt_args *arg = (*args).get_opt_args(i);
        const opt_config *config = get_opt(*arg);

        // Is this a valid option?
        if (config == NULL) {
            msgs << error_msg_header_ << "Unrecognized option: "
                 << arg->option << endl;
            success = false;
            error(config, arg, cmd_line_args::OPT_UNRECOGNIZED_OPT,
                  on_error_show_opt_help_, msgs);
            continue;
        }

        // What do we do if duplicates were found?
        if (!ignore_duplicates_ && arg->duplicate) {
            msgs << error_msg_header_ <<
                "Duplicate options found: "
                 << arg->option << endl;
            success = false;
            error(config, on_error_show_opt_help_, msgs);
            continue;
        }

        // Check the number of arguments passed to the option (check
        // each case to provide a more specific error message).
        unsigned int n_args = arg->get_num_args();
        if (n_args == 0 && config->min_args > 0) {
            msgs << error_msg_header_ << "Missing required "
                 << "argument(s) for option: "
                 << arg->option << endl;
            success = false;
            error(config, arg,
                  cmd_line_args::OPT_INCORRECT_NUM_ARGS,
                  on_error_show_opt_help_, msgs);
            continue;
        } else if (n_args > 0 && config->max_args == 0) {
            msgs << error_msg_header_ << "Option does not take "
                 << "arguments: "
                 << arg->option << endl;
            success = false;
            error(config, arg,
                  cmd_line_args::OPT_INCORRECT_NUM_ARGS,
                  on_error_show_opt_help_, msgs);
            continue;
        } else if (n_args < config->min_args ||
                   n_args > config->max_args) {
            msgs << error_msg_header_ << "Incorrect number of "
                 << "arguments for option: "
                 << arg->option << endl;
            success = false;
            error(config, arg,
                  cmd_line_args::OPT_INCORRECT_NUM_ARGS,
                  on_error_show_opt_help_, msgs);
            continue;
        }

        // Call the validator (if defined).
        if (config->validator != NULL) {
            string err_msg;
            arg->error_code = (*config->validator)(*arg, &err_msg);
            if (arg->error_code != cmd_line_args::OPT_OK) {
                msgs << error_msg_header_ << "In option "
                     << arg->option << ": "
                     << err_msg << endl;
                success = false;
                error(config, on_error_show_opt_help_, msgs);
                continue;
            }
        }

        // If we made it here, the option is valid
        arg->error_code = cmd_line_args::OPT_OK;
    }

    if (!silent_)
        out << msgs.str();

    return success;
}

cmd_line_args::opt_error_code cmd_config::basic_uint_validator(
    const cmd_line_args::opt_args &args, string *err_msg)
{
    for (size_t i = 0; i < args.get_num_args(); i++) {
        stringstream ss;
        unsigned int x;
        ss << args.args[i];
        ss >> x;
        if (!ss) {
            *err_msg = "argument is not a valid integer";
            return cmd_line_args::OPT_INCORRECT_ARG_TYPE;
        }
    }

    return cmd_line_args::OPT_OK;
}

inline void cmd_config::error(const opt_config *config,
                              cmd_line_args::opt_args *arg,
                              cmd_line_args::opt_error_code error,
                              bool show_help,
                              ostream &out)
{
    arg->error_code = error;
    if (show_help && config != NULL && !config->help.empty())
        out << "Help: " << endl << config->help << endl;
}

inline void cmd_config::error(const opt_config *config,
                              bool show_help,
                              ostream &out)
{
    if (show_help && config != NULL && !config->help.empty())
        out << "Help: " << endl << config->help << endl;
}
