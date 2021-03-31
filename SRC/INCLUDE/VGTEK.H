#ifndef VGTEK_H
#define VGTEK_H

/* output devices */
#define MAXRG 		8
#define DEVNULL 	0
#define EGA 		1
#define PS 			2
#define HERCULES 	3
#define NO9 		4
#define CGA 		5
#define HP 			6
#define VGA			7
#define PREDCOUNT 	50

typedef struct 
{
void 	(*init)(void);
void	(*point)(int w, int x, int y);
void	(*drawline)(int w, int a, int b, int c, int d);
int		(*newwin)(void);
void	(*clrscr)(int w); 
void	(*pagedone)(int w); 
void	(*dataline)(int w, char *data, int count);		
void	(*pencolor)(int w, int color);
void 	(*charmode)(int w, int rotation, int size);		
void 	(*showcur)(void);
void 	(*lockcur)(void); 
void	(*hidecur)(void); 
void	(*close)(int w); 
void	(*uncover)(int w); 
void	(*info)(int w, int a, int b, int c, int d, int v);
void 	(*gmode)(void);
void	(*tmode)(void);
char*	(*devname)(void);
int		(*bell)(int w);
} RGLINK;

static RGLINK RG[MAXRG]={
	RG0init, 
	RG0point, 
	RG0drawline, 
	RG0newwin,
	RG0clrscr, 
	RG0pagedone, 
	RG0dataline, 
	RG0pencolor,
	RG0charmode, 
	RG0showcur, 
	RG0lockcur, 
	RG0hidecur,
	RG0close, 
	RG0uncover, 
	RG0info,
	RG0gmode, 
	RG0tmode, 
	RG0devname, 
	RG0bell,

	RGEinit, 
	RGEpoint, 
	RGEdrawline, 
	RGEnewwin,
	RGEclrscr, 
	RGEpagedone, 
	RGEdataline, 
	RGEpencolor,
	RGEcharmode, 
	RGEshowcur, 
	RGElockcur, 
	RGEhidecur,
	RGEclose, 
	RGEuncover, 
	RGEinfo,
	RGEgmode, 
	RGEtmode, 
	RGEdevname, 
	RGEbell,

	RGPinit, 
	RGPpoint, 
	RGPdrawline, 
	RGPnewwin,
	RGPclrscr, 
	RGPpagedone, 
	RGPdataline, 
	RGPpencolor,
	RGPcharmode, 
	RGPshowcur, 
	RGPlockcur, 
	RGPhidecur,
	RGPclose, 
	RGPuncover, 
	RGPinfo,
	RGPgmode, 
	RGPtmode, 
	RGPdevname, 
	RGPbell,

	RGHinit, 
	RGHpoint, 
	RGHdrawline, 
	RGHnewwin,
	RGHclrscr, 
	RGHpagedone, 
	RGHdataline, 
	RGHpencolor,
	RGHcharmode, 
	RGHshowcur, 
	RGHlockcur, 
	RGHhidecur,
	RGHclose, 
	RGHuncover, 
	RGHinfo,
	RGHgmode, 
	RGHtmode, 
	RGHdevname, 
	RGHbell,

	RG9init, 
	RG9point, 
	RG9drawline, 
	RG9newwin,
	RG9clrscr, 
	RG9pagedone, 
	RG9dataline, 
	RG9pencolor,
	RG9charmode, 
	RG9showcur, 
	RG9lockcur, 
	RG9hidecur,
	RG9close, 
	RG9uncover, 
	RG9info,
	RG9gmode, 
	RG9tmode, 
	RG9devname, 
	RG9bell,

	RGCinit, 
	RGCpoint, 
	RGCdrawline, 
	RGCnewwin,
	RGCclrscr, 
	RGCpagedone, 
	RGCdataline, 
	RGCpencolor,
	RGCcharmode, 
	RGCshowcur, 
	RGClockcur, 
	RGChidecur,
	RGCclose, 
	RGCuncover, 
	RGCinfo,
	RGCgmode, 
	RGCtmode, 
	RGCdevname, 
	RGCbell,

	RGHPinit, 
	RGHPpoint,
	RGHPdrawline, 
	RGHPnewwin,
	RGHPclrscr, 
	RGHPpagedone, 
	RGHPdataline, 
	RGHPpencolor,
	RGHPcharmode, 
	RGHPshowcur, 
	RGHPlockcur, 
	RGHPhidecur,
	RGHPclose, 
	RGHPuncover, 
	RGHPinfo,
	RGHPgmode, 
	RGHPtmode, 
	RGHPdevname, 
	RGHPbell,

	RGVinit, 
	RGVpoint,
	RGVdrawline, 
	RGVnewwin,
	RGVclrscr, 
	RGVpagedone, 
	RGVdataline, 
	RGVpencolor,
	RGVcharmode, 
	RGVshowcur, 
	RGVlockcur, 
	RGVhidecur,
	RGVclose, 
	RGVuncover, 
	RGVinfo,
	RGVgmode, 
	RGVtmode, 
	RGVdevname, 
	RGVbell,
};

#endif
