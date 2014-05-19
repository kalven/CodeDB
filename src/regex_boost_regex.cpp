// CodeDB - public domain - 2014 Daniel Andersson

#ifdef CDB_USE_REGEX_BOOST_REGEX

#include "regex.hpp"

#include <boost/regex.hpp>

namespace
{

boost::regex_constants::syntax_option_type boost_regex_options(const char* flags)
{
  // Default options
  boost::regex_constants::syntax_option_type res =
    boost::regex_constants::ECMAScript|
    // TODO: Fix this
    //            boost::regex_constants::not_dot_newline|
    boost::regex_constants::optimize;

  for(; *flags; ++flags)
  {
    if(*flags == 'i')
      res = res | boost::regex_constants::icase;
  }

  return res;
}

class boost_regex : public regex
{
 public:
  boost_regex(const std::string& expr, const char* flags)
   : m_regex(expr, boost_regex_options(flags))
  {
  }

 private:

  match_range what(int n)
  {
    return match_range(m_what[n].first, m_what[n].second);
  }

  bool do_match(const char* begin, const char* end)
  {
    return boost::regex_match(begin, end, m_what, m_regex);
  }

  bool do_search(const char* begin, const char* end)
  {
    return boost::regex_search(begin, end, m_what, m_regex);
  }

  boost::regex m_regex;
  boost::cmatch m_what;
};

} // namespace

regex_ptr compile_regex_boost_regex(const std::string& expr, int, const char* flags)
{
  try
  {
    return regex_ptr(new boost_regex(expr, flags));
  }
  catch(const boost::regex_error& error)
  {
    throw regex_error(error.what(), expr);
  }
}

#endif
