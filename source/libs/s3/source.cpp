#include "stdafx.h"
#include "slot.h"


namespace s3
{
#include "player_data.h"

Source::Source(Player *player, SoundGroup soundGroup) : player(player), soundGroup(soundGroup), nextSourceToDelete(nullptr), slotIndex(-1), pitch(1), volume(1), minDist(1), maxDist(1e9)
{
	memset(position, 0, sizeof(position));
	memset(velocity, 0, sizeof(velocity));
}

Source *Source::firstSourceToDelete = nullptr;

void Source::autoDeleteSources()
{
	for (Source **s=&firstSourceToDelete; *s;)
	{
		if ((*s)->slotIndex == -1)//���� �� ������������� - ����� �������
		{
			Source *next = (*s)->nextSourceToDelete;
			(*s)->die();
			*s = next;
		}
		else
			s=&(*s)->nextSourceToDelete;
	}
}

void Source::autoDelete()
{
	nextSourceToDelete = firstSourceToDelete;
	firstSourceToDelete = this;
}

void Source::play(bool looping, float time)
{
    player_data_s &pd = *(player_data_s *)&player->data;

	if (pd.pDS == nullptr) return;
    LOCK4WRITE(pd.sync);

	autoDeleteSources();

	if (slotIndex != -1 && time > 0)//� ���� ���������� ��������� ��������� ��������� ������������ ����� ��� ������� ������� time, �.�. � ���� ������ ������ ��������� ����� �� �����
	{
		pd.soundGroups[soundGroup].slots[slotIndex].fadeSpeed = 1/time;
		return;
	}
	if (slotIndex != -1) {ErrorHandler(EL_ERROR, "Sound source is already playing"); return;}
	//if (format.channels == 0) {ErrorHandler(EL_ERROR, "Sound format is not specified"); return;}

	rewind(true);
	if (!pd.gdecoder->init(readFunc, this)) {ErrorHandler(EL_ERROR, "Decoder error"); return;}

	int found = -1;
	SoundGroupSlots &sg = pd.soundGroups[soundGroup];
	for (int i=0; i<sg.active; i++)//�������, �� ����������� �� �������� ����
	{
		if (sg.slots[i].source == nullptr)
		{
			found = i;
			if (memcmp(&sg.slots[i].format, &pd.gdecoder->format, sizeof(Format)) == 0) goto skip_create;//perfect match!
		}
	}

	if (found != -1)//���� ��������, �� ������ �� ��������� - ����� ����������� �����
	{
		sg.slots[found].releaseBuffer();
	}
	else//��������� ������ ���
	{
		if (sg.active == sg.max) {ErrorHandler(EL_WARNING, "Not enought slots to play sound"); return;}
		found = sg.active++;//add new slot
	}

	if (!sg.slots[found].createBuffer(player, pd.gdecoder->format, sg.is3d)) return;

skip_create:

	Slot &slot = sg.slots[found];
	std::swap(pd.gdecoder, slot.decoder);

	slotIndex = found;
	slot.source = this;
	slot.looping = looping;
	slot.startPlay(player,time);
}

void Source::stop(float time)
{
	if (slotIndex == -1) return;//��������������� �������, �� �������� ��������, ������� ��������� �� �������, ��� ������ int - ��������� ��������, � � ������������ ������ ����� ���� �������� ������ ���� �������� (-1)
    player_data_s &pd = *(player_data_s *)&player->data;
	if (pd.pDS == nullptr) return;
    LOCK4WRITE(pd.sync);
	if (slotIndex == -1) {/*ErrorHandler(EL_ERROR, "Sound source is already stopped");*/ return;}
	pd.soundGroups[soundGroup].slots[slotIndex].stop(time);
}


int MSource::read(char *dest, int sz)
{
	int r = std::min(sz, size-pos);
	memcpy(dest, data+pos, r);
	pos += r;
	return r;
}

void MSource::init(const void *data_, int size_)
{
	if (slotIndex != -1) {ErrorHandler(EL_ERROR, "init() can not be called during playing"); return;}

	data = (const char*)data_;
	size = size_;
//	pos = 0;//��� �� �����������, �.�. ����� ������� ������������ �������������� ��������� rewind()
	if (startPlayOnInit >= 0 && data)
	{
		Source::play(true, startPlayOnInit);
		startPlayOnInit = -1;
	}
}

void MSource::play(bool looping, float time)
{
    player_data_s &pd = *(player_data_s *)&player->data;
	if (pd.pDS == nullptr) return;

	if (data == nullptr) // no data (yet)
	{
		if (looping) startPlayOnInit = time; // start playing on data
		return;
	}

	Source::play(looping, time);
}


PSource::PSource(Player *player, SoundGroup soundGroup, bool looped, bool activate_, int prefetchBufferSize, char *extPrefetchBuffer) : Source(player, soundGroup), looped(looped), startPlayOnPrefetchComplete(-1), active(false)
{
	if (prefetchBufferSize >= 0) init(activate_, prefetchBufferSize, extPrefetchBuffer);
}

void PSource::init(bool activate_, int prefetchBufferSize, char *extPrefetchBuffer)
{
	prefetchBytes = prefetchBufferSize ? prefetchBufferSize/2 : player->params.prefetchBytes;
	if (extPrefetchBuffer) {prefetchBuffer = nullptr; buf[0] = extPrefetchBuffer;} else
	buf[0] = prefetchBuffer = new char [2*prefetchBytes];
	buf[1] = buf[0] + prefetchBytes;
	if (activate_) activate();
//	prefetch(prefetchBuffer, 2*prefetchBytes);
}

void PSource::activate()
{
	if (active) {ErrorHandler(EL_ERROR, "Already in an active state"); return;}

	curBuf = prBuf = bufPos = 0;
	actualDataSize[0] = actualDataSize[1] = 0;
	eofPos[0] = eofPos[1] = prefetchBytes;
	waitingForPrefetchComplete = false;
//	eof = false;//��� �� �����������, �.�. ����� ������� ������������ �������������� ��������� rewind()
	active = true;
}

int PSource::read(char *dest, int sz)
{
	if (eof) return 0;

	int total = 0;

	while (sz > 0 && actualDataSize[curBuf] > 0)
	{
		int r = std::min(sz, std::min(actualDataSize[curBuf], eofPos[curBuf]) - bufPos);
		memcpy(dest + total, buf[curBuf] + bufPos, r);
		bufPos += r;
		sz -= r;
		total += r;
		if (bufPos == prefetchBytes)
		{
			bufPos = 0;
			actualDataSize[curBuf] = 0;
			eofPos[curBuf] = prefetchBytes;
			curBuf ^= 1;
		}
		else if (bufPos == eofPos[curBuf])
		{
			eofPos[curBuf] = actualDataSize[curBuf];//������� �������, ���� ������������ � �� ����. ���� ������
			eof = true;
			break;
		}
	}

	if (sz > 0)//eof reached (eof == true ��� ������ �� ������ ������������)
		if (!looped) active = false;//��� ������ ������ - ����� �������������� PSource

	return total;
}

void PSource::rewind(bool start)
{
	eof = false;
	if (!start && eofPos[curBuf] < prefetchBytes)
	{
		bufPos = eofPos[curBuf];//���������� "�����" � ����� ����� (��������� ��������� ����� ��������� ��������� ����� � �����), ����� ������������ �� ����� �������� � ���� �������
		eofPos[curBuf] = actualDataSize[curBuf];//������� �������, ���� ������������ � �� ����. ���� ������
	}
}

bool PSource::prefetchComplete(int size)
{
    player_data_s &pd = *(player_data_s *)&player->data;
    LOCK4WRITE(pd.sync);//�������� ������ � actualDataSize �� read(), ������� ���������� � ������������ ������
	bool res = false;

	do//���� ����� ������ ��� ��������� ������ ��������� �������� 2*prefetchBytes ����, �.�. ����� ����� ����� ���� ��������� �� ������� ���-�� ����
	{
		int r = std::min(size, prefetchBytes - actualDataSize[prBuf]);
		actualDataSize[prBuf] += r;
		size -= r;
		if (actualDataSize[prBuf] == prefetchBytes) prBuf ^= 1;
		else//������ �������� ������ ��������, ��� ��������� ����� ����� - ������ ��������������� �������
		{
			if (eofPos[prBuf] == prefetchBytes)//�� ������ ��� �������, ��� ��� ������� ��� �� ����� (����� ��� �������� ������, ������ ������� ������ prefetchBytes)
			{
				_ASSERT(size == 0);
				eofPos[prBuf] = actualDataSize[prBuf];//��� ���� ����������� ���������� ������� (��� ��� ���������� �����)
			}
			res = true;
		}
	} while (size > 0);

	waitingForPrefetchComplete = false;

// 	if (startPlayOnPrefetchComplete >= 0)//�.�. prefetchComplete ����� ���������� �� ������� ������, ����� ��������� ���� � update (the thread that plays the buffer must continue to exist as long as the buffer is playing)
// 	{
// 		Source::play(looped, startPlayOnPrefetchComplete);
// 		startPlayOnPrefetchComplete = -1;
// 	}

	return res;
}

void PSource::play(float time)
{
	if (!active) {ErrorHandler(EL_ERROR, "PSource should be activated before play"); return;}
    
    player_data_s &pd = *(player_data_s *)&player->data;

	if (pd.pDS == nullptr) return;
    LOCK4WRITE(pd.sync); //�������� ������ � actualDataSize �� read(), ������� ���������� � ������������ ������

	if (waitingForPrefetchComplete || (actualDataSize[0] == 0 && actualDataSize[1] == 0))
	{
		startPlayOnPrefetchComplete = time;
		return;
	}

	Source::play(looped, time);
}

bool PSource::update()
{
    player_data_s &pd = *(player_data_s *)&player->data;
	if (pd.pDS == nullptr) return false;
    LOCK4WRITE(pd.sync);//�������� ������ � actualDataSize �� read(), ������� ���������� � ������������ ������

	if (!active) return slotIndex == -1;

	DWORD status;
	if (slotIndex != -1 && pd.soundGroups[soundGroup].slots[slotIndex].buffer->GetStatus(&status) == DSERR_BUFFERLOST)//����� ����� ���� ��� ������������ ��������� � Windows 7
	{
		player->Shutdown(true);//try reinit
		if (pd.pDS == nullptr) return false;
	}

	if (!waitingForPrefetchComplete)
	{
		if (!looped && eofPos[prBuf] < prefetchBytes) ;//��������� ����� ����� �������������� ����� - ������ �� ������
		else if (actualDataSize[prBuf] == (eofPos[prBuf] < prefetchBytes ? eofPos[prBuf] : 0))
		{
			waitingForPrefetchComplete = true;

			if (actualDataSize[0] == 0 && actualDataSize[1] == 0)//������ ��� ������ ��� ������ �����
				prefetch(buf[0], 2*prefetchBytes);
			else
				prefetch(buf[prBuf]+actualDataSize[prBuf], prefetchBytes-actualDataSize[prBuf]);
		}

		if (startPlayOnPrefetchComplete >= 0 && actualDataSize[0] > 0)//�������� ������ ��� ������ ��������� ������
		{
			Source::play(looped, startPlayOnPrefetchComplete);
			startPlayOnPrefetchComplete = -1;
		}
	}

	return slotIndex == -1;
}


int RawSource::read(char *dest, int sz)
{
	switch (readStage)
	{
	case 0:
		_ASSERT(sz == 4);
		*(DWORD*)dest = MAKEFOURCC('R','A','W',' ');
		readStage++;
		return 4;

	case 1:
		_ASSERT(sz == sizeof(Format));
		memcpy(dest, &format, sizeof(Format));
		readStage++;
		return sizeof(Format);

	default:
		return rawRead(dest, sz);
	}
}

void RawSource::rewind(bool start)
{
	readStage = 0;
	rawRewind(start);
}
}
