# proccache

proccache is a output cache for Oracle's Pro*C pre-compiler. It uses a similar idea like ccache. This tool requires GLib library (2.16 or later version) and pkg-config for compilation.

## Installation
```
git clone https://github.com/eg/proccache.git
cd proccache
make
make install
```

## Usage
Just add proccache before every Pro*C pre-compiler execution, for example:
```
proccache proc demo.pc
```

## Tested on
FreeBSD 7.2, gcc 4.2.1, glib 2.20.4

SLES 10 SP2 (x86_64), gcc 4.1.2, glib 2.20.4
