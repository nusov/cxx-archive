/*
 * WaveIn - audio capture for windows
 * Copyright (c) 2010 Alexander Nusov <alexander.nusov <at> gmail.com>
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libavformat/avformat.h"
#include <windows.h>
#include <mmsystem.h>


#define WAVEIN_CAST(x)              (struct wavein_data *)x
#define WAVEIN_RET_VAL_IF_NULL(x,y) if ((x) == NULL) return (y)
#define WAVEIN_RET_IF_NULL(x)       if ((x) == NULL) return

#define WAVEIN_OPENED               1
#define WAVEIN_CLOSED               0

#define WAVEIN_CODEC_ID             CODEC_ID_PCM_S16LE
#define WAVEIN_BITS_PER_SAMPLE      16

/**
 * Denominator for a PCM buffer
 */
#ifndef WAVEIN_DENOMINATOR
#define WAVEIN_DENOMINATOR          1
#endif

/**
 * Defines for missed from MinGW
 */
#ifndef WAVE_FORMAT_48M16
#define WAVE_FORMAT_48M16      0x00004000
#endif

#ifndef WAVE_FORMAT_48S16
#define WAVE_FORMAT_48S16      0x00008000
#endif

#ifndef WAVE_FORMAT_96M16
#define WAVE_FORMAT_96M16      0x00040000
#endif

#ifndef WAVE_FORMAT_96S16
#define WAVE_FORMAT_96S16      0x00080000
#endif


/**
 * Forward declarations
 */
static int wavein_close      (AVFormatContext *);
static int wavein_read_close (AVFormatContext *);


/**
 * wavein device demuxer context
 */
struct wavein_data {
	HANDLE  wavein_mutex;  /**< waveIn Mutex         */
	HANDLE  wavein_event;  /**< waveIn Event         */

	HWAVEIN hwi;           /**< Input device ID      */
	WAVEHDR wavehdr;       /**< PCM data buffer      */

	int dev_id;            /**< Device ID (int)      */
	int state;             /**< Device state         */
	int recording;         /**< Recording state      */

	int sample_rate;       /**< Sampling rate        */
	int channels;          /**< Audio channels       */

	enum CodecID codec_id; /**< PCM codec identifier */
	unsigned int bufsize;  /**< PCM buffer size      */
	AVPacketList *pktl;    /**< List of packets      */
};

/**
 * Check audio device capabilities
 *
 * @param formats Available formats
 * @param ar sampling rate
 * @param ac channels
 * @return if success - 1
 */
static int
wavein_check_capabilities(int32_t formats, int ar, int ac)
{
	if (ar == 11025 && ac == 1 && (formats & WAVE_FORMAT_1M16)) {
		return TRUE;
	}

	if (ar == 11025 && ac == 2 && (formats & WAVE_FORMAT_1S16)) {
		return TRUE;
	}

	if (ar == 22050 && ac == 1 && (formats & WAVE_FORMAT_2M16)) {
		return TRUE;
	}

	if (ar == 22050 && ac == 2 && (formats & WAVE_FORMAT_2S16)) {
		return TRUE;
	}

	if (ar == 44100 && ac == 1 && (formats & WAVE_FORMAT_4M16)) {
		return TRUE;
	}

	if (ar == 44100 && ac == 2 && (formats & WAVE_FORMAT_4S16)) {
		return TRUE;
	}

	if (ar == 48000 && ac == 1 && (formats & WAVE_FORMAT_48M16)) {
		return TRUE;
	}

	if (ar == 48000 && ac == 2 && (formats & WAVE_FORMAT_48S16)) {
		return TRUE;
	}

	if (ar == 96000 && ac == 1 && (formats & WAVE_FORMAT_96M16)) {
		return TRUE;
	}

	if (ar == 96000 && ac == 2 && (formats & WAVE_FORMAT_96S16)) {
		return TRUE;
	}

	return FALSE;
}


/**
 * Find all devices.
 *
 * @param s1 Context from avformat core
 */
