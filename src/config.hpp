// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_CONFIG_HPP
#define CODEDB_CONFIG_HPP

#include "nsalias.hpp"

#include <boost/filesystem/path.hpp>

#include <iostream>
#include <string>
#include <map>

struct options;

class config {
 public:
  void load(std::istream&);
  void save(std::ostream&) const;

  static config default_config();

  std::string get_value(const std::string& key) const;
  void set_value(const std::string& key, const std::string& value);

 private:
  typedef std::map<std::string, std::string> cfg_map;

  cfg_map m_cfg;
};

void save_config(const config&, const bfs::path&);
config load_config(const bfs::path&);

void run_config(const bfs::path& cdb_path, const options& opt);

#endif
