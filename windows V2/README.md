# filewatch
filewatch monitors for any changes in a directory and recompiles code automatically.
It is a tool for helping developers with compilation of small projects.

`filewatch -c 'command' -r 'program to run'`

To watch a list of files, use [filelist](https://github.com/scarface382/filelist)

`filelist | grep -v -E "exe|git|txt" | filewatch -c make -r demo.exe`

This command will remove exe, git, and txt files from the list and the rest of the files will be watched by `filewatch`

## Example

### Entering files to watch
![Alt text](examples/3.png?raw=true "file watch")

### Error in the code, gcc error message
![Alt text](examples/4.png?raw=true "file watch")

### Issue fixed, make is called by filewatch automatically
![Alt text](examples/5.png?raw=true "file watch")