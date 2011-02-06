# CodeDB

CodeDB is a set of tools for quickly searching through large amounts of code.

## Why not use grep, ack or something else?

Performance! The code base that I wrote CodeDB for consists of 1.9 MLOC spread out over 11000 files. CodeDB can search through this database in about 0.2 seconds. The same operation can easily take minutes using grep or ack.

### How can it be that fast?

CodeDB works by building a set of index files. The search operation uses only these files, the original files are not used. Tools that search directly in the source tree have to scan thousands of files and this comes at a great performance impact, especially on Windows.

### So I need to keep the database updated?

Yes! I usually set up an automated task that does this every hour. The database is incredibly simple and this operation is quite fast.

## Usage

CodeDB is a command line application so open up your favorite terminal and navigate to the root directory of the code you want to index.

    $ cd /home/user/project
    $ cdb init
    empty db initialized in /home/user/project/.codedb

CodeDB creates a directory called `.codedb` which it uses for configuration and the index. We are now ready to configure CodeDB.

    $ cdb config
    dir-exclude  = (\.codedb|\.git|\.svn|\.hg|_darcs)
    file-include = .*?\.(h|hpp|inl|c|cpp)

The `config` command shows the current configuration. The `dir-exclude` and `file-include` keys are both regular expressions which control what files are added to the index. Right now we only wish to index .c and .h files:

    $ cdb config file-include ".*?\.[hc]"
    Config updated

We are now ready to build our index. This is done with the `build` command.

    $ cdb build

CodeDB has now built its index and is ready to serve queries. Let's find all lines with an `#include` directive.

    $ cdb find ^#include
    <... lots of lines>

By default, CodeDB will search files based on where it is invoked. If we navigate to a sub directory and use the `find` command there, only files from that directory and down will be considered.

    $ cd subdir
    $ cdb find ^#include
    <... fewer lines>
    
## License

CodeDB is public domain.
