// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_REGEX_HPP
#define CODEDB_REGEX_HPP

#include <boost/xpressive/xpressive_fwd.hpp>
#include <boost/xpressive/regex_constants.hpp>

#include <boost/exception/all.hpp>

typedef boost::error_info<struct tag_regex_string, std::string> errinfo_regex_string;

boost::xpressive::sregex compile_sregex(
    const std::string& regex_str, boost::xpressive::regex_constants::syntax_option_type opts);

boost::xpressive::cregex compile_cregex(
    const std::string& regex_str, boost::xpressive::regex_constants::syntax_option_type opts);

#endif
