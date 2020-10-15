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

// JackAU.h


#include <AUEffectBase.h>
#include "../JARInsert/JARInsert.h"
#include "JackUnitVersion.h"
#include <AudioUnit/AudioUnitCarbonView.h>

class ElCAJAS : public AUEffectBase
{
    public:
        ElCAJAS(AudioUnit component);
        ~ElCAJAS();

        virtual OSStatus ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags,
                                             const AudioBufferList& inBuffer,
                                             AudioBufferList& outBuffer,
                                             UInt32	inFramesToProcess);

        virtual UInt32 SupportedNumChannels(const AUChannelInfo** outInfo);

        int	GetNumCustomUIComponents()
        {
            return 1;
        }

        void GetUIComponentDescs(ComponentDescription* inDescArray)
        {
            inDescArray[0].componentType = kAudioUnitCarbonViewComponentType;
            inDescArray[0].componentSubType = 'JASb';
            inDescArray[0].componentManufacturer = 'ElCa';
            inDescArray[0].componentFlags = 0;
            inDescArray[0].componentFlagsMask = 0;
        }

        virtual ComponentResult	Initialize();

        virtual void Cleanup();

        virtual	ComponentResult	GetParameterValueStrings(AudioUnitScope	inScope,
                AudioUnitParameterID inParameterID,
                CFArrayRef*	outStrings);

        virtual ComponentResult	GetParameterInfo(AudioUnitScope	inScope, AudioUnitParameterID inParameterID,
                AudioUnitParameterInfo& outParameterInfo );

        virtual bool ValidFormat(AudioUnitScope	inScope,
                                  AudioUnitElement inElement,
                                  const CAStreamBasicDescription& inNewFormat)
        {
            return true;
        }

        virtual bool StreamFormatWritable(AudioUnitScope scope, AudioUnitElement element)
        {
            return true;
        }

        virtual	ComponentResult	ChangeStreamFormat(AudioUnitScope inScope,
                AudioUnitElement inElement,
                const CAStreamBasicDescription& inPrevFormat,
                const CAStreamBasicDescription& inNewFormat);
				
		virtual ComponentResult	Version() { return kJackUnitVersion; }
		
	private:
        JARInsert *c_jar;
        int c_error;

        enum {
            kNumSupportedNumChannels = 2,
		};

        static AUChannelInfo m_aobSupportedNumChannels[kNumSupportedNumChannels];
};
