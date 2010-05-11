// CodeDB - public domain - 2010 Daniel Andersson

#include "config.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>

#include <sstream>

namespace bxp = boost::xpressive;

void config::load(std::istream& in)
{
    static const bxp::sregex re(
        bxp::sregex::compile("([a-z\\-]+)=(.*)"));

    bxp::smatch what;

    std::string line;
    while(getline(in, line))
    {
        if(line.empty() || line[0] == '#')
            continue;
        if(regex_match(line, what, re))
            m_cfg.insert(std::make_pair(what[1], what[2]));
    }
}

void config::save(std::ostream& out) const
{
    for(auto i = m_cfg.begin(); i != m_cfg.end(); ++i)
        out << i->first << '=' << i->second << std::endl;
}

config config::default_config()
{
    static const std::string config_str =
        "dir-exclude=(\\.svn|\\.codedb)\n"
        "file-include=.*?\\.(hpp|cpp)\n";

    std::istringstream is(config_str);
    config c;
    c.load(is);

    return c;
}

std::string config::get_value(const std::string& key)
{
    auto i = m_cfg.find(key);
    if(i == m_cfg.end())
        return "";
    return i->second;
}

void save_config(const config& c, const boost::filesystem::path& p)
{
    boost::filesystem::ofstream out(p);
    c.save(out);
}

config load_config(const boost::filesystem::path& p)
{
    boost::filesystem::ifstream in(p);

    config c;
    c.load(in);

    return c;
}
