#include "stdafx.h"

#include "slot.h"
#include "fmt_wav.h"

namespace s3
{

#include "player_data.h"
#include "capture_data.h"

    void DefaultErrorHandler(ErrorLevel el, const char *err)
    {
        printf(el == EL_ERROR ? "s3 ERROR: " : "s3 warning: ");
        printf(err);
        printf("\n");
    }


    struct player_core_data_s
    {
        //LPDIRECTSOUND8 pDS = nullptr;
        //HWND hwnd = nullptr;
        //LPDIRECTSOUND3DLISTENER8 dsListener = nullptr;
        //LPDIRECTSOUNDBUFFER dsPrimaryBuffer = nullptr;
        //HANDLE hQuitEvent = nullptr, hUpdateThread = nullptr;

        void run_thread(Player *pl);

        static player_core_data_s & cvt(player_data_s &pd)
        {
            return *(player_core_data_s *)&pd.coredata;
        }

    };

    //static_assert(sizeof(player_core_data_s) == PLAYER_COREDATA_SIZE, "player data size miss");

    /*
    DWORD WINAPI UpdateThreadProc(LPVOID pData)
    {
        Player *player = (Player *)pData;
        player_data_s &pd = player_data_s::cvt(player);
        player_core_data_s &pcd = player_core_data_s::cvt(pd);

        DWORD prevTime = timeGetTime();

        while (WaitForSingleObject(pcd.hQuitEvent, 17) != WAIT_OBJECT_0)
        {
            DWORD time = timeGetTime();
            float dt = (time - prevTime)*(1 / 1000.f);
            if (pd.update(player, dt))
                prevTime = time;
        }

        return 0;
    }
    */

    void player_core_data_s::run_thread(Player *pl)
    {
        /*
        if (hQuitEvent && !hUpdateThread)
        {
            hUpdateThread = CreateThread(nullptr, 0, &UpdateThreadProc, pl, 0, nullptr);
            SetThreadPriority(hUpdateThread, THREAD_PRIORITY_HIGHEST);
        }
        */
    }


    struct slot_data_s
    {
        //LPDIRECTSOUNDBUFFER8 buffer;
        //LPDIRECTSOUND3DBUFFER8 buffer3D;
        int origfreq;
        int bufferSize;
        unsigned long prevPos;
        unsigned long filledBytes;
        bool prevposEqPlaypos;

        static slot_data_s & cvt(Slot *slot)
        {
            return *(slot_data_s *)&slot->coredata;
        }

    };

    //static_assert( sizeof(slot_data_s) == SLOT_COREDATA_SIZE, "slot data size miss" );

    bool slot_coredata_prepare(Slot *slot, Player *player, bool is3d)
    {

        return true;
    }

    void slot_coredata_clear(Slot *slot)
    {
        slot_data_s &sd = slot_data_s::cvt(slot);

        //if (sd.buffer) sd.buffer->Release(), sd.buffer = nullptr;
        //if (sd.buffer3D) sd.buffer3D->Release(), sd.buffer3D = nullptr;
    }

    bool slot_coredata_fail(Slot *slot)
    {
        slot_data_s &sd = slot_data_s::cvt(slot);
        //DWORD status;
        //return sd.buffer->GetStatus(&status) == DSERR_BUFFERLOST; // can be happen on windows 7 when headphones unplugged
        return false;
    }


    bool player_is_coredata_initialized(const char *coredata)
    {
        const player_core_data_s *me = (const player_core_data_s *)coredata;
        //return me->pDS != nullptr;
        return false;
    }

