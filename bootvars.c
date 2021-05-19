/*
 * Boot Variables - a program to display and change Open Firmware
 * variables under MacOS.
 *
 * This file and the accompanying resource file (bootvars.rsrc) are
 * Copyright (C) 1996 Paul Mackerras.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
 
/* 
 * 12-28-96 Michael Tesch:
 * Modified to support a config file & AppleEvent beginnings.
 * Added an auto-reboot feature.
 * Added some error reporting stuff.
 *
 * 12 January 97 Fred Dushin <fadushin@top.cis.syr.edu>
 * Re-wrote gui elements in gui.cp module.
 * Put some types and defines in bootvars header file
 * Added #includes for all prototypes.
 */
#include "bootvars.h" // added fadushin 
#include <stdio.h>
#include <string.h> // for memcpy() and memset()


short *item_vars;
int nvitems;
int file_io;
int config_file;
short fsRefNum;
char reboot;
buffer nvbuf;

int nv_access_method;          


asm void eieio()
{
	eieio
	blr
}



int
nvreadb(unsigned adr)
{
        switch (nv_access_method) {
        default:
                *NVADRS = adr >> 5;
                eieio();
                return NVDATA[(adr & 0x1f) << 4];
        case 1:
                return NVDATA_OHARE[(adr & 0x1fff) << 4];
        }
}


void
nvwriteb(unsigned adr, int v)
{
        switch (nv_access_method) {
        default:
                *NVADRS = adr >> 5;
                eieio();
                NVDATA[(adr & 0x1f) << 4] = v;
                break;
        case 1:
                NVDATA_OHARE[(adr & 0x1fff) << 4] = v;
                break;
        }
        eieio();
}

/*
If it's a powerbase, we'll read 0 from nvram location 0.  But location
0 might have 0 in it anyway, so we write something else and see if it
sticks.  The read of location 0x1fff is just to make sure that the bus
signals get set to something different before we see whether the write
to location 0 worked or not.  If you don't do that, you can read back
what you've written just because of the capacitance of the bus lines -
they will tend to hold the last value driven on to them.  The write of
0 to location 0 is to restore what was there before.
*/
void
nvinit( void )
{
	//unsigned char x;
	int x;
	
	//SysBeep( 30 );
	nv_access_method = 0;
	if ( nvreadb(0) != 0 ){
		return;
	}
	nvwriteb(0, 0xaa);
	nvreadb(0x1fff);
	x = nvreadb(0);
	nvwriteb(0, 0);
	if (x == 0xaa){
		return;
	}

	nv_access_method = 1;
	//SysBeep( 30 );
}            


int
nvcsum()
{
	int i;
	unsigned c;
	
	c = 0;
	for (i = 0; i < NVSIZE/2; ++i)
		c += nvbuf.s[i];
	c = (c & 0xffff) + (c >> 16);
	c += (c >> 16);
	return c & 0xffff;
}

int
nvload()
{
	int i;
	int s;
	
	for (i = 0; i < NVSIZE; ++i) {
		nvbuf.c[i] = nvreadb(i + NVSTART);
	}
		
	s = nvcsum();
	return s == 0xffff;
}

void
nvstore()
{
	int i;
		for (i = 0; i < NVSIZE; ++i)
			nvwriteb(i + NVSTART, nvbuf.c[i]);
}



nvvar nvvars[] = {
	"little-endian?", boolean, /* 0 */
	"real-mode?", boolean,
	"auto-boot?", boolean,
	"diag-switch?", boolean,
	"fcode-debug?", boolean,
	"oem-banner?", boolean,    /* 5 */
	"oem-logo?", boolean,
	"use-nvramrc?", boolean,
	"real-base", word,
	"real-size", word,
	"virt-base", word,         /* 10 */
	"virt-size", word,
	"load-base", word,
	"pci-probe-list", word,
	"screen-#columns", word,
	"screen-#rows", word,      /* 15 */
	"selftest-#megs", word,
	"boot-device", string,
	"boot-file", string,
	"diag-device", string,
	"diag-file", string,       /* 20 */
	"input-device", string,
	"output-device", string,
	"oem-banner", string,
	"oem-logo", string,
	"nvramrc", string,         /* 25 */
	"boot-command", string,
};

#define N_NVVARS	(sizeof(nvvars) / sizeof(nvvars[0]))

// added fadushin (because above macro won't work for extern declarations)
size_t
getNumNVVars()
{
	return N_NVVARS;
}

nvval nvvals[N_NVVARS];

unsigned char nvstrbuf[NVSIZE];
int nvstr_used;

// reads the nvbuf data, and copies it into the local buffer (nvvals)
void nvunpack()
{
	int i;
	unsigned long bmask;
	int vi, off, len;

	nvstr_used = 0;	
	bmask = 0x80000000;
	vi = 0;
	for (i = 0; i < N_NVVARS; ++i) {
		switch (nvvars[i].type) {
		case boolean:
			nvvals[i].word_val = (nvbuf.nv.bits & bmask)? 1: 0;
			bmask >>= 1;
			break;
		case word:
			nvvals[i].word_val = nvbuf.nv.vals[vi++];
			break;
		case string:		
			off = nvbuf.nv.vals[vi] >> 16;
			len = nvbuf.nv.vals[vi++] & 0xffff;
			if (off < NVSTART + sizeof(struct nvram) || off >= NVSTART + NVSIZE
				|| len > NVSTART + NVSIZE - off) {
				len = 0;
				off = NVSTART;
			}
			nvvals[i].str_val = nvstrbuf + nvstr_used + 1;
			nvvals[i].word_val = len;
			nvstrbuf[nvstr_used] = len < 255? len: 255;
			memcpy(nvvals[i].str_val, nvbuf.c + off - NVSTART, len);
			nvvals[i].str_val[len] = 0;
			nvstr_used += len + 2;
			break;
		}
	}
}

// copies&organizes all the buffered states into the nvbuf structure.
void
nvpack()
{
	int i, off, len, vi;
	unsigned long bmask;
	
	bmask = 0x80000000;
	vi = 0;
	off = NVSIZE;
	nvbuf.nv.bits = 0;
	for (i = 0; i < N_NVVARS; ++i) {
		switch (nvvars[i].type) {
		case boolean:
			if (nvvals[i].word_val)
				nvbuf.nv.bits |= bmask;
			bmask >>= 1;
			break;
		case word:
			nvbuf.nv.vals[vi++] = nvvals[i].word_val;
			break;
		case string:
			len = nvvals[i].word_val;
			if (len > off - (NVSTART + sizeof(struct nvram)))
				len = 0;
			off -= len;
			memcpy(nvbuf.c + off, nvvals[i].str_val, len);
			nvbuf.nv.vals[vi++] = ((off + NVSTART) << 16) + len;
			break;
		}
	}
	nvbuf.nv.magic = 0x1275;
	nvbuf.nv.cksum = 0;
	nvbuf.nv.end_vals = NVSTART + (unsigned) &nvbuf.nv.vals[vi] - (unsigned) &nvbuf;
	nvbuf.nv.start_strs = off + NVSTART;
	memset(&nvbuf.c[nvbuf.nv.end_vals - NVSTART], 0, nvbuf.nv.start_strs - nvbuf.nv.end_vals);
	nvbuf.nv.cksum = ~nvcsum();
}

