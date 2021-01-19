/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * main.c - p-sprint
 *
 * Copyright (c) 2005 Arwin van Arum
 * special thanks to Shine for leading the way
 *
 * $Id$
 */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "p_sprint.h"

/* Define the module info section */
PSP_MODULE_INFO("P-SPRINT", 0, 1, 1);

/* Define the main thread's attribute value (optional) */
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* Define printf, just to make typing easier */
#define printf	pspDebugScreenPrintf

/* Exit callback */
int exit_callback(void)
{
	sceKernelExitGame();

	return 0;
}

/* Callback thread */
void CallbackThread(void *arg)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);

	sceKernelSleepThreadCB();
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}



#define putChar(x, y, c) pspDebugScreenPutChar((x)*7, (y)*8, 0x0FFFFFFF, c)
#define putCharC(x, y, color, c) pspDebugScreenPutChar(x, y, color, c)

struct p_sp_structKey
{
	int keychar;
	int keycode;
	int modifiers;

};
//static int g_keyRepeatRate = 2;
//static int g_keyRepeatFirst = 12;
int g_keyRepeatFirst_Hold = 12;
int g_LastKeyBuffer = 0;
int g_LastCSBuffer = 0;
char KeyCodeNames[255][16];
unsigned int oldbuttons=0;
//int g_active_group=0;
static u32* g_vram_base = (u32*)(0x04000000+0x40000000);
int g_iScreenShotCount = 0;

void putString(int x, int y, int c, char str[]) 
{
	int i = 0;
//	while(str[i] != '\0')

	while(str[i] != '\0')
	{
		putCharC(x,y,c,str[i]);
		i++;
		x=x+7;
	}

}


void putPixel(int x, int y, int color)
{
	g_vram_base[x + y * 512] = color;
}

void drawLine(int x0, int y0, int x1, int y1, int color)
{
	#define SWAP(a, b) tmp = a; a = b; b = tmp;
	int x, y, e, dx, dy, tmp;
	if (x0 > x1) {
		SWAP(x0, x1);
		SWAP(y0, y1);
	}
	e = 0;
	x = x0;
	y = y0;
	dx = x1 - x0;
	dy = y1 - y0;
	if (dy >= 0) {
		if (dx >= dy) {
			for (x = x0; x <= x1; x++) {
				putPixel(x, y, color);
				if (2 * (e + dy) < dx) {
					e += dy;
				} else {
					y++;
					e += dy - dx;
				}
			}
		} else {
			for (y = y0; y <= y1; y++) {
				putPixel(x, y, color);
				if (2 * (e + dx) < dy) {
					e += dx;
				} else {
					x++;
					e += dx - dy;
				}
			}
		}
	} else {
		if (dx >= -dy) {
			for (x = x0; x <= x1; x++) {
				putPixel(x, y, color);
				if (2 * (e + dy) > -dx) {
					e += dy;
				} else {
					y--;
					e += dy + dx;
				}
			}
		} else {   	
			SWAP(x0, x1);
			SWAP(y0, y1);
			x = x0;
			dx = x1 - x0;
			dy = y1 - y0;
			for (y = y0; y <= y1; y++) {
				putPixel(x, y, color);
				if (2 * (e + dx) > -dy) {
					e += dx;
				} else {
					x--;
					e += dx + dy;
				}
			}
		}
	}
}

void writeByte(int fd, unsigned char data)
{
	sceIoWrite(fd, &data, 1);
}

void screenshot()
{
	g_iScreenShotCount++;
	const char tgaHeader[] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	const int width = 480;
	const int lineWidth = 512;
	const int height = 272;
	unsigned char lineBuffer[width*4];
	u32* vram = g_vram_base;
	int x, y;
	int fd = sceIoOpen("ms0:/screenshot.tga", PSP_O_CREAT | PSP_O_TRUNC | PSP_O_WRONLY, 0777);
	if (!fd) return;
	sceIoWrite(fd, tgaHeader, sizeof(tgaHeader));
	writeByte(fd, width & 0xff);
	writeByte(fd, width >> 8);
	writeByte(fd, height & 0xff);
	writeByte(fd, height >> 8);
	writeByte(fd, 24);
	writeByte(fd, 0);
	for (y = height - 1; y >= 0; y--) {
		for (x = 0; x < width; x++) {
			u32 color = vram[y * lineWidth + x];
			unsigned char red = color & 0xff;
			unsigned char green = (color >> 8) & 0xff;
			unsigned char blue = (color >> 16) & 0xff;
			lineBuffer[3*x] = blue;
			lineBuffer[3*x+1] = green;
			lineBuffer[3*x+2] = red;
		}
		sceIoWrite(fd, lineBuffer, width * 3);
	}
	sceIoClose(fd);
}

