#
#   Borland C makefile for NCSA Telnet 2.3
#

I	= ..\include
CC	= bcc -c -ml -Od -DTELBIN -DPKT_ONLY -DNOTEK -DMSC -I\bc31\include -I$(I)

MM	= tasm -mx -dMicrosoft -I..\..\include

LINKER	= bcc -ml -l-c -lm -P -L\bc31\lib
LINKLINE= ncsaio.obj ncsabrk.obj look.obj telnet.lib tcp.lib vs.lib sess.lib enet.lib msmouse.lib

telbin.exe : engine\look.obj engine\ncsaio.obj engine\ncsabrk.obj lib\telnet.lib lib\sess.lib lib\tcp.lib lib\vs.lib lib\enet.lib lib\msmouse.lib
	cd lib
	$(LINKER) $(LINKLINE)
	copy ncsaio.exe ..\telbin.exe
	del ncsaio.exe
	cd ..

net\enet\net.obj : net\enet\net.c include\protocol.h include\data.h
	cd net\enet
	$(CC) /I..\..\include net.c
	cd ..\..
	tlib lib\enet.lib -+net\enet\net

engine\ip.obj : engine\ip.c include\protocol.h include\data.h
	cd engine
	$(CC) ip.c
	cd ..
	tlib lib\tcp.lib -+engine\ip

engine\bootp.obj : engine\bootp.c include\protocol.h include\data.h include\bootp.h include\windat.h include\hostform.h
	cd engine
	$(CC) bootp.c
	cd ..
	tlib lib\sess.lib -+engine\bootp

engine\user.obj : engine\user.c include\protocol.h include\data.h
	cd engine
	$(CC) user.c
	cd ..
	tlib lib\tcp.lib -+engine\user

engine\mouse.obj : engine\mouse.c include\mouse.h
	cd engine
	$(CC) mouse.c
	cd ..
#	tlib lib\telnet.lib -+engine\mouse

engine\rspc.obj : engine\rspc.c include\windat.h include\vskeys.h include\whatami.h
	cd engine
	$(CC) rspc.c
	cd ..
	tlib lib\telnet.lib -+engine\rspc

engine\tools.obj : engine\tools.c include\protocol.h include\data.h
	cd engine
	$(CC) /DSERVICES tools.c
	cd ..
	tlib lib\tcp.lib -+engine\tools

engine\udp.obj : engine\udp.c include\protocol.h include\data.h
	cd engine
	$(CC) udp.c
	cd ..
	tlib lib\tcp.lib -+engine\udp

engine\arp.obj : engine\arp.c include\protocol.h include\data.h
	cd engine
	$(CC) arp.c
	cd ..
	tlib lib\tcp.lib -+engine\arp

engine\domain.obj : engine\domain.c include\protocol.h include\data.h
	cd engine
	$(CC) domain.c
	cd ..
	tlib lib\tcp.lib -+engine\domain

engine\bkgr.obj : engine\bkgr.c include\hostform.h include\whatami.h
	cd engine
	$(CC) bkgr.c
	cd  ..
	tlib lib\sess.lib -+engine\bkgr

engine\util.obj : engine\util.c include\whatami.h include\hostform.h
	cd engine
	$(CC) util.c
	cd ..
	tlib lib\sess.lib -+engine\util

engine\tcp.obj : engine\tcp.c include\protocol.h include\data.h
	cd engine
	$(CC) tcp.c
	cd ..
	tlib lib\tcp.lib -+engine\tcp

engine\pcutil.obj : engine\pcutil.c include\whatami.h
	cd engine
	$(CC) pcutil.c
	cd ..
	tlib lib\sess.lib -+engine\pcutil

engine\protinit.obj : engine\protinit.c include\protocol.h include\data.h
	cd engine
	$(CC) protinit.c
	cd ..
	tlib lib\tcp.lib -+engine\protinit

engine\look.obj : engine\look.c include\windat.h include\whatami.h include\nkeys.h include\hostform.h include\data.h include\telopts.h
	cd engine
	$(CC) look.c
	copy look.obj ..\lib
	cd ..

engine\negotiat.obj : engine\negotiat.c include\telopts.h
	cd engine
	$(CC) negotiat.c
	cd ..
	tlib lib\telnet.lib -+engine\negotiat

engine\linemode.obj : engine\linemode.c include\telopts.h
	cd engine
	$(CC) linemode.c
	cd ..
	tlib lib\telnet.lib -+engine\linemode

