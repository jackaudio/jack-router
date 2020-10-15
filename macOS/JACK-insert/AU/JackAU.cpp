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
#define DEBUG 1

int ElCAJAS::instances = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENT_ENTRY(ElCAJAS);
COMPONENT_REGISTER(ElCAJAS, 'aufx' , 'JASb', 'ElCA');
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUChannelInfo	ElCAJAS::m_aobSupportedNumChannels[ ElCAJAS::kNumSupportedNumChannels ] = 
    { { 1, 2 }, { 2, 2 } };
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ElCAJAS::ElCAJAS(AudioUnit component) : AUEffectBase(component)
{
    CreateElements();
    
    SetParameter( kDryLevel,4 );
    
    status = 2;
}


ElCAJAS::~ElCAJAS() {
    if(status!=2) {
        status=2;
        flush();
    }
    
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

UInt32		ElCAJAS::SupportedNumChannels ( const AUChannelInfo**	outInfo )
{
    if ( outInfo != NULL )
        *outInfo = &m_aobSupportedNumChannels[0];
    return kNumSupportedNumChannels;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ComponentResult	ElCAJAS::Initialize()
{

#ifdef DEBUG
    printf("-----------JASBus-----------log-----\n");
    printf("There are %d istances of JASBus\n",instances);
#endif
    
        jackIsOn = false;

        openAudioFTh();
#ifdef DEBUG
        if(!jackIsOn) printf("CLIENT is NULL\n");
#endif
        else {
        
            int nPorte=2;
        
            inPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorte);
            outPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorte);
        
            nInPorts = nOutPorts =  nPorte;
            
            jBufferSize = jack_get_buffer_size(client);
        
        
            for(int i=0;i<nInPorts;i++) {
                char *newName = (char*)calloc(256,sizeof(char));
                sprintf(newName,"AUreturn%d",instances+i+1);
                inPorts[i] = jack_port_register(client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);
#ifdef DEBUG
                printf("Port: %s created\n",newName);
#endif
                free(newName);
            }
        
            for(int i=0;i<nOutPorts;i++) {
                char *newName = (char*)calloc(256,sizeof(char));
                sprintf(newName,"AUsend%d",instances+i+1);
                outPorts[i] = jack_port_register(client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);
#ifdef DEBUG
                printf("Port: %s created\n",newName);
#endif
                free(newName);
            }
			
			float *out1 = (float*) jack_port_get_buffer(outPorts[0],(jack_nframes_t)jBufferSize);
			float *out2 = (float*) jack_port_get_buffer(outPorts[1],(jack_nframes_t)jBufferSize);
			
			if(out1) memset(out1,0x0,sizeof(float)*jBufferSize);
			if(out2) memset(out2,0x0,sizeof(float)*jBufferSize);
        
            if(!isRunning) { 
#ifdef DEBUG
				printf("Jack client activated\n"); 
#endif
				jack_activate(client); 
				needsDeactivate = true;
			}
            else needsDeactivate = false;
        
            status = 77;
            instances += 2;
            
            rBufOn = false;
        } 
    
    return noErr;
}

