/*
 *
 *    Virtual Screen Kernel Interface
 *            (vsinterf.c)
 *
 *    by Gaige B. Paulsen
 *
 *  This file contains the control and interface calls for the NCSA
 *  Virtual Screen Kernal.
 *
 *    VSinit(maxscreens)                - Initialize the VSK
 *    VSnewscreen(maxlines,scrnsave)    - Initialize a new screen.
 *    VSdetatch(w)                      - Detach screen w
 *    VSredraw(w,x1,y1,x2,y2)           - redraw region for window w
 *    VSwrite(w,ptr,len)                - write text @ptr, length len
 *    VSdirectwrite(w,ptr,len)          - write text @ptr, length len (w/o terminal interpretation)
 *    VSclear(w)                        - clear w's real screen
 *    VSkbsend(w,k,echo)                - send keycode k's rep. out window w (w/echo if req.)
 *    VSclearall(w)                     - clear w's real and saved screen
 *    VSreset(w)                        - reset w's emulator (as per TERM)
 *    VSgetline(w,y)                    - get a ptr to w's line y
 *    VSsetrgn(w,x1,y1,x2,y2)           - set local display region
 *    VSscrolback(w,n)                  - scrolls window w back n lines
 *    VSscrolforward(w,n)               - scrolls window w forward n lines
 *    VSscrolleft(w,n)                  - scrolls window w left  n columns
 *    VSscrolright(w,n)                 - scrolls window w right n columns
 *    VSscrolcontrol(w,scrlon,offtop)   - sets scroll vars for w
 *    VSgetrgn(w,&x1,&y1,&x2,&y2)       - returns set region
 *    VSsnapshot(w)                     - takes a snapshot of w
 *    VSgetlines(w)                     - Returns current # of lines
 *    VSsetlines(w, lines)              - Sets the current # of lines to lines
 *  
 *      Version Date    Notes
 *      ------- ------  ---------------------------------------------------
 *      0.01    861102  Initial coding -GBP
 *      0.10    861113  Added some actual program to this file -GBP
 *      0.15    861114  Initiated Kludge Operation-GBP
 *      0.50    8611VSPBOTTOM  Parameters added to VSnewscreen -GBP
 *      0.90    870203  Added the kbsend routine -GBP
 *      2.1     871130  NCSA Telnet 2.1 -GBP
 *      2.2     880715  NCSA Telnet 2.2 -GBP
 *      2.3     900630  NCSA Telnet 2.3 -QAK
 *
 */

#define VSMASTER

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#ifdef MSC
#include <malloc.h>
#endif
#include "vsdata.h"
#include "vskeys.h"
#include "vsinit.h"
#include "externs.h"

#ifdef DEBUGCGW
    int VSckconsist(int out, int verbose);
#endif

#define VS_UNUSED       0
#define VS_ACTIVE       1
#define VS_DETACHED     2

int VSmax=0,VSinuse=0;  /* Internal variables for use in managing windows */
VSscrndata *VSscreens;

int VSinit(int max)
{
    int i;

    RSinitall();
    VSmax=max;
    VSIwn=0;
    if((VSscreens=(VSscrndata *)malloc(max*sizeof(VSscrndata)))==NULL) 
        return(-2);
    for(i=0; i<max; i++) {
        VSscreens[i].loc=NULL;
        VSscreens[i].stat=VS_UNUSED;
      }

    return(0);
}

#ifdef NOT_USED
VSscrn *VSwhereis(int i)
{
    VSvalids(i);
    return(VSIw);
}
#endif

