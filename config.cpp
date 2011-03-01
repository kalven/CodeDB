// CodeDB - public domain - 2010 Daniel Andersson

#include "config.hpp"
#include "options.hpp"
#include "regex.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <cctype>
#include <map>

namespace
{
    // The value validators throw an exception if the parameter
    // doesn't fall into the range of allowed values.

    void validate_regex(const std::string& value)
    {
        compile_sregex(value, bxp::regex_constants::ECMAScript);
    }

    void validate_bool(const std::string& value)
    {
        if(value != "on" && value != "off")
            throw std::logic_error("'" + value + "' is not a valid boolean, expected 'on' or 'off'");
    }

    void validate_port(const std::string& value)
    {
        static const bxp::sregex re(
            bxp::sregex::compile("\\d{1,5}"));

        if(!regex_match(value, re) || boost::lexical_cast<int>(value) > 65535)
            throw std::logic_error("'" + value + "' is not a valid port number");
    }

    struct cfg_key
    {
        typedef void (*validator)(const std::string&);

        cfg_key(const std::string& def, validator val)
          : m_default(def)
          , m_validator(val)
        {
        }

        std::string m_default;
        validator   m_validator;
    };
    
    typedef std::map<std::string, cfg_key> config_keys;

    const config_keys& get_config_map()
    {
        static config_keys keys;
        if(keys.empty())
        {
            keys.insert(std::make_pair("dir-exclude", cfg_key("(\\.codedb|\\.git|\\.svn|\\.hg|_darcs)", &validate_regex)));
            keys.insert(std::make_pair("file-include", cfg_key(".*?\\.(h|hpp|inl|c|cpp)", &validate_regex)));
            keys.insert(std::make_pair("nocase-file-match", cfg_key("off", &validate_bool)));
            keys.insert(std::make_pair("build-trim-ws", cfg_key("on", &validate_bool)));
            keys.insert(std::make_pair("find-trim-ws", cfg_key("off", &validate_bool)));
            keys.insert(std::make_pair("serve-port", cfg_key("8080", &validate_port)));
        }

        return keys;
    }

    const cfg_key& require_key(const std::string& name)
    {
        const config_keys& keys = get_config_map();

        auto i = keys.find(name);
        if(i == keys.end())
            throw std::runtime_error("No config key named '" + name + "'");

        return i->second;
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
    config c;

    const config_keys& keys = get_config_map();
    for(auto i = keys.begin(); i != keys.end(); ++i)
        c.m_cfg.insert(std::make_pair(i->first, i->second.m_default));

    return c;
}

std::string config::get_value(const std::string& name)
{
    const cfg_key& key = require_key(name);

    auto i = m_cfg.find(name);
    if(i == m_cfg.end())
        return key.m_default;
    return i->second;
}

void config::set_value(const std::string& name, const std::string& value)
{
    require_key(name).m_validator(value);
    m_cfg[name] = value;
}

void save_config(const config& c, const bfs::path& p)
{
    bfs::ofstream out(p);
    c.save(out);
}

config load_config(const bfs::path& p)
{
    bfs::ifstream in(p);

    config c;
    c.load(in);

    return c;
}

void run_config(const bfs::path& cdb_path, const options& opt)
{
    config cfg = load_config(cdb_path / "config");

    switch(opt.m_args.size())
    {
        case 0:
        {
            std::size_t max_size = 0;
            const config_keys& keys = get_config_map();

            for(auto i = keys.begin(); i != keys.end(); ++i)
                max_size = std::max(max_size, i->first.size());

            for(auto i = keys.begin(); i != keys.end(); ++i)
            {
                std::cout << std::left << std::setw(max_size)
                          << i->first << " = " << cfg.get_value(i->first) << std::endl;
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
