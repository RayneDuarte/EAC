// If the compiler mostly UNIX compatible
#if defined __unix__ || defined __MINGW32__ || defined __MINGW64__
	#define EAC_UNIX
#endif

#ifdef EAC_UNIX
	#include "eac_unix.cpp"
#endif

#include <iostream>
#include <tchar.h>
#include "codex.h"
#include "huffcodex.h"
#include "refcodex.h"
#include "btreecodex.h"
#include "jdlz_compression.h"
#include "ea_comp.h"
#include <locale.h>

int ReadUint32(FILE *f);
void WriteUint32(FILE *f, int n);
unsigned char *alloc_mem(int size);
void CloseFiles(FILE *infile, FILE *outfile, char *outfilename);
void ED_Error(unsigned char *unp_data, unsigned char *comp_data, char *infilename, int err_code);
void OutOfMemory(FILE *infile, FILE *outfile, char *outfilename, int err_code);
void WriteUint32LE_InBuf(unsigned char *data, int n);
void CreateHUFFHeader(unsigned char *header, int ulen, int zsize);
int GetFilesize(FILE *f);
void Help();

int _tmain(int argc, _TCHAR* argv[])
{
	setlocale(LC_ALL, "Portuguese");
	char *infilename, *outfilename;
	if (argc == 2 && strcmp(argv[1], "-h") == 0)
	{
		Help();
		return 0;
	}

	//ea_compression_tool.exe mode cformat infilename outfilename
	if (argc > 6 || argc < 4)
	{
		printf("Too many or too few args. Must be 5 args to encode or 4 args to decode");
		return 0;
	}

	int huff_comp_type = -1;

	if (argc == 5 || argc == 6)
	{
		if (strcmp(argv[2], "HUFF") != 0 &&
			strcmp(argv[2], "JDLZ") != 0 &&
			strcmp(argv[2], "REF") != 0 &&
			strcmp(argv[2], "BTREE") != 0 &&
			strcmp(argv[2], "COMP") != 0)
		{
			printf("The '%s' compression format is not supported! Must be HUFF, JDLZ, REF or BTREE", argv[2]);
			return 0;
		}
		if (strcmp(argv[2], "HUFF") == 0)
		{
			if (strcmp(argv[3], "-0") == 0)
				huff_comp_type = 0;
			else if (strcmp(argv[3], "-1") == 0)
				huff_comp_type = 1;
			else if (strcmp(argv[3], "-2") == 0)
				huff_comp_type = 2;
			else
			{
				printf("The compression mode for the HUFF compression is invalid.\n");
				printf("Must be -0, -1 or -2. The NFS Most Wanted and NFS Carbon\n");
				printf("games uses the 0 mode, 0x30FB header.\n");
				return 0;
			}
			infilename = argv[4];
			outfilename = argv[5];
		}
		else
		{
			infilename = argv[3];
			outfilename = argv[4];
		}
	}
	else
	{
		infilename = argv[2];
		outfilename = argv[3];
	}

	if (strcmp(argv[1], "-d") != 0 &&
		strcmp(argv[1], "-c") != 0)
	{
		printf("Invalid modes. Must be -d to decode a file or -c to encode");
		return 0;
	}


	FILE *infile = fopen(infilename, "rb");
	if (!infile)
	{
		printf("Unable to access the '%s' input file", infilename);
		return 0;
	}
	FILE *outfile = fopen(outfilename, "wb");
	if (!outfile)
	{
		printf("Unable to create the '%s' output file", outfilename);
		fclose(infile);
		return 0;
	}


	unsigned char *comp_data = NULL;
	unsigned char *unp_data = NULL;
	int ret_value = 0;
	int unpacked_size, z_size;

	if (strcmp(argv[1], "-d") == 0)
	{ //decompress a file
		char hdr[5];
		fread(hdr, 1, 4, infile);
		hdr[4] = 0;

		if (strcmp(hdr, "JDLZ") == 0 ||
			strcmp(hdr, "HUFF") == 0 ||
			strcmp(hdr, "COMP") == 0)
		{
			fseek(infile, 8, SEEK_SET);
			unpacked_size = ReadUint32(infile);
			z_size = ReadUint32(infile);
			comp_data = alloc_mem(z_size);
			if (!comp_data)
			{
				OutOfMemory(infile, outfile, outfilename, 1);
				return 0;
			}

			unp_data = alloc_mem(unpacked_size);
			if (!unp_data)
			{
				free(comp_data);
				OutOfMemory(infile, outfile, outfilename, 2);
				return 0;
			}
			if (hdr[0] == 'C') z_size -= 16;
			fread(comp_data, 1, z_size, infile);

			if (strcmp(hdr, "JDLZ") == 0)
			{
				ret_value = JDLZ_Decompress(comp_data, z_size, unp_data, unpacked_size);
			}
			else if (strcmp(hdr, "COMP") == 0)
			{
				ret_value = COMP_Decompress(comp_data, z_size, unp_data, unpacked_size);
			}
			else
			{
				if (HUFF_is(comp_data))
					ret_value = HUFF_decode(unp_data, comp_data, &z_size);
			}
		}
		else
		{
			z_size = GetFilesize(infile);
			comp_data = alloc_mem(z_size);
			if (!comp_data)
			{
				OutOfMemory(infile, outfile, outfilename, 1);
				return 0;
			}
			fread(comp_data, 1, z_size, infile);
			if (REF_is(comp_data))
				unpacked_size = REF_size(comp_data);
			else if (BTREE_is(comp_data))
				unpacked_size = BTREE_size(comp_data);

			if (unpacked_size != 0)
			{
				unp_data = alloc_mem(unpacked_size);
				if (!unp_data)
				{
					free(comp_data);
					OutOfMemory(infile, outfile, outfilename, 2);
					return 0;
				}
				if (REF_is(comp_data))
					ret_value = REF_decode(unp_data, comp_data, &z_size);
				else
					ret_value = BTREE_decode(unp_data, comp_data, &z_size);
			}
			else unpacked_size = 1;
		}

		if (ret_value != unpacked_size)
		{
			ED_Error(unp_data, comp_data, infilename, 1);
			CloseFiles(infile, outfile, outfilename);
			return 0;
		}
		//write the decompressed data to disk
		fwrite(unp_data, 1, unpacked_size, outfile);
	}
	else
	{  //compress a file
		fseek(infile, 0, SEEK_END);
		int in_sz = ftell(infile);
		rewind(infile);

		unp_data = alloc_mem(in_sz);
		if (!unp_data)
		{
			OutOfMemory(infile, outfile, outfilename, 1);
			return 0;
		}

		comp_data = alloc_mem(in_sz * 2);
		if (!comp_data)
		{
			free(unp_data);
			OutOfMemory(infile, outfile, outfilename, 2);
			return 0;
		}
		fread(unp_data, 1, in_sz, infile);

		if (strcmp(argv[2], "HUFF") == 0)
		{
			ret_value = HUFF_encode(comp_data, unp_data, in_sz, &huff_comp_type);
		}
		else if (strcmp(argv[2], "JDLZ") == 0)
		{
			ret_value = JDLZ_Compress(unp_data, in_sz, comp_data);
		}
		else if (strcmp(argv[2], "REF") == 0)
		{
			ret_value = REF_encode(comp_data, unp_data, in_sz, 0);
		}
		else if (strcmp(argv[2], "BTREE") == 0)
		{
            ret_value = BTREE_encode(comp_data, unp_data, in_sz, 0);
		}

		if (!ret_value)
		{
			ED_Error(unp_data, comp_data, infilename, 2);
			CloseFiles(infile, outfile, outfilename);
			return 0;
		}

		if (strcmp(argv[2], "HUFF") == 0)
		{
			unsigned char huff_hdr[16];
			CreateHUFFHeader(huff_hdr, in_sz, ret_value);
			fwrite(huff_hdr, 1, 16, outfile);
		}
		//write the compressed data to disk
		fwrite(comp_data, 1, ret_value, outfile);
	}

	fclose(infile);
	fclose(outfile);
	free(unp_data);
	free(comp_data);
	return 1;
}