void drawRect(int x0, int y0, int x1, int y1, int color, int fill)
{
	int i = 0;
	if(fill)
	{
		
		while((i+y0)<=y1)
		{
			drawLine(x0, y0+i, x1, y0+i, color);
			i++;
		}
	}
	else
	{
		drawLine(x0, y0, x1, y0, color);
		drawLine(x1, y0, x1, y1, color);
		drawLine(x1, y1, x0, y1, color);
		drawLine(x0, y1, x0, y0, color);
	}
}

void write2TextFile(unsigned char writeLine[256])
{
	int i = 0;	

	int fd = sceIoOpen("ms0:/p-spout.txt", PSP_O_APPEND | PSP_O_WRONLY, 0777);
	if (!fd) return;
	
	/*pos = sceIoLseek(fd, -1, SEEK_END);*/

	while(!writeLine[i]==0)
	{
		writeByte(fd,writeLine[i]);
		i++;
		if (i>254){break;}
	}
	writeByte(fd,13);
	writeByte(fd,10);
	/*writeByte(fd,0);*/
	sceIoClose(fd);
}


/*

void loadKeys(unsigned int iButtons, int iMode)
{
}

*/


void initScreen()
{
	/* init the p-sprint main test/input screen */
	pspDebugScreenInit();
	//printf ("   left      up        right     square    triangle  circle\n\n");
	//printf ("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n                p-sprint 0.50a - by Arwin van Arum");
	//drawLower(0);
	putString(31*7,1*8,0x0FFFFFFF,"p-sprint 0.62a - (c) Arwin van Arum");
	drawLine(0,22,479,22,0x00000FFF);
	drawLine(0,242,479,242,0x00000FFF);
}

