# NCSA Telnet for MS-DOS

## Overview

- **NCSA Telnet** for _MS-DOS_
  - **Version 2.3.08** (_April 20th 1996_)

## Bugs Fixed

- Help screen keys limited to avoid paramater menu.
- Fixed redraw for console when no open connections.
- Command-line usage display: added newlines.
- _BOOTP_ repaired to not crash packet drivers.
- `GIN` repair patch added.
- Improper fragmentation identification fixed (_Solaris_ users will notice).
- _Net14_ uses `port=xx` option in `config.tel`.
- `Ftpbin` cursor repositions after shell out.
- More `GIN` tuning
- `Ftpbin` control messages all shown.
- `^Z`s removed from _ASCII_ transfers
- `Ftpbin` internals (better) fixed.
- UDP portlist structure minimized.
- _FTP_ `mget` response fixed.
- _Alt-T_ reloads keymap.
- `telpkt.exe` binary added - packet driver only version (_smaller_)

## Building

- **NCSA Telnet** is compiled with _Microsoft C/C++ 7.0_ and _MASM 5.1_.

## License

- **NCSA Telnet** is _**Public Domain** software_.
