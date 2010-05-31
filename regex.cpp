// CodeDB - public domain - 2010 Daniel Andersson

#include "regex.hpp"

#include <boost/xpressive/xpressive_dynamic.hpp>

namespace bxp = boost::xpressive;

namespace
{
    template<class T>
    T compile(const std::string& regex_str, bxp::regex_constants::syntax_option_type opts)
    {
        try
        {
            return T::compile(regex_str, opts);
        }
        catch(bxp::regex_error& error)
        {
            error << errinfo_regex_string(regex_str);
            throw;
        }
    }
}


bxp::sregex compile_sregex(const std::string& regex_str, bxp::regex_constants::syntax_option_type opts)
{
    return compile<bxp::sregex>(regex_str, opts);
}

bxp::cregex compile_cregex(const std::string& regex_str, bxp::regex_constants::syntax_option_type opts)
{
    return compile<bxp::cregex>(regex_str, opts);
}
