// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_REGEX_HPP
#define CODEDB_REGEX_HPP

#include "nsalias.hpp"

#include <boost/xpressive/xpressive_fwd.hpp>
#include <boost/xpressive/regex_constants.hpp>
#include <boost/exception/all.hpp>

typedef boost::error_info<struct tag_regex_string, std::string> errinfo_regex_string;

bxp::sregex compile_sregex(
    const std::string& regex_str, bxp::regex_constants::syntax_option_type opts);

bxp::cregex compile_cregex(
    const std::string& regex_str, bxp::regex_constants::syntax_option_type opts);

#endif
