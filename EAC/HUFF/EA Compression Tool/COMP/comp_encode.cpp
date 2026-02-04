//---------------------------------------------------------------------------

#pragma hdrstop

#include "comp_encode.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

int COMP_Encoder::COMP_encode(const unsigned char *src, int sourcesize, unsigned char *out)
{
	int pos = 0, out_size = 0;

	while (pos < sourcesize)
	{
		uint16_t flags = 0;
		int flagBit = 0;
		size_t flagsPos = out_size;

		out[out_size++] = 0; //placeholder
		out[out_size++] = 0;

		uint8_t chunk[48];
		int chunk_sz = 0;

		for (int i = 0; i < 16 && pos < sourcesize; i++)
		{
			int bestLen = 0;
			int bestOff = 0;

			//int start = std::max(0, pos - 4095);
			int start = (pos > 4095) ? int(pos - 4095) : 0;

			for (int j = start; j < pos; j++)
			{
				int k = 0;
				while (k < 18 &&
					   pos + k < sourcesize &&
					   src[j + k] == src[pos + k])
				{
					k++;
				}

				if (k > bestLen)
				{
					bestLen = k;
					bestOff = pos - j;
				}
			}

			if (bestLen >= 3)
			{
				flags |= (1 << flagBit);

				int length = bestLen;
				int offset = bestOff;

				uint8_t c = ((offset >> 4) & 0xF0) | ((length - 3) & 0x0F);
				uint8_t d = offset & 0xFF;

				chunk[chunk_sz++] = c;
				chunk[chunk_sz++] = d;
				pos += length;
			}
			else
			{
				chunk[chunk_sz++] = src[pos++];
			}

			flagBit++;
		}

		// write flags
		out[flagsPos + 0] = flags & 0xFF;
		out[flagsPos + 1] = (flags >> 8) & 0xFF;

		//write data
		unsigned char *o = out + out_size;
		memcpy(o, chunk, chunk_sz);
		out_size += chunk_sz;
	}
	return out_size;
}

void COMP_Encoder::COMP_createHeader(unsigned char *hdr, int usize, int zsize)
{
	int *h = (int*)hdr;
	*h++ = COMP_MagicID;
	*h++ = version;
	*h++ = usize;
	*h = (zsize + 16);
}

