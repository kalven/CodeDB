// CodeDB - public domain - 2010 Daniel Andersson

#include "config.hpp"
#include "options.hpp"

#include <boost/array.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace bxp = boost::xpressive;

namespace
{
    const boost::array<std::string, 2> s_keys =
    {
        {
            "dir-exclude",
            "file-include"
        }
    };

    void validate_key(const std::string& key)
    {
        if(std::find(s_keys.begin(), s_keys.end(), key) == s_keys.end())
            throw std::runtime_error("No config key named '" + key + "'");
    }
}

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
        "dir-exclude=(\\.codedb|\\.git|\\.svn|_darcs)\n"
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

void config::set_value(const std::string& key, const std::string& value)
{
    validate_key(key);
    m_cfg[key] = value;
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

void run_config(const boost::filesystem::path& cdb_path, const options& opt)
{
    config cfg = load_config(cdb_path / "config");

    std::size_t max_size = 0;
    for(unsigned i = 0; i != s_keys.size(); ++i)
        max_size = std::max(max_size, s_keys[i].size());

    switch(opt.m_args.size())
    {
        case 0:
            for(unsigned i = 0; i != s_keys.size(); ++i)
            {
                std::cout << std::left << std::setw(max_size)
                          << s_keys[i] << " = " << cfg.get_value(s_keys[i]) << std::endl;
            }
            break;
        case 1:
            validate_key(opt.m_args[0]);
            std::cout << opt.m_args[0] << " = " << cfg.get_value(opt.m_args[0]) << std::endl;
            break;
        case 2:
            cfg.set_value(opt.m_args[0], opt.m_args[1]);
            save_config(cfg, cdb_path / "config");
            std::cout << "Config updated" << std::endl;
            break;
    }
}