int VSnewscreen(int maxlines,int screensave,int maxwid,int IDC)
{
    int i;
    VSline *tt,*last;

    if(maxlines<VSDEFLINES)
        maxlines=VSDEFLINES;
    if(VSinuse>=VSmax) 
        return(-1);
    VSIwn=0;
    while((VSIwn<VSmax) && (VSscreens[VSIwn].stat==VS_ACTIVE))
        VSIwn++;
    if(VSIwn>=VSmax) 
        return(-1);
    if(VSscreens[VSIwn].stat==VS_DETACHED) {
        VSIw=VSscreens[VSIwn].loc;
        if(VSIw==NULL) 
            return(-7);
      } /* end if */
    else
        if((VSscreens[VSIwn].loc=VSIw=(VSscrn *)malloc(sizeof(VSscrn)))==NULL) {
            VSscreens[VSIwn].loc=NULL;
            return(-2);
          } /* end if */
    if(VSscreens[VSIwn].stat!=VS_DETACHED) {
        VSIw->maxlines=maxlines;
        VSIw->numlines=VSDEFLINES;
      } /* end if */
    VSIw->maxwidth=maxwid-1;
    VSIw->allwidth=maxwid-1;
    VSIw->savelines=screensave;
    VSIw->attrib=0;
    VSIw->x=0;
    VSIw->y=0;
    VSIw->charset=0;
    VSIw->G0=0;
    VSIw->G1=1;
    VSIw->VSIDC=IDC;
    VSIw->DECAWM=0;
    VSIw->DECCKM=0;
    VSIw->DECPAM=0;
    VSIw->IRM=0;
    VSIw->escflg=0;
    VSIw->top=0;
    VSIw->bottom=23;
    VSIw->parmptr=0;
    VSIw->Rtop=0;
    VSIw->Rleft=0;
    VSIw->Rright=79;
    VSIw->Rbottom=23;
    VSIw->ESscroll=1;
    VSIw->lines=23;
    VSIw->localprint=0;     /* set local printing to be off initially */
    EXCHECK((VSIw->linest=(VSline **)malloc(sizeof(VSline *)*(VSIw->lines+1)))==NULL,linestAllocFailed);
    EXCHECK((VSIw->attrst=(VSline **)malloc(sizeof(VSline *)*(VSIw->lines+1)))==NULL,attrstAllocFailed);
    VSinuse++;
    if(VSscreens[VSIwn].stat==VS_DETACHED) {
        VSscreens[VSIwn].stat=VS_ACTIVE;
        VSiclrbuf();
        VSItabinit();
        return(VSIwn);
      } /* end if */
    VSscreens[VSIwn].stat=VS_ACTIVE;
    EXCHECK((VSIw->tabs=(char *)malloc(maxwid))==NULL,tabsAllocFailed);
    EXCHECK((last=(VSline *)malloc(sizeof(VSline)))==NULL,buftopAllocFailed);
    VSIw->buftop=last;
    EXCHECK((last->text=(char *)malloc(maxwid))==NULL,textAllocFailed);
    memset(last->text,' ',maxwid);      /* clear line to blanks */
    last->next=NULL;
    last->prev=NULL;
    for(i=0; i<=(VSPBOTTOM+VSDEFLINES); i++) {
        EXCHECK((tt=(VSline *)malloc(sizeof(VSline)))==NULL,textinfoAllocFailed);
        if((tt->text=(char *)malloc(maxwid))==NULL) {
            free(tt);
            goto textinfoAllocFailed;
          } /* end if */
        memset(tt->text,' ',maxwid);        /* clear line to blanks */
        tt->next=NULL;
        tt->prev=last;
        last->next=tt;
        last=tt;
      } /* end for */
    VSIw->scrntop=last;
    for(i=VSPBOTTOM; i>0; i--) 
        VSIw->scrntop=VSIw->scrntop->prev;  /* Go to right place... */

    EXCHECK((last=(VSline *)malloc(sizeof(VSline)))==NULL,attrtopAllocFailed);
    VSIw->attrst[0]=last;
    EXCHECK((last->text=(char *)malloc(maxwid))==NULL,attrAllocFailed);
    memset(last->text,0,maxwid);        /* clear attributes to none */
    last->next=NULL;
    last->prev=NULL;
    for(i=0; i<=VSPBOTTOM; i++) {
        EXCHECK((tt=(VSline *)malloc(sizeof(VSline)))==NULL,attrinfoAllocFailed);
        if((tt->text=(char *)malloc(maxwid))==NULL) {
            free(tt);
            goto attrinfoAllocFailed;
          } /* end if */
        memset(tt->text,0,maxwid);      /* clear attributes to none */
        tt->next=NULL;
        tt->prev=last;
        last->next=tt;
        last=tt;
      } /* end for */
    VSIlistndx(VSIw->scrntop, VSIw->attrst[0]);         /* Assign lists */
    VSIw->attrst[0]->prev=VSIw->attrst[VSPBOTTOM];  /* Make attr circ. */
    VSIw->attrst[VSPBOTTOM]->next=VSIw->attrst[0];
#ifdef OLD_WAY
    VSiclrbuf();
#endif
    VSItabinit();
    VSIw->vistop=VSIw->scrntop;
    if(VSIw->linest[VSPBOTTOM]->next!=NULL)
        return(-42);
#ifdef DEBUGCGW
    VSckconsist(1,1);
#endif
    return(VSIwn);

attrinfoAllocFailed:    /* failed to allocate room for the attr. information */
    tt=last;
    while(tt!=NULL) {   /* walk through the linked list of lines freeing them */
        last=tt->next;
        free(tt->text);     /* free the attr. line */
        free(tt);           /* free the attr. information */
        tt=last;
      } /* end while */
attrAllocFailed:        /* failed to allocate space for the first attr. line */
    free(last);
attrtopAllocFailed:     /* failed to allocate space for the first attribute */
    last=VSIw->buftop;  /* set the correct pointer for free'ing the text lines */
textinfoAllocFailed:    /* failed to allocate room for the text information */
    tt=last;
    while(tt!=NULL) {   /* walk through the linked list of lines freeing them */
        last=tt->next;
        free(tt->text);     /* free the text line */
        free(tt);           /* free the text information */
        tt=last;
      } /* end while */
textAllocFailed:        /* failed to allocate room for first text array */
    free(last);
buftopAllocFailed:      /* failed to allocate room for the buftop array */
    free(VSIw->tabs);
tabsAllocFailed:        /* failed to allocate room for the tabs array */
    VSinuse--;
    VSscreens[VSIwn].stat=VS_UNUSED;
    free(VSIw->attrst);
    free(VSIw->linest);
    free(VSIw);     /* free the window */
    VSscreens[VSIwn].loc=NULL;
    return(-2);

attrstAllocFailed:      /* failed to allocate room for the attrst buffer */
    free(VSIw->linest);
linestAllocFailed:      /* failed to allocate room for the linest buffer */
    if(VSscreens[VSIwn].stat!=VS_DETACHED) {
        free(VSIw);     /* free the window */
        VSscreens[VSIwn].loc=NULL;
      } /* end if */
    return(-2);
}

