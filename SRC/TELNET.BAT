@echo off
rem If there are no arguments, then telnet is started with default options.
if .%1 == . goto defh
telbin %1 %2 %3 %4 %5 %6
goto exith
:defh
rem Here the default option is to connect to the machine uxa.
telbin uxa
:exith
echo.
