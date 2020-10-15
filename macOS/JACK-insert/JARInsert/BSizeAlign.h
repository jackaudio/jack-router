/*
BSizeAlign.h

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

#include <stdlib.h>
#include "../JARInsert/ringbuffer.h"

class BSizeAlign
{
    public:
        BSizeAlign(long in_frames, long out_frames);
        ~BSizeAlign();
        bool Ready()
        {
            return c_ready;
        }
        bool AddBuffer(float* buffer);
        bool GetBuffer(float* buffer);
        bool CanGet()
        {
            return c_can_get;
        }
    private:
        enum {
            typeINbig, typeOUTbig
		};
        RingBuffer c_rb;
        float* c_rb_data;
        long c_in_frames;
        long c_out_frames;
        bool c_ready;
        bool c_can_get;
        int c_type;
};

