// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_REGEX_HPP
#define CODEDB_REGEX_HPP

#include <stdexcept>
#include <memory>
#include <string>

class match_range {
 public:
  match_range() : m_begin(0), m_end(0) {}

  match_range(const char* begin, const char* end)
      : m_begin(begin), m_end(end) {}

  const char* begin() const { return m_begin; }

  const char* end() const { return m_end; }

  operator std::string() const { return std::string(m_begin, m_end); }

 private:
  const char* m_begin;
  const char* m_end;
};

class regex_error : public std::runtime_error {
 public:
  regex_error(const std::string& msg, const std::string& expr)
      : std::runtime_error(msg), m_expr(expr) {}

  ~regex_error() throw() {}

  std::string m_expr;
};

class regex {
 public:
  virtual ~regex();

  bool match(const std::string& str);
  bool match(const char* begin, const char* end);

  bool search(const std::string& str);
  bool search(const char* begin, const char* end);

  virtual match_range what(int n) = 0;

 private:
  virtual bool do_match(const char* begin, const char* end) = 0;
  virtual bool do_search(const char* begin, const char* end) = 0;
};

typedef std::unique_ptr<regex> regex_ptr;

regex_ptr compile_regex(const std::string& expr, int caps = 0,
                        const char* flags = "");

std::string escape_regex(const std::string& text);

#endif
