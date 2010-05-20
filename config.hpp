// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_CONFIG_HPP
#define CODEDB_CONFIG_HPP

#include <iostream>

#include <string>
#include <map>

#include <boost/filesystem.hpp>

struct options;

class config
{
  public:
    void load(std::istream&);
    void save(std::ostream&) const;

    static config default_config();

    std::string get_value(const std::string& key);
    void set_value(const std::string& key, const std::string& value);

  private:
    typedef std::map<std::string,std::string> cfg_map;

    cfg_map m_cfg;
};

void save_config(const config&, const boost::filesystem::path&);
config load_config(const boost::filesystem::path&);

void run_config(const boost::filesystem::path& cdb_path, const options& opt);

#endif
