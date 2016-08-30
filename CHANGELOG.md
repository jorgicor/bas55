Change Log
==========

Important changes on this project are documented in this file.

*Source changes* documents the changes that may be of interest for people
obtaining the source code distribution.

*What's new* documents the changes that may be of interest for people obtaining
only the binary distribution.

1.11 - Unreleased
-----------------

### What's new on this version

- This file now documents the latest changes in the program. It combines the
  `NEWS` and `ChangeLog` files that we were using before. These files are kept
  for historic purposes but have been renamed to `NEWS.old` and
  `ChangeLog.old`.

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

