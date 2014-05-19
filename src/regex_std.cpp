// CodeDB - public domain - 2014 Daniel Andersson

#ifdef CDB_USE_REGEX_STD_REGEX

#include "regex.hpp"

#include <regex>

namespace
{

std::regex_constants::syntax_option_type std_regex_options(const char* flags)
{
  // Default options
  std::regex_constants::syntax_option_type res =
    std::regex_constants::ECMAScript|
    // TODO: Fix this
    //bxp::regex_constants::not_dot_newline|
    std::regex_constants::optimize;

  for(; *flags; ++flags)
  {
    if(*flags == 'i')
      res = res | std::regex_constants::icase;
  }

  return res;
}

class std_regex : public regex
{
 public:
  std_regex(const std::string& expr, const char* flags)
   : m_regex(expr, std_regex_options(flags))
  {
  }

 private:
  match_range what(int n)
  {
    return match_range(m_what[n].first, m_what[n].second);
  }

  bool do_match(const char* begin, const char* end)
  {
    return std::regex_match(begin, end, m_what, m_regex);
  }

  bool do_search(const char* begin, const char* end)
  {
    return std::regex_search(begin, end, m_what, m_regex);
  }

  std::regex m_regex;
  std::cmatch m_what;
};

} // namespace

regex_ptr compile_regex_std(const std::string& expr, int, const char* flags)
{
  try
  {
    return regex_ptr(new std_regex(expr, flags));
  }
  catch(const std::regex_error& error)
  {
    throw regex_error(error.what(), expr);
  }
}

#endif