engine\scrlback.obj : engine\scrlback.c include\windat.h include\whatami.h include\nkeys.h include\hostform.h
	cd engine
	$(CC) scrlback.c
	cd ..
	tlib lib\telnet.lib -+engine\scrlback

engine\services.obj : engine\services.c
	cd engine
	$(CC) services.c
	cd ..
	tlib lib\telnet.lib -+engine\services

engine\menu.obj : engine\menu.c include\windat.h include\whatami.h include\nkeys.h include\hostform.h
	cd engine
	$(CC) menu.c
	cd ..
	tlib lib\telnet.lib -+engine\menu

engine\confile.obj : engine\confile.c include\whatami.h include\hostform.h include\confile.h
	cd engine
	$(CC) confile.c
	cd ..
	tlib lib\telnet.lib -+engine\confile

vs\vsinterf.obj : vs\vsinterf.c include\vsdata.h include\vsinit.h include\vskeys.h
	cd vs
	$(CC) vsinterf.c
	cd ..
	tlib lib\vs.lib -+vs\vsinterf

vs\vsem.obj : vs\vsem.c include\vsdata.h include\vsinit.h include\vskeys.h
	cd vs
	$(CC) vsem.c
	cd ..
	tlib lib\vs.lib -+vs\vsem

vs\vsintern.obj : vs\vsintern.c include\vsdata.h include\vsinit.h include\vskeys.h
	cd vs
	$(CC) vsintern.c
	cd ..
	tlib lib\vs.lib -+vs\vsintern

rg\rg0.obj : rg\rg0.c
	cd rg
	$(CC) rg0.c
	cd ..
	tlib lib\tek.lib -+rg\rg0

rg\rge.obj : rg\rge.c
	cd rg
	$(CC) rge.c
	cd ..
	tlib lib\tek.lib -+rg\rge

rg\rgv.obj : rg\rgv.c
	cd rg
	$(CC) rgv.c
	cd ..
	tlib lib\tek.lib -+rg\rgv

rg\rgh.obj : rg\rgh.c
	cd rg
	$(CC) rgh.c
	cd ..
	tlib lib\tek.lib -+rg\rgh

rg\rg9.obj : rg\rg9.c
	cd rg
	$(CC) rg9.c
	cd ..
	tlib lib\tek.lib -+rg\rg9

rg\rgp.obj : rg\rgp.c
	cd rg
	$(CC) rgp.c
	cd ..
	tlib lib\tek.lib -+rg\rgp

rg\rgc.obj : rg\rgc.c
	cd rg
	$(CC) rgc.c
	cd ..
	tlib lib\tek.lib -+rg\rgc

rg\rgep.obj : rg\rgep.c
	cd rg
	$(CC) rgep.c
	cd ..
	tlib lib\tek.lib -+rg\rgep

rg\rghp.obj : rg\rghp.c
	cd rg
	$(CC) rghp.c
	cd ..
	tlib lib\tek.lib -+rg\rghp

tek\vgtek.obj : tek\vgtek.c
	cd tek
	$(CC) vgtek.c
	cd ..
	tlib lib\tek.lib -+tek\vgtek

tek\tekstor.obj : tek\tekstor.c
	cd tek
	$(CC) tekstor.c
	cd ..
	tlib lib\tek.lib -+tek\tekstor

rg\egaset.obj : rg\egaset.asm
	cd rg
	$(MM) egaset.asm
	cd ..
	tlib lib\tek.lib -+rg\egaset

rg\vgaset.obj : rg\vgaset.asm
	cd rg
	$(MM) vgaset.asm
	cd ..
	tlib lib\tek.lib -+rg\vgaset

net\enet\netbicc.obj : net\enet\netbicc.c include\protocol.h include\data.h
	cd net\enet
	$(CC) /I..\..\include netbicc.c
	cd ..\..
	tlib lib\enet.lib -+net\enet\netbicc

net\enet\netbicc2.obj : net\enet\netbicc2.asm
	cd net\enet
	$(MM) netbicc2.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\netbicc2

net\enet\packet.obj : net\enet\packet.c include\windat.h include\whatami.h include\packet.h
	cd net\enet
	$(CC) /I..\..\include packet.c
	cd ..\..
	tlib lib\enet.lib -+net\enet\packet

net\enet\packet2.obj : net\enet\packet2.asm
	cd net\enet
	$(MM) packet2.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\packet2

