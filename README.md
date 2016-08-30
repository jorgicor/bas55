Intro
=====

This is the `README` file for `bas55`, the *Minimal BASIC System* distribution.

`bas55` is an editor, byte compiler and interpreter for the *Minimal BASIC*
programming language as described by the [ECMA standard 55][1].

bas55 is free software. See the file `COPYING` for copying conditions.

Copyright (c) 2014, 2015, 2016 Jorge Giner Cordero

Home page: http://jorgicor.sdfeu.org/bas55  
Send bug reports to: jorge.giner@hotmail.com

Windows precompiled binaries
============================

A precompiled distribution for *Microsoft Windows* is provided in a ZIP file.
It can be found at http://jorgicor.sdfeu.org/bas55 . After decompressing it,
you will have these files:

~~~
bas55.exe	The program.
bas55.html	The manual.
readme.txt	This file.
copying.txt	License.
changelog.txt	What's new in this version.
news-old.txt	Historic file documenting changes in previous releases.
sieve.bas	The sieve of Eratosthenes algorithm in Minimal BASIC.
~~~

Double-click `bas55.exe` or open a system console and go to the folder where
you have decompressed the ZIP file and type `bas55`. Then type `HELP` to
see a list of available commands.  Double-click `bas55.html` to see the
program documentation in your web browser.

Compiling
=========

Getting the code from revision control
--------------------------------------

If you cloned the project from a revision control system (i.e. GitHub), you
will need first to use the GNU autotools to generate some files, in particular,
the `configure` script. Use:

    $ ./bootstrap

to generate the required files. You will need *GNU autoconf*, *GNU automake*,
*GNU texinfo*, *help2man*, and a *yacc* parser (tested only with *GNU bison*).

Compiling from the source distribution
--------------------------------------

If you have the official source package, and you are building on a Unix
environment (this includes *Cygwin* on *Windows*), you can find detailed
instructions in the file `INSTALL`. The complete source distribution can always
be found at http://jorgicor.sdfeu.org/bas55 .  

After installing, you can type `man bas55` to see a brief explanation on how to
use the `bas55` program.  More detailed documentation, including a tutorial on
the *Minimal BASIC* programming language, can be found using the GNU
documentation system: type `info bas55` to read it.

Additionally, a sample *Minimal BASIC* program called `sieve.bas`, containing
the sieve of Eratosthenes algorithm, is included in the source distribution.

Normally, after installing from source, you can find this on your system:

~~~
/usr/local/bin/bas55			The program executable.
/usr/local/share/man/man1/bas55.1	The manual page.
/usr/local/share/info/bas55.info	The info manual.
/usr/local/share/doc/bas55/COPYING	License.
/usr/local/share/doc/bas55/README.md	This file.
/usr/local/share/doc/bas55/CHANGELOG.md	What's new in this version.
/usr/local/share/doc/bas55/NEWS.old	Historic NEWS file.
/usr/local/share/doc/bas55/ChangeLog	Historic ChangeLog file.
/usr/local/share/bas55/sieve.bas	The sieve of Eratosthenes algorithm.
~~~

If you are installing bas55 using your OS distribution package system, these
folders will probably be different.  Try changing `/usr/local` to `/usr`.

[1]: http://www.ecma-international.org/publications/standards/Standardwithdrawn.htm
[2]: http://jorgicor.sdfeu.org/bas55  
