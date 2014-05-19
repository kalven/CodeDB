// CodeDB - public domain - 2014 Daniel Andersson

#ifdef CDB_USE_REGEX_RE2

#include "regex.hpp"

#include <re2/re2.h>
#include <cstring>

namespace {

class re2_regex : public regex {
 public:
  static const int max_caps = 3;

  re2_regex(const char* expr, const re2::RE2::Options& opts, int caps)
      : m_regex(expr, opts), m_caps_count(caps) {
    if (!m_regex.ok())
      throw regex_error(m_regex.error(),
                        std::string(expr + 1, std::strlen(expr) - 2));

    for (int i = 0; i != m_caps_count; ++i) {
      m_args[i] = re2::RE2::Arg(&m_caps[i]);
      m_pargs[i] = &m_args[i];
    }
  }

 private:
  match_range what(int n) {
    return match_range(m_caps[n].data(), m_caps[n].data() + m_caps[n].size());
  }

  bool do_match(const char* begin, const char* end) {
    re2::StringPiece text(begin, static_cast<int>(end - begin));
    return re2::RE2::FullMatchN(text, m_regex, m_pargs, m_caps_count);
  }

  bool do_search(const char* begin, const char* end) {
    re2::StringPiece text(begin, static_cast<int>(end - begin));
    return re2::RE2::PartialMatchN(text, m_regex, m_pargs, m_caps_count);
  }

  re2::StringPiece m_caps[max_caps];
  re2::RE2::Arg m_args[max_caps];
  re2::RE2::Arg* m_pargs[max_caps];
  re2::RE2 m_regex;
  int m_caps_count;
};

}  // namespace

regex_ptr compile_regex_re2(const std::string& expr, int caps,
                            const char* flags) {
  re2::RE2::Options opts;

  opts.set_log_errors(false);
  opts.set_never_nl(true);

  for (; *flags; ++flags) {
    if (*flags == 'i') opts.set_case_sensitive(false);
  }

  if (caps > re2_regex::max_caps)
    throw std::runtime_error("Internal error: caps > max_caps");

  return regex_ptr(new re2_regex(('(' + expr + ')').c_str(), opts, caps + 1));
}

#endif