ComponentResult		ElCAJAS::GetParameterValueStrings(	AudioUnitScope			inScope,
                                                                AudioUnitParameterID	inParameterID,
                                                                CFArrayRef *			outStrings)
{

    
    return kAudioUnitErr_InvalidProperty;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ComponentResult	ElCAJAS::GetParameterInfo( AudioUnitScope			inScope,									AudioUnitParameterID		inParameterID,
                                                AudioUnitParameterInfo		&outParameterInfo )
{
    if (inScope != kAudioUnitScope_Global)
        return kAudioUnitErr_InvalidParameter;

    ComponentResult result = noErr;

    outParameterInfo.flags = 	kAudioUnitParameterFlag_IsWritable |
                    		kAudioUnitParameterFlag_IsReadable |
                    		kAudioUnitParameterFlag_Global;
	
    char *pcName = outParameterInfo.name;
	
    switch(inParameterID)
    {

        case kDryLevel:
            if(status==77) strcpy( pcName, "Jack is ONLINE" );
            else strcpy( pcName, "Jack is OFFLINE" );
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

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OSStatus	ElCAJAS::ProcessBufferLists( AudioUnitRenderActionFlags &	ioActionFlags,
                                            const AudioBufferList &		inBuffer,
                                            AudioBufferList &			outBuffer,
                                            UInt32				inFramesToProcess)
{
    ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
    
    UInt32 uiInBuffers = inBuffer.mNumberBuffers;
    UInt32 uiOutBuffers = outBuffer.mNumberBuffers;
    
    if ( uiInBuffers == 1 )
    {
        if ( uiOutBuffers == 1 )
        {
            if ( outBuffer.mBuffers[0].mNumberChannels != 2 )
                return kAudioUnitErr_FormatNotSupported;
            UInt32 uiInChannels = inBuffer.mBuffers[0].mNumberChannels;
            if ( uiInChannels > 2 )
                return kAudioUnitErr_FormatNotSupported;
            if ( uiInChannels == 1 )
                return ProcessInterleavedMonoInput( inBuffer.mBuffers[0], 
                                                    outBuffer.mBuffers[0], 
                                                    inFramesToProcess, ioActionFlags );
            return ProcessInterleavedStereoInput(   inBuffer.mBuffers[0], 
                                                    outBuffer.mBuffers[0], 
                                                    inFramesToProcess, ioActionFlags );
        }
        if ( uiOutBuffers != 2 )
            return kAudioUnitErr_FormatNotSupported;
        return ProcessDeInterleavedMonoInput(	inBuffer, 
                                                outBuffer, 
                                                inFramesToProcess, ioActionFlags );
    }
    
    if ( uiInBuffers != 2 || uiOutBuffers != 2 )
        return kAudioUnitErr_FormatNotSupported;
    
    return ProcessDeInterleavedStereoInput(	inBuffer, 
                                                outBuffer, 
                                                inFramesToProcess, ioActionFlags );
}
        
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
OSStatus	ElCAJAS::ProcessInterleavedMonoInput( const AudioBuffer& obInBuffer, 
                                                            AudioBuffer& obOutBuffer,
                                                            UInt32 inFramesToProcess,
                                                            AudioUnitRenderActionFlags& ioActionFlags )
{
    ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

	Float32 *pfInBuffer0 = ( Float32 *)obInBuffer.mData;
    Float32 *pfOutBuffer0 = (Float32 *)obOutBuffer.mData;
    Float32 *pfOutBuffer1 = pfOutBuffer0+1;
    
    return ProcessCore( pfInBuffer0, pfInBuffer0, pfOutBuffer0, pfOutBuffer1,
                            1, 2, inFramesToProcess );
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
           
OSStatus	ElCAJAS::ProcessInterleavedStereoInput( const AudioBuffer& obInBuffer, 
                                                            AudioBuffer& obOutBuffer,
                                                            UInt32 inFramesToProcess,
                                                            AudioUnitRenderActionFlags& ioActionFlags )
{
    ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

	Float32 *pfInBuffer0 = ( Float32 *)obInBuffer.mData;
	Float32 *pfInBuffer1 = pfInBuffer0+1;
    Float32 *pfOutBuffer0 = (Float32 *)obOutBuffer.mData;
    Float32 *pfOutBuffer1 = pfOutBuffer0+1;
    
    return ProcessCore( pfInBuffer0, pfInBuffer1, pfOutBuffer0, pfOutBuffer1,
                            2, 2, inFramesToProcess );
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            
OSStatus	ElCAJAS::ProcessDeInterleavedMonoInput( const AudioBufferList& obInBuffers, 
                                                            AudioBufferList& obOutBuffers,
                                                            UInt32 inFramesToProcess,
                                                            AudioUnitRenderActionFlags& ioActionFlags )
{
    ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

    const AudioBuffer& obInBuffer0 = obInBuffers.mBuffers[0];
    AudioBuffer& obOutBuffer0 = obOutBuffers.mBuffers[0];
    AudioBuffer& obOutBuffer1 = obOutBuffers.mBuffers[1];

	Float32 *pfInBuffer0 = ( Float32 *)obInBuffer0.mData;
    Float32 *pfOutBuffer0 = (Float32 *)obOutBuffer0.mData;
    Float32 *pfOutBuffer1 = (Float32 *)obOutBuffer1.mData;
    
    return ProcessCore( pfInBuffer0, pfInBuffer0, pfOutBuffer0, pfOutBuffer1,
                            1, 1, inFramesToProcess );
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
             
OSStatus	ElCAJAS::ProcessDeInterleavedStereoInput( const AudioBufferList& obInBuffers, 
                                                            AudioBufferList& obOutBuffers,
                                                            UInt32 inFramesToProcess,
                                                            AudioUnitRenderActionFlags& ioActionFlags )
{
    ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

    const AudioBuffer& obInBuffer0 = obInBuffers.mBuffers[0];
    const AudioBuffer& obInBuffer1 = obInBuffers.mBuffers[1];
    AudioBuffer& obOutBuffer0 = obOutBuffers.mBuffers[0];
    AudioBuffer& obOutBuffer1 = obOutBuffers.mBuffers[1];

	Float32 *pfInBuffer0 = ( Float32 *)obInBuffer0.mData;
	Float32 *pfInBuffer1 = ( Float32 *)obInBuffer1.mData;
    Float32 *pfOutBuffer0 = (Float32 *)obOutBuffer0.mData;
    Float32 *pfOutBuffer1 = (Float32 *)obOutBuffer1.mData;
    
    return ProcessCore( pfInBuffer0, pfInBuffer1, pfOutBuffer0, pfOutBuffer1,
                            1, 1, inFramesToProcess );
}


OSStatus	ElCAJAS::ProcessCore(Float32 *pfInBuffer0,
												Float32 *pfInBuffer1,
                                                Float32 *pfOutBuffer0,
                                                Float32 *pfOutBuffer1,
                                                int iInStride,
                                                int iOutStride,
                                                UInt32 inFramesToProcess )
{
	
    if(status==77) {
    
		if(jBufferSize>(long)inFramesToProcess && !rBufOn) { 
#ifdef DEBUG
			printf("Setting up a ring buffer\nJack buffer size: %ld , host buffer size: %ld\n",jBufferSize,inFramesToProcess); 
#endif
			vRBuffer1 = (float*)malloc(sizeof(float)*jBufferSize*2);
			sRingBuffer1 = (RingBuffer*)malloc(sizeof(RingBuffer));
			long size = RingBuffer_Init(sRingBuffer1,sizeof(float)*jBufferSize*2,vRBuffer1);
        
			manyInBuffers = (jBufferSize/inFramesToProcess)*2;
        
			inFromRing = (float**)malloc(sizeof(float*)*manyInBuffers);
			for(int i=0;i<manyInBuffers;i++) {
				inFromRing[i] = (float*)malloc(sizeof(float)*inFramesToProcess);
			}
        
        
			if(size==-1) { 
#ifdef DEBUG
				printf("Cannot create a correct ring buffer\n"); 
#endif  
				return noErr; 
			}
        
			vRBuffer2 = (float*)malloc(sizeof(float)*jBufferSize*2);
			sRingBuffer2 = (RingBuffer*)malloc(sizeof(RingBuffer));
			size = RingBuffer_Init(sRingBuffer2,sizeof(float)*jBufferSize*2,vRBuffer2);
			outFromRing = (float*)malloc(sizeof(float)*inFramesToProcess);
        
			if(size==-1) {
#ifdef DEBUG
				printf("Cannot create a correct ring buffer\n"); 
#endif
				return noErr; 
			}
        
#ifdef DEBUG
			printf("ring buffer created\n");
#endif
			rBufOn = true;
		}
    
		if(jBufferSize>(long)inFramesToProcess && rBufOn) {
                
			if(RingBuffer_GetWriteAvailable(sRingBuffer1)>=(long)(sizeof(float)*inFramesToProcess*2)) {
				RingBuffer_Write(sRingBuffer1,(void*)pfInBuffer0,sizeof(float)*inFramesToProcess);
				RingBuffer_Write(sRingBuffer1,(void*)pfInBuffer1,sizeof(float)*inFramesToProcess);
			}
                
			if(RingBuffer_GetReadAvailable(sRingBuffer1)==(long)(sizeof(float)*jBufferSize*2)) {
				float *out1 = (float*) jack_port_get_buffer(outPorts[0],(jack_nframes_t)jBufferSize);
				float *out2 = (float*) jack_port_get_buffer(outPorts[1],(jack_nframes_t)jBufferSize);
            
				int manyLoops = manyInBuffers/2;
            
				int a = 0;
            
				for(int i=0;i<manyLoops;i++) {
					RingBuffer_Read(sRingBuffer1,inFromRing[a],sizeof(float)*inFramesToProcess);
					for(int l=0;l<(long)inFramesToProcess;l++) {
						*out1 = inFromRing[a][l];
						*out1++;
					}
					a++;
					RingBuffer_Read(sRingBuffer1,inFromRing[a],sizeof(float)*inFramesToProcess);
					for(int l=0;l<(long)inFramesToProcess;l++) {
						*out2 = inFromRing[a][l];
						*out2++; 
					}
					a++;
				}
			}
        
			if(RingBuffer_GetWriteAvailable(sRingBuffer2)==(long)(sizeof(float)*jBufferSize*2)) {
				float *in1 = (float*) jack_port_get_buffer(inPorts[0],(jack_nframes_t)jBufferSize);
				float *in2 = (float*) jack_port_get_buffer(inPorts[1],(jack_nframes_t)jBufferSize);
            
				int manyLoops = manyInBuffers/2;
            
				int a = 0;
            
				for(int i=0;i<manyLoops;i++) {
					for(int l=0;l<(long)inFramesToProcess;l++) {
						outFromRing[l] = *in1;
						*in1++;
					}
					RingBuffer_Write(sRingBuffer2,outFromRing,sizeof(float)*inFramesToProcess);
					a++;
					for(int l=0;l<(long)inFramesToProcess;l++) {
						outFromRing[l] = *in2;
						*in2++;
					}
					RingBuffer_Write(sRingBuffer2,outFromRing,sizeof(float)*inFramesToProcess);
					a++;
				}
			}
        
			if(RingBuffer_GetReadAvailable(sRingBuffer2)>=(long)(sizeof(float)*inFramesToProcess*2)) {
				RingBuffer_Read(sRingBuffer2,pfOutBuffer0,sizeof(float)*inFramesToProcess);
				RingBuffer_Read(sRingBuffer2,pfOutBuffer1,sizeof(float)*inFramesToProcess);
			}
        
		}
    
		if(jBufferSize==(long)inFramesToProcess) {
			float *in1 = (float*) jack_port_get_buffer(inPorts[0],(jack_nframes_t)inFramesToProcess);
			float *in2 = (float*) jack_port_get_buffer(inPorts[1],(jack_nframes_t)inFramesToProcess);
    
			float *out1 = (float*) jack_port_get_buffer(outPorts[0],(jack_nframes_t)inFramesToProcess);
			float *out2 = (float*) jack_port_get_buffer(outPorts[1],(jack_nframes_t)inFramesToProcess);
    
			memcpy(pfOutBuffer0,in1,sizeof(float)*inFramesToProcess);
			memcpy(pfOutBuffer1,in2,sizeof(float)*inFramesToProcess);
			memcpy(out1,pfInBuffer0,sizeof(float)*inFramesToProcess);
			memcpy(out2,pfInBuffer1,sizeof(float)*inFramesToProcess);
		}
	
    }  else {
		memcpy(pfOutBuffer0,pfInBuffer0,sizeof(float)*inFramesToProcess);
		memcpy(pfOutBuffer1,pfInBuffer1,sizeof(float)*inFramesToProcess);
	}
    
    return noErr;
}

void ElCAJAS::openAudioFTh() {
    OSStatus err;
    UInt32 size;
    Boolean isWritable;
    AudioDeviceID jackDevID = 9999;
    
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,&isWritable);
    
    int nDevices = size/sizeof(AudioDeviceID); //here I'm counting how many devices are present
#ifdef DEBUG
    printf("There are %d audio devices\n",nDevices);
#endif
    AudioDeviceID *device = (AudioDeviceID*)calloc(nDevices,sizeof(AudioDeviceID));
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,device);
    
    for(int i=0;i<nDevices;i++) {
#ifdef DEBUG
        printf("ID: %ld\n",device[i]);
#endif
        char name[256];
        size = sizeof(char)*256;
        AudioDeviceGetProperty(device[i],0,true,kAudioDevicePropertyDeviceName,&size,&name);
#ifdef DEBUG
        printf("Name: %s\n",name);
#endif
        if(strcmp(&name[0],"Jack Audio Server")==0) { jackDevID=device[i]; jackIsOn = true; if(device!=NULL) free(device); device=NULL; break; }
    }
    
    if(device!=NULL) free(device);
    
    if(jackDevID==9999) return;
    jackID = jackDevID;
    err = AudioDeviceGetProperty(jackDevID,0,true,kAudioDevicePropertyGetJackClient,&size,&client);
    err = AudioDeviceGetProperty(jackDevID,0,true,kAudioDevicePropertyDeviceIsRunning,&size,&isRunning);

    if(client!=NULL) printf("Client exist.\n");
    else { client = NULL; jackIsOn = false; }
}

void ElCAJAS::flush() {
#ifdef DEBUG
    printf("Running flush\n");
#endif
    if(client!=NULL) {
        
        if(rBufOn) {
#ifdef DEBUG
            printf("Flushing ring buffer 1\n");
#endif
            free(vRBuffer1);
            free(vRBuffer2);
            free(outFromRing);
            for(int i = 0;i<manyInBuffers;i++) {
                free(inFromRing[i]);
            }
            free(inFromRing);
            RingBuffer_Flush(sRingBuffer1);
            RingBuffer_Flush(sRingBuffer2);
            free(sRingBuffer1);
            free(sRingBuffer2);
        }
		
        if(needsDeactivate) { 
#ifdef DEBUG
			printf("Needs Deactivate client\n");
#endif
			jack_deactivate(client); 
		}
    
        for(int i=0;i<nInPorts;i++) {
            jack_port_unregister(client,inPorts[i]);
        }
        free(inPorts);
        for(int i=0;i<nOutPorts;i++) {
            jack_port_unregister(client,outPorts[i]);
        }
        
        UInt32 size;
		AudioDeviceGetProperty(jackID,0,true,kAudioDevicePropertyReleaseJackClient,&size,&client);
		
		
        
        free(outPorts);        
    }
    
}