int VSdestroy(int w)
{
    VSline *p,*oldp;

    if(VSvalids(w)!=0)
        return(-3);
    p=VSIw->attrst[0];
    do {
        free(p->text);
        oldp=p;
        p=p->next;
        free(oldp);
      } while((p!=NULL) && (p!=VSIw->attrst[0]));
    p=VSIw->buftop;
    do {
        free(p->text);
        oldp=p;
        p=p->next;
        free(oldp);
      } while((p!=NULL) && (p!=VSIw->buftop));
    free(VSIw->tabs);
    free(VSIw);
    VSscreens[w].stat=VS_UNUSED;
    VSIwn=-1;
    VSinuse--;          /* SCA '87 */
    return(0);
}

#ifdef DEBUGPC
int VSckconsist(int out,int verbose)
{
    VSline *topa,*topt,*a,*t;
    int line, i;
    char tmpstr[256];

    for(i=0; i<VSmax; i++) {                /* For all possible screens... */
        switch(VSscreens[i].stat) {
            case VS_UNUSED:
                if(out&&verbose) {
                    sprintf(tmpstr,"Screen %d inactive\n",i);
                    vprint(console->vs,tmpstr);
                }
                break;
    
            case VS_ACTIVE:
                if(out) {
                    sprintf(tmpstr,"Screen %d active\n",i);
                    vprint(console->vs,tmpstr);
                }
                VSvalids(i);
                topa=VSIw->attrst[ 0]->prev; 
                topt=VSIw->linest[ 0]->prev;
                a=VSIw->attrst[0]->next;  
                t= VSIw->linest[0]->next;
                if(topa&&out) {
                    sprintf(tmpstr,"  Attrib thinks its circular\n");
                    vprint(console->vs,tmpstr);
                }
                if(topa!=VSIw->attrst[VSPBOTTOM]) {
                    sprintf(tmpstr,"***********BUT IT'S NOT*************\n");
                    vprint(console->vs,tmpstr);
                }
                for(line=1; line<=VSPBOTTOM; line++) {
                    if(a!=VSIw->attrst[ line])  {
                        if(out) {
                            sprintf(tmpstr,"    Attrib line %d is wrong !\n", line);
                            vprint(console->vs,tmpstr);
                        }
                        else
                            return(-1);
                      }
                    a=a->next;
                    if(!a) {
                        if(out) {
                            sprintf(tmpstr,"    Attrib line %d is NULL\n", line);
                            vprint(console->vs,tmpstr);
                        }
                        else
                            if(!out&&line!=VSPBOTTOM)
                                return(-2);
                      }
                  }
                if(topt&&out) {
                    sprintf(tmpstr,"  Linest thinks its circular\n");
                    vprint(console->vs,tmpstr);
                }
                if(VSIw->linest[VSPBOTTOM]->next) {
                    sprintf(tmpstr," More than VSPBOTTOM lines.... \n");
                    vprint(console->vs,tmpstr);
                }
                for(line=1; line<=VSPBOTTOM; line++) {
                    if(t!=VSIw->linest[ line])  {
                        if(out) {
                            sprintf(tmpstr,"    Linest line %d is wrong !\n", line);
                            vprint(console->vs,tmpstr);
                        }
                        else
                            return (-3);
                      }
                    t=t->next;
                    if(!t) {
                        if(out) {
                            sprintf(tmpstr,"    Linest line %d is NULL\n", line);
                            vprint(console->vs,tmpstr);
                        }
                        else
                            if(line!=VSPBOTTOM) 
                                return(-4);
                      }
                  }
                if(VSIw->numlines>0) {
                    if(out) {
                        sprintf(tmpstr,"    Thinks that there is scrollback of %d lines ", VSIw->numlines);
                        vprint(console->vs,tmpstr);
                    }
                    t=VSIw->linest[VSPBOTTOM]->next;
                    line=0;
                    while(t!=NULL && t!=VSIw->buftop) {
                        t=t->next;
                        line++;
                      }
                    if(out) {
                        sprintf(tmpstr,"    [ Actual is %d ]\n", line);
                        vprint(console->vs,tmpstr);
                    }
                    if(out&&t==NULL) {
                        sprintf(tmpstr,"    Lines end in a null\n");
                        vprint(console->vs,tmpstr);
                    }
                    if(out&&t==VSIw->buftop) {
                        sprintf(tmpstr,"    Lines end in a wraparound\n");
                        vprint(console->vs,tmpstr);
                    }
                  }
                else
                    if(out) {
                        sprintf(tmpstr,"    There is no scrollback");
                        vprint(console->vs,tmpstr);
                    }
                break;

            case VS_DETACHED:
                if(out&&verbose) {
                    sprintf(tmpstr,"Screen %d detached\n",i);
                    vprint(console->vs,tmpstr);
                }
                break;
    
            default:
                if(out) {
                    sprintf(tmpstr,"Screen %d invalid stat\n",i);
                    vprint(console->vs,tmpstr);
                }
                break;

          }
      }
    return(0);
}
#endif