    void player_coredata_exchange(Player *coredatato, Player *coredatafrom)
    {
        player_data_s &pdmy = player_data_s::cvt(coredatato);
        player_data_s &pdother = player_data_s::cvt(coredatafrom);

        player_core_data_s &me = player_core_data_s::cvt(pdmy);
        player_core_data_s &from = player_core_data_s::cvt(pdother);

        /*
        if (me.hQuitEvent)
            SetEvent(me.hQuitEvent);
        if (from.hQuitEvent)
            SetEvent(from.hQuitEvent);

        if (me.hUpdateThread)
        {
            WaitForSingleObject(me.hUpdateThread, INFINITE);
            CloseHandle(me.hUpdateThread);
            me.hUpdateThread = nullptr;
            ResetEvent(me.hQuitEvent);
        }
        if (from.hUpdateThread)
        {
            WaitForSingleObject(from.hUpdateThread, INFINITE);
            CloseHandle(from.hUpdateThread);
            from.hUpdateThread = nullptr;
            ResetEvent(from.hQuitEvent);
        }
        */

        char tmp[sizeof(player_data_s)];
        memcpy(tmp, &pdmy, sizeof(player_data_s));
        memcpy(&pdmy, &pdother, sizeof(player_data_s));
        memcpy(&pdother, tmp, sizeof(player_data_s));

        me.run_thread(coredatato);
        from.run_thread(coredatafrom);
    }

    void slot_coredata_start_play(Slot *slot, Player *player)
    {
        slot_data_s &sd = slot_data_s::cvt(slot);

        /*
        sd.buffer->GetCurrentPosition(&sd.prevPos, nullptr);
        sd.filledBytes = 0;
        sd.prevposEqPlaypos = true;
        slot->update(player, 0);
        if (player->params.ctrlFrequency) sd.buffer->SetFrequency(DWORD(slot->decoder->format.sampleRate * slot->source->pitch));
        sd.buffer->Play(0, 0, DSBPLAY_LOOPING);
        */

    }

    void slot_coredata_stop_play(Slot *slot)
    {
        slot_data_s &sd = slot_data_s::cvt(slot);
        //sd.buffer->Stop();
    }

    void slot_coredata_update(Slot *slot, Player *player)
    {

        player_data_s &pd = player_data_s::cvt(player);
        slot_data_s &sd = slot_data_s::cvt(slot);

        //sd.buffer->SetVolume(LinearToDirectSoundVolume(slot->fade * slot->source->volume * pd.soundGroups[slot->source->soundGroup].volume));

        // fill sound buffer
        /*
        DWORD playPos, writePos;
        bool initial = false;
        if (SUCCEEDED(sd.buffer->GetCurrentPosition(&playPos, &writePos)))
        {
            initial = playPos == sd.prevPos && sd.filledBytes == 0;

            DWORD readSize = 0;
            if (initial)
            {
                readSize = sd.bufferSize;
            }
            else
            {

                if (sd.prevposEqPlaypos && sd.prevPos == playPos)
                {
                    return; // not yet played
                }

                if ((sd.prevPos >= playPos && sd.prevPos < writePos) ||
                    (writePos < playPos && !(sd.prevPos >= writePos && sd.prevPos < playPos)))
                {
                    player->nodata = true;
                    sd.prevPos = writePos; // oops. we can only write to write pos;
                }

                DWORD newfreespace = (playPos >= sd.prevPos) ? playPos - sd.prevPos : playPos + sd.bufferSize - sd.prevPos;

                if (newfreespace >= sd.filledBytes)
                {
                    sd.filledBytes = 0;

                    if (slot->silenceSize >= sd.bufferSize)
                    {
                        slot->stop();
                        return;
                    }

                } else
                    sd.filledBytes -= newfreespace;
                readSize = sd.bufferSize - sd.filledBytes;
            }

            if (readSize == 0)
                return;

            char * buf = (char *)_alloca(readSize);
            s3int r = slot->read(buf, readSize);
            if (r <= 0)
            {
                if (!initial)
                    return;
            }

            if (pd.pitchchanged)
            {
                DWORD newfreq = 0;
                {
                    pd.pitchchanged = false; // because new pitch will be applied right now
                    newfreq = (DWORD)((float)sd.origfreq * pd.pitch);
                }
                if (newfreq)
                    sd.buffer->SetFrequency(newfreq);
            }

            LPVOID ptr1, ptr2;
            DWORD size1, size2;
            HRESULT hr = sd.buffer->Lock(sd.prevPos, initial ? readSize : (DWORD)r, &ptr1, &size1, &ptr2, &size2, 0);
            if (hr == DSERR_BUFFERLOST)
            {
                sd.buffer->Restore();
                hr = sd.buffer->Lock(sd.prevPos, initial ? readSize : (DWORD)r, &ptr1, &size1, &ptr2, &size2, 0);
            }
            memcpy(ptr1,buf,size1);
            if (ptr2)
                memcpy(ptr2, buf+size1, size2);

            sd.buffer->Unlock(ptr1, size1, ptr2, size2);

            sd.filledBytes += (DWORD)r;
            sd.prevPos += (DWORD)r;
            if ((int)sd.prevPos > sd.bufferSize)
                sd.prevPos -= sd.bufferSize;
            sd.prevposEqPlaypos = sd.prevPos == playPos;

            if (slot->silenceSize >= sd.bufferSize)
            {
                slot->stop();
                return;
            }


        }

        //��������� 3D-���������
        if (sd.buffer3D)
        {
            DS3DBUFFER pars;
            pars.dwSize = sizeof(DS3DBUFFER);
            ConvertVectorToDirectSoundCS(pars.vPosition, slot->source->position, player->params.rightHandedCS);
            ConvertVectorToDirectSoundCS(pars.vVelocity, slot->source->velocity, player->params.rightHandedCS);
            pars.vConeOrientation.x = 0;
            pars.vConeOrientation.y = 0;
            pars.vConeOrientation.z = 1;
            pars.dwInsideConeAngle = 360;
            pars.dwOutsideConeAngle = 360;
            pars.lConeOutsideVolume = 0;
            pars.flMinDistance = slot->source->minDist;
            pars.flMaxDistance = slot->source->maxDist;
            pars.dwMode = DS3DMODE_NORMAL;
            sd.buffer3D->SetAllParameters(&pars, initial ? DS3D_IMMEDIATE : DS3D_DEFERRED);
        }
            */

    }

