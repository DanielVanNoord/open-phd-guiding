/*
 *  usimage.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "phd.h"
#include "image_math.h"

bool usImage::Init(int xsize, int ysize) {
// Allocates space for image and sets params up
// returns true on error
    if (ImageData) {
        delete[] ImageData;
        ImageData = NULL;
    }
    NPixels = xsize * ysize;
    Size = wxSize(xsize,ysize);
    Subframe = wxRect(0, 0, 0, 0);
    Min = Max = 0;
    if (NPixels) {
        ImageData = new unsigned short[NPixels];
        if (!ImageData) return true;
    }
    return false;
}

void usImage::CalcStats() {
    int i, d;
    unsigned short *ptr, *tmpdata;

    if ((!ImageData) || (!NPixels))
        return;

    tmpdata = new unsigned short[NPixels];

    Min = 65535; Max = 0;
    FiltMin = 65535; FiltMax = 0;

    if (Subframe.width <= 0 || Subframe.height <= 0)
    {
        for (i=0, ptr=ImageData; i<NPixels; i++, ptr++) {
            d = (int) ( *ptr);
            if (d < Min) Min = d;
            if (d > Max) Max = d;
            tmpdata[i]=d;
        }

        Median3(tmpdata,Size.GetWidth(),Size.GetHeight());
        for (i=0, ptr=tmpdata; i<NPixels; i++, ptr++) {
            d = (int) ( *ptr);
            if (d < FiltMin) FiltMin = d;
            if (d > FiltMax) FiltMax = d;
        }
    }
    else
    {  // Subframe
        int x, y;
        i=0;
        for (y=0; y<Subframe.height; y++) {
            ptr = ImageData + Subframe.x + (Subframe.y + y)*Size.GetWidth();
            for (x=0; x<Subframe.width; x++, ptr++, i++) {
               d = (int) ( *ptr);
               if (d < Min) Min = d;
               if (d > Max) Max = d;
               tmpdata[i]=d;
           }
        }

        Median3(tmpdata, Subframe.width, Subframe.height);
        for (i=0, ptr=tmpdata; i<(Subframe.width*Subframe.height); i++, ptr++) {
            d = (int) ( *ptr);
            if (d < FiltMin) FiltMin = d;
            if (d > FiltMax) FiltMax = d;
        }
    }

    delete [] tmpdata;
}

bool usImage::Clean() {

    return false;
}

bool usImage::CopyToImage(wxImage **rawimg, int blevel, int wlevel, double power)
{
    wxImage *img = *rawimg;

    if (!img->Ok() || (img->GetWidth() != Size.GetWidth()) || (img->GetHeight() != Size.GetHeight()) ) // can't reuse bitmap
    {
        if (img->Ok())
        {
            delete img;  // Clear out current image if it exists
            img = (wxImage *) NULL;
        }
        img = new wxImage(Size.GetWidth(), Size.GetHeight(), false);
    }

    unsigned char *ImgPtr = img->GetData();
    unsigned short *RawPtr = ImageData;

    if (power == 1.0 || blevel >= wlevel)
    {
        float range = wlevel;  // Go 0-max
        for (int i = 0; i < NPixels; i++, RawPtr++)
        {
            float d;
            if (*RawPtr >= wlevel)
                d = 255.0;
            else
                d = ((float) (*RawPtr) / range) * 255.0;

            *ImgPtr++ = (unsigned char) d;
            *ImgPtr++ = (unsigned char) d;
            *ImgPtr++ = (unsigned char) d;
        }
    }
    else
    {
        float range = (float) (wlevel - blevel);
        for (int i = 0; i < NPixels; i++, RawPtr++ )
        {
            float d;
            if (*RawPtr <= blevel)
                d = 0.0;
            else if (*RawPtr >= wlevel)
                d = 255.0;
            else
            {
                d = ((float) (*RawPtr) - (float) blevel) / range;
                d = pow(d, (float) power) * 255.0;
            }
            *ImgPtr++ = (unsigned char) d;
            *ImgPtr++ = (unsigned char) d;
            *ImgPtr++ = (unsigned char) d;
        }
    }

    *rawimg = img;
    return false;
}

bool usImage::BinnedCopyToImage(wxImage **rawimg, int blevel, int wlevel, double power) {
    wxImage *img;
    unsigned char *ImgPtr;
    unsigned short *RawPtr;
    int x,y;
    float d;
    //, s_factor;
    int full_xsize, full_ysize;
    int use_xsize, use_ysize;

    full_xsize = Size.GetWidth();
    full_ysize = Size.GetHeight();
    use_xsize = full_xsize;
    use_ysize = full_ysize;
    if (use_xsize % 2) use_xsize--;
    if (use_ysize % 2) use_ysize--;
//  Binsize = 2;

    img = *rawimg;
    if ((!img->Ok()) || (img->GetWidth() != (full_xsize/2)) || (img->GetHeight() != (full_ysize/2)) ) {  // can't reuse bitmap
        if (img->Ok()) {
            delete img;  // Clear out current image if it exists
            img = (wxImage *) NULL;
        }
        img = new wxImage(full_xsize/2, full_ysize/2, false);
    }
    ImgPtr = img->GetData();
    RawPtr = ImageData;
//  s_factor = (((float) Max - (float) Min) / 255.0);
    float range = (float) (wlevel - blevel);

    if ((power == 1.0) || (range == 0.0)) {
        range = wlevel;  // Go 0-max
        if (range == 0.0) range = 0.001;
        for (y=0; y<use_ysize; y+=2) {
            for (x=0; x<use_xsize; x+=2) {
                RawPtr = ImageData + x + y*full_xsize;
                d = (float) (*RawPtr + *(RawPtr+1) + *(RawPtr+full_xsize) + *(RawPtr+1+full_xsize)) / 4.0;
                d = (d / range) * 255.0;
                if (d < 0.0) d = 0.0;
                else if (d > 255.0) d = 255.0;
                *ImgPtr = (unsigned char) d;
                ImgPtr++;
                *ImgPtr = (unsigned char) d;
                ImgPtr++;
                *ImgPtr = (unsigned char) d;
                ImgPtr++;
            }
        }
    }
    else {
        for (y=0; y<use_ysize; y+=2) {
            for (x=0; x<use_xsize; x+=2) {
                RawPtr = ImageData + x + y*full_xsize;
                d = (float) (*RawPtr + *(RawPtr+1) + *(RawPtr+full_xsize) + *(RawPtr+1+full_xsize)) / 4.0;
                d = (d - (float) blevel) / range ;
                if (d < 0.0) d= 0.0;
                else if (d > 1.0) d = 1.0;
                d = pow(d, (float) power) * 255.0;
                *ImgPtr = (unsigned char) d;
                ImgPtr++;
                *ImgPtr = (unsigned char) d;
                ImgPtr++;
                *ImgPtr = (unsigned char) d;
                ImgPtr++;
            }
        }
    }
    *rawimg = img;
    return false;
}

void usImage::InitDate() {
    time_t now;
    struct tm *timestruct;
    time(&now);
    timestruct=gmtime(&now);
    ImgStartDate=wxString::Format("%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",timestruct->tm_year+1900,timestruct->tm_mon+1,
        timestruct->tm_mday,timestruct->tm_hour,timestruct->tm_min,timestruct->tm_sec);

}

bool usImage::Save(const wxString& fname)
{
    bool bError = false;

    try
    {
        fitsfile *fptr;  // FITS file pointer
        int status = 0;  // CFITSIO status value MUST be initialized to zero!
        long fpixel[3] = {1,1,1};
        long fsize[3];
        int output_format=USHORT_IMG;
        fsize[0] = (long) Size.GetWidth();
        fsize[1] = (long) Size.GetHeight();
        fsize[2] = 0;
        fits_create_file(&fptr,(const char*) fname.mb_str(wxConvUTF8),&status);
        if (!status) fits_create_img(fptr,output_format, 2, fsize, &status);
        if (!status) fits_write_pix(fptr,TUSHORT,fpixel,NPixels,ImageData,&status);
        fits_close_file(fptr,&status);
        bError = status?true:false;

        if (bError)
            pFrame->SetStatusText(wxString::Format(_("%s Not saved"), fname));
        else
            pFrame->SetStatusText(wxString::Format(_("%s saved"),fname));
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool usImage::Load(const wxString& fname)
{
    bool bError = false;

    try
    {
        fitsfile *fptr;  // FITS file pointer
        int status = 0;  // CFITSIO status value MUST be initialized to zero!
        long fpixel[3] = {1,1,1};
        long fsize[3];
        int hdutype, naxis;
        int nhdus=0;

        if (!wxFileExists(fname))
        {
            pFrame->Alert(_("File does not exist - cannot load ") + fname);
            throw ERROR_INFO("File does not exist");
        }
        if (!fits_open_diskfile(&fptr, fname.c_str(), READONLY, &status))
        {
            if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU)
            {
                pFrame->Alert(_("FITS file is not of an image: ") + fname);
                throw ERROR_INFO("Fits file is not an image");
            }

            // Get HDUs and size
            fits_get_img_dim(fptr, &naxis, &status);
            fits_get_img_size(fptr, 2, fsize, &status);
            fits_get_num_hdus(fptr,&nhdus,&status);
            if ((nhdus != 1) || (naxis != 2)) {
                pFrame->Alert(_("Unsupported type or read error loading FITS file ") + fname);
                throw ERROR_INFO("unsupported type");
            }
            if (Init((int) fsize[0],(int) fsize[1]))
            {
                pFrame->Alert(_("Memory allocation error loading FITS file ") + fname);
                throw ERROR_INFO("Memory Allocation failure");
            }
            if (fits_read_pix(fptr, TUSHORT, fpixel, (int) (fsize[0] * fsize[1]), NULL, ImageData, NULL, &status) ) { // Read image
                pFrame->Alert(_("Error reading data from FITS file ") + fname);
                throw ERROR_INFO("Error reading");
            }
            fits_close_file(fptr,&status);
        }
        else
        {
            pFrame->Alert(_("Error opening FITS file ") + fname);
            throw ERROR_INFO("error opening file");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool usImage::Rotate(double theta, bool mirror)
{
    wxImage *pImg = new wxImage();

    CalcStats();

    CopyToImage(&pImg, Min, Max, 1.0);

    wxImage mirrored = *pImg;

    if (mirror)
    {
        mirrored = pImg->Mirror(false);
    }

    wxImage rotated = mirrored.Rotate(theta, wxPoint(0,0));

    CopyFromImage(rotated);

    delete pImg;

    return false;
}

bool usImage::CopyFromImage(const wxImage &img)
{
    Init(img.GetWidth(), img.GetHeight());

    unsigned char *pSrc = img.GetData();
    unsigned short *pDest = ImageData;

    for (int i=0; i<NPixels;i++)
    {
        double val = *pSrc;

        val *=255.0;

        if (val < 0)
        {
            val = 0.0;
        }
        else if (val > 65535.0)
        {
            val = 65535;
        }

        *pDest++ = (unsigned short)val;

        pSrc += 3;
    }

    return false;
}