int main(void)
{
	char cLines[20][66];
	int iLines = 4;
	int iChars = 1;
	int iDone = 0;
	int iBlink = 0;
	int prevmods = 0;
	int prevgroup = 1;
	char keyName[20];
	//char myChar = 0;
	//struct p_sp_structKey myKey;
	struct p_sp_Key myKey;

	int fd = sceIoOpen("ms0:/p-spout.txt", PSP_O_APPEND | PSP_O_WRONLY, 0777);
	if(!fd)
	{
		int fd = sceIoOpen("ms0:/p-spout.txt", PSP_O_CREAT | PSP_O_WRONLY, 0777);
	}
	sceIoClose(fd);
	SetupCallbacks();
	initScreen();


	/* debug welcome message */
	sceDisplayWaitVblankStart(); 
	/* main loop waiting for input from pad*/

	while(!iDone)
	{
		/*keyChar = p_spGetChar();*/
		/*keyChar = KeyChars[myKey.keycode][myKey.modifiers];*/
		/*p_spGetKey(&myKey);*/
		p_spReadKey(&myKey);
		//putString(2*7,26*8,0x0FFFFFFF,"Test/0");

		//pspDebugScreenInit();
		if(myKey.keycode)
		{
			keyName[0] = myKey.keychar;
			keyName[1] = 0;

			p_spGetKeycodeFriendlyName(myKey.keycode,keyName);
			
			putString(2*7,31*8,0x0FFFFFFF,"                ");
			putString(2*7,31*8,0x0FFFFFFF,keyName);

		}
		if(myKey.modifiers!=prevmods)
		{
			putString(2*7,1*8,0x0FFFFFFF,"                  ");
			switch(myKey.modifiers)
			{
			case 1:
				putString(2*7,1*8,0x0FFFFFFF,"CTRL");
				break;
			case 2:
				putString(2*7,1*8,0x0FFFFFFF,"SHIFT");
				break;
			case 3:
				putString(2*7,1*8,0x0FFFFFFF,"CTRL-SHIFT");
				break;
			case 4:
				putString(2*7,1*8,0x0FFFFFFF,"ALT");
				break;
			case 5:
				putString(2*7,1*8,0x0FFFFFFF,"CTRL-ALT");
				break;
			case 6:
				putString(2*7,1*8,0x0FFFFFFF,"SHIFT-ALT");
				break;
			case 7:
				putString(2*7,1*8,0x0FFFFFFF,"CTRL-SHIFT-ALT");
				break;
			case 8:
				putString(2*7,1*8,0x0FFFFFFF,"PSP");
				break;
			case 9:
				putString(2*7,1*8,0x0FFFFFFF,"CTRL-PSP");
				break;
			case 10:
				putString(2*7,1*8,0x0FFFFFFF,"SHIFT-PSP");
				break;
			case 11:
				putString(2*7,1*8,0x0FFFFFFF,"CTRL-SHIFT-PSP");
				break;
			case 12:
				putString(2*7,1*8,0x0FFFFFFF,"ALT-PSP");
				break;
			case 13:
				putString(2*7,1*8,0x0FFFFFFF,"CTRL-ALT-PSP");
				break;
			case 14:
				putString(2*7,1*8,0x0FFFFFFF,"CTRL-SHIFT-PSP");
				break;
			case 15:
				putString(2*7,1*8,0x0FFFFFFF,"CTRL-SHIFT-ALT-PSP");
				break;
			}
			prevmods = myKey.modifiers;
		} 
		if(myKey.keygroup!=prevgroup)
		{
			putString(50*7,31*8,0x0FFFFFFF,"                  ");
			switch(myKey.keygroup)
			{
			case 0:
				putString(50*7,31*8,0x0FFFFFFF,"Alphabet");
				break;
			case 1:
				putString(50*7,31*8,0x0FFFFFFF,"Numbers & F-Keys");
				break;
			case 2:
				putString(50*7,31*8,0x0FFFFFFF,"Control keys");
				break;
			case 3:
				putString(50*7,31*8,0x0FFFFFFF,"Custom 1");
				break;
			case 4:
				putString(50*7,31*8,0x0FFFFFFF,"Custom 2");
				break;
			case 5:
				putString(50*7,31*8,0x0FFFFFFF,"Custom 3");
				break;
			}
			prevgroup = myKey.keygroup;
		} 
		
		//drawLower(myKey.modifiers);
		putCharC((iChars+1)*7,iLines*8,0x0FFFFFFF,' ');
		if(myKey.keychar == 13)
		{
			putCharC((iChars+1)*7,iLines*8,0x0FFFFFFF,' ');
			cLines[iLines][iChars+1]=0;
			//write2TextFile(cLines[iLines]);
			iLines++;
			iChars = 1;
		}
		else if(myKey.keycode==93)
		{
			/* Context MENU button was pressed - default to screenshot for now */
			screenshot();
		}
		else if(myKey.keychar == 8)
		{
			iChars--;
			if(iChars==0)
			{
				if(iLines>4)
				{
					putCharC((iChars+1)*7,iLines*8,0x0FFFFFFF,' ');

					iLines--;
					iChars=64;
				}
				else
				{
					iChars=1;
					putCharC((iChars+2)*7,iLines*8,0x0FFFFFFF,' ');
				}
			}
			else
			{
				putCharC((iChars+2)*7,iLines*8,0x0FFFFFFF,' ');
			}

		}
		else if(!myKey.keychar==0)
		{
			iChars++;
			if(iChars>64)
			{
				cLines[iLines][iChars+1]=0;
				//write2TextFile(cLines[iLines]);
				iLines++;
				iChars = 1;
			}
			putCharC(iChars*7,iLines*8,0x0FFFFFFF,myKey.keychar);
			cLines[iLines][iChars]=myKey.keychar;
			/*sceDisplayWaitVblankStart(); */
		}
		if(iLines>26)
		{
			cLines[iLines][iChars+1]=0;
			//write2TextFile(cLines[iLines]);
			iChars = 1;
			initScreen();
			iLines=4;
		}
		iBlink++;
		if(iBlink<20)
		{
			putCharC((iChars+1)*7,iLines*8,0x0FFFFFFF,'_');
		}
		else if(iBlink<40)
		{
			putCharC((iChars+1)*7,iLines*8,0x0FFFFFFF,' ');
		}
		else
		{
			iBlink=0;
		}
		sceDisplayWaitVblankStart(); 
	}
	return 0;
}