    void player_coredata_on_update(Player *player)
    {
        player_core_data_s &cd = player_core_data_s::cvt(player_data_s::cvt(player));
        //if (cd.pDS == nullptr) return;
        //cd.dsListener->CommitDeferredSettings();

    }

    bool player_coredata_init(Player *player)
    {
        player_data_s &pd = *(player_data_s *)&player->data;
        player_core_data_s &cd = *(player_core_data_s *)pd.coredata;

        /*
        // Direct Sound initialization
        if (FAILED(DirectSoundCreate8(player->params.device == DEFAULT_DEVICE ? nullptr : (GUID *)&player->params.device.guid, &cd.pDS, nullptr))) { player->Shutdown(); return false; }

        // create hidden window
        //if (player->params.hwnd == nullptr)
        {
            WNDCLASSW wc = { 0, DefWindowProc, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"s3hwndclass" };
            RegisterClassW(&wc);
            cd.hwnd = CreateWindowW(wc.lpszClassName, NULL, WS_POPUP, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
        }

        //.. ��� ����, ����� ���������� Cooperative Level
        if (FAILED(cd.pDS->SetCooperativeLevel(cd.hwnd, DSSCL_PRIORITY))) { player->Shutdown(); return false; }

        //�������� listener-� (��� ����� ���������� ������� primary buffer)
        DSBUFFERDESC dsbd = { sizeof(DSBUFFERDESC), DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME };
        if (SUCCEEDED(cd.pDS->CreateSoundBuffer(&dsbd, &cd.dsPrimaryBuffer, nullptr)))
        {
            cd.dsPrimaryBuffer->QueryInterface(MY_IID_IDirectSound3DListener, (LPVOID*)&cd.dsListener);

            //����� ������������� ������ primary buffer-� (������-�� �� ��������� ��� 8��� 22���)
            WAVEFORMATEX wf = BuildWaveFormat(player->params.channels, player->params.sampleRate, player->params.bitsPerSample);
            cd.dsPrimaryBuffer->SetFormat(&wf);
        }
        else { player->Shutdown(); return false; }


        cd.hQuitEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        cd.run_thread(player);
        */
        return true;
    }

