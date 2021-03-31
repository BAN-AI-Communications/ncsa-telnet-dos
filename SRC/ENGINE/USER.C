/*
*  USER.C
*  Network library interface routines
*  Generally called by the session layer
*
*	User interface routines 
*
*****************************************************************************
*																			*
*	  part of:																*
*	  TCP/IP kernel for NCSA Telnet											*
*	  by Tim Krauskopf														*
*																			*
*	  National Center for Supercomputing Applications						*
*	  152 Computing Applications Building									*
*	  605 E. Springfield Ave.												*
*	  Champaign, IL  61820													*
*																			*
*     This program is in the public domain.                                 *
*                                                                           *
****************************************************************************
*  Revisions:
***************************************************************************
*
*	Revision history:
*
*	10/87  Initial source release, Tim Krauskopf
*	2/88  typedef support for other compilers (TK)
*	5/88	clean up for 2.3 release, JKM	
*
*/

#define MASTERDEF 1								/* add user variables */

/*#define DEBUG*/
#include "debug.h"

/*
 *	INCLUDES
 */

#include <stdio.h>
#include <stdarg.h>
#include "protocol.h"
#include "data.h"
#include "externs.h"
#include "defines.h"

#define LOWWATER 600

/***************************************************************************/
/*
*	netread ( pnum, buffer, n )
*
*	Read data from the connection buffer (specified by *pnum*) into the
*	user buffer for up to *n* bytes.
*
*	Returns number of bytes read, or<0 if error.  Does not block.
*
*/
int netread(int pnum,char *buffer,int n)
{
	int howmany,i,lowwater;
	struct port *p;

	if(pnum<0)			/* check validity */
		return(-2);

	if(NULL==(p=portlist[pnum]))
		return(-2);

  if(p->state!=SEST) {          /* foreign or netclose */
    if(p->state==SCWAIT) {      /* ready for me to close my side? */
      if(!p->in.contain) {
        p->tcpout.t.flags=TFIN | TACK;
        tcpsend(p,0);
        p->state=SLAST;
        return(-1);
      } /* end if */
    /* else, still data to be read */
    } /* end if */
		else
			return(-1);
  } /* end if */

	howmany=dequeue(&p->in,buffer,n);			/* read from tcp buffer */
  i=p->in.size;                         /* how much before? */
  p->in.size+=howmany;                  /* increment leftover room  */

	lowwater=p->credit>>1;
  if((i<lowwater) && ((p->in.size)>=(uint)lowwater))   /* we passed mark */
		p->out.lasttime=0L;

  if(p->in.contain)                     /* if still data to be read */
		netputuev(CONCLASS,CONDATA,pnum);		/* don't forget it */
	return(howmany);
}

#ifndef NET14
/**********************************************************************
*  Function :   netprintf
*  Purpose  :   Write printf()-like formatted data into an output queue.
*                   uses netwrite to actually get the data out.
*  Parameters	:
*           pnum - the port to write the data out to.
*           fmt - character string specifying the format
*  Returns  :   the number of bytes sent, or <0 if an error
*  Calls    :   netwrite()
*  Called by    :   all over...
**********************************************************************/
int netprintf(int pnum,char *fmt,...)
{
    char temp_str[256];         /* this may be a problem, if the string is too long */
	va_list arg_ptr;
    int str_len;

	va_start(arg_ptr,fmt);		/* get a pointer to the variable arguement list */
    str_len=vsprintf(temp_str,fmt,arg_ptr); /* print the formatted string into a buffer */
	va_end(arg_ptr);
    if(str_len>0)      /* good string to transmit */
        return(netwrite(pnum,temp_str,str_len));
    else
        return(-3);
}   /* end netprintf() */
#endif

/************************************************************************/
/*
*	netwrite ( pnum, buffer, n )
*	Write data into the output queue (specified by *pnum*).  netsleep will
* come around and distribute the data onto the wire later.
*
*	Returns number of bytes sent or<0 if an error.
*
*/
int
#if defined(NET14)
CDECL
#endif
netwrite(int pnum,char *buffer,int n)
{
	int nsent,before;
	struct port *p;

	if(pnum<0)
		return(-2);

	if((p=portlist[pnum])==NULL)
		return(-2);

  if(p->state!=SEST)      /* must be established connection */
		return(-1);

  before=p->out.contain;
    nsent=enqueue(&p->out,buffer,n);

	if(!before) {					/* if this is something new, */
    p->out.lasttime=0L; /* cause timeout to be true */
		p->out.push=1;			/* set the push flag */
  } /* end if */

	return(nsent);
}

