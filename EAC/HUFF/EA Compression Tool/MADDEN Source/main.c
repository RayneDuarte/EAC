#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "ea_madden.h"

/* run this program using the console pauser or add your own getch, system("pause") or input loop */

int main()
{
	FILE *filelist = fopen("ea_madden_files", "r");
	char infilename[MAX_PATH], outfilename[MAX_PATH];

	fscanf(filelist, "%s\n", infilename);
	fscanf(filelist, "%s\n", outfilename);
	fclose(filelist);
	
	FILE *in = fopen(infilename, "rb");
	FILE *out = fopen(outfilename, "wb");
	
	fclose(in);
	fclose(out);
	return 0;
}

