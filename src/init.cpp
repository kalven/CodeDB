// CodeDB - public domain - 2010 Daniel Andersson

#include "init.hpp"
#include "config.hpp"
#include "nsalias.hpp"

#include <boost/filesystem/operations.hpp>

void init()
{
    bfs::path cdb_path = bfs::initial_path() / ".codedb";
    if(bfs::exists(cdb_path))
    {
        std::cerr << cdb_path << " already exists\n";
        return;
    }

    bfs::create_directory(cdb_path);
    save_config(config::default_config(), cdb_path / "config");

    std::cout << "empty db initialized in " << cdb_path << std::endl;
}
