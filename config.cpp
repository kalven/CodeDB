// CodeDB - public domain - 2010 Daniel Andersson

#include "config.hpp"
#include "options.hpp"
#include "regex.hpp"

#include <boost/array.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace bxp = boost::xpressive;

namespace
{
    struct cfg_key
    {
        enum type
        {
            regex
        };

        std::string m_name;
        type        m_type;
    };
    
    const boost::array<cfg_key, 2> s_keys =
    {
        {
            { "dir-exclude", cfg_key::regex },
            { "file-include", cfg_key::regex }
        }
    };

    const cfg_key& require_key(const std::string& name)
    {
        for(auto i = s_keys.begin(); i != s_keys.end(); ++i)
            if(i->m_name == name)
                return *i;
        throw std::runtime_error("No config key named '" + name + "'");
    }

    void validate_value(const cfg_key& k, const std::string& value)
    {
        if(k.m_type == cfg_key::regex)
            // compile_regex will throw if the regex is invalid
            compile_sregex(value, bxp::regex_constants::ECMAScript);
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
    require_key(key);

    auto i = m_cfg.find(key);
    if(i == m_cfg.end())
        return "";
    return i->second;
}

void config::set_value(const std::string& name, const std::string& value)
{
    validate_value(require_key(name), value);
    m_cfg[name] = value;
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

    switch(opt.m_args.size())
    {
        case 0:
        {
            std::size_t max_size = 0;
            for(auto i = s_keys.begin(); i != s_keys.end(); ++i)
                max_size = std::max(max_size, i->m_name.size());

            for(auto i = s_keys.begin(); i != s_keys.end(); ++i)
            {
                std::cout << std::left << std::setw(max_size)
                          << i->m_name << " = " << cfg.get_value(i->m_name) << std::endl;
            }
            break;
        }
        case 1:
        {
            std::string value = cfg.get_value(opt.m_args[0]);
            std::cout << opt.m_args[0] << " = " << value << std::endl;
            break;
        }
        case 2:
            cfg.set_value(opt.m_args[0], opt.m_args[1]);
            save_config(cfg, cdb_path / "config");
            std::cout << "Config updated" << std::endl;
            break;
    }
}