#ifdef USEDETATCH
VSdetatch(int w)
{
    if(VSscreens[w].stat!=VS_ACTIVE)
        return(-1);
    VSscreens[w].stat=VS_DETACHED;
    VSIwn=-1;
    VSinuse--;          /* SCA '87 */
}
#else

void VSdetatch(int w)
{
    VSdestroy(w);
}
#endif 

void VSiclrbuf(void )
{
    int j,i;
    char *ta,*tx;

    for(i=0; i<=VSPBOTTOM; i++) {
        ta=&VSIw->attrst[i]->text[0];
        tx=&VSIw->linest[i]->text[0];
        for(j=0; j<=VSIw->allwidth; j++) {
            *ta++=0;
            *tx++=' ';
          }
      }
}

int VSredraw(int w,int x1,int y1,int x2,int y2)
{
    char *pt,*pa;
    int cc,cm;
    char lc,la;
    VSline *yp;
    int y;
    int ox,tx1,tx2,ty1,ty2, tn = -1,offset;

    if(VSvalids(w)!=0)
        return(-3);
    VSIcuroff(w);
    x1+=VSIw->Rleft;
    x2+=VSIw->Rleft;    /* Make local coords global again */
    y1+=VSIw->Rtop;  
    y2+=VSIw->Rtop;
    if(x2<0)
        x2=0;
    if(x1<0)
        x1=0;
    if(x2>VSIw->maxwidth)
        x2=VSIw->maxwidth;
    if(x1>VSIw->maxwidth)
        x1=VSIw->maxwidth;
    if(y2<-VSIw->maxlines)
        y2=-VSIw->maxlines;
    if(y1<-VSIw->maxlines)
        y1=-VSIw->maxlines;
    if(y2>VSPBOTTOM)
        y2=VSPBOTTOM;
    if(y1>VSPBOTTOM)
        y1=VSPBOTTOM;
    tx1=x1;
    tx2=x2;
    ty1=y1;
    ty2=y2;     /* Set up VSIclip call */
    if(!VSIclip(&tx1,&ty1,&tx2,&ty2,&tn,&offset))
        RSerase(w,tx1,ty1,tx2,ty2);     /* Erase the offending area */
    if(y1<0) {
        yp=VSIw->vistop;
        y=y1-VSIw->Rtop;
        while(y-->0)
            yp=yp->next;    /* Get pointer to top line we need */
      }
    y=y1;
    while((y<0) && (y<=y2)) {
        ox=tx1=x1; 
        tx2=x2; 
        ty1=ty2=y; 
        tn = -1;
        if(!VSIclip(&tx1,&ty1,&tx2,&ty2,&tn,&offset))
          RSdraw(w,tx1,ty1,0,tn,&yp->text[ox+offset]);
        yp=yp->next;
        y++;
      }
    while(y<=y2) {
        pt=&VSIw->linest[y]->text[x2];
        pa=&VSIw->attrst[y]->text[x2];
        cm=x2; 
        cc=1;
        lc=*pt; 
        la=*pa;
        while(cm>=x1) {
            if((lc==' ')&&(la==0)) {
                while((cm>x1)&&(*(pt-1)==' ')&&(*(pa-1)==0)) {
                    pa--;
                    pt--;
                    cm--;
                    cc++;
                  }
                pa--;
                pt--;
                cm--;
                cc=1;
                lc= *pt; 
                la= *pa;
                continue;
              }
            while((cm>x1)&&(la == *(pa-1))) {
                pa--;
                pt--;
                cm--;
                cc++;
              }
            if(cm>=x1)
#ifdef LK       /* This change was suggested by Larry Krone to help extended */
                /* ASCII char sets to work. (only it broke something  rmg 931100) */
                VSIdraw(w,cm,y,0,cc,pt);
#else
                VSIdraw(w,cm,y,la,cc,pt);
#endif
            pa--;
            pt--;
            cm--;
            cc=1;
            lc= *pt;
            la= *pa;
          }
        y++;
      }
    VSIcurson(w,VSIw->x,VSIw->y,0); /* Don't force move */
    tx1=ty1=0;
    tn=132;
    if(!VSIclip(&tx1,&ty1,&tx2,&ty2,&tn,&offset))
        RSdrawsep(w,ty1,1);                 /* Draw Separator */
    return(0);
}

int VSwrite(int w,char *ptr,int len)
{
    if(VSvalids(w)!=0)
        return(-3);
    VSIcuroff(w);
    VSem(ptr,len);
    VSIcurson(w,VSIw->x,VSIw->y,1); /* Force Move */
    return(0);
}