net\enet\dndll.obj : net\enet\dndll.c include\protocol.h include\decnet.h
	cd net\enet
	$(CC) /I..\..\include dndll.c
	cd ..\..
	tlib lib\enet.lib -+net\enet\dndll

net\enet\dndll2.obj : net\enet\dndll2.asm
	cd net\enet
	$(MM) dndll2.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\dndll2

net\enet\netatt10.obj : net\enet\netatt10.asm
	cd net\enet
	$(MM) netatt10.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\netatt10

net\enet\net3com.obj : net\enet\net3com.asm
	cd net\enet
	$(MM) net3com.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\net3com

net\enet\net501.obj : net\enet\net501.asm
	cd net\enet
	$(MM) net501.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\net501

net\enet\nov3com.obj : net\enet\nov3com.asm
	cd net\enet
	$(MM) nov3com.asm
	cd ..\..
#       tlib lib\enet.lib -+net\enet\nov3com

net\enet\net503.obj : net\enet\net503.asm
	cd net\enet
	$(MM) net503.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\net503

net\enet\net505.obj : net\enet\net505.asm
	cd net\enet
	$(MM) net505.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\net505

net\enet\net9210.obj : net\enet\net9210.asm
	cd net\enet
	$(MM) net9210.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\net9210

net\enet\ltalk.obj : net\enet\ltalk.c include\kip.h
	cd net\enet
	$(CC) /I..\..\include ltalk.c
	cd ..\..
	tlib lib\enet.lib -+net\enet\ltalk

net\enet\ltlisten.obj : net\enet\ltlisten.asm
	cd net\enet
	$(MM) ltlisten.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\ltlisten

net\enet\net523.obj : net\enet\net523.asm
	cd net\enet
	$(MM) net523.asm
	cd..\..
	tlib lib\enet.lib -+net\enet\net523

net\enet\netub.obj : net\enet\netub.asm
	cd net\enet
	$(MM) netub.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\netub

net\enet\net5210.obj : net\enet\net5210.asm
	cd net\enet
	$(MM) net5210.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\net5210

net\enet\net8003.obj : net\enet\net8003.asm
	cd net\enet
	$(MM) net8003.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\net8003

net\enet\net8003a.obj : net\enet\net8003a.asm
	cd net\enet
	$(MM) net8003a.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\net8003a

net\enet\netzyp.obj : net\enet\netzyp.asm
	cd net\enet
	$(MM) netzyp.asm
	cd ..\..
	tlib lib\enet.lib -+net\enet\netzyp

engine\ipasm.obj : engine\ipasm.asm
	cd engine
	$(MM) ipasm.asm
	cd ..
	tlib lib\tcp.lib -+engine\ipasm

engine\ncsaio.obj : engine\ncsaio.asm
	cd engine
	$(MM) ncsaio
	copy ncsaio.obj ..\lib
	cd ..

engine\ncsabrk.obj : engine\ncsabrk.asm
	cd engine
	$(MM) ncsabrk
	copy ncsabrk.obj ..\lib
	cd ..

#debug\memdebug.obj : debug\memdebug.c include\memdebug.h
#	cd debug
#	$(CC) /I..\include memdebug.c
#	cd ..
#	tlib lib\debug.lib -+debug\memdebug

#debug\maldebug.obj : debug\maldebug.c include\maldebug.h
#	cd debug
#	$(CC) maldebug.c
#	cd ..
#	tlib lib\debug.lib -+debug\maldebug

#debug\strdebug.obj : debug\strdebug.c include\strdebug.h
#	cd debug
#	$(CC) strdebug.c
#	cd ..
#	tlib lib\debug.lib -+debug\strdebug

#debug\mcb.obj : debug\mcb.c
#	cd debug
#	$(CC) /I..\include mcb.c
#	cd ..
#	tlib lib\debug.lib -+debug\mcb

engine\keymap.obj : engine\keymap.c include\keymap.h
	cd engine
	$(CC) /I..\include keymap.c
	copy keymap.obj ..\lib
	cd ..
	tlib lib\telnet.lib -+engine\keymap

engine\map_out.obj : engine\map_out.c include\map_out.h
	cd engine
	$(CC) /I..\include map_out.c
	copy map_out.obj ..\lib
	cd ..
	tlib lib\telnet.lib -+engine\map_out

engine\scr_rest.obj : engine\scr_rest.c include\vidinfo.h
	cd engine
	$(CC) /I..\include scr_rest.c
	copy scr_rest.obj ..\lib
	cd ..
	tlib lib\telnet.lib -+engine\scr_rest

