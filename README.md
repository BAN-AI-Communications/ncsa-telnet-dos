# NCSA Telnet for MS-DOS

## Overview

* **NCSA Telnet** (v*2.3.08*) for *MS-DOS*

## Bugs Fixed

* Help screen keys limited to avoid paramater menu.
* Fixed redraw for console when no open connections.
* Command-line usage display: added newlines.
* *BOOTP* repaired to not crash packet drivers.
* `GIN` repair patch added.
* Improper fragmentation identification fixed (*Solaris* users will notice).
* *Net14* uses `port=xx` option in `config.tel`.
* `Ftpbin` cursor repositions after shell out.
* More `GIN` tuning
* `Ftpbin` control messages all shown.
* `^Z`s removed from *ASCII* transfers
* `Ftpbin` internals (better) fixed.
* UDP portlist structure minimized.
* *FTP* `mget` response fixed.
* *Alt-T* reloads keymap.
* `telpkt.exe` binary added - packet driver only version (*smaller*)

## Building

* **NCSA Telnet** is compiled with *Microsoft C/C++ 7.0* and *MASM 5.1*.

## License

* **NCSA Telnet** is _**Public Domain** software_.