int VSdirectwrite(int w,char *ptr,int len)
{
    if(VSvalids(w)!=0)
        return(-3);
    VSIcuroff(w);
    VSIstring(ptr,len);
    VSIcurson(w,VSIw->x,VSIw->y,1); /* Force Move */
    return(0);
}

#ifdef NOT_USED
int VSclear(int w)
{
    if(VSvalids(w)!=0)
        return(-3);
    VSIes();
    VSIw->x=VSIw->y=0;
    VSIcurson(w,VSIw->x,VSIw->y,1); /* Force Move */
    return(0);
}

int VSpossend(int w,int x,int y,int echo)
{
    static char VSkbax[]="\033O ",      /* prefix for auxiliary code*/
                VSkban[]="\033[ ";      /* prefix for arrows normal */
    char *vskptr;

    if(VSvalids(w)!=0)
        return(-3);
    if(VSIw->DECPAM&&VSIw->DECCKM)
        vskptr=VSkbax;
    else
        vskptr=VSkban;
    if(x<0||y<0||x>VSIw->maxwidth||y>VSPBOTTOM)
        return(-10);
    x-=VSIw->x;
    y-=VSIw->y;
    vskptr[2]='B';
    while(y>0) {
        y--; 
        RSsendstring(w,vskptr,3); 
      }
    vskptr[2]='A';
    while(y<0)  {
        y++; 
        RSsendstring(w,vskptr,3); 
      }
    vskptr[2]='C';
    while(x>0) {
        x--; 
        RSsendstring(w,vskptr,3); 
      }
    vskptr[2]='D';
    while(x<0) {
        x++; 
        RSsendstring(w,vskptr,3); 
      }
    if(echo) {
        VSIcuroff(w);
        VSIw->x=x;
        VSIw->y=y;
        VSIcurson(w,VSIw->x,VSIw->y,1); /* Force Move */
      }
    return(0);
}
#endif

int VSkbsend(int w,unsigned char k,int echo)
{
    static char VSkbkn[]="\033O ",      /* prefix for keypad normal */
                VSkbax[]="\033O ",      /* prefix for auxiliary code*/
                VSkban[]="\033[ ",      /* prefix for arrows normal */
                VSkbfn[]="\033O ";      /* prefix for function keys */
    char *vskptr;

    if(VSvalids(w)!=0)
        return(-3);
    if(k<VSUP) {    /* not a character which we handle (shouldn't even get here) */
        RSsendstring(w,&k,1);
        if(echo)
            VSwrite(w,&k,1);
        return(0);
      }
    if((k>VSLT) && (k<VSF1) && (!VSIw->DECPAM)) {       /* send keypad codes */
        RSsendstring(w,&VSIkpxlate[0][k-VSUP],1);
        if(echo)
            VSwrite(w,&VSIkpxlate[0][k-VSUP],1);
        if(k==VSKE) 
            RSsendstring(w,"\012",1);
        return(0);
      }
#ifdef QAK
    if(k>=VSUP_SPECIAL) {
        vskptr=VSkban;  /* prefix for arrow keys */
        k-=64;          /* decrement the key back to the proper range of values */
      } /* end if */
    else
#endif
        if(VSIw->DECPAM && VSIw->DECCKM) /* prefix aux keypad keys */
            vskptr=VSkbax;
        else
            if(k<VSK0)          /* prefix normal arrow keys */
                vskptr=VSkban;
            else
                if(k<VSF1)      /* prefix for keypad normal */
                    vskptr=VSkbkn;
                else            /* prefix for function keys */
                    vskptr=VSkbfn;
    vskptr[2]=VSIkpxlate[1][k-VSUP];
    RSsendstring(w,vskptr,3);
    if(echo)
        VSwrite(w,vskptr,3);
    return(0);
}

#ifdef NOT_USED
int VSclearall(int w)
{
    if(VSvalids(w)!=0) 
        return(-3);
    return(0);
}
#endif

int VSreset(int w)
{
    if(VSvalids(w)!=0) 
        return(-3);
    VSIreset();
    VSIcurson(w,VSIw->x,VSIw->y,1); /* Force Move */
    return(0);
}   /* end VSreset() */

#ifdef NOT_USED
char *VSgetline(int w,int y)
{
    if(VSvalids(w)!=0)
        return((char *)-3);
    return(VSIw->linest[y]->text);              /* Need to add code for scrolled back lines */
}
#endif