void ED_Error(unsigned char *unp_data, unsigned char *comp_data, char *infilename, int err_code)
{
	if (err_code)
	{
		printf("Has ocurred an error during the decompression of file '%s'\n", infilename);
		printf("The file it seems corrupted\n");
	}
	else
		printf("Has ocurred an error during the compression of file '%s'\n", infilename);

	if (unp_data != NULL) free(unp_data);
	if (comp_data != NULL) free(comp_data);
}

void OutOfMemory(FILE *infile, FILE *outfile, char *outfilename, int err_code)
{
	if (err_code == 1) printf("Unable to allocate memory to read the input data");
	else printf("Unable to allocate memory to save the output data");
	CloseFiles(infile, outfile, outfilename);
}

void CloseFiles(FILE *infile, FILE *outfile, char *outfilename)
{
	fclose(infile);
	fclose(outfile);
	remove(outfilename);
}

int ReadUint32(FILE *f)
{
	unsigned char data[4];
	fread(data, 1, 4, f);
	return (((data[3] << 24) | (data[2] << 16)) | ((data[1] << 8) | data[0]));
}

void WriteUint32(FILE *f, int n)
{
	unsigned char data[4];
	data[0] = n;
	data[1] = n >> 8;
	data[2] = n >> 16;
	data[3] = n >> 24;
	fwrite(data, 1, 4, f);
}

