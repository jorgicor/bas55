Change Log
==========

Important changes on this project are documented in this file.

*Source changes* documents the changes that may be of interest for people
obtaining the source code distribution.

*What's new* documents the changes that may be of interest for people obtaining
only the binary distribution.

1.11 - Unreleased
-----------------

### What's new

- This file now documents the latest changes in the program. It combines the
  `NEWS` and `ChangeLog` files that we were using before.

### Source changes

- Prepared for VPATH builds. This includes changes in the tests. Now it is
  possible to compile the program in a folder different from the source folder.
  Still some tools generate files in `srcdir` (*textinfo*) but that seems the
  accepted behaviour.
- Added `bootstrap` script to regenerate the *autotools* files for people
  cloning the project from revision control.
- Renamed `ecma/` folder to `src/` . 
- Changed in *configure.ac* from `gnu` to `foreign`. Removed README and added a
  better README.md.
- Added `-Wall -Wdeclaration-after-statement` to the C compiler if allowed.
  Included the macro `m4/m4_ax_check_compile_flag.m4` to check if the compiler
  supports that.
- Fixed all tests to work on *Windows*. Now we don't take into account
  end of line differences. Still some tests don't pass due to implementation
  diferences in the math functions.
- Removed NEWS file. Information included in this file. Renamed `ChangeLog` to
  `ChangeLog.old`.

1.10 - 2016-08-09
-----------------

### What's new

- Updated copyright notices to 2016.

### Source changes

- The configure script and Makefiles are generated from autoconf 2.69
  and automake 1.15.
- For the p117.test, we only check if the test passed, we do not check for
  individual line changes, because depending on the C standard library used we
  can get different results.
- Including mkwin script to build on Windows.

1.09 - 2016-01-23
-----------------

### What's new

- Bug fixed: a program that called `RANDOMIZE` was generating the same sequence
  if the program was executed several times in the same second.  Thanks to
  *John Gatewood Ham* for reporting.

1.08 - 2016-01-04
-----------------

### What's new

- Bug fixed: do not accept a program with lines after an `END` statement.
  Thanks to *John Gatewood Ham* for reporting.

### Source changes

- Added automatic tests for the NBS basic tests and some others. `make check`
  runs the tests.

1.07 - 2015-11-23
-----------------

### What's new

- The fixed error in the previous version introduced a serious bug with `TAB`,
  as it expected a space after it. Fixed. Thanks to *John Gatewood Ham* for
  reporting.

1.06 - 2015-11-15
-----------------

### What's new

- Issue errors when a keyword is not followed by a space. For example,
  `PRINT"HELLO"` is not accepted now; it must be `PRINT "HELLO"`. This is
  required by the standard. Thanks to *John Gatewood Ham* for reporting.

1.05 - 2015-11-03
-----------------

### What's new

- Bug fix: `READ` had a bug for arrays with two dimensions: the data was read
  in the wrong slot. Thanks to *John Gatewood Ham* for reporting this serious
  bug.

1.04 - 2015-09-04
-----------------

### What's new

- Bug fix: running a program that uses `DATA` statements more than once in edit
  mode, now behaves correctly. Thanks to *John Gatewood Ham* for reporting.

1.03 - 2015-08-31
-----------------

### What's new

- Catching `Ctrl+C` signal to break running programs instead of aborting.
- Fixed reseting the print column to zero after input operations.
- Implemented debug mode to warn about variables that are used before they are
  assigned a value. Documented.
- Increased the print margin to 80 characters.
- Fixed precision when reading numbers in some cases.
- Better error messages.
- Documented implementation-defined features.

1.02 - 2015-05-11
-----------------

### What's new

- Paths passed to `LOAD` and `SAVE` were changed to upper case. It was
  impossible on Unix systems to specify a lower case path.  Fixed.
- When a file was loaded using `LOAD`, the spaces between the line number and
  the actual instructions were discarded. Fixed to allow indentation.
- Fixed some compiling warnings that could cause problems.

### Source changes

- The source distribution has been fixed and improved to compile correctly on
  Unix systems (not only on Cygwin).