#engine\translat.obj : engine\translat.asm
#	cd engine
#	$(MM) translat
#	copy translat.obj ..\lib
#	cd ..

# Library dependencies.
#   Split up to fit into DOS command line

#lib\debug.lib: debug\memdebug.obj debug\maldebug.obj debug\strdebug.obj debug\mcb.obj
#	tlib lib\debug.lib +debug\memdebug.obj +debug\maldebug.obj +debug\strdebug.obj +debug\mcb.obj

lib\telnet.lib: engine\mouse.obj engine\rspc.obj engine\negotiat.obj engine\linemode.obj engine\scrlback.obj engine\services.obj engine\menu.obj engine\confile.obj engine\keymap.obj engine\map_out.obj engine\scr_rest.obj
	tlib lib\telnet.lib +engine\mouse.obj +engine\rspc.obj +engine\negotiat.obj
	tlib lib\telnet.lib +engine\linemode.obj +engine\scrlback.obj +engine\services.obj
	tlib lib\telnet.lib +engine\menu.obj +engine\confile.obj +engine\keymap.obj +engine\map_out.obj +engine\scr_rest.obj

lib\enet.lib: net\enet\net.obj net\enet\netbicc.obj net\enet\netbicc2.obj net\enet\packet.obj net\enet\packet2.obj net\enet\netatt10.obj net\enet\net3com.obj net\enet\net501.obj net\enet\nov3com.obj net\enet\net503.obj net\enet\net505.obj net\enet\net9210.obj net\enet\ltalk.obj net\enet\ltlisten.obj net\enet\net523.obj net\enet\netub.obj net\enet\net5210.obj net\enet\net8003.obj net\enet\net8003a.obj net\enet\netzyp.obj
	tlib lib\enet.lib +net\enet\net.obj +net\enet\netbicc.obj +net\enet\netbicc2.obj
	tlib lib\enet.lib +net\enet\packet.obj +net\enet\packet2.obj +net\enet\dndll.obj
	tlib lib\enet.lib +net\enet\dndll2.obj +net\enet\netatt10.obj +net\enet\net3com.obj
	tlib lib\enet.lib +net\enet\net501.obj +net\enet\nov3com.obj +net\enet\net503.obj
	tlib lib\enet.lib +net\enet\net505.obj +net\enet\net9210.obj +net\enet\ltalk.obj
	tlib lib\enet.lib +net\enet\ltlisten.obj +net\enet\net523.obj +net\enet\netub.obj
	tlib lib\enet.lib +net\enet\net5210.obj +net\enet\net8003.obj +net\enet\net8003a.obj +net\enet\netzyp.obj

lib\tcp.lib: engine\ip.obj engine\user.obj engine\tools.obj engine\udp.obj engine\arp.obj engine\domain.obj engine\tcp.obj engine\protinit.obj engine\ipasm.obj
	tlib lib\tcp.lib +engine\ip.obj +engine\user.obj +engine\tools.obj +engine\udp.obj
	tlib lib\tcp.lib +engine\arp.obj +engine\domain.obj +engine\tcp.obj +engine\protinit.obj
	tlib lib\tcp.lib +engine\ipasm.obj

lib\vs.lib: vs\vsinterf.obj vs\vsem.obj vs\vsintern.obj
	tlib lib\vs.lib +vs\vsinterf.obj +vs\vsem.obj +vs\vsintern.obj

lib\sess.lib: engine\bootp.obj engine\bkgr.obj engine\util.obj engine\pcutil.obj
	tlib lib\sess.lib +engine\bootp.obj +engine\bkgr.obj +engine\util.obj +engine\pcutil.obj

lib\tek.lib: rg\rg0.obj rg\rge.obj rg\rgv.obj rg\rgh.obj rg\rg9.obj rg\rgp.obj rg\rgc.obj rg\rgep.obj rg\rghp.obj tek\vgtek.obj tek\tekstor.obj rg\egaset.obj rg\vgaset.obj
	tlib lib\tek.lib +rg\rg0.obj +rg\rge.obj +rg\rgv.obj +rg\rgh.obj +rg\rg9.obj
	tlib lib\tek.lib +rg\rgp.obj +rg\rgc.obj +rg\rgep.obj +rg\rghp.obj +tek\vgtek.obj
	tlib lib\tek.lib +tek\tekstor.obj +rg\egaset.obj +rg\vgaset.obj
