# CodeDB

CodeDB is a set of tools for quickly searching through large amounts of code.

## Why not use grep, ack or something else?

Performance! The codebase that I wrote CodeDB for consists of 1.9 MLOC spread out over 11000 files. CodeDB can search through this database in about 0.2 seconds. The same operation can easily take minutes using grep or ack.

### How can it be that fast?

CodeDB works by building a set of index files. The search operation uses only these files, the original files are not used. Tools that search directly in the source tree have to scan thousands of files and this comes at a great performance impact, especially on Windows.

### So I need to keep the database updated?

Yes! I usually set up an automated task that does this every hour. The database is incredibly simple and this operation is quite fast.

## License

CodeDB is public domain.
