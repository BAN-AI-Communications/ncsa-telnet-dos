These are the Makefile and the changed files needed to make NCSA Telnet
2.308 compile under Borland C++ 3.1. The sources were originally built
with MSC 6.0. I have verified that it will read the config file and
create a telnet session, but I have not tested it extensively. The fact
that it ran after the compile completed was a surprise for me. I had
expected it would crash the system first time.

I have only selected a minimal configuration: packet driver only, no mouse
and no Tektronix emulation, so not all the code is has been exercised. Let
me know if you find any deviations from the original behaviour. Anyway
packet drivers are what you should use instead of the built-in drivers.

If you are wondering why I am bothering with this old program, I want
to make it run standalone without DOS so that it can be dowmloaded via
the network to diskless machines.

Ken Yap
August 1998
ken_yap@hotmail.com
