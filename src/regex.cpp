// CodeDB - public domain - 2010 Daniel Andersson

#include "regex.hpp"
#include "nsalias.hpp"

#include <cstring>

regex::~regex() {}

bool regex::match(const std::string& str) {
  const char* p = str.c_str();
  return do_match(p, p + str.size());
}

bool regex::match(const char* begin, const char* end) {
  return do_match(begin, end);
}

bool regex::search(const std::string& str) {
  const char* p = str.c_str();
  return do_search(p, p + str.size());
}

bool regex::search(const char* begin, const char* end) {
  return do_search(begin, end);
}

// Regex engine registration. These are ordered from least to most desirable.

#ifdef CDB_USE_REGEX_STD_REGEX
regex_ptr compile_regex_std(const std::string& expr, int caps,
                            const char* flags);
#define CDB_SELECTED_REGEX compile_regex_std
#endif

#ifdef CDB_USE_REGEX_BOOST_REGEX
regex_ptr compile_regex_boost_regex(const std::string& expr, int caps,
                                    const char* flags);
#undef CDB_SELECTED_REGEX
#define CDB_SELECTED_REGEX compile_regex_boost_regex
#endif

#ifdef CDB_USE_REGEX_BOOST_XPRESSIVE
regex_ptr compile_regex_boost_xpressive(const std::string& expr, int caps,
                                        const char* flags);
#undef CDB_SELECTED_REGEX
#define CDB_SELECTED_REGEX compile_regex_boost_xpressive
#endif

#ifdef CDB_USE_REGEX_RE2
regex_ptr compile_regex_re2(const std::string& expr, int caps,
                            const char* flags);
#undef CDB_SELECTED_REGEX
#define CDB_SELECTED_REGEX compile_regex_re2
#endif

#ifndef CDB_SELECTED_REGEX
#error No regex engine selected.
#endif

regex_ptr compile_regex(const std::string& expr, int caps, const char* flags) {
  if (const char* selected = std::getenv("CDB_REGEX")) {
#ifdef CDB_USE_REGEX_RE2
    if (!std::strcmp(selected, "re2")) {
      return compile_regex_re2(expr, caps, flags);
    }
#endif
#ifdef CDB_USE_REGEX_BOOST_REGEX
    if (!std::strcmp(selected, "boost_regex")) {
      return compile_regex_boost_regex(expr, caps, flags);
    }
#endif
#ifdef CDB_USE_REGEX_BOOST_XPRESSIVE
    if (!std::strcmp(selected, "boost_xpressive")) {
      return compile_regex_boost_xpressive(expr, caps, flags);
    }
#endif
#ifdef CDB_USE_REGEX_STD_REGEX
    if (!std::strcmp(selected, "std")) {
      return compile_regex_std(expr, caps, flags);
    }
#endif

    throw regex_error("Unknown regex engine", "");
  }

  return CDB_SELECTED_REGEX(expr, caps, flags);
}

std::string escape_regex(const std::string& text) {
  std::string res;

  for (auto i = text.begin(); i != text.end(); ++i) {
    switch (*i) {
      case '^':
      case '.':
      case '$':
      case '|':
      case '(':
      case ')':
      case '[':
      case ']':
      case '*':
      case '+':
      case '?':
      case '/':
      case '\\':
        // Fall-through intended
        res.push_back('\\');
      default:
        res.push_back(*i);
    }
  }

  return res;
}
