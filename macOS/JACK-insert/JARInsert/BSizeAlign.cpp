/*
BSizeAlign.cpp

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.


This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.


You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

(c) 2004, elementicaotici - by Johnny (Giovanni) Petrantoni, ITALY - Rome.
e-mail: johnny@meskalina.it web: http://www.meskalina.it 
*/

#include "BSizeAlign.h"

BSizeAlign::BSizeAlign(long in_frames, long out_frames) : c_in_frames(in_frames), c_out_frames(out_frames), c_can_get(false)
{
    if (c_out_frames > c_in_frames)
        c_type = typeOUTbig;
    else
        c_type = typeINbig;

    if (c_type == typeOUTbig) {
        c_rb_data = (float*)malloc(sizeof(float) * c_out_frames);
        memset(c_rb_data, 0x0, sizeof(float)*c_out_frames);

        long rb_res = RingBuffer_Init(&c_rb, sizeof(float) * c_out_frames, c_rb_data);
        if (rb_res == -1) {
            free(c_rb_data);
            c_ready = false;
            return ;
        } else
            c_ready = true;
    } else {
        c_rb_data = (float*)malloc(sizeof(float) * c_in_frames);
        memset(c_rb_data, 0x0, sizeof(float)*c_in_frames);

        long rb_res = RingBuffer_Init(&c_rb, sizeof(float) * c_in_frames, c_rb_data);
        if (rb_res == -1) {
            free(c_rb_data);
            c_ready = false;
            return ;
        } else
            c_ready = true;
    }
}

BSizeAlign::~BSizeAlign()
{
    if (c_ready) {
        RingBuffer_Flush(&c_rb);
        free(c_rb_data);
    }
}

bool BSizeAlign::AddBuffer(float *buffer)
{
    if (buffer == NULL)
        return false;
    if (c_type == typeOUTbig) {
        long size = sizeof(float) * c_in_frames;
        long res = RingBuffer_Write(&c_rb, (void*)buffer, size);
        long canRead = RingBuffer_GetReadAvailable(&c_rb);
        if (canRead == (long)(sizeof(float)*c_out_frames))
            c_can_get = true;
        if (res == size)
            return true;
    } else {
        long size = sizeof(float) * c_in_frames;
        long res = RingBuffer_Write(&c_rb, (void*)buffer, size);
        long canRead = RingBuffer_GetReadAvailable(&c_rb);
        if (canRead > (long)(sizeof(float)*c_out_frames))
            c_can_get = true;
        if (res == size)
            return true;
    }
    return false;
}

bool BSizeAlign::GetBuffer(float *buffer)
{
    if (buffer == NULL)
        return false;
    if (c_can_get) {
        if (c_type == typeOUTbig) {
            long size = sizeof(float) * c_out_frames;
            long res = RingBuffer_Read(&c_rb, buffer, size);
            c_can_get = false;
            if (res == size)
                return true;
        } else {
            long size = sizeof(float) * c_out_frames;
            long res = RingBuffer_Read(&c_rb, buffer, size);
            if (RingBuffer_GetReadAvailable(&c_rb) == 0)
                c_can_get = false;
            if (res == size)
                return true;
        }
    }
    return false;
}