    void player_coredata_pre_shutdown(Player *player, bool reinit)
    {
        player_data_s &pd = player_data_s::cvt(player);
        player_core_data_s &cd = player_core_data_s::cvt(pd);

        /*
        if (cd.hUpdateThread)
        {
            SetEvent(cd.hQuitEvent);
            if (reinit) spinlock::simple_unlock(pd.sync); //Shutdown(true) ���������� � ������, ���������� AutoCriticalSection, ������� ������� �� ����. ������, ����� �������� � �������!
            WaitForSingleObject(cd.hUpdateThread, INFINITE);
            CloseHandle(cd.hQuitEvent);
            CloseHandle(cd.hUpdateThread);
            cd.hQuitEvent = cd.hUpdateThread = nullptr;
            if (reinit) spinlock::simple_lock(pd.sync);
        }
        */

    }

    void player_coredata_post_shutdown(Player *player)
    {
        player_core_data_s &cd = player_core_data_s::cvt(player_data_s::cvt(player));

        //if (cd.dsListener) cd.dsListener->Release(), cd.dsListener = nullptr;
        //if (cd.dsPrimaryBuffer) cd.dsPrimaryBuffer->Release(), cd.dsPrimaryBuffer = nullptr;
        //if (cd.pDS) cd.pDS->Release(), cd.pDS = nullptr;
        //if (cd.hwnd) DestroyWindow(cd.hwnd), cd.hwnd = nullptr;
    }

    void player_coredata_set_master_vol(Player *player, float vol)
    {
        player_core_data_s &cd = player_core_data_s::cvt(player_data_s::cvt(player));

        //if (cd.dsPrimaryBuffer) cd.dsPrimaryBuffer->SetVolume(LinearToDirectSoundVolume(vol));
    }

    void player_coredata_set_listner_params(Player *player, const float pos[3], const float front[3], const float top[3], const float vel[3], float distFactor, float rolloffFactor, float dopplerFactor, bool rightHandedCS)
    {
        player_core_data_s &cd = player_core_data_s::cvt(player_data_s::cvt(player));

        /*
        if (cd.dsListener)
        {
            DS3DLISTENER pars;
            pars.dwSize = sizeof(DS3DLISTENER);
            ConvertVectorToDirectSoundCS(pars.vPosition, pos, rightHandedCS);
            ConvertVectorToDirectSoundCS(pars.vOrientFront, front, rightHandedCS);
            ConvertVectorToDirectSoundCS(pars.vOrientTop, top, rightHandedCS);
            pars.flDistanceFactor = distFactor;
            pars.flRolloffFactor = rolloffFactor;
            pars.flDopplerFactor = dopplerFactor;
            if (vel) ConvertVectorToDirectSoundCS(pars.vVelocity, vel, rightHandedCS); else memset(&pars.vVelocity, 0, sizeof(pars.vVelocity));
            cd.dsListener->SetAllParameters(&pars, DS3D_DEFERRED);
        }
        */

    }


    void enum_sound_play_devices(device_enum_callback *cb, void * lpContext)
    {
        //enumdata d = { cb, lpContext };
        //DirectSoundEnumerateW(device_enum_callback_win32, &d);
    }


    // capture

    struct capture_core_data_s
    {
        //LPDIRECTSOUNDCAPTURE8 pDSCapture = nullptr;
        //LPDIRECTSOUNDCAPTUREBUFFER pDSCaptureBuffer = nullptr;
        int next_offset = 0;

        static capture_core_data_s & cvt(const capturer_c *c)
        {
            return *(capture_core_data_s *)&c->coredata;
        }

    };

    //static_assert(sizeof(capture_core_data_s) == CAPTURE_COREDATA_SIZE, "capture data size miss");

    void capturer_c::capture_tick(capture_callback * data_callback, void *context)
    {
        capture_core_data_s &cd = capture_core_data_s::cvt( this );

        /*
        if (cd.pDSCaptureBuffer)
        {
            DWORD readp;
            cd.pDSCaptureBuffer->GetCurrentPosition(nullptr, &readp);

            int lock_size = static_cast<int>(readp) - cd.next_offset;
            if (lock_size < 0)
                lock_size += static_cast<int>(format.nAvgBytesPerSec);

            if (lock_size > 0)
            {
                void*   pbCaptureData = nullptr;
                DWORD   dwCaptureLength = 0;
                void*   pbCaptureData2 = nullptr;
                DWORD   dwCaptureLength2 = 0;

                // Lock the capture buffer down
                if (FAILED(cd.pDSCaptureBuffer->Lock(cd.next_offset, lock_size,
                    &pbCaptureData, &dwCaptureLength,
                    &pbCaptureData2, &dwCaptureLength2, 0L)))
                    return;

                data_callback(pbCaptureData, dwCaptureLength, context);
                if (pbCaptureData2)
                    data_callback(pbCaptureData2, dwCaptureLength2, context);

                cd.pDSCaptureBuffer->Unlock(pbCaptureData, dwCaptureLength, pbCaptureData2, dwCaptureLength2);
                cd.next_offset = readp;
            }
        }
        */
    }

