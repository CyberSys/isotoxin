#pragma once


struct OggVorbis_File;

#include "s3.h"

namespace s3
{

class Decoder
{
	void *flac;//������������ FLAC__StreamDecoder* ����� ������, �.�. ��-�� ���������� ����������� ���� ��������� ����� typedef � ����� stream_decoder.h, � ������������� ����������
	OggVorbis_File *oggVorbis;

	typedef int ReadFunc(char *dest, int size, void *userPtr);
	ReadFunc *readFunc;
	void *userPtr;
	unsigned signature, signatureRead;
	enum {TYPE_UNKNOWN, TYPE_FLAC, TYPE_OGGVORBIS, TYPE_WAV, TYPE_RAW} type;

	class DecIl;
	const int *flacbuffer[2];//�������� 2 ������ (������)
	int fbSize, fbRead;
	int readCompressed(char *buffer, int size);

public:
	Decoder();
	~Decoder();

	Format format;
	int samples, sampleSize;

	bool init(ReadFunc *readFunc, void *userPtr);//������������� � ���������� ��������� format
	int read(void *buffer, int size);//������ � ����� size ���� ������������� ������
};
}
