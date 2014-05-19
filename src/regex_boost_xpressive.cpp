// CodeDB - public domain - 2014 Daniel Andersson

#ifdef CDB_USE_REGEX_BOOST_XPRESSIVE

#include "regex.hpp"

#include <boost/xpressive/xpressive_dynamic.hpp>

namespace
{

namespace bxp = boost::xpressive;

bxp::regex_constants::syntax_option_type xpressive_options(const char* flags)
{
  // Default options
  bxp::regex_constants::syntax_option_type res =
    bxp::regex_constants::ECMAScript|
    bxp::regex_constants::not_dot_newline|
    bxp::regex_constants::optimize;

  for(; *flags; ++flags)
  {
    if(*flags == 'i')
      res = res | bxp::regex_constants::icase;
  }

  return res;
}

class xpressive_regex : public regex
{
 public:
  xpressive_regex(const std::string& expr, const char* flags)
   : m_regex(bxp::cregex::compile(expr, xpressive_options(flags)))
  {
  }

 private:

  match_range what(int n)
  {
    return match_range(m_what[n].first, m_what[n].second);
  }

  bool do_match(const char* begin, const char* end)
  {
    return bxp::regex_match(begin, end, m_what, m_regex);
  }

  bool do_search(const char* begin, const char* end)
  {
    return bxp::regex_search(begin, end, m_what, m_regex);
  }

  bxp::cregex m_regex;
  bxp::cmatch m_what;
};

} // namespace

regex_ptr compile_regex_boost_xpressive(const std::string& expr, int, const char* flags)
{
  try
  {
    return regex_ptr(new xpressive_regex(expr, flags));
  }
  catch(const bxp::regex_error& error)
  {
    throw regex_error(error.what(), expr);
  }
}

#endif