static void
wavein_findalldevs(AVFormatContext *s1)
{
	unsigned int i, alldevs;
	WAVEINCAPS   wic;
	MMRESULT     mmres;

	av_log(s1, AV_LOG_INFO, "Audio devices:\n");
	
	alldevs = waveInGetNumDevs();
	if (alldevs < 1) {
		av_log(s1, AV_LOG_INFO, "No devices found.\n");
	}

	for (i = 0; i < alldevs; i++) {
		ZeroMemory(&wic, sizeof(WAVEINCAPS));
		mmres = waveInGetDevCaps(i, &wic, sizeof(WAVEINCAPS));
		
		if (mmres == MMSYSERR_NOERROR) {
			av_log(s1, AV_LOG_INFO, "%d - %s\n", i, wic.szPname);
		}
	}
}


/**
 * WaveIn callback
 *
 * @param hwi Waveform-audio input device
 * @param message Notification message
 * @param dwInstance User data (handle to wavein_data)
 * @param dwParam1 Parameter for a message
 * @param dwParam2 Parameter for a message
 */
void CALLBACK
wavein_callback(HWAVEIN hwi, UINT message, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	struct wavein_data *self = WAVEIN_CAST(dwInstance);
	AVPacketList **ppktl, *pktl_next;
	WAVEHDR *pwavehdr = NULL;
	MMRESULT mmres;

	switch (message)
	{
	case WIM_OPEN:
		self->state = WAVEIN_OPENED;
		break;

	case WIM_DATA:		
		if (self->recording) {
			WaitForSingleObject(self->wavein_mutex, INFINITE);
		}

		pwavehdr = (WAVEHDR *) dwParam1;

		if (WHDR_DONE == (pwavehdr->dwFlags & WHDR_DONE)) {
			pktl_next = av_mallocz(sizeof(AVPacketList));

			if (pktl_next == NULL) {
				ReleaseMutex(self->wavein_mutex);
				return;
			}

			if (av_new_packet(&pktl_next->pkt, pwavehdr->dwBufferLength) < 0) {
				av_free(pktl_next);
				ReleaseMutex(self->wavein_mutex);
				return;
			}

			self->bufsize += pwavehdr->dwBufferLength;

			pktl_next->pkt.pts = av_gettime();
			memcpy(pktl_next->pkt.data, pwavehdr->lpData, pwavehdr->dwBufferLength);

			for (ppktl = &self->pktl; *ppktl; ppktl = &(*ppktl)->next);
			*ppktl = pktl_next;

			if (self->recording) {
				mmres = waveInAddBuffer(self->hwi, pwavehdr, sizeof(WAVEHDR));
			}
			

			SetEvent(self->wavein_event);
		}

		//SetEvent(self->wavein_event);
		ReleaseMutex(self->wavein_mutex);
		break;

	case WIM_CLOSE:
		self->state = WAVEIN_CLOSED;
		break;
	}
}



/**
 * Open input audio device.
 *
 * @param s1 Context from avformat core
 * @param ap Parameters from avformat core
 * @return 0 if success
 */
static int
wavein_open(AVFormatContext *s1, AVFormatParameters *ap)
{
	struct wavein_data *self = WAVEIN_CAST(s1->priv_data);
	char *param, *ps;
	int dev_id = 0;
	int den    = 0;
	int compat = 1;

	MMRESULT     mmres;
	WAVEINCAPS   wic = {0};
	WAVEFORMATEX wfx = {0};

	/* Get device id/den from filename, otherwise use default */
	param = av_strdup(s1->filename);
	if (strcmp(param, "alldevs") == 0) {
		wavein_findalldevs(s1);
		return AVERROR(EIO);
	}

	if (strstr(param, ":nocaps")) {
		compat = 0;
	}

	ps = strrchr(param, ':');
	if (ps) {
		sscanf(ps, ":%d", &den);
		*ps = 0;
	}

	if (strcmp(param, "mapper") == 0) {
		dev_id = WAVE_MAPPER;
	}
	else {
		dev_id = atoi(param);
	}

	/* Set default denominator if needed */
	if (den == 0) {
		den = WAVEIN_DENOMINATOR;
	}

	/* Get device capabilities */
	mmres = waveInGetDevCaps(dev_id, &wic, sizeof(WAVEINCAPS));
	if (mmres != MMSYSERR_NOERROR) {
		wavein_findalldevs(s1);
	
		av_log(s1, AV_LOG_ERROR, "Cannot get device capabilities.\n");
		return AVERROR(EINVAL);
	}

	/* Check audio modes */
	if (!wavein_check_capabilities(wic.dwFormats, ap->sample_rate, ap->channels)) {
		av_log(s1, AV_LOG_ERROR, "Audio mode is not supported.\n");
		return AVERROR(EINVAL);
	}


	/* Fill WAVEFORMATEX */
	wfx.wFormatTag      = WAVE_FORMAT_PCM;
	wfx.wBitsPerSample  = WAVEIN_BITS_PER_SAMPLE;
	wfx.nChannels       = ap->channels;
	wfx.nSamplesPerSec  = ap->sample_rate;
	wfx.nBlockAlign     = wfx.nChannels * WAVEIN_BITS_PER_SAMPLE/8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize          = sizeof(WAVEFORMATEX);


	/* Open device */
	mmres = waveInOpen(&self->hwi, dev_id, &wfx, (DWORD_PTR)wavein_callback, (DWORD_PTR)self, CALLBACK_FUNCTION);
	if (mmres != MMSYSERR_NOERROR) {
		av_log(s1, AV_LOG_ERROR, "Cannot open device.\n");
		return AVERROR(EINVAL);
	}

	/* Prepare buffer */
	self->wavehdr.lpData = (LPSTR)HeapAlloc(GetProcessHeap(), 8, wfx.nAvgBytesPerSec / den);
	self->wavehdr.dwBufferLength = wfx.nAvgBytesPerSec / den;
	self->wavehdr.dwUser = 0;

	mmres = waveInPrepareHeader(self->hwi, &self->wavehdr, sizeof(WAVEHDR));
	if (mmres != MMSYSERR_NOERROR) {
		av_log(s1, AV_LOG_ERROR, "Cannot prepare buffer.\n");

		wavein_read_close(s1);
		return AVERROR(EINVAL);
	}

	/* Add buffer */
	mmres = waveInAddBuffer(self->hwi, &self->wavehdr, sizeof(WAVEHDR));
	if (mmres != MMSYSERR_NOERROR) {
		av_log(s1, AV_LOG_ERROR, "Cannot add buffer.\n");
		
		wavein_read_close(s1);
		return AVERROR(EINVAL);
	}


	/* Set wavein data structure */
	self->dev_id      = dev_id;
	self->codec_id    = WAVEIN_CODEC_ID;
	self->channels    = ap->channels;
	self->sample_rate = ap->sample_rate;
	
	self->bufsize     = 0;
	self->state       = 0;
	self->recording   = 1;
	
	return 0;
}


/**
 * Close audio device
 *
 * @param Context from avformat core
 * @return 0 if success
 */
static int
wavein_close(AVFormatContext *s1)
{
	struct wavein_data *self = WAVEIN_CAST(s1->priv_data);
	MMRESULT mmres;

	self->recording = 0;
	Sleep(0);

	if (self->hwi) {
		mmres = waveInStop(self->hwi);
		if (mmres != MMSYSERR_NOERROR) {
			av_log(s1, AV_LOG_ERROR, "Cannot stop device.\n");
		}

		mmres = waveInClose(self->hwi);
		if (mmres != MMSYSERR_NOERROR) {
			av_log(s1, AV_LOG_ERROR, "Cannot close device.\n");
		}
	}

	if (self->wavein_mutex) {
		CloseHandle(self->wavein_mutex);
	}

	if (self->wavein_event) {
		CloseHandle(self->wavein_event);
	}

	return 0;
}


/**
 * Read the format header and initialize the AVFormatContext structure. 
 *
 * @param s1 Context from avformat core
 * @param ap Parameters from avformat core
 * @return 0 if success
 */
static int
wavein_read_header(AVFormatContext *s1, AVFormatParameters *ap)
{
	struct wavein_data *self = WAVEIN_CAST(s1->priv_data);
	AVStream *st = NULL;

	/* Checking parameters */
	if (!ap || ap->sample_rate <= 0 || ap->channels <= 0) {
		av_log(s1, AV_LOG_ERROR, "Sampling rate and/or channels are not defined.\n");
		return AVERROR(EIO);
	}

	/* Create stream */
	st = av_new_stream(s1, 0);
	if (st == NULL) {
		av_log(s1, AV_LOG_ERROR, "Cannot add stream.\n");
		return AVERROR(ENOMEM);
	}

	/* Opening audio device */
	if (wavein_open(s1, ap)) {
		return AVERROR(EIO);
	}

	/* Set codecs parameters */
	st->codec->codec_type  = CODEC_TYPE_AUDIO;
	st->codec->codec_id    = self->codec_id;
	st->codec->sample_rate = self->sample_rate;
	st->codec->channels    = self->channels;

	av_set_pts_info(st, 32, 1, 1000);

	/* Create mutex/event */
	self->wavein_mutex = CreateMutex(NULL, 0, NULL);
	if (self->wavein_mutex == NULL) {
		av_log(s1, AV_LOG_ERROR, "Cannot create mutex.\n");

		wavein_read_close(s1);
		return AVERROR(EIO);
	}

	self->wavein_event = CreateEvent(NULL, 1, 0, NULL);
	if (self->wavein_event == NULL) {
		av_log(s1, AV_LOG_ERROR, "Cannot create event.\n");

		wavein_read_close(s1);
		return AVERROR(EIO);
	}

	/* Start recording */
	if (waveInStart(self->hwi) != MMSYSERR_NOERROR) {
		av_log(s1, AV_LOG_ERROR, "Cannot start recording.\n");

		wavein_read_close(s1);
		return AVERROR(EIO);
	}

	return 0;
}


/**
 * Read one packet and put it in 'pkt'.
 *
 * @param s1 Context from avformat core
 * @param pkt Packet
 * @return 0 if success
 */
static int
wavein_read_packet(AVFormatContext *s1, AVPacket *pkt)
{
	struct wavein_data *self = WAVEIN_CAST(s1->priv_data);
	AVPacketList *hpktl = NULL;

	/* Wait until WAVEHDR buffer has been filled */
	while (!hpktl) {
		WaitForSingleObject(self->wavein_mutex, INFINITE);

		hpktl = self->pktl;
		if (hpktl) {
			*pkt = self->pktl->pkt;
			self->pktl = self->pktl->next;
			av_free(hpktl);
		}

		ResetEvent(self->wavein_event);
		ReleaseMutex(self->wavein_mutex);

		if (hpktl == NULL) {
			if (s1->flags & AVFMT_FLAG_NONBLOCK) {
				return AVERROR(EAGAIN);
			}
			else {
				WaitForSingleObject(self->wavein_event, INFINITE);
			}
		}
	}

	/* Align current buffer size */
	self->bufsize -= pkt->size;

    return 0;
}


/**
 * Close the stream
 *
 * @param s1 Context from avformat core
 * @return 0 if success
 */
static int
wavein_read_close(AVFormatContext *s1)
{
	wavein_close(s1);
	return 0;
}


/**
 * wavein demuxer declaration
 */
AVInputFormat wavein_demuxer = {
	"wavein",
	NULL_IF_CONFIG_SMALL("wavein demuxer"),
	sizeof(struct wavein_data),
	NULL,
	wavein_read_header,
	wavein_read_packet,
	wavein_read_close,
	NULL,
	NULL,
	AVFMT_NOFILE,
};
