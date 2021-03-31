/*
*   NCSA Telpass - edit password files for NCSA Telnet.
*   Tim Krauskopf 6/88
*
*   This program is rough-cut but functional.
*   If you improve it substantially, let me know.
*/
/************************************************************************/
#define PASS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#ifdef MSC
#include <malloc.h>
#endif

#include "version.h"
#include "externs.h"
static int  dochoice(int c);
static void dodirs(int c);
static void noecho(char *s);
static void passwrite(char *s);
static void passlist(char *s);
static void Sencompass(char *ps);

void noecho();
void passwrite();
void Sencompass();

char space[256];
char space2[256],*p;
char *lines[400];

FILE *fout,*fin;
int nnames;

void main(argc,argv)
int argc;
char *argv[];
{
	int i,choice;

	nnames=0;
	n_clear();
	n_cur(0,0);
	n_puts("National Center for Supercomputing Applications");
  sprintf(space,"Password file editor for %s\n",TEL_VERSION);
  n_puts(space);
	n_puts("Used by the background FTP server only.\n");
	n_puts("Note:  This encryption of passwords is to hide clear text from casual viewing, ");
	n_puts("       not to protect them from decryption.  You must assume that anyone with");
  n_puts("       access to the password file can decrypt the passwords.\n");
  n_puts("Also:  The password file format has changed since 2.3.06, read the documents.\n");
	if(argc<2) {
		n_puts("A filename is required.  Usage: telpass filename");
		exit(0);
	  }
	passlist(argv[1]);
	do {
		n_puts("List of names:");
		for(i=0; i<nnames; i++) {
      sprintf(space,"%3d. %s",i+1,lines[2*i]);
      n_puts(space);
      sprintf(space,"      %s",lines[2*i+1]);
      n_puts(space);
    }
    n_puts("");
    n_puts("Enter the number of a user to change the password for, or:");
    n_puts("'a' to add a user, 'd #' to delete a user,");
    n_puts("'e #' to edit a users permissions, 'x' to exit:");
		gets(space);
		n_row();
		choice=atoi(space);
		if(choice)
      dochoice(2*(choice-1));
    else if(*space=='a') {
      n_puts("Enter the username to add:");
      gets(space);
      n_row();
      lines[2*nnames] = malloc(strlen(space)+1);
      strcpy(lines[2*nnames],space);
      if(dochoice(2*nnames)) {   /* get the password */
        dodirs(2*nnames+1);      /* get permissions */
        nnames++;
      }
    }
    else if(*space=='d') {
      choice=atoi(space+1);
      if(choice<=nnames&&choice>0) {
        for (i=choice-1; i<nnames; i++) {
          free(lines[2*i]);             /* be nice */
          free(lines[2*i+1]);
          lines[2*i]=lines[2*i+2];      /* shuffle down */
          lines[2*i+1]=lines[2*i+3];
        }
        nnames--;
      }
    }
    else if(*space=='e') {
      choice=atoi(space+1);
      if(choice<=nnames&&choice>0) {
        dodirs(2*(choice-1)+1);
      }
    }
    else if(*space=='x')
      choice = -1;
	  }while(choice>=0);
	passwrite(argv[1]);
	exit(0);
}

/****************************************************************************/
/* dochoice
*  prompt for a certain password
*/
static int dochoice(int c)
{
	char *p;
	char pwd[256],ver[256];

	strcpy(space,lines[c]);
	do{
		p=strchr(space,':');
		if(!p)
			strcat(space,":");
	  }while(!p);						/* make sure we get a : */
	*p='\0';
	p++;
  sprintf(space2,"Enter new password for user: %s, \"\\ \" to allow any password.",space);
	n_puts(space2);
	noecho(pwd);	
	n_puts("Verify password by entering again:");
	noecho(ver);
	if(strcmp(pwd,ver)) {
		n_puts("Password not verified");
    return 0;
	  }
  if(!strcmp("\\ ",pwd))
    strcpy(space2,"");
  else
    Sencompass(pwd);          /* take password */
	sprintf(ver,"%s:%s",space,space2);
	lines[c]=malloc(strlen(ver)+1);
	strcpy(lines[c],ver);
return 1;
}