    void capturer_c::stop()
    {
        capture_core_data_s &cd = capture_core_data_s::cvt(this);

        /*
        if (cd.pDSCaptureBuffer)
        {
            LPDIRECTSOUNDCAPTUREBUFFER b = cd.pDSCaptureBuffer;
            cd.pDSCaptureBuffer = nullptr;

            DWORD status = 0;
            b->GetStatus(&status);
            if (0 != (status & DSCBSTATUS_CAPTURING)) b->Stop();
            b->Release();
        }
        if (cd.pDSCapture) { cd.pDSCapture->Release(); cd.pDSCapture = nullptr; }
*/
    }

    bool capturer_c::is_capturing() const
    {
        capture_core_data_s &cd = capture_core_data_s::cvt(this);

        /*
        if (cd.pDSCaptureBuffer)
        {
            DWORD status = 0;
            cd.pDSCaptureBuffer->GetStatus(&status);
            if (0 != (status & DSCBSTATUS_CAPTURING)) return true;
        }
        */
        return false;

    }

    bool capturer_c::start_capture(Format & cfmt, const Format * tryformats, int try_formats_cnt)
    {
        capture_core_data_s &cd = capture_core_data_s::cvt(this);

        //DEVICE device = (capturedevice == DEFAULT_DEVICE) ? (DEVICE &)MY_DSDEVID_DefaultVoiceCapture : capturedevice;

        /*
        if (FAILED(DirectSoundCaptureCreate8((GUID *)&device.guid, &cd.pDSCapture, nullptr)))
            return false;

        DSCBUFFERDESC dscbd;

        memset(&format, 0, sizeof(format));
        format.wFormatTag = WAVE_FORMAT_PCM;

        memset(&dscbd, 0, sizeof(dscbd));
        dscbd.dwSize = sizeof(dscbd);
        */

        // Try 16 different standard format to see if they are supported
        for (int i = 0, cnt = 16 + try_formats_cnt; i < cnt; ++i)
        {
            if (i < try_formats_cnt)
            {
                const Format &tf = tryformats[i];
                format.nChannels = tf.channels;
                format.nSamplesPerSec = tf.sampleRate;
                format.wBitsPerSample = tf.bitsPerSample;
                format.nAvgBytesPerSec = tf.avgBytesPerSec();
                format.nBlockAlign = static_cast<uint16_t>(tf.sampleSize());
            }
            else
            {
                format_by_index(i - try_formats_cnt);
            }

            //dscbd.dwBufferBytes = format.nAvgBytesPerSec;
            //dscbd.lpwfxFormat = (LPWAVEFORMATEX)&format;

            //if (FAILED(cd.pDSCapture->CreateCaptureBuffer(&dscbd, &cd.pDSCaptureBuffer, nullptr)))
              //  continue;
            break;
        }

        /*
        if (cd.pDSCaptureBuffer)
        {
            cd.next_offset = 0;
            cd.pDSCaptureBuffer->Start(DSCBSTART_LOOPING);

            cfmt.channels = format.nChannels;
            cfmt.sampleRate = format.nSamplesPerSec;
            cfmt.bitsPerSample = format.wBitsPerSample;
        }

        return cd.pDSCaptureBuffer != nullptr;
        */
        return false;

    }


    void capturer_c::enum_sound_capture_devices(device_enum_callback *cb, void * lpContext)
    {
        //enumdata d = { cb, lpContext };
        //DirectSoundCaptureEnumerateW(capture_device_enum_callback_win32, &d);
    }

}