int VSsetrgn(int w,int x1,int y1,int x2,int y2)
{
    int n,offset;

    if(VSvalids(w)!=0)
        return(-3);
    VSIw->Rbottom=VSIw->Rtop+(y2-y1);   /* Reduce window size first */
    if(x2>VSIw->maxwidth) {
        n=x2-VSIw->maxwidth;            /* Get the maximum width */
        if((x1-n)<0)
            n=x1;                       /* never a pos. offset from the left */
        x1-=n;                          /* Adjust left  */
        x2-=n;                          /* Adjust right */
      } /* end if */
    if(VSIw->Rleft!=x1) {               /* If the left margin changes then do scroll immediately */
        n=x1-VSIw->Rleft;               /* Because LM change, TM change and Size change are the */
        if(n>0)
            VSscrolright( w,n);         /* Only valid ways to call this proc */
        else 
            VSscrolleft(w,-n);
      } /* end if */
    else 
        RSmargininfo(w,VSIw->maxwidth-(x2-x1),x1);
    VSIw->Rleft=x1;                     /* Same with the margins */
    VSIw->Rright=x2;
    if(VSIw->Rbottom>VSPBOTTOM)
        n=VSIw->Rbottom-VSPBOTTOM;
    else
        n=VSIw->Rtop-y1;
    if(n>0)
        VSscrolback(w,n);       /* Go forward */
    else {
        if(n<0) 
            VSscrolforward(w,-n);       /* And backward on command */
        else {
            x1=y1=1;n=132;
            if(!VSIclip(&x1,&y1,&x2,&y2,&n,&offset))
                RSdrawsep(w,n,1);                   /* Draw Separator */
            RSbufinfo(w,VSIw->numlines,VSIw->Rtop,VSIw->Rbottom);
          } /* end else */
      } /* end else */
    return(0);
}   /* end VSsetrgn() */

int VSscrolback(int w,int in)
{
    int sn,n;

    n=in;
    if(VSvalids(w)!=0)
        return(-3);
    if(VSIw->numlines<(n-VSIw->Rtop))   /* Check against bounds */
        n=VSIw->Rtop+VSIw->numlines;
    if(n<=0)
        return(0);          /* Dont be scrollin' no lines.... */
    VSIcuroff(w);
    VSIw->Rtop=VSIw->Rtop-n;                /* Make the new region */
    VSIw->Rbottom=VSIw->Rbottom-n;
    sn=n;
    while(sn-->0)
        VSIw->vistop=VSIw->vistop->prev;
    sn=VSIw->Rbottom-VSIw->Rtop;
    RSbufinfo( w, VSIw->numlines, VSIw->Rtop, VSIw->Rbottom);
    if(n<=VSPBOTTOM) {
        RSinslines(w,0,sn,n,0);             /* Dont be destructive */
        VSIcurson(w,VSIw->x,VSIw->y,0);     /* Dont force move */
        VSredraw(w,0,0,VSIw->maxwidth,n-1);
      } /* end if */
    else
        VSredraw(w,0,0,VSIw->maxwidth,sn);
    return(0);
}   /* end VSscrolback() */

int VSscrolforward(int w,int n)
{
    int sn;

    if(VSvalids(w)!=0)
        return(-3);
    if(VSIw->Rtop+n>(VSPBOTTOM-(VSIw->Rbottom-VSIw->Rtop))) /* Check against bounds */
        n=VSPBOTTOM-(VSIw->Rbottom-VSIw->Rtop)-VSIw->Rtop;
    if(n<=0)
        return(0);          /* Dont be scrollin' no lines.... */
    VSIcuroff(w);
    VSIw->Rtop=n+VSIw->Rtop;                /* Make the new region */
    VSIw->Rbottom=n+VSIw->Rbottom;
    sn=n;
    while(sn-->0)
        VSIw->vistop=VSIw->vistop->next;
    sn=VSIw->Rbottom-VSIw->Rtop;
    RSbufinfo( w, VSIw->numlines, VSIw->Rtop, VSIw->Rbottom);
    if(n<=VSPBOTTOM) {
        RSdellines(w,0,sn,n,0);           /* Dont be destructive */
        VSIcurson(w,VSIw->x, VSIw->y, 0); /* Dont force move */
        VSredraw(w,0,(sn+1)-n,VSIw->maxwidth, sn);
      } /* end if */
    else
        VSredraw(w,0,0,VSIw->maxwidth, sn);
    return(0);
}   /* end VSscrolforward() */

int VSscrolright(int w,int n)
{
    int sn,lmmax;

    if(VSvalids(w)!=0)
        return(-3);
    lmmax=(VSIw->maxwidth-(VSIw->Rright-VSIw->Rleft));
    if(VSIw->Rleft+n>lmmax)                             /* Check against bounds */
        n=lmmax-VSIw->Rleft;
    if(n==0)
        return(0);                                  /* Do nothing if appropriate */
    VSIcuroff(w);
    VSIw->Rleft+=n;                         /* Make new region */
    VSIw->Rright+=n;
    sn=VSIw->Rbottom-VSIw->Rtop;
    RSmargininfo(w,lmmax,VSIw->Rleft);
    RSdelcols(w,n);
    VSIcurson(w,VSIw->x,VSIw->y, 0); /* Don't force move */
    VSredraw(w,(VSIw->Rright-VSIw->Rleft)-n,0,(VSIw->Rright-VSIw->Rleft), sn);
}   /* end VSscrolright() */

int VSscrolleft(int w,int n)
{
    int sn,lmmax;

    if(VSvalids(w)!=0)
        return(-3);
    lmmax=(VSIw->maxwidth-(VSIw->Rright-VSIw->Rleft));
    if(VSIw->Rleft-n<0)                             /* Check against bounds */
        n=VSIw->Rleft;
    if(n==0)
        return(0);                                  /* Do nothing if appropriate */
    VSIcuroff(w);
    VSIw->Rleft-=n;                         /* Make new region */
    VSIw->Rright-=n;
    sn=VSIw->Rbottom-VSIw->Rtop;
    RSmargininfo(w,lmmax,VSIw->Rleft);
    RSinscols(w,n);
    VSIcurson(w,VSIw->x,VSIw->y,0); /* Don't force move */
    VSredraw(w,0,0,n, sn);
    return(0);
}   /* end VSscrolleft() */