/**************************************************************************/
/*
*	netpush ( pnum )
*
*	Sets the push bit on the specified port data, and returns whether the
* queue is empty or how many bytes in the queue. <0 if an error.
*
*/
int netpush(int pnum)
{
	struct port *p;

	if(pnum<0)
		return(-2);

	if(NULL==(p=portlist[pnum]))
		return(-2);

	p->out.push=1;
	return((int)p->out.contain);
}	

#ifndef NET14
/**************************************************************************/
/*
*	netqlen ( pnum )
*
*	Returns the number of bytes waiting to be read from the incoming queue
* or<0 if error.
*
*/
int netqlen(int pnum)
{
	if(portlist[pnum]==NULL)
		return(-2);

	return((int)portlist[pnum]->in.contain);
}

/**************************************************************************/
/*
*	netroom ( pnum )
*
*	Returns how many bytes left in the output buffer for port *pnum*. <0 if
* error.
*
*/
int netroom(int pnum)
{
	if(portlist[pnum]==NULL || portlist[pnum]->state!=SEST)
		return(-2);

	return((int)(WINDOWSIZE-portlist[pnum]->out.contain));
}

/**************************************************************************/
/*
*	netsegsize ( newsize )
*	Set the segment size.
*
*	Returns the old size.
*
*/
int netsegsize(int newsize)
{
	int i;

	i=nnsegsize;
	nnsegsize=newsize;
	return(i);
}
#endif

/*
*	netarptime ( t )
*
*	Set the dlayer timeout ( in seconds )
*
*/
void netarptime(int t)                  /* dlayer timeout in secs */
{
	nndto=t;
}

/*
*	netsetip ( st )
*
*	Set a new ip number.  This goes through and changes all sorts of ip
* numbers.
*
*	This routine assumes that there are currently NO open tcp connections.
*
*/
void netsetip(unsigned char *st)
{
/*
*  change all dependent locations relating to the IP number
*  don't worry about open connections, they must be closed by higher layer
*/
	movebytes(nnipnum,st,4);						/* main ip number */
	movebytes(arp.spa,nnipnum,4);					/* arp */
	movebytes(blankip.i.ipsource,nnipnum,4);		/* ip source */
    movebytes(udplist[DNRINDEX]->tcps.source,nnipnum,4);         /* udp source - eej */
    movebytes(udplist[DNRINDEX]->udpout.i.ipsource,nnipnum,4);   /* more udp source - eej */
}

/*
*	netgetip ( st )
*
*	Sets *st* to the current up number 
*
*/
void netgetip(unsigned char *st)
{
	movebytes(st,nnipnum,4);
}

/*
*	netsetbroad ( st )
*
*	Set the network broadcast IP address.
*
*/
void netsetbroad(unsigned char *st)
{
	movebytes(broadip,st,4);
}

/*
*	netsetmask ( st )
*
*	Set the network mask.
*
*/
void netsetmask(unsigned char *st)
{
	movebytes(nnmask,st,4);
}

/*
*	netgetmask ( st )
*
*	Get the network mask.
*
*/
void netgetmask(unsigned char *st)
{
	movebytes(st,nnmask,4);
}


#if !defined NET14 || defined NETSPACE || defined WIN   /* rmg  hahaha! */
/*
*	netfromport ( port )
*
*	This sets the port that the next open will use to be *port*
*
*/
void netfromport(int16 port)
{
	nnfromport=port;
}

/**************************************************************************/
/*
*	netest ( pn ) 
*
*	Checks to see if a particular session has been established yet.
*
*	Returns 0 if the connection is in "established" state.
*
*/
int netest(int pn)
{
	struct port *p;

	if(pn<0 || pn>NPORTS)
		return(-2);

	if(NULL==(p=portlist[pn]))
		return(-2);

	if(p->state==SEST)
		return(0);
	else
		if(p->state==SCWAIT) {
			if(!p->in.contain) {
				p->tcpout.t.flags=TFIN | TACK;
				tcpsend(p,0);
				p->state=SLAST;
				return(-1);
			  }	/* end if */
			else 
				return(0);				/* still more data to be read */
		  }	/* end if */
	return(-1);
}

/**************************************************************************/
/*
*	netlisten ( serv )
*
*   Listen to a TCP port number and make the connection automatically when
*   the SYN packet comes in.  The TCP layer will notify the higher layers
*   with a CONOPEN event.  Save the port number returned to refer to this
*   connection.
*
*	example usage : portnum=netlisten ( service )
*
*	Returns<0 if error 
*
*/
int netlisten(uint serv)
{
	int	pnum;
	struct port *prt;
	uint16 nn;

	if((pnum=makeport())<0)
		return(-2);

	if(NULL==(prt=portlist[pnum]))
		return(-2);

	prt->in.port=serv;
	prt->out.port=0;						/* accept any outside port #*/
    prt->in.lasttime=n_clicks();            /* set time we started */
	prt->state=SLISTEN;
	prt->credit=512;						/* default value until changed */
	prt->tcpout.i.protocol=PROTTCP;
	prt->tcpout.t.source=intswap(serv);		/* set service here too */

/*
*  install maximum segment size which will be sent out in the first
*  ACK-SYN packet
*/
	prt->tcpout.x.options[0]=2;
	prt->tcpout.x.options[1]=4;
/* install maximum segment size */
	nn=intswap(nnsegsize);
	movebytes((char *)&prt->tcpout.x.options[2],(char *)&nn,2);
	return(pnum);
}

