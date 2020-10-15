/*
Copyright ©  Johnny Petrantoni 2003

This library is free software; you can redistribute it and modify it under
the terms of the GNU Library General Public License as published by the
Free Software Foundation version 2 of the License, or any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License
for more details.

You should have received a copy of the GNU Library General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// JackAU.cpp

#include <JackAU.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENT_ENTRY(ElCAJAS);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUChannelInfo	ElCAJAS::m_aobSupportedNumChannels[ ElCAJAS::kNumSupportedNumChannels ] =
    { {1, 2}, {2, 2} };

ElCAJAS::ElCAJAS(AudioUnit component) : AUEffectBase(component), c_jar(NULL), c_error(JARInsert::kNoErr)
{
    CreateElements();
}

void ElCAJAS::Cleanup()
{
    if (c_jar) {
        delete c_jar;
        c_jar = NULL;
    }
}

ElCAJAS::~ElCAJAS()
{
    Cleanup();
}

UInt32	ElCAJAS::SupportedNumChannels(const AUChannelInfo** outInfo)
{
    if (outInfo != NULL)
        *outInfo = &m_aobSupportedNumChannels[0];
    return kNumSupportedNumChannels;
}

ComponentResult	ElCAJAS::Initialize()
{
    c_jar = new JARInsert('au  ');
    c_error = c_jar->GetError();
    return noErr;
}

ComponentResult	ElCAJAS::GetParameterValueStrings(AudioUnitScope inScope,
        AudioUnitParameterID inParameterID,
        CFArrayRef*	outStrings)
{
    return kAudioUnitErr_InvalidProperty;
}

ComponentResult	ElCAJAS::ChangeStreamFormat(AudioUnitScope inScope,
        AudioUnitElement inElement,
        const CAStreamBasicDescription& inPrevFormat,
        const CAStreamBasicDescription& inNewFormat)
{
    if (inScope == 1) {
        int reqChans = inNewFormat.NumberChannels();
        if (reqChans > 2 || reqChans < 1)
            return kAudioUnitErr_FormatNotSupported;
        else
            return noErr;
    } else if (inScope == 2) {
        int reqChans = inNewFormat.NumberChannels();
        if (reqChans != 2)
            return kAudioUnitErr_FormatNotSupported;
        else
            return noErr;
    }
    return kAudioUnitErr_FormatNotSupported;
}

ComponentResult	ElCAJAS::GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID,
        AudioUnitParameterInfo	&outParameterInfo)
{
    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidParameter;

    ComponentResult result = noErr;

    outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable |
                             kAudioUnitParameterFlag_IsReadable |
                             kAudioUnitParameterFlag_Global;

    char* pcName = outParameterInfo.name;

    switch (inParameterID) {

        case 0:
            if (c_error == JARInsert::kNoErr)
                strcpy(pcName, "Jack is online");
            else
                strcpy(pcName, "Jack is offline");
            outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
            outParameterInfo.minValue = 4;
            outParameterInfo.maxValue = 5;
            outParameterInfo.defaultValue = 4;
            break;
        default:
            result = kAudioUnitErr_InvalidParameter;
            break;
    }

    return result;
}

OSStatus ElCAJAS::ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags,
                                     const AudioBufferList&	inBuffer,
                                     AudioBufferList& outBuffer,
                                     UInt32	inFramesToProcess)
{
    int i;
    bool inIsInterleaved = false;
    bool outIsInterleaved = false;
	
	if (inBuffer.mNumberBuffers == 1 && inBuffer.mBuffers[0].mNumberChannels > 1)
        inIsInterleaved = true;
    if (outBuffer.mNumberBuffers == 1 && outBuffer.mBuffers[0].mNumberChannels > 1)
        outIsInterleaved = true;

    if (!inIsInterleaved && !outIsInterleaved) {
	     if (c_error == JARInsert::kNoErr && c_jar) {
            float* inBuf[2];
            float* outBuf[2];
            for (i = 0; i < 2 && i < (int)inBuffer.mNumberBuffers; i++) {
                inBuf[i] = (float*)inBuffer.mBuffers[i].mData;
                outBuf[i] = (float*)outBuffer.mBuffers[i].mData;
            }
            if (inBuffer.mNumberBuffers == 1)
                memcpy(inBuf[1], inBuf[0], sizeof(float)*inFramesToProcess);
            if (c_jar->GetError() == JARInsert::kNoErr) {
                if (!c_jar->CanProcess())
                    c_jar->AllocBSizeAlign(inFramesToProcess);
                if (c_jar->CanProcess())
                    c_jar->Process(inBuf, outBuf, inFramesToProcess);
            }
        } else
            for (i = 0;i < (int)outBuffer.mNumberBuffers;i++)
                memset(outBuffer.mBuffers[i].mData, 0x0, outBuffer.mBuffers[i].mDataByteSize);
    } else {
        if (c_error == JARInsert::kNoErr && c_jar) {
			float* buffer = (float*)inBuffer.mBuffers[0].mData;
            float inBufs[2][inFramesToProcess];
            for (i = 0; i < 2 && i < (int)inBuffer.mBuffers[0].mNumberChannels;i++) {
                long nframes = inFramesToProcess;
                long count = 0;
                while (count < nframes) {
                    inBufs[i][count] = buffer[i + (inBuffer.mBuffers[0].mNumberChannels * count)];
                    count++;
                }
            }
            if (inBuffer.mBuffers[0].mNumberChannels == 1)
                memcpy(inBufs[1], inBufs[0], sizeof(float)*inFramesToProcess);
            buffer = (float*)outBuffer.mBuffers[0].mData;
            float outBufs[2][inFramesToProcess];
            for (i = 0; i < 2; i++) {
                long nframes = inFramesToProcess;
                long count = 0;
                while (count < nframes) {
                    outBufs[i][count] = buffer[i + (outBuffer.mBuffers[0].mNumberChannels * count)];
                    count++;
                }
            }
            if (c_jar->GetError() == JARInsert::kNoErr) {
                if (!c_jar->CanProcess())
                    c_jar->AllocBSizeAlign(inFramesToProcess);
                if (c_jar->CanProcess())
                    c_jar->Process((float**)inBufs, (float**)outBufs, inFramesToProcess);
            }
        } else
            for (i = 0; i < (int)outBuffer.mNumberBuffers; i++)
                memset(outBuffer.mBuffers[i].mData, 0x0, outBuffer.mBuffers[i].mDataByteSize);
    }

    return noErr;
}

