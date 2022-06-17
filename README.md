# raid-dbg

## Introduction

raid is a GDB-like tiny debugger which is based on ptrace.

## Build

Before everything start, we'll need to clone submodules
[linenoise](https://github.com/antirez/linenoise) and
[elfutils](http://sourceware.org/git/elfutils.git) first.

```
$ git submodule update --init
```

We may need to install some dependencies to build elfutils, and you'll need to
move `libdw.so` to the top level of repository after build. All of these works can be done
using the following script simply. But I'll recommand you to take a look at what will be
installed after running the script first.

```
$ ./scripts/install-libdw.sh
```

Then we can build the debugger with all of the dependencies included.
```
$ make
```

Finally, it's time to play with raid now! Debugging your executable with
the following command.
```
$ ./build/raid executable
```

## Command

raid has been implemented a small set of command, which can let you do some simple
trace on the debuggee.

Command     | Description
------------|------------------
help        | List all of the supported command of raid
cont        | restart tracee until it hits a breakpoint or finishes execution
break       | Set a breakpoint on tracee
step        | Step in to the next line of source file
next        | Step over to the next line of current function
backtrace   | Unwind stack for the chain of function calls which leads to the current point
print       | Print current value of a variable or register
quit        | End the execution of debugger

## Reference

* [headcrab-rs/headcrab](https://github.com/headcrab-rs/headcrab)
* [bet4it/gdbserver](https://github.com/bet4it/gdbserver)
* [Writing a Linux Debugger](https://blog.tartanllama.xyz/writing-a-linux-debugger-setup/)
* [How debuggers work](https://eli.thegreenplace.net/2011/01/23/how-debuggers-work-part-1)
