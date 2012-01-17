# CodeDB

CodeDB is a tool for quickly searching through large amounts of source code.

## Design goals

CodeDB was designed to handle millions of lines of code. It does this by
building an index which is used for searching. The index is compressed and gives
CodeDB two advantages over tools such as grep and ack:

 * The source tree is not scanned for every search. The cost of scanning
   thousands of directories and files can easily dominate a grep. This is
   especially problematic on Windows.

 * The index can be searched in parallel. CodeDB defaults to launching as many
   worker threads as there are cores.

## Benchmarks

The following are some benchmarks comparing CodeDB with GNU grep. The data set
is the Linux-3.1.6 source tree which consists of about 365MB of source spread
out over 11.6 million lines and 30,252 files. The first test platform is a
laptop with an Intel Core i7 @ 1.73GHz running Linux 3.1. The second test
platform is a workstation with an Intel W3530 @ 2.80GHz running Windows 7. The
regex `int(8|16|32|64)_t` is used for searching and yields 40,577 hits.

![charts](https://github.com/kalven/CodeDB/raw/master/img/benchmark.png)

## Usage

CodeDB is a command line application so open up your favorite terminal and
navigate to the root directory of the code you want to index.

    $ cd /home/user/project
    $ cdb init
    empty db initialized in /home/user/project/.codedb

CodeDB creates a directory called `.codedb` which it uses for configuration and
the index. We are now ready to configure CodeDB.

    $ cdb config
    dir-exclude  = (\.codedb|\.git|\.svn|\.hg|_darcs)
    file-include = .*?\.(h|hpp|inl|c|cpp)

The `config` command shows the current configuration. The `dir-exclude` and
`file-include` keys are both regular expressions which control what files are
added to the index. Right now we only wish to index .c and .h files:

    $ cdb config file-include ".*?\.[hc]"
    Config updated

We are now ready to build our index. This is done with the `build` command.

    $ cdb build

CodeDB has now built its index and is ready to serve queries. Let's find all
lines with an `#include` directive.

    $ cdb find ^#include
    <... lots of lines>

By default, CodeDB will search files based on where it is invoked. If we
navigate to a sub directory and use the `find` command there, only files from
that directory and down will be considered.

    $ cd subdir
    $ cdb find ^#include
    <... fewer lines>
    
## Licenses

CodeDB is public domain.

Source code in ext/snappy is:
  Copyright 2011, Google Inc.
  All rights reserved.

See ext/snappy/COPYING for details.
