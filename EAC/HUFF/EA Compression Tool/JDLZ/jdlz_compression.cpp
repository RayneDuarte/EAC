//---------------------------------------------------------------------------

#pragma hdrstop

#include "jdlz_compression.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

int JDLZ_Decompress(unsigned char *in, int insz, unsigned char *out, int outsz)
{
    unsigned char *inl = in + insz;
    unsigned char *o = out;
    unsigned char *outl = out + outsz;
    unsigned short flags1 = 1, flags2 = 1;
    int i, t, length;

    while ((in < inl) && (o < outl))
	{
        if (flags1 == 1) flags1 = *in++ | 0x100;
        if (flags2 == 1) flags2 = *in++ | 0x100;
        if (flags1 & 1)
		{
            if (flags2 & 1)
			{
                length = (in[1] | ((*in & 0xF0) << 4)) + 3;
                t = (*in & 0xF) + 1;
            }
			else
			{
                t = (in[1] | ((*in & 0xE0) << 3)) + 17;
                length = (*in & 0x1F) + 3;
            }
            in += 2;
            if ((o - t) < out) return -1;
            for (i = 0; i < length; i++)
			{
                if ((o + i + 1) > outl) break;
                o[i] = o[i - t];
			}
			o += i;
			flags2 >>= 1;
		}
		else if (o < outl) *o++ = *in++;
		flags1 >>= 1;
	}
	return o - out;
}

int JDLZ_Compress(unsigned char *input, int in_sz, unsigned char *output)
{
	int hashSize = 0x2000;
	int maxSearchDepth = 16;
	const int MinMatchLength = 3;
	int inputBytes = in_sz;

	int *hashPos = new int[hashSize];
	if (hashPos == nullptr)
	{
		return 0;
	}

	int *hashChain = new int[inputBytes];
	if (hashChain == nullptr)
	{
		delete[] hashPos;
		return 0;
	}

	int outPos = 0;
	int inPos = 0;
	unsigned char flags1bit = 1;
	unsigned char flags2bit = 1;
	unsigned char flags1 = 0;
	unsigned char flags2 = 0;

	output[outPos++] = 0x4A; // 'J'
	output[outPos++] = 0x44; // 'D'
	output[outPos++] = 0x4C; // 'L'
	output[outPos++] = 0x5A; // 'Z'
	output[outPos++] = 0x02;
	output[outPos++] = 0x10;
	output[outPos++] = 0x00;
	output[outPos++] = 0x00;
	output[outPos++] = inputBytes;
	output[outPos++] = inputBytes >> 8;
	output[outPos++] = inputBytes >> 16;
	output[outPos++] = inputBytes >> 24;
	outPos += 4;

	int flags1Pos = outPos++;
	int flags2Pos = outPos++;

	flags1bit <<= 1;
	output[outPos++] = input[inPos++];
	inputBytes--;

	while (inputBytes > 0)
	{
		int bestMatchLength = MinMatchLength - 1;
		int bestMatchDist = 0;

		if (inputBytes >= MinMatchLength)
		{
			int hash = (-0x1A1 * (input[inPos] ^ ((input[inPos + 1] ^ (input[inPos + 2] << 4)) << 4))) & (hashSize - 1);
			int matchPos = hashPos[hash];
			hashPos[hash] = inPos;
			hashChain[inPos] = matchPos;
			int prevMatchPos = inPos;

			for (int i = 0; i < maxSearchDepth; i++)
			{
				int matchDist = inPos - matchPos;

				if (matchDist > 2064 || matchPos >= prevMatchPos)
					break;

				int matchLengthLimit = matchDist <= 16 ? 4098 : 34;
				int maxMatchLength = inputBytes;

				if (maxMatchLength > matchLengthLimit)
				{
					maxMatchLength = matchLengthLimit;
				}
				if (bestMatchLength >= maxMatchLength)
					break;

				int matchLength = 0;
				while ((matchLength < maxMatchLength) && (input[inPos + matchLength] == input[matchPos + matchLength]))
					matchLength++;

				if (matchLength > bestMatchLength)
				{
					bestMatchLength = matchLength;
					bestMatchDist = matchDist;
				}

				prevMatchPos = matchPos;
				matchPos = hashChain[matchPos];
			}
		}

		if (bestMatchLength >= MinMatchLength)
		{
			flags1 |= flags1bit;
			inPos += bestMatchLength;
			inputBytes -= bestMatchLength;
			bestMatchLength -= MinMatchLength;

			if (bestMatchDist < 17)
			{
				flags2 |= flags2bit;
				output[outPos++] = ((bestMatchDist - 1) | ((bestMatchLength >> 4) & 0xF0));
				output[outPos++] = bestMatchLength;
			}
			else
			{
				bestMatchDist -= 17;
				output[outPos++] = (bestMatchLength | ((bestMatchDist >> 3) & 0xE0));
				output[outPos++] = bestMatchDist;
			}

			flags2bit <<= 1;
		}
		else
		{
			output[outPos++] = input[inPos++];
			inputBytes--;
		}

		flags1bit <<= 1;

		if (flags1bit == 0)
		{
			output[flags1Pos] = flags1;
			flags1 = 0;
			flags1Pos = outPos++;
			flags1bit = 1;
		}

		if (flags2bit == 0)
		{
			output[flags2Pos] = flags2;
			flags2 = 0;
			flags2Pos = outPos++;
			flags2bit = 1;
		}
	}

	if (flags2bit > 1)
	{
		output[flags2Pos] = flags2;
	}
	else if (flags2Pos == outPos - 1)
	{
		outPos = flags2Pos;
	}

	if (flags1bit > 1)
	{
		output[flags1Pos] = flags1;
	}
	else if (flags1Pos == outPos - 1)
	{
		outPos = flags1Pos;
	}

	delete[] hashPos;
	delete[] hashChain;

	output[12] = outPos;
	output[13] = outPos >> 8;
	output[14] = outPos >> 16;
	output[15] = outPos >> 24;
	return outPos;
}