unsigned char *alloc_mem(int size)
{
	return (((unsigned char*)malloc(sizeof(unsigned char)* size)));
}

void CreateHUFFHeader(unsigned char *header, int ulen, int zsize)
{
	const char *huff_id = "HUFF";
	memset(header, 0, 16);
	memcpy(header, huff_id, 4);
	header[4] = 0x01;
	header[5] = 0x10;
	WriteUint32LE_InBuf(header + 8, ulen);
	WriteUint32LE_InBuf(header + 12, zsize);
}

void WriteUint32LE_InBuf(unsigned char *data, int n)
{
	*data++ = n;
	*data++ = n >> 8;
	*data++ = n >> 16;
	*data++ = n >> 24;
}

int GetFilesize(FILE *f)
{
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	rewind(f);
	return size;
}

void Help()
{
	printf("\nEA Compression Tool\n\n");
	#ifndef EAC_UNIX
		wprintf(L"Coded by Raynê Games\n\n");
	#else
		printf("Coded by Rayne Games\n\n");
	#endif
	printf("WARNING: Contains proprietary code of EA!!\n\n");
	printf("Use this tool to encode/decode the files compressed in HUFF, JDLZ, REF and BTREE formats.\n\n");
	printf("The JDLZ compression is based on LZMA and it is often used to compress VPAK files\n");
	printf("and also BUN/LZC files and some other EA games, outside of Need for Speed.\n\n");
	printf("The HUFF compression is based on Huffman coding (Huffman with Runlength Codex),\n");
	printf("which it is used in many EA games as Need for Speed Most Wanted and the Carbon\n");
	printf("to store texture data. The Huffman reach a compression ratio better than JDLZ.\n\n");
	printf("Usage:\nea_compression_tool.exe mode cformat -v infile outfile\n\n");
	printf("mode: -d to decode a file, -c to encode a file\n\n");
	printf("cformat: HUFF, JDLZ, REF and BTREE. If the -c mode is used,\nallows you choose which compression format ");
	printf("will used to compress the input file.\n\n");
	printf("-v: Only to the HUFF format. Allow you choose a HUFF compression variant.\n");
	printf("The following variants are available:\n");
	printf("-0: 0x30fb header. Used in games like NFS Most Wanted and NFS Carbon\n");
	printf("-1: 0x32fb header. Probably used in other EA games\n");
    printf("-2: 0x34fb header. Probably used in other EA games\n");
	printf("\n\nExample:\n");
	printf("ea_compression_tool.exe -c -0 HUFF infile outfile\n");
	printf("The args above compress the input file with the HUFF compression and save the data to output file.\n");
	printf("The file is compressed with the -0 variant, 0x30fb header\n");
	printf("\n\nTo decompress a file, select the -d mode and the infile name and the outfile name\n");
	printf("Example: ea_compression_tool.exe -d GAMEPLAY.LZC decoded\n");
	printf("The tool automatically detect the compression format\n\n");
    printf("About the REF and BTREE formats\n\n");
	printf("The REF a.k.a refpack is other compression format developed by EA for use in some of its games.\n");
	printf("I don't know exactly which games use this compression, but you can encode and decode files with\n");
	printf("the REF encoding when choosing the REF argument in the -c mode.\n\n");
	printf("For files larger than 0xffffff, the 0x90fb header is used.\n");
	printf("For files smaller than 0xffffff, the 0x10fb header is used.\n\n");
	printf("BTREE format\n\n");
	printf("Is other compression format also developed by EA. I don't know which games use this compression,\n");
	printf("but you can encode/decode files encoded with BTREE using this tool.\n");
    printf("The headers used by this format can be 0x46fb or 0x47fb.\n");
}

