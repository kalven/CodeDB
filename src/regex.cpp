// CodeDB - public domain - 2010 Daniel Andersson

#include "regex.hpp"
#include "nsalias.hpp"

#ifdef CDB_USE_RE2
#include <re2/re2.h>
#include <cstring>
#else
#include <boost/xpressive/xpressive_dynamic.hpp>
#endif

// regex base class ----------

regex::~regex()
{
}

bool regex::match(const std::string& str)
{
    const char* p = str.c_str();
    return do_match(p, p + str.size());
}

bool regex::match(const char* begin, const char* end)
{
    return do_match(begin, end);
}

bool regex::search(const std::string& str)
{
    const char* p = str.c_str();
    return do_search(p, p + str.size());
}

bool regex::search(const char* begin, const char* end)
{
    return do_search(begin, end);
}

namespace // regex implementations
{
#ifdef CDB_USE_RE2 // re2 regex -----------------
    class re2_regex : public regex
    {
      public:
        static const int max_caps = 3;

        re2_regex(const char* expr, const re2::RE2::Options& opts, int caps)
          : m_regex(expr, opts)
          , m_caps_count(caps)
        {
            if(!m_regex.ok())
                throw regex_error(m_regex.error(),
                                  std::string(expr+1, std::strlen(expr) - 2));

            for(int i = 0; i != m_caps_count; ++i)
            {
                m_args[i] = re2::RE2::Arg(&m_caps[i]);
                m_pargs[i] = &m_args[i];
            }
        }

      private:

        match_range what(int n)
        {
            return match_range(m_caps[n].data(),
                               m_caps[n].data() + m_caps[n].size());
        }

        bool do_match(const char* begin, const char* end)
        {
            re2::StringPiece text(begin, static_cast<int>(end - begin));
            return re2::RE2::FullMatchN(text, m_regex, m_pargs, m_caps_count);
        }

        bool do_search(const char* begin, const char* end)
        {
            re2::StringPiece text(begin, static_cast<int>(end - begin));
            return re2::RE2::PartialMatchN(text, m_regex, m_pargs, m_caps_count);
        }

        re2::StringPiece m_caps[max_caps];
        re2::RE2::Arg    m_args[max_caps];
        re2::RE2::Arg*   m_pargs[max_caps];
        re2::RE2         m_regex;
        int              m_caps_count;
    };

    regex_ptr compile_re2(const char* expr, int caps, const char* flags)
    {
        re2::RE2::Options opts;

        opts.set_log_errors(false);
        opts.set_never_nl(true);

        for(; *flags; ++flags)
        {
            if(*flags == 'i')
                opts.set_case_sensitive(false);
        }

        if(caps > re2_regex::max_caps)
            throw std::runtime_error("Internal error: caps > max_caps");

        return regex_ptr(new re2_regex(expr, opts, caps));
    }

#else // xpressive regex -----------

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
        xpressive_regex(const char* expr, const char* flags)
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

    regex_ptr compile_xpressive(const char* expr, const char* flags)
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
}

// ---------------------------

regex_ptr compile_regex(const std::string& expr, int caps, const char* flags)
{
#ifdef CDB_USE_RE2
    return compile_re2(('(' + expr + ')').c_str(), caps + 1, flags);
#else
    return compile_xpressive(expr.c_str(), flags);
#endif
}

std::string escape_regex(const std::string& text)
{
    std::string res;

    for(auto i = text.begin(); i != text.end(); ++i)
    {
        switch(*i)
        {
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