int VSscrolcontrol(int w,int scrolon,int offtop)
{
    if(VSvalids(w)!=0) 
        return(-3);
    if(scrolon>-1)
        VSIw->savelines=scrolon;
    if(offtop>-1)
        VSIw->ESscroll=offtop;
    return(0);
}   /* end VSscrolcontrol() */

int VSgetrgn(int w,int *x1,int *y1,int *x2,int *y2)
{
    if(VSvalids(w)!=0)
        return(-3);
    *x1=VSIw->Rleft;
    *y1=VSIw->Rtop;
    *x2=VSIw->Rright;
    *y2=VSIw->Rbottom;
    return(0);
}   /* end VSgetrgn() */

#ifdef NOT_USED
int VSsnapshot(int w)
{
    if(VSvalids(w)!=0) 
        return(-3);
    return(0);
}
#endif

int VSvalids(int w)
{
    if(VSinuse==0) 
        return(-5); /* -5=no ports in use */
    if((w>VSmax) || (w<0)) 
        return(-6); /* blown out the top of the stuff */
    if(VSIwn==w) 
        return(0);  /* Currently set to that window */
    VSIwn=w;
    if(VSscreens[w].stat!=VS_ACTIVE)
        return(-3);/* not currently active */
    VSIw=VSscreens[w].loc;
    if(VSIw==NULL) 
        return(-4); /* no space allocated */
    return(0);
}   /* end VSvalids() */

#ifdef NOT_USED
int VSmaxwidth(int w)
{
    if(VSvalids(w)!=0) 
        return(-3);
    return(VSIw->maxwidth);
}
#endif

VSline *VSIGetLineStart(int w,int y1)
{
    VSline *ptr;
    int n;

    if(VSvalids(w)!=0) 
        return((VSline *)-3);
    if(y1>=0)
        return(VSIw->linest[y1]);
    n=y1-VSIw->Rtop;                /* Number of lines from VISTOP to scroll forward */
    ptr=VSIw->vistop;
    while(n>0) {
        n--; 
        ptr=ptr->next; 
      } /* end while */
    while(n<0) { 
        n++; 
        ptr=ptr->prev; 
      } /* end while */
    return( ptr);
}   /* end VSIGetLineStart() */

char *VSIstrcopy(char *src,int len,char *dest,int table)
{
    char *p, *tempp;
    int tblck;

    p=src+len-1;
    while((*p==' ') && (p>=src))
        p--;
    if(p<src) 
        return(dest);
    if(!table)
        while(src<=p) 
            *dest++=*src++;
    else
        while(src<=p) {
            while((src<=p) && (*src != ' '))
                *dest++=*src++;
            if(src<p) {
                tempp=dest;
                tblck=0;
                while((src<=p) && (*src== ' ')) {
                    *dest++=*src++;
                    tblck++;
                  } /* end while */
                if(tblck>=table) {
                    *tempp++='\011';
                    dest=tempp;
                  } /* end if */
              } /* end if */
          } /* end while */
    return(dest);
}   /* end VSIstrcopy() */

long VSgettext(int w,int x1,int y1,int x2,int y2,char *charp,long max,char *EOLS,int table)
{
    int lx,ly,                  /* Upper bounds of selection */
        ux,uy;                  /* Lower bounds of selection */
    int maxwid;
    char *origcp;
    VSline *t;

    if(VSvalids(w)!=0) 
        return(-3);
    maxwid=VSIw->maxwidth;
    origcp=charp;

    if(y1<-VSIw->numlines) {
        y1=-VSIw->numlines;
        x1=-1;
      } /* end if */
    if(y1==y2) {
        t=VSIGetLineStart(w,y1);
        if(x1<x2) { /* Order the lower and */
            ux=x1; 
            uy= y1; 
            lx=x2; 
            ly=y2; 
          } /* end if */
        else {  /* upper bounds */
            ux=x2; 
            uy= y2; 
            lx=x1; 
            ly=y1; 
          } /* end else */
        if((long)(lx-ux)<max)
            charp=VSIstrcopy(&t->text[ux+1],lx-ux,charp,table);
        else
            charp=VSIstrcopy(&t->text[ux+1],(int)(max-(long)(charp-origcp)),charp,table);

        if(lx==maxwid)
            *charp++=*EOLS;
      } /* end if */
    else {
        if(y1<y2) {     /* Order the lower and */
            ux=x1; 
            uy= y1; 
            lx=x2; 
            ly=y2; 
          } /* end if */
        else {  /* upper bounds */
            ux=x2; 
            uy=y2; 
            lx=x1; 
            ly=y1; 
          } /* end else */
        t=VSIGetLineStart(w,uy);
        if(((long)(maxwid-ux) < max))
            charp=VSIstrcopy(&t->text[ux+1],maxwid-ux,charp,table);
        else
            charp=VSIstrcopy(&t->text[ux+1],(int) (max-(long)(charp-origcp)),charp,table);
        *charp++=*EOLS; 
        uy++; 
        t=t->next;
        while(uy<ly&&uy<VSPBOTTOM) {
            if((long)(maxwid+1) < max)
                charp=VSIstrcopy(t->text,maxwid+1,charp,table);
            else
                 charp=VSIstrcopy(t->text,(int)(max - (long) (charp-origcp)),charp, table);
            *charp++=*EOLS;
            t=t->next; 
            uy++;
          } /* end while */
        if(ly>VSPBOTTOM) 
            lx=maxwid;
        if((long)(lx+1) < max)
            charp=VSIstrcopy(t->text,lx+1,charp,table);
        else
            charp=VSIstrcopy(t->text,(int)(max - (long)(charp-origcp)),charp,table);
        if(lx>=maxwid)
            *charp++=*EOLS;
      } /* end else */
    return((long)(charp - origcp) );
}   /* end VSgettext() */