/***********************************************************************/
/*
*	netgetftp ( a, pnum )
*
*	This routine provides the information needed to open an ftp connection
* back to the originatior of the command connection.  The other side's IP
* number and the port numbers are returned in an INTEGER array ( convenient
* for use in PORT commands ).
*
*/
void netgetftp(int a[],int pnum)
{
	struct port *p;
	uint i;

	p=portlist[pnum];

	a[0]=p->tcpout.i.ipdest[0];
	a[1]=p->tcpout.i.ipdest[1];
	a[2]=p->tcpout.i.ipdest[2];
	a[3]=p->tcpout.i.ipdest[3];
	i=intswap(p->tcpout.t.source);
	a[4]=i>>8;
	a[5]=i & 255;
	i=intswap(p->tcpout.t.dest);
	a[6]=i>>8;
	a[7]=i & 255;
}
#endif

/**************************************************************************/
/*
*	netxopen ( machine, service, rto, mtu, mseg, mwin )
*
*	Open a network socket for the user to *machine* using port *service*.
* The rest of the parameters are self-explanatory.
*
*/
int netxopen(uint8 *machine,uint service,uint rto,uint mtu,uint mseg,uint mwin)
{
	struct port *p;
	int pnum,ret,i;
	uint8 *pc,*hiset;

/*
*  check the IP number and don't allow broadcast addresses
*/
#ifdef OLD_WAY
	if(machine[3]==255 || !machine[3]) {
		nnerror(506);
		return(-4);
	  }
#else
    if(machine[3]==255) {
		nnerror(506);
		return(-4);
	  }
#endif

	netsleep(0);					/* make sure no waiting packets */

	if((pnum=makeport())<0)			/* set up port structure and packets */
		return(-3);

	p=portlist[pnum];				/* create a new port */
/*
*  make a copy of the ip number that we are trying for
*/
	movebytes(p->tcpout.i.ipdest,machine,4);
	movebytes(p->tcps.dest,machine,4);		/* pseudo header needs it */

/*
*  get the hardware address for that host, or use the one for the gateway
*  all handled by 'netdlayer' by ARPs.
*/

	pc=netdlayer(machine);					/* we have ether? */
	if(pc==NULL) {							/* cannot connect to local machine */
		nnerror(504);
		return(-2);
	  }

	movebytes(p->tcpout.d.dest,pc,DADDLEN);		/* load it up */

/*
*   Add in machine specific settings for performance tuning
*/
	if(rto>=MINRTO)
		p->rto=rto;			/* starting retrans timeout */
	if(mtu<=TMAXSIZE)		/* largest packet space we have */
		p->sendsize=mtu;	/* maximum transmit size for that computer */
	if(mwin<=WINDOWSIZE)		/* buffer size is the limit */
		p->credit=mwin;		/* most data that we can receive at once */

#ifdef OLD_WAY
	if(nnemac) {
/*
*   quick check to see if someone else is using your IP number
*   Some boards receive their own broadcasts and cannot use this check.
*   The Mac will only use this check if it is using EtherTalk.
*/
		i=cachelook(nnipnum,0,0);				/* don't bother to ARP */
		if(i >= 0)	{				/* if it is not -1, we are in trouble */
			hiset=(uint8 *)arpc[i].hrd;
			pc=neterrstring(-1);
			sprintf(pc,"Conflict with Ethernet hardware address: %2x:%2x:%2x:%2x:%2x:%2x",
			hiset[0],hiset[1],hiset[2],hiset[3],hiset[4],hiset[5]);
			nnerror(-1);
			nnerror(102);
			netclose(pnum);
			return(-3);
		  }
	  }
#endif

/*
*  make the connection, if you can, we will get an event notification later
*  if it connects.  Timeouts must be done at a higher layer.
*/
	ret=doconnect(pnum,service,mseg);
	return(ret);
}

