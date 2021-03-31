@echo off
echo Wiping the teldev directories...
del *.err *.~* >& nul
cd ftp
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\engine
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\debug
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\include
del *.obj *.~* *.bak *.lob *.fnd *.err *.tmp        >& nul
cd ..\net\enet
del *.obj *.~* *.bak *.lob *.cod *.fnd *.err *.tmp  >& nul
cd ..\..\vs
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\rg
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
echo ...
cd ..\ftp
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\minitel
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\lpr
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\tek
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\finger
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\rexec
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\setclock
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\rsh
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\telpass
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\net14
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
cd ..\whois
del *.obj *.~* *.bak *.cod *.lob *.fnd *.err *.tmp  >& nul
rem cd ..\lint
rem del *.~* *.lob del *.err                            >& nul
cd ..\lib
del *.~* *.bak *.lib *.lst *.lob *.fnd *.err *.tmp  >& nul
copy msmouse.l msmouse.lib >& nul
del *.exe *.map *.obj                               >& nul
cd ..