#ifdef NOT_USED
int VSgtlines(int w)
{
    if(VSvalids(w)!=0) 
        return(-3);
    return(VSIw->lines+1);
}
#endif

int VSsetlines(int w,int lines)
{
    VSline **attrst,**linest;               /* For storage of old ones */
    int i,offset;
    
    if(VSvalids(w)!=0) 
        return(-3);
    lines--;                                /* Correct for internal use */
    if(lines==VSIw->lines)
        return(0);
    attrst=VSIw->attrst;                    /* Save previous line pointers */
    linest=VSIw->linest;
    if((VSIw->linest=(VSline **)malloc(sizeof(VSline *)*(lines+1)))==NULL) {
        VSIw->linest=linest;            /* restore old line pointers */
        return(-1);
      } /* end if */
    if((VSIw->attrst=(VSline **)malloc(sizeof(VSline *)*(lines+1)))==NULL) {
        free(VSIw->linest);
        VSIw->linest=linest;            /* restore old line pointers */
        VSIw->attrst=attrst;
        return(-1);
      } /* end if */
    if(lines<VSIw->lines) {
        offset=VSIw->lines-lines;
        VSIw->numlines+=offset;             /* That stuff is now scrollback */
        VSIw->scrntop=linest[offset];       /* Move the top of the screen */
        VSIw->vistop=VSIw->scrntop;         /* Force a move to the top of the screen */
        attrst[offset]->prev=attrst[VSIw->lines];   /* Make attr circ. */
        attrst[VSIw->lines]->next=attrst[offset];
        for(i=0; i<offset; i++) {
            free(attrst[i]->text);          /* Free attribute data */
            free(attrst[i]);                /*  and header */
          } /* end for */
        VSIw->lines=lines;
        VSIlistndx(VSIw->scrntop,attrst[offset]);       /* Re-sync the indices */
      } /* end if */
    else {
        for(i=0; i<=VSIw->lines; i++) {             /* Get old Attribute lines */
            VSIw->attrst[i]=attrst[i];              /*  and text lines */
            VSIw->linest[i]=linest[i];
          } /* end for */
        for(i=VSIw->lines; i<lines; i++) {
            VSIw->attrst[i+1]=VSInewline();         /* Make new attribute lines */
            VSIw->attrst[i]->next=VSIw->attrst[i+1];
            VSIw->attrst[i+1]->prev=VSIw->attrst[i];
            VSIw->linest[i+1]=VSInewline();         /* Make new text  lines */
            VSIw->linest[i]->next=VSIw->linest[i+1];
            VSIw->linest[i+1]->prev=VSIw->linest[i];
            memset(VSIw->linest[i+1]->text,' ',VSIw->allwidth+1);
            memset(VSIw->attrst[i+1]->text,0,VSIw->allwidth+1);
          } /* end for */
        VSIw->attrst[0]->prev=VSIw->attrst[lines];  /* Make attr circ. */
        VSIw->attrst[lines]->next=VSIw->attrst[0];
        VSIw->linest[lines]->next=NULL;             /* for the last line of text only */
        VSIw->lines=lines;
        VSIlistndx(VSIw->scrntop,VSIw->attrst[0]);  /* Re-sync the indices */
      } /* end else */
    VSIw->top=0;
    VSIw->bottom=lines;
    VSIw->Rtop=lines-(VSIw->Rbottom-VSIw->Rtop);
    VSIw->Rbottom=lines;
    if(VSIw->Rtop>=0)
        VSIw->vistop=VSIw->linest[VSIw->Rtop];
    else {
        int n=-VSIw->Rtop;

        VSIw->vistop=VSIw->linest[0];
        while(n--)
            VSIw->vistop=VSIw->vistop->prev;
      } /* end else */
    free(attrst);           /* Nuke the previous pointers */
    free(linest);
    RSbufinfo(w,VSIw->numlines,VSIw->Rtop,VSIw->Rbottom);
    return(VSIw->lines);
}   /* end VSsetlines() */

