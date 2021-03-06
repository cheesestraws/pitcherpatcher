#include <stdio.h>
#include <string.h>

#include <Resources.h>
#include <Memory.h>

#include "bootvars.h"

#define USE_NVRAMRC_ID 7
#define NVRAMRC_ID 25

// print n characters from a char*
void printl(char* c, int len) {
	int i;
	char* k;
	
	for (i = 0; i < len; i++) {
		k = c + i;
		printf("%c", *k);
	}

}

int main(void)
{
	Handle resHndl;
	int resSize;
	int nvrl;
	int w;
	char inbuf[256];

	printf("The pitcher patcher, a stupid program that does stupid things\n\n");
	
	/* Get our resource */
	resHndl = Get1Resource('NVRM', 128);
	if (ResError() < 0) {
		printf("Could not load patch resource: %d\n", ResError());
	}
	
	resSize = GetHandleSize(resHndl);
	printf("Patch size: %d bytes\n\nPatch content:\n", resSize);	
	printl(*resHndl, resSize);
	
	printf("\n\nAre you sure you want to continue?  If this breaks, you get to keep both pieces.\n[y/n]?\n");
	gets(inbuf);
	printf("\n");
	if (inbuf[0] != 121) {
		printf("OK, bye\n");
		return 0;
	}
	
	// Grab the contents of VRAM and unpack it
	
	nvinit();
	if (!nvload()) {
		printf("Your NVRAM seems to have a gibberish checksum.  Are you sure you want to continue? [y/n]\n");
		gets(inbuf);
		printf("\n");
		if (inbuf[0] != 121) {
			printf("OK, bye\n");
			return 0;
		}
	}
	nvunpack();
	
	// Now extract the current nvramrc and print it	
	nvrl = nvvals[NVRAMRC_ID].word_val;
	
	printf("\nYour current nramrc (%d bytes) is:\n", nvrl);
	printl((char*)nvvals[NVRAMRC_ID].str_val, nvrl);
	
	printf("\n\nReally replace this [y/n]?\n");
	gets(inbuf);
	printf("\n");
	if (inbuf[0] != 121) {
		printf("OK, bye\n");
		return 0;
	}
	
	// Let's try replacing it
	
	// Do we need to move up the rest of the strings?
	if (nvrl != resSize) {
			memmove(nvvals[NVRAMRC_ID].str_val + resSize, nvvals[NVRAMRC_ID].str_val + nvrl,
				(nvstrbuf + nvstr_used) - (nvvals[NVRAMRC_ID].str_val + nvrl));
			for (w = NVRAMRC_ID; ++w < getNumNVVars(); )
				if (nvvars[w].type == string)
					nvvals[w].str_val += resSize - nvrl;
			nvstr_used += resSize - nvrl;
	}
	
	memcpy(nvvals[NVRAMRC_ID].str_val, *resHndl, resSize);
	nvvals[NVRAMRC_ID].word_val += resSize - nvrl;
	
	// We've installed it, now we set it to be used
	nvvals[USE_NVRAMRC_ID].word_val = 1;
	
	nvpack();
	nvstore();
	
	printf("nvramrc now installed.  Have a good day.\n");
	
	ReleaseResource(resHndl);
	
	return 0;
}