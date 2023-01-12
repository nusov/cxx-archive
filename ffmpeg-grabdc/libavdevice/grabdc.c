/*
 * GrabDC
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



/**
 * Converts BITMAP to DIB (from MSDN Examples)
 *
 * @param hbitmap BITMAP
 * @param bits depth
 */
HANDLE
BitmapToDIB (HBITMAP hbitmap, UINT bits)
{
    HANDLE              hdib ;
    HDC                 hdc ;
    BITMAP              bitmap ;
    UINT                wLineLen ;
    DWORD               dwSize ;
    DWORD               wColSize ;
    LPBITMAPINFOHEADER  lpbi ;
    LPBYTE              lpBits ;
    
    GetObject(hbitmap,sizeof(BITMAP),&bitmap) ;

    //
    // DWORD align the width of the DIB
    // Figure out the size of the colour table
    // Calculate the size of the DIB
    //
    wLineLen = (bitmap.bmWidth*bits+31)/32 * 4;
    wColSize = sizeof(RGBQUAD)*((bits <= 8) ? 1<<bits : 0);
    dwSize = sizeof(BITMAPINFOHEADER) + wColSize +
        (DWORD)(UINT)wLineLen*(DWORD)(UINT)bitmap.bmHeight;

    //
    // Allocate room for a DIB and set the LPBI fields
    //
    hdib = GlobalAlloc(GHND,dwSize);
    if (!hdib)
        return hdib ;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib) ;

    lpbi->biSize = sizeof(BITMAPINFOHEADER) ;
    lpbi->biWidth = bitmap.bmWidth ;
    lpbi->biHeight = bitmap.bmHeight ;
    lpbi->biPlanes = 1 ;
    lpbi->biBitCount = (WORD) bits ;
    lpbi->biCompression = BI_RGB ;
    lpbi->biSizeImage = dwSize - sizeof(BITMAPINFOHEADER) - wColSize ;
    lpbi->biXPelsPerMeter = 0 ;
    lpbi->biYPelsPerMeter = 0 ;
    lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;
    lpbi->biClrImportant = 0 ;

    //
    // Get the bits from the bitmap and stuff them after the LPBI
    //
    lpBits = (LPBYTE)(lpbi+1)+wColSize ;

    hdc = CreateCompatibleDC(NULL) ;

    GetDIBits(hdc,hbitmap,0,bitmap.bmHeight,lpBits,(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;

    DeleteDC(hdc) ;
    GlobalUnlock(hdib);

    return hdib ;
}


/**
 * GrabDC Device Demuxer Context
 */
struct grabdc_ctx
{
    HDC hdc;               /* Device context (DC)                   */
    HDC hd;                /* Memory device context (DC)            */
    
    AVRational time_base;  /* Time base                             */
    int64_t    time_frame; /* Current time                          */
    
    int frame_size;        /* Frame size length in bytes            */
    int frame_num;         /* Number of the current frame           */
    int frame_bpp;         /* Bits per pixel                        */

    int width;             /* Width of the frame                    */
    int height;            /* Height of the frame                   */   
    int x_offset;          /* Horizontal top-left corner coordinate */
    int y_offset;          /* Vertical top-left corner coordinate   */

};


/**
 * Initializes the GrabDC device context demuxer
 *
 * @param s1 Context from avformat core
 * @param ap Parameters from avformat core
 */
static int
grabdc_read_header(AVFormatContext *s1, AVFormatParameters *ap)
{
    struct grabdc_ctx *grabdc = s1->priv_data;
    enum   PixelFormat input_pixfmt;
    
    HDC hdc;
    HDC hd;

    AVStream *st = NULL;
    
    int bpp;
    int x_offset = 0;
    int y_offset = 0;
    
    int cx;
    int cy;
    
    char *param, *offset;
    
    /* Get parameters                  */
    param  = av_strdup(s1->filename);
    offset = strchr(param, '+');

    if (offset) {
        sscanf(offset, "%d,%d", &x_offset, &y_offset);
        *offset = 0;
    }

    /* Checking parameters             */
    if (!ap || ap->width <= 0 || ap->height <= 0 || ap->time_base.den <= 0) {
        av_log(s1, AV_LOG_ERROR, "Video size and/or rate is not defined. Use -s/-r\n");
        return AVERROR(EIO);
    }
    
    /* Get screen metrics              */
    cx = GetSystemMetrics(SM_CXSCREEN);
    cy = GetSystemMetrics(SM_CYSCREEN);

    /* Check input position/size       */
    if (x_offset < 0 || (x_offset + ap->width) > cx) {
        av_log(s1, AV_LOG_ERROR, "Bad X coordinate or width.\n");
        return AVERROR(EIO);
    }

    if (y_offset < 0 || (y_offset + ap->height) > cy) {
        av_log(s1, AV_LOG_ERROR, "Bad Y coordinate or height.\n");
        return AVERROR(EIO);
    }

    /* Bytes per pixel and pixelformat  */
    bpp = 32;

    /* Create stream and set parameters */
    st = av_new_stream(s1, 0);
    if (!st) {
        return AVERROR(ENOMEM);
    }

    av_set_pts_info(st, 64, 1, 1000000);
    
    grabdc->hdc        = hdc;
    grabdc->hd         = hd;
    grabdc->frame_size = ap->width * ap->height * bpp/8;
    grabdc->frame_num  = 0;
    grabdc->frame_bpp  = bpp;
    grabdc->time_base  = ap->time_base;
    grabdc->time_frame = av_gettime() / av_q2d(ap->time_base);
    grabdc->width      = ap->width;
    grabdc->height     = ap->height;
    grabdc->x_offset   = x_offset;
    grabdc->y_offset   = y_offset;

    st->codec->codec_type = CODEC_TYPE_VIDEO;
    st->codec->codec_id   = CODEC_ID_RAWVIDEO;
    st->codec->width      = ap->width;
    st->codec->height     = ap->height;
    st->codec->pix_fmt    = PIX_FMT_RGB32;
    st->codec->time_base  = ap->time_base;

    return 0;
}


/**
 * Grabs a frame from GrabDC
 *
 * @param s1 Context from avformat core
 * @param pkt Packet
 */
static int
grabdc_read_packet(AVFormatContext *s1, AVPacket *pkt)
{
    struct grabdc_ctx *s = s1->priv_data;
    int64_t curtime, delay;

    /* Calculate the time of the next frame */
    s->time_frame += INT64_C(1000000);

    /* wait based on the frame rate */
    for(;;) {
        curtime = av_gettime();
        delay = s->time_frame * av_q2d(s->time_base) - curtime;
        if (delay <= 0) {
            if (delay < INT64_C(-1000000) * av_q2d(s->time_base)) {
                s->time_frame += INT64_C(1000000);
            }
            break;
        }
       Sleep(0);
    }

    if (av_new_packet(pkt, s->frame_size) < 0) {
        return AVERROR(EIO);
    }

    pkt->pts = curtime;

    int width  = s->width;
    int height = s->height;
    int left   = s->x_offset;
    int top    = s->y_offset;

    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC    = CreateCompatibleDC(hScreenDC);

    HBITMAP hbm;
    HBITMAP oldbm;

    hbm   = CreateCompatibleBitmap(hScreenDC, width, height);
    oldbm = (HBITMAP) SelectObject(hMemDC, hbm);

    StretchBlt(hMemDC, 0, 0, width, height, hScreenDC, left, top+height, width, -height, SRCCOPY);
    SelectObject(hMemDC,oldbm); 

    LPBITMAPINFOHEADER alpbi = (LPBITMAPINFOHEADER)GlobalLock(BitmapToDIB(hbm, s->frame_bpp));

    memcpy(pkt->data, (LPBYTE) alpbi + alpbi->biSize + alpbi->biClrUsed * sizeof(RGBQUAD), alpbi->biSizeImage);

    GlobalFree(alpbi);

    DeleteObject(hbm);
    DeleteDC(hMemDC);
    
    ReleaseDC(NULL, hScreenDC);

    return s->frame_size;
}


/**
 * Close GrabDC
 *
 * @param s1 Context from avformat core
 */
static int
grabdc_read_close(AVFormatContext *s1)
{
    struct grabdc_ctx *grabdc = s1->priv_data;
    return 0;
}


/**
 * GrabDC demuxer declaration
 */
AVInputFormat grabdc_demuxer = {
    "grabdc",
    NULL_IF_CONFIG_SMALL("GrabDC Video Capture"),
    sizeof(struct grabdc_ctx),
    NULL,
    grabdc_read_header,
    grabdc_read_packet,
    grabdc_read_close,
    .flags = AVFMT_NOFILE,
};