/**********************************************************************/
/*
*	doconnect ( pnum, service, mseg )
*
*	This routine sends the actual packet out to try and establish a
* connection.
*
*/
doconnect(int pnum,int service,int mseg)
{
	uint16 seg;
	struct port *p;

	p=portlist[pnum];

	p->tcpout.i.protocol=PROTTCP;			/* this will be TCP socket */
	p->tcpout.t.dest=intswap(service);		/* for example, telnet=23 */
	p->out.port=service;					/* service is same as port num*/
	p->tcpout.t.flags=TSYN;					/* want to start up sequence */
	p->tcpout.t.ack=0;						/* beginning has no ACK */
	p->state=SSYNS;							/* syn sent */
/*
*  install maximum segment size which will be sent out in the first
*  ACK-SYN packet
*/
	p->tcpout.x.options[0]=2;
	p->tcpout.x.options[1]=4;

/* install maximum segment size */
	seg=intswap(mseg);
	movebytes((char *)&p->tcpout.x.options[2],(char *)&seg,2);
	p->tcpout.t.hlen=96;					/* include one more word in hdr */
	tcpsend(p,4);							/* send opening volley */
	p->tcpout.t.hlen=80;					/* normal hdr len */

/*	savenxt=p->out.nxt; */
    p->out.nxt+=1;                          /* ack should be for next byte */
	return(pnum);							/* do not wait for connect */
}

/*************************************************************************/
/*
*	netopen2 ( pnum )
*
*	Send out repeat SYN on a connection which is not open yet.  This checks
* to make sure if it is actually needed.  Timing is handled at a higher
* layer.
*
*	Returns 1 if the state is still at SYNS other wise 0 if connection is
* proceeding.
* 
*/
int netopen2(int pnum)
{
	struct port *p;

	if(pnum<0 || pnum>NPORTS)
		return(-1);

	if(NULL==(p=portlist[pnum]))
		return(-2);

	if(p->state != SSYNS)
			return(0);				/* done our job */
/*
*  The connection has not proceeded to further states, try retransmission
*/
	p->out.nxt--;
	p->tcpout.t.hlen=96;		/* include one more word in hdr */
	tcpsend(p,4);				/* try sending again */
	p->tcpout.t.hlen=80;		/* normal hdr len */
	p->out.nxt++;
	return(1);
}

/**************************************************************************/
/*
*	netclose ( pnum )
*
*	Start the closing process on port pnum.
*
*/
int
#if defined(NET14)
CDECL
#endif
netclose(int pnum)
{
	struct port *p;

	if(pnum<0 || pnum>NPORTS)			/* is a valid port? */
		return(-1);

	if((p=portlist[pnum])!=NULL) {		/* something there */

/*
*printf("p->state=%d\n",p->state);
*/
		switch (p->state) {
			case SLISTEN:				/* we don't care anymore */
			case SSYNS:
				p->state=SCLOSED;
				break;

			case SEST:					/* must initiate close */
				/* send FIN */
				p->tcpout.t.flags=TACK | TFIN;
				tcpsend(p,0);
				p->state=SFW1;			/* wait for ACK of FIN */
				break;					/* do nothing for now ?*/

			case SCWAIT:				/* other side already closed */
				p->tcpout.t.flags=TFIN | TACK;
				tcpsend(p,0);
				p->state=SLAST;
				break;

			case STWAIT:				/* time out yet? */
                if(portlist[pnum]->out.lasttime + WAITTIME<n_clicks())
					p->state=SCLOSED;
				break;

			case SLAST:					/* five minute time out */
                if(portlist[pnum]->out.lasttime + LASTTIME<n_clicks())
					p->state=SCLOSED;
				break;

			default:
				break;
		  }
	  }
	else
		return(1);
	return(0);
}

/**************************************************************************/
/*
*	netinit ()
*
*	Handles all the initialization to bring up the network connection.
*	Assumes that the configuration file has already been read up.
* (called from Snetinit () )
*
*	Returns 0 on successful initialization.
*
*/
int netinit(void )
{
	int ret;

	BUG("in netinit");
/*
*   Initializes all buffers and hardware for data link layer.
*   Machine/board dependent.
*/
    ret=dlayerinit();
    BUG("after dlayerinit");
	if(ret) {
		switch (ret) {
			case -10:
				printf("Need a function for opening board!!\n");
				break;

			default:
				printf("Board initialization failed!.  Error code=%d\n",ret);
				break;
		  }	/* end switch */
		nnerror(101);
		return(ret);
	  }	/* end if */
/*
*  initialize the template packets needed for transmission
*/
    protinit();             /* set up empty packets */
    return(0);
}

/*************************************************************************/
/*
*	netshut ()
*
*	Shut down the hardware.  This tries to close any active connections and
* then turn off the hardware ( via dlayershut )
*
*/
void netshut()
{
	int i;

	for(i=0; i<NPORTS ; i++) 
		if(portlist[i]!=NULL)
			netclose(i);
	netsleep(1);
	dlayershut();
}