/****************************************************************************/
/* dodirs
*  prompt for the directory permissions
*/
static void dodirs(int c)
{
  char line[256],q=0;
  int a=1,i=1;

*line='\0';

  sprintf(space,"Directories were %s",lines[c]);
  n_puts(space);

for(a=1;a;) {
  n_puts("");
  n_puts("Enter a directory you wish to give this user access to.");
  n_puts("They will have access to all subdirectories as well.");
  sprintf(space2,"Enter directory path #%d here (include drive):",i);
  n_puts(space2);
  gets(space2);
  n_row();

  q=0;
  if(!strcmp("root",space2)) {
    q=1;
    a=7;  /* irrelevent if root, but needed to parse */
  }
  for(;!q;) {
    a=1;
    n_puts("Do you wish to allow any access to this directory? (y/n)");
    q=getch();
    q=tolower(q);
    if(q=='y')
      a |= 1;
    else
      if(q=='n')
        a=0;
      else
        q=0;
  }
  if(a != 7)
    q=0;
  for(;!q && a;) {
    n_puts("Do you wish to allow read access to this directory? (y/n)");
    q=getch();
    q=tolower(q);
    if(q=='y')
      a |= 4;
    else
      if(q=='n')
        ;
      else
        q=0;
  }
  if(a != 7)
    q=0;
  for(;!q && a;) {
    n_puts("Do you wish to allow write access to this directory? (y/n)");
    q=getch();
    q=tolower(q);
    if(q=='y')
      a |= 2;
    else
      if(q=='n')
        ;
      else
        q=0;
  }
  n_row();

  sprintf(space," %d ",a);
  strcat(space,space2);
  strcat(line,space);

  q=0;
  if(!strcmp("root",space2)) {
    q=1;
    a=0;
  }
  for(;!q;) {
    n_puts(line);
    n_puts("Do you have more directories to add for this user? (y/n)");
    q=getch();
    q=tolower(q);
    if(q=='y')
      a=1;
    else
      if(q=='n')
        a=0;
      else
        q=0;
  }
  n_row();
  i++;
}

lines[c]=malloc(strlen(line)+1);
strcpy(lines[c],line);
}


static void noecho(char *s)
{
	int c;

	do {
		c=n_getchar();
		if(c>31&&c<128)
			*s++= (char) c;
	}while(c>31&&c<128);
	*s='\0';
}

/****************************************************************************/
/* passwrite
*  write them out
*/
static void passwrite(char *s)
{
	int i;

	if(NULL==(fout=fopen(s,"w"))) {
		n_puts("Cannot open file to write passwords. ");
		exit(0);
	  }
	for(i=0; i<nnames; i++) {
		fputs(lines[2*i],fout);
		fputs("\n",fout);
		fputs(lines[2*i+1],fout);
		fputs("\n",fout);
	  }
	fclose(fout);
}

/****************************************************************************/
/*  passlist
*   List the current file
*/
static void passlist(char *s)
{
	if(NULL==(fin=fopen(s,"r"))) {
		n_puts("Starting new file.");
		return;
	  }
	while(NULL!=fgets(space,250,fin)) {
		space[strlen(space)-1]='\0';
		lines[2*nnames]=malloc(strlen(space)+1);
		strcpy(lines[2*nnames],space);
    fgets(space,250,fin);
		space[strlen(space)-1]='\0';
		lines[2*nnames+1]=malloc(strlen(space)+1);
    strcpy(lines[2*nnames+1],space);
    nnames++;
	  }
	fclose(fin);
}

/****************************************************************************/
/* Scompass
*  compute and check the encrypted password
*/
static void Sencompass(char *ps)
{
	int i,ck;
	char *p,c,*en;

	en=space2;
	ck=0;
	p=ps;
	while(*p)				/* checksum the string */
		ck+=*p++;
	c=(char) ck;
	for(i=0; i<10; i++) {
		*en=(char) (((*ps ^ c)|32)&127); 	/* XOR with checksum */
		if(*ps)
			ps++;
		else
			c++;		/* to hide length */
		en++;
	  }
	*en=0;
}
