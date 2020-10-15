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
#include <CoreAudio/CoreAudio.h>
#include <Jack/jack.h>
#include "ringbuffer.h"



enum Parameters {
        
        kDryLevel,kSelectMode
        
};

enum {
    kAudioDevicePropertyGetJackClient  = 'jasg', kAudioDevicePropertyReleaseJackClient  = 'jasr'
};


class ElCAJAS : public AUEffectBase
{
public:
    ElCAJAS(AudioUnit component);
    ~ElCAJAS();

    virtual OSStatus	ProcessBufferLists( AudioUnitRenderActionFlags &	ioActionFlags,
                                            const AudioBufferList &		inBuffer,
                                            AudioBufferList &			outBuffer,
                                            UInt32				inFramesToProcess);

    virtual UInt32	SupportedNumChannels (	const AUChannelInfo**		outInfo);
    
    int		GetNumCustomUIComponents () { return 1; }
        
    void	GetUIComponentDescs (ComponentDescription* inDescArray) {
        inDescArray[0].componentType = kAudioUnitCarbonViewComponentType;
        inDescArray[0].componentSubType = 'JASb';
        inDescArray[0].componentManufacturer = 'ElCA';
        inDescArray[0].componentFlags = 0;
        inDescArray[0].componentFlagsMask = 0;
    }

    virtual ComponentResult	Initialize();

    virtual	ComponentResult		GetParameterValueStrings(	AudioUnitScope			inScope,
                                                            AudioUnitParameterID	inParameterID,
                                                            CFArrayRef *			outStrings	);

    virtual ComponentResult	GetParameterInfo( AudioUnitScope		inScope,									AudioUnitParameterID		inParameterID,
                                                AudioUnitParameterInfo		&outParameterInfo );

private:
    int manyInBuffers;
    bool rBufOn;
    float *vRBuffer1;
    float *vRBuffer2;
    float **inFromRing;
    float *outFromRing;
    RingBuffer *sRingBuffer1;
    RingBuffer *sRingBuffer2;
    long jBufferSize;
    bool needsDeactivate;
    UInt32 isRunning;
    
    static int instances;
    jack_client_t *client;
    jack_port_t **inPorts;
    jack_port_t **outPorts;
    int nInPorts,nOutPorts;
    int conto;
    int vectors;
    bool jackIsOn;
    void openAudioFTh();
    void flush();
    int status;
    AudioDeviceID jackID;
    
    static const float kDefault_DryLevel	= 1.0f;
    
    

    enum {
        kNumSupportedNumChannels = 2,
    };
    static AUChannelInfo	m_aobSupportedNumChannels[ kNumSupportedNumChannels ];

    OSStatus	ProcessInterleavedMonoInput( const AudioBuffer& obInBuffer, 
                                                            AudioBuffer& obOutBuffer,
                                                            UInt32 inFramesToProcess,
                                                            AudioUnitRenderActionFlags& ioActionFlags );
    OSStatus	ProcessInterleavedStereoInput( const AudioBuffer& obInBuffer, 
                                                            AudioBuffer& obOutBuffer,
                                                            UInt32 inFramesToProcess,
                                                            AudioUnitRenderActionFlags& ioActionFlags );
    OSStatus	ProcessDeInterleavedMonoInput( const AudioBufferList& obInBuffers, 
                                                            AudioBufferList& obOutBuffers,
                                                            UInt32 inFramesToProcess,
                                                            AudioUnitRenderActionFlags& ioActionFlags );
    OSStatus	ProcessDeInterleavedStereoInput( const AudioBufferList& obInBuffers, 
                                                            AudioBufferList& obOutBuffers,
                                                            UInt32 inFramesToProcess,
                                                            AudioUnitRenderActionFlags& ioActionFlags );

    OSStatus	ProcessCore(	Float32 *pfInBuffer0,
												Float32 *pfInBuffer1,
                                                Float32 *pfOutBuffer0,
                                                Float32 *pfOutBuffer1,
                                                int iInStride,
                                                int iOutStride,
                                                UInt32 inFramesToProcess );
                                                

};
