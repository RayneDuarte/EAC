//---------------------------------------------------------------------------

#ifndef comp_encodeH
#define comp_encodeH
//---------------------------------------------------------------------------
#endif

#pragma once
class COMP_Encoder
{
	private: int COMP_MagicID = 0x504D4F43;
    private: int version = 0x00001001;

	public: int COMP_encode(const unsigned char *src, int sourcesize, unsigned char *out);
	public: void COMP_createHeader(unsigned char *hdr, int usize, int zsize);
};
