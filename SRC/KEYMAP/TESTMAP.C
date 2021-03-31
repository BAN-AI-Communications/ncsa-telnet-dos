#include <stdio.h>
#include <stdlib.h>

extern unsigned int n_newgetchar(void );
extern void install_keyboard(void );
extern unsigned char keyboard_type;
main()
{
	unsigned int c;

	install_keyboard();
	printf("keyboard_type=%u\n",(unsigned int)keyboard_type);
	while(1) {
		c=n_newgetchar();
		printf("c=%u, %X\n",c,c);
	  }	/* end while */
}	/* end testmap() */