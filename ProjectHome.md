proccache is a output cache for Oracle's [Pro\*C](http://download.oracle.com/docs/cd/B19306_01/appdev.102/b14407/toc.htm) pre-compiler. It uses a similar idea like [ccache](http://ccache.samba.org). This tool requires [GLib](http://library.gnome.org/devel/glib/) library (2.16 or later version) and [pkg-config](http://pkg-config.freedesktop.org/wiki/) for compilation.

### Installation ###
  1. wget http://proccache.googlecode.com/files/proccache-0.0.1.tar.gz
  1. tar xvzf proccache-0.0.1.tar.gz
  1. cd proccache
  1. make
  1. make install

### Usage ###
Just add proccache before every Pro\*C pre-compiler execution, for example:
```
proccache proc demo.pc
```

### Tested on ###
  * FreeBSD 7.2, gcc 4.2.1, glib 2.20.4
  * SLES 10 SP2 (x86\_64), gcc 4.1.2, glib 2.20.4
