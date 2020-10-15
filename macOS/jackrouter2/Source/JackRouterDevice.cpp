/*	Copyright � 2007 Apple Inc. All Rights Reserved.
	
	Disclaimer: IMPORTANT:  This Apple software is supplied to you by 
			Apple Inc. ("Apple") in consideration of your agreement to the
			following terms, and your use, installation, modification or
			redistribution of this Apple software constitutes acceptance of these
			terms.  If you do not agree with these terms, please do not use,
			install, modify or redistribute this Apple software.
			
			In consideration of your agreement to abide by the following terms, and
			subject to these terms, Apple grants you a personal, non-exclusive
			license, under Apple's copyrights in this original Apple software (the
			"Apple Software"), to use, reproduce, modify and redistribute the Apple
			Software, with or without modifications, in source and/or binary forms;
			provided that if you redistribute the Apple Software in its entirety and
			without modifications, you must retain this notice and the following
			text and disclaimers in all such redistributions of the Apple Software. 
			Neither the name, trademarks, service marks or logos of Apple Inc. 
			may be used to endorse or promote products derived from the Apple
			Software without specific prior written permission from Apple.  Except
			as expressly stated in this notice, no other rights or licenses, express
			or implied, are granted by Apple herein, including but not limited to
			any patent rights that may be infringed by your derivative works or by
			other works in which the Apple Software may be incorporated.
			
			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
			MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
			THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
			FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
			OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
			
			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
			OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
			SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
			INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
			MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
			AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
			STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
			POSSIBILITY OF SUCH DAMAGE.
*/
/*=============================================================================
	JackRouterDevice.cpp
=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

//	Self Include
#include "JackRouterDevice.h"
#include "JARLog.h"
#include "bequite.h"

//	Internal Includes
#include "JackRouterControl.h"
#include "JackRouterPlugIn.h"
#include "JackRouterStream.h"

//	HPBase Includes
#include "HP_DeviceSettings.h"
#include "HP_HogMode.h"
#include "HP_IOCycleTelemetry.h"
#include "HP_IOProcList.h"
#include "HP_IOThread.h"

//	PublicUtility Includes
#include "CAAudioBufferList.h"
#include "CAAudioTimeStamp.h"
#include "CAAutoDisposer.h"
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAHostTimeBase.h"
#include "CALogMacros.h"
#include "CAMutex.h"

#include <Accelerate/Accelerate.h>

#define OPTIMIZE_PROCESS 1

using namespace std;

//=============================================================================
//	JackRouterDevice
//=============================================================================

#define kAudioTimeFlags kAudioTimeStampSampleTimeValid | kAudioTimeStampHostTimeValid | kAudioTimeStampRateScalarValid

int JackRouterDevice::fInputChannels = 0;
int JackRouterDevice::fOutputChannels = 0;

bool JackRouterDevice::fAutoConnect = true;

bool JackRouterDevice::fDefaultInput = true;	
bool JackRouterDevice::fDefaultOutput = true;	
bool JackRouterDevice::fDefaultSystem = true;	

int JackRouterDevice::fBufferSize;
float JackRouterDevice::fSampleRate;

UInt64 JackRouterDevice::fSampleCount = 0;

char JackRouterDevice::fCoreAudioDriverUID[128];

// Additional thread for deferred commands execution
CommandThread::CommandThread(JackRouterDevice* inDevice):
	mDevice(inDevice),
	mCommandGuard("CommandGuard"),
	mCommandThread(reinterpret_cast<CAPThread::ThreadRoutine>(ThreadEntry), this, CAPThread::kDefaultThreadPriority)
{}

CommandThread::~CommandThread()
{}

void* CommandThread::ThreadEntry(CommandThread* inIOThread)
{
	inIOThread->WorkLoop();
	return NULL;
}

void CommandThread::WorkLoop()
{
	while (true) {
		mDevice->ExecuteAllCommands(); 
	}
}

void CommandThread::Start()
{
	mCommandThread.Start();
}

JackRouterDevice::JackRouterDevice(AudioDeviceID inAudioDeviceID, JackRouterPlugIn* inPlugIn)
:JackRouterDeviceInterface(inAudioDeviceID, kAudioDeviceClassID, inPlugIn, 1, false),
	mSHPPlugIn(inPlugIn),
	mIOGuard("IOGuard"),
    mAnchorHostTime(0),
    mAnchorSampleTime(0),
	fClient(NULL),
	fInputList(NULL),
	fOutputList(NULL),
	fOutputListTemp(NULL),
	fFirstActivate(true),
   	mLogFile(NULL)
{}

JackRouterDevice::~JackRouterDevice()
{}

void JackRouterDevice::Initialize()
{
	JARLog("JackRouterDevice Initialize\n");
	
	HP_Device::Initialize();
	
	//	allocate the IO thread implementation
	mCommandThread = new CommandThread(this);
	mCommandThread->Start();
	
	// JACK
	fInputList = (AudioBufferList*)malloc(sizeof(UInt32) + sizeof(AudioBuffer) * JackRouterDevice::fInputChannels);
    assert(fInputList);
    fOutputList = (AudioBufferList*)malloc(sizeof(UInt32) + sizeof(AudioBuffer) * JackRouterDevice::fOutputChannels);
    assert(fOutputList);

    fInputList->mNumberBuffers = JackRouterDevice::fInputChannels;
    fOutputList->mNumberBuffers = JackRouterDevice::fOutputChannels;

    fOutputListTemp = (float**)malloc(sizeof(float*) * JackRouterDevice::fOutputChannels);
    assert(fOutputListTemp);

    for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
        fOutputListTemp[i] = (float*)malloc(sizeof(float) * JackRouterDevice::fBufferSize);
		assert(fOutputListTemp[i]);
        memset(fOutputListTemp[i], 0, JackRouterDevice::fBufferSize * sizeof(float));
    }

    fInputPortList = (jack_port_t**)malloc(JackRouterDevice::fInputChannels * sizeof(jack_port_t*));
    assert(fInputPortList);
    fOutputPortList = (jack_port_t**)malloc(JackRouterDevice::fOutputChannels * sizeof(jack_port_t*));
    assert(fOutputPortList);
    
    for (int i = 0; i < fInputChannels; i++) {
        fInputPortList[i] = NULL;
    }
    
    for (int i = 0; i < fOutputChannels; i++) {
        fOutputPortList[i] = NULL;
    }

 	//	create the streams
	CreateStreams();
	
	//  set the default buffer size before we go any further
	mIOBufferFrameSize = fBufferSize;
	JARLog("JackRouterDevice Initialize OK\n");
}

void JackRouterDevice::Teardown()
{
	JARLog("JackRouterDevice Teardown\n");
		
	//	stop things
	Do_StopAllIOProcs();
	ReleaseStreams();	
	
	// JACK
    free(fInputList);
    free(fOutputList);
    
    free(fInputPortList);
    free(fOutputPortList);

    for (int i = 0; i < JackRouterDevice::fOutputChannels; i++)
        free(fOutputListTemp[i]);
    free(fOutputListTemp);
	
	delete mCommandThread;
	HP_Device::Teardown();
}

void JackRouterDevice::Finalize()
{
	//	Finalize() is called in place of Teardown() when we're being lazy about
	//	cleaning up. The idea is to do as little work as possible here.
	
	//	go through the streams and finalize them
	JackRouterStream* theStream;
	UInt32 theStreamIndex;
	UInt32 theNumberStreams;
	
	//	input
	theNumberStreams = GetNumberStreams(true);
	for(theStreamIndex = 0; theStreamIndex != theNumberStreams; ++theStreamIndex)
	{
		theStream = static_cast<JackRouterStream*>(GetStreamByIndex(true, theStreamIndex));
		theStream->Finalize();
	}
	
	//	output
	theNumberStreams = GetNumberStreams(false);
	for(theStreamIndex = 0; theStreamIndex != theNumberStreams; ++theStreamIndex)
	{
		theStream = static_cast<JackRouterStream*>(GetStreamByIndex(false, theStreamIndex));
		theStream->Finalize();
	}
}

CFStringRef	JackRouterDevice::CopyDeviceName() const
{
	CFStringRef theAnswer = CFSTR("JackRouter");
	CFRetain(theAnswer);
	return theAnswer;
}

CFStringRef	JackRouterDevice::CopyDeviceManufacturerName() const
{
	CFStringRef theAnswer = CFSTR("Grame");
	CFRetain(theAnswer);
	return theAnswer;
}

CFStringRef	JackRouterDevice::CopyDeviceUID() const
{
	CFStringRef theAnswer = CFSTR("JackRouter:0");
	CFRetain(theAnswer);
	return theAnswer;
}

bool JackRouterDevice::CanBeDefaultDevice(bool /*inIsInput */, bool inIsSystem) const
{
	return (inIsSystem) ? false : true;
}

bool JackRouterDevice::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	JARPrint4CharCode("JackRouterDevice::HasProperty ", inAddress.mSelector);
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackRouterDevice*>(this)->GetStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyGetJackClient:
        case kAudioDevicePropertyReleaseJackClient:
		case kAudioDevicePropertyAllocateJackPortVST:
        case kAudioDevicePropertyAllocateJackPortAU:
        case kAudioDevicePropertyGetJackPortVST:
        case kAudioDevicePropertyGetJackPortAU:
        case kAudioDevicePropertyReleaseJackPortVST:
        case kAudioDevicePropertyReleaseJackPortAU:
        case kAudioDevicePropertyDeactivateJack:
        case kAudioDevicePropertyActivateJack:
     		JARLog("JackRouterDevice::HasProperty JACK special\n");
			theAnswer = true;
			break;
            
        case kAudioDevicePropertyLatency:
			theAnswer = true;
			break;
		
		default:
			theAnswer = HP_Device::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool JackRouterDevice::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	JARPrint4CharCode("JackRouterDevice::IsPropertySettable ", inAddress.mSelector);
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackRouterDevice*>(this)->GetStateMutex());
	
	switch(inAddress.mSelector)
	{
    
		case kAudioDevicePropertyBufferFrameSize:
		case kAudioDevicePropertyBufferSize:
			theAnswer = true;
			break;
	
    	case kAudioDevicePropertyIOProcStreamUsage:
			theAnswer = true;
			break;
         
		case kAudioDevicePropertyGetJackClient:
        case kAudioDevicePropertyReleaseJackClient:
            theAnswer = false;
            break;

        case kAudioDevicePropertyAllocateJackPortVST:
        case kAudioDevicePropertyAllocateJackPortAU:
        case kAudioDevicePropertyReleaseJackPortVST:
        case kAudioDevicePropertyReleaseJackPortAU:
        case kAudioDevicePropertyDeactivateJack:
        case kAudioDevicePropertyActivateJack:
            theAnswer = true;
            break;
			
		case kAudioDevicePropertyGetJackPortVST:
        case kAudioDevicePropertyGetJackPortAU:
            theAnswer = false;
            break;

		case kAudioDevicePropertyIOCycleUsage:
			theAnswer = false;
			break;
		
		default:
			theAnswer = HP_Device::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32 JackRouterDevice::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32	theAnswer = 0;
	
	JARPrint4CharCode("JackRouterDevice::GetPropertyDataSize ", inAddress.mSelector);
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackRouterDevice*>(this)->GetStateMutex());
	
	switch(inAddress.mSelector)
	{
			
		case kAudioDevicePropertyGetJackClient:
        case kAudioDevicePropertyReleaseJackClient:
			JARLog("JackRouterDevice::GetPropertyDataSize kAudioDevicePropertyGetJackClient\n");
            theAnswer = sizeof(jack_client_t*);
            break;

        case kAudioDevicePropertyAllocateJackPortVST:
        case kAudioDevicePropertyAllocateJackPortAU:
        case kAudioDevicePropertyReleaseJackPortVST:
        case kAudioDevicePropertyReleaseJackPortAU:
        case kAudioDevicePropertyDeactivateJack:
        case kAudioDevicePropertyActivateJack:
            theAnswer = sizeof(UInt32);
            break;
			
		case kAudioDevicePropertyGetJackPortVST:
        case kAudioDevicePropertyGetJackPortAU:
            theAnswer = sizeof(float*);
            break;
		
		default:
			theAnswer = HP_Device::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void JackRouterDevice::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackRouterDevice*>(this)->GetStateMutex());
	
	JARPrint4CharCode("JackRouterDevice::GetPropertyData ", inAddress.mSelector);
	
	switch(inAddress.mSelector)
	{
    
        case kAudioDevicePropertyLatency:
        
            if (inAddress.mScope == kAudioDevicePropertyScopeInput && fClient) {
                const char** ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
                if (ports != NULL && ports[0]) {
                    jack_port_t* port = jack_port_by_name(fClient, ports[0]);
                    jack_latency_range_t range;
                    jack_port_get_latency_range(port, JackCaptureLatency, &range);
                     *static_cast<UInt32*>(outData) = range.min - fBufferSize;
                }
            
            } else if (inAddress.mScope == kAudioDevicePropertyScopeOutput && fClient) {
                const char** ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
                if (ports != NULL && ports[0]) {
                    jack_port_t* port = jack_port_by_name(fClient, ports[0]);
                    jack_latency_range_t range;
                    jack_port_get_latency_range(port, JackPlaybackLatency, &range);
                    *static_cast<UInt32*>(outData) = range.min - fBufferSize;
                }
            }
            break;
		
		case kAudioDevicePropertyBufferFrameSize:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "JackRouterDevice::GetPropertyData: wrong data size for kAudioDevicePropertyLatency");
			*static_cast<UInt32*>(outData) = const_cast<JackRouterDevice*>(this)->GetBufferSize();
			break;
			
		case kAudioDevicePropertyGetJackClient: 
			JARLog("JackRouterDevice::GetPropertyData kAudioDevicePropertyGetJackClient\n");
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "JackRouterDevice::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceUID");
			if (fClient) {
				*static_cast<jack_client_t**>(outData) = fClient;
			} else {
                throw CAException(kAudioHardwareIllegalOperationError);
            }
			break;
			
		case kAudioDevicePropertyReleaseJackClient:
			JARLog("JackRouterDevice::GetPropertyData kAudioDevicePropertyReleaseJackClient\n");
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "JackRouterDevice::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceUID");
			break;
			
		case kAudioDevicePropertyGetJackPortVST: 
			JARLog("JackRouterDevice::GetPropertyData kAudioDevicePropertyGetJackPortVST\n");
			*static_cast<float**>(outData) = const_cast<JackRouterDevice*>(this)->GetPlugInPortVST(ioDataSize);
			break;
		
        case kAudioDevicePropertyGetJackPortAU: 
			JARLog("JackRouterDevice::GetPropertyData kAudioDevicePropertyGetJackPortAU\n");
			*static_cast<float**>(outData) = const_cast<JackRouterDevice*>(this)->GetPlugInPortAU(ioDataSize);
			break;
		
		default:
			HP_Device::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void JackRouterDevice::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	JARPrint4CharCode("JackRouterDevice::SetPropertyData ", inAddress.mSelector);
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(GetStateMutex());
	
	switch(inAddress.mSelector)
	{
		
        case kAudioDevicePropertyAllocateJackPortVST: 
			JARLog("JackRouterDevice::SetPropertyData kAudioDevicePropertyAllocateJackPortVST\n");
			const_cast<JackRouterDevice*>(this)->AllocatePlugInPortVST(inDataSize);
			break;
     
            // Special Property to allocate Jack port from AU plug-in code
        case kAudioDevicePropertyAllocateJackPortAU: 
			JARLog("JackRouterDevice::SetPropertyData kAudioDevicePropertyAllocateJackPortAU\n");
            const_cast<JackRouterDevice*>(this)->AllocatePlugInPortAU(inDataSize);
			break;
     
            // Special Property to release Jack port from VST plug-in code
        case kAudioDevicePropertyReleaseJackPortVST: 
			JARLog("JackRouterDevice::SetPropertyData kAudioDevicePropertyReleaseJackPortVST\n");
            const_cast<JackRouterDevice*>(this)->ReleasePlugInPortVST(inDataSize);
			break;
     
            // Special Property to release Jack port from AU plug-in code
        case kAudioDevicePropertyReleaseJackPortAU: 
			JARLog("JackRouterDevice::SetPropertyData kAudioDevicePropertyReleaseJackPortAU\n");
			const_cast<JackRouterDevice*>(this)->ReleasePlugInPortAU(inDataSize);
			break;
      
            // Special Property to deactivate jack from plug-in code
        case kAudioDevicePropertyDeactivateJack: 
			break;
    
        case kAudioDevicePropertyActivateJack: 
			break;
     		
		case kAudioDevicePropertyIOProcStreamUsage: {
            
			AudioHardwareIOProcStreamUsage* inData1 = (AudioHardwareIOProcStreamUsage*)inData;
			JARLog("DeviceSetProperty inAddress.mScope %ld : mNumberStreams %d\n", inAddress.mScope, inData1->mNumberStreams);

            // Autoconnect is only done for the first activation
			if (fFirstActivate) {
				AutoConnect();
				fFirstActivate = false;
			} else {
				RestoreConnections();
			}
			
			HP_Device::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
            
            if (inAddress.mScope == kAudioDevicePropertyScopeInput && fClient) {
                JARLog("DeviceSetProperty input : mNumberStreams %d\n", inData1->mNumberStreams);
				for (UInt32 i = 0; i < inData1->mNumberStreams; i++) {
                    bool activated = false;
                    
                    // Look for at least one activated stream
                    for (UInt32 j = 0; j < mIOProcList->GetNumberIOProcs(); j++) {
                        if (mIOProcList->GetIOProcByIndex(j)->IsStreamEnabled(true, i)) {
                            activated = true;
                            break;
                        }
                    }
                    
                    // If at least one stream activated...
                    if (activated && fInputPortList[i] == 0) {
                        char in_port_name [JACK_PORT_NAME_LEN];
                        sprintf(in_port_name, "in%lu", i + 1);
                        fInputPortList[i] = jack_port_register(fClient, in_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                        JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_register %ld \n", i);
                    }
                
                    // Otherwise desactivate it
                    if (!activated && fInputPortList[i]) {
                        jack_port_unregister(fClient, fInputPortList[i]);
                        fInputPortList[i] = 0;
                        JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_unregister %ld \n", i);
                    }
                }
            }
            
            if (inAddress.mScope == kAudioDevicePropertyScopeOutput && fClient) {
                JARLog("DeviceSetProperty output : mNumberStreams %d\n", inData1->mNumberStreams);
				for (UInt32 i = 0; i < inData1->mNumberStreams; i++) {
                    bool activated = false;
                    
                    // Look for at least one activated stream
                    for (UInt32 j = 0; j < mIOProcList->GetNumberIOProcs(); j++) {
                        if (mIOProcList->GetIOProcByIndex(j)->IsStreamEnabled(false, i)) {
                            activated = true;
                            break;
                        }
                    }
                    
                    // If at least one stream activated...
                    if (activated && fOutputPortList[i] == 0) {
                        char out_port_name [JACK_PORT_NAME_LEN];
                        sprintf(out_port_name, "out%lu", i + 1);
                        fOutputPortList[i] = jack_port_register(fClient, out_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                        JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_register %ld \n", i);
                    }
                    
                    // Otherwise desactivate it
                    if (!activated && fOutputPortList[i]) {
                        jack_port_unregister(fClient, fOutputPortList[i]);
                        fOutputPortList[i] = 0;
                        JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_unregister %ld \n", i);
                    }
                }
            }
            
			break;
		}
				
		default:
			HP_Device::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

void JackRouterDevice::AddIOProc(AudioDeviceIOProc inProc, void* inClientData)
{
	HP_Device::AddIOProc(inProc, inClientData);
	JARLog("JackRouterDevice::AddIOProc\n");
	
	// First IO proc start JACK
	if (mIOProcList->GetNumberIOProcs() == 1) {
		if (!Open())
            goto error;
		if (!AllocatePorts())
            goto error;
		if (!Activate())
            goto error;
	}
    
    return;
    
error:
    throw CAException(kAudioHardwareIllegalOperationError);
}

void JackRouterDevice::RemoveIOProc(AudioDeviceIOProc inProc)
{
	HP_Device::RemoveIOProc(inProc);
	JARLog("JackRouterDevice::RemoveIOProc\n");
	
	// Last IO proc stop JACK
	if (mIOProcList->GetNumberIOProcs() == 0) {
		Desactivate();
		DisposePorts();
		Close();
	}
}

AudioDeviceIOProcID	JackRouterDevice::Do_CreateIOProcID(AudioDeviceIOProc inProc, void* inClientData)
{
	AudioDeviceIOProcID res = HP_Device::Do_CreateIOProcID(inProc, inClientData);
	JARLog("JackRouterDevice::Do_CreateIOProcID\n");
	
	// First IO proc start JACK
	if (mIOProcList->GetNumberIOProcs() == 1) {
		if (!Open())
            goto error;
		if (!AllocatePorts())
            goto error;
		if (!Activate())
            goto error;
	}
	
	return res;

error:
    throw CAException(kAudioHardwareIllegalOperationError);
    return NULL;
}	

void JackRouterDevice::StopAllIOProcs()
{
	HP_Device::StopAllIOProcs();
	JARLog("JackRouterDevice::StopAllIOProcs\n");
	
	// Last IO proc stop JACK
	if (mIOProcList->GetNumberIOProcs() == 0) {
		Desactivate();
		DisposePorts();
		Close();
	}
}

void JackRouterDevice::PropertyListenerAdded(const AudioObjectPropertyAddress& inAddress)
{
	JARLog("JackRouterDevice::PropertyListenerAdded\n");
	HP_Object::PropertyListenerAdded(inAddress);
}

void JackRouterDevice::Do_StartIOProc(AudioDeviceIOProc inProc)
{
	//	start
	HP_Device::Do_StartIOProc(inProc);
}

void JackRouterDevice::Do_StartIOProcAtTime(AudioDeviceIOProc inProc, AudioTimeStamp& ioStartTime, UInt32 inStartTimeFlags)
{
    //	start
	HP_Device::Do_StartIOProcAtTime(inProc, ioStartTime, inStartTimeFlags);
}

void JackRouterDevice::Do_StopIOProc(AudioDeviceIOProc inProc)
{
	//	stop
	HP_Device::Do_StopIOProc(inProc);
}

void JackRouterDevice::StartIOEngine()
{
	JARLog("JackRouterDevice::StartIOEngine\n");
	if (!IsIOEngineRunning()) {
		StartHardware();
	}
}

void JackRouterDevice::StartIOEngineAtTime(const AudioTimeStamp&  inStartTime, UInt32 inStartTimeFlags)
{
	JARLog("JackRouterDevice::StartIOEngineAtTime\n");
	if (!IsIOEngineRunning()) {
		StartHardware();
	} else {
        // Copied from SampleHardwarePlugIn
        
		//	the engine is already running, so we have to resynch the IO thread to the new start time
		AudioTimeStamp theStartSampleTime = inStartTime;
		theStartSampleTime.mFlags = kAudioTimeStampSampleTimeValid;
		
		//	factor out the input/output-ness of the start time to get the sample time of the anchor point
		if((inStartTimeFlags & kAudioDeviceStartTimeIsInputFlag) != 0)
		{
			theStartSampleTime.mSampleTime += GetIOBufferFrameSize();
			theStartSampleTime.mSampleTime += GetSafetyOffset(true);
		}
		else
		{
			theStartSampleTime.mSampleTime -= GetIOBufferFrameSize();
			theStartSampleTime.mSampleTime -= GetSafetyOffset(false);
		}
		
		//	need an extra cycle to ensure correctness
		theStartSampleTime.mSampleTime -= GetIOBufferFrameSize();
		
		//	calculate the host time of the anchor point
		AudioTimeStamp theStartTime;
		theStartTime.mFlags = kAudioTimeStampSampleTimeValid | kAudioTimeStampHostTimeValid;
		TranslateTime(theStartSampleTime, theStartTime);
    }

}

void JackRouterDevice::StopIOEngine()
{
	JARLog("JackRouterDevice::StopIOEngine\n");
	if (IsIOEngineRunning()) {
		StopHardware();
	}
}

void JackRouterDevice::StartHardware()
{
	//	the calling thread must have already locked the Guard prior to calling this method
	JARLog("JackRouterDevice::StartHardware\n");

	//	set the device state to know the engine is running
	IOEngineStarted();
	
	//	notify clients that the engine is running
	CAPropertyAddress theIsRunningAddress(kAudioDevicePropertyDeviceIsRunning);
	PropertiesChanged(1, &theIsRunningAddress);	
}

void JackRouterDevice::StopHardware()
{
	JARLog("JackRouterDevice::StopHardware\n");
	
	//	set the device state to know the engine has stopped
	IOEngineStopped();
	
	//	Notify clients that the IO callback is stopping
	CAPropertyAddress theIsRunningAddress(kAudioDevicePropertyDeviceIsRunning);
	PropertiesChanged(1, &theIsRunningAddress);
}

bool JackRouterDevice::IsSafeToExecuteCommand()
{
	bool theAnswer = true;

	if (fClient) {
		JARLog("JackRouterDevice::IsSafeToExecuteCommand jack_client_thread_id = %ld pthread_self = %ld\n", jack_client_thread_id(fClient), pthread_self());
		theAnswer = jack_client_thread_id(fClient) != pthread_self();
	}
	
	JARLog("JackRouterDevice::IsSafeToExecuteCommand theAnswer = %ld\n", theAnswer);
	return theAnswer;
}

bool JackRouterDevice::StartCommandExecution(void** outSavedCommandState)
{
	JARLog("JackRouterDevice::StartCommandExecution \n");
	*outSavedCommandState = mIOGuard.Lock() ? (void*)1 : (void*)0;
	return true;
}

void JackRouterDevice::FinishCommandExecution(void* inSavedCommandState)
{
	JARLog("JackRouterDevice::FinishCommandExecution \n");
	if (inSavedCommandState != 0) {
		mIOGuard.Unlock();
	}
}

void JackRouterDevice::StartIOCycleTimingServices()
{
	//	Note that the IOGuard is _not_ held during this call!
	
	//	This method is called when an IO thread is in it's initialization phase
	//	prior to it requiring any timing services. The device's timing services
	//	should be initialized when this method returns.
	
	//	in this sample driver, we base our timing on the CPU clock and assume a perfect sample rate
	mAnchorHostTime = CAHostTimeBase::GetCurrentTime();
    
    mAnchorSampleTime = fSampleCount = float(jack_frame_time(fClient)) - fBufferSize;  // To avoid negative time
    
    //printf("JackRouterDevice::StartIOCycleTimingServices %lld %ld %f\n", mAnchorHostTime, jack_frame_time(fClient), mAnchorSampleTime);
}

void JackRouterDevice::StopIOCycleTimingServices()
{
	//	This method is called when an IO cycle has completed it's run and is tearing down.
	mAnchorHostTime = 0;
    mAnchorSampleTime = 0;
}

Float64	JackRouterDevice::GetCurrentActualSampleRate() const
{
	UInt64 cur_host_time = CAHostTimeBase::GetTheCurrentTime();
    jack_nframes_t cur_sample_time = jack_frame_time(fClient);
    
    Float64 cur_sample_rate = (1000000000 * (Float64(cur_sample_time) - mAnchorSampleTime)) / (Float64(CAHostTimeBase::ConvertToNanos(cur_host_time - mAnchorHostTime)));
    //printf("GetCurrentActualSampleRate %f\n", cur_sample_rate);
	return cur_sample_rate;
}

void JackRouterDevice::GetCallbackCurrentTime(AudioTimeStamp& outTime, Float64 callback_sample_time)
{
    outTime.mSampleTime = callback_sample_time - mAnchorSampleTime;
    outTime.mHostTime = CAHostTimeBase::GetTheCurrentTime();
    outTime.mRateScalar = (outTime.mSampleTime / (Float64(CAHostTimeBase::ConvertToNanos(outTime.mHostTime - mAnchorHostTime)) / 1000000000)) / fSampleRate;
  	outTime.mFlags = kAudioTimeFlags;
}

void JackRouterDevice::GetCurrentTime(AudioTimeStamp& outTime)
{
    /*
    outTime.mSampleTime = float(jack_frame_time(fClient));
    outTime.mHostTime = CAHostTimeBase::GetTheCurrentTime();
    //printf("outTime.mHostTime %lld mAnchorHostTime %lld outTime.mSampleTime %f mAnchorSampleTime %f\n", outTime.mHostTime, mAnchorHostTime, outTime.mSampleTime, mAnchorSampleTime);
	//outTime.mRateScalar = float(outTime.mHostTime - mAnchorHostTime) / float(outTime.mSampleTime - mAnchorSampleTime);
    outTime.mRateScalar = 1.0;
	outTime.mFlags = kAudioTimeFlags;
    */
    
    outTime.mSampleTime = Float64(jack_frame_time(fClient)) - mAnchorSampleTime;
    outTime.mHostTime = CAHostTimeBase::GetTheCurrentTime();
    outTime.mRateScalar = (outTime.mSampleTime / (Float64(CAHostTimeBase::ConvertToNanos(outTime.mHostTime - mAnchorHostTime)) / 1000000000)) / fSampleRate;
 	outTime.mFlags = kAudioTimeFlags;

    /*
 	//	compute the host ticks pere frame
    Float64 theActualHostTicksPerFrame = CAHostTimeBase::GetFrequency() / fSampleRate;
	
	//	clear the output time stamp
	outTime = CAAudioTimeStamp::kZero;
	
	//	put in the current host time
	outTime.mHostTime = CAHostTimeBase::GetTheCurrentTime();
	
	//	calculate how many host ticks away from the anchor time stamp the current host time is
	Float64 theSampleOffset = 0.0;
	if(outTime.mHostTime >= mAnchorHostTime)
	{
		theSampleOffset = outTime.mHostTime - mAnchorHostTime;
	}
	else
	{
		//	do it this way to avoid overflow problems with the unsigned numbers
		theSampleOffset = mAnchorHostTime - outTime.mHostTime;
		theSampleOffset *= -1.0;
	}
	
	//	convert it to a number of samples
	theSampleOffset /= theActualHostTicksPerFrame;
	
	//	lop off the fractional sample
	theSampleOffset = floor(theSampleOffset);
	
	//	put in the sample time
	outTime.mSampleTime = theSampleOffset;
	
	//	put in the rate scalar
	outTime.mRateScalar = 1.0;
	
	//	set the flags
	outTime.mFlags = kAudioTimeFlags;
    */
}

void JackRouterDevice::SafeGetCurrentTime(AudioTimeStamp& outTime)
{
	//	The difference between GetCurrentTime and SafeGetCurrentTime is that GetCurrentTime should only
	//	be called in situations where the device state or clock state is in a known good state, such
	//	as during the IO cycle. Being in a known good state allows GetCurrentTime to bypass any
	//	locks that ensure coherent cross-thread access to the device time base info.
	//	SafeGetCurrentTime, then, will be called when the state is in question and all the locks should
	//	be obeyed.
	
	//	Our state here in the sample device has no such threading issues, so we pass this call on
	//	to GetCurrentTime.
	GetCurrentTime(outTime);
}

void JackRouterDevice::TranslateTime(const AudioTimeStamp& inTime, AudioTimeStamp& outTime)
{
     //	the input time stamp has to have at least one of the sample or host time valid
	ThrowIf((inTime.mFlags & kAudioTimeStampSampleHostTimeValid) == 0, CAException(kAudioHardwareIllegalOperationError), "SHP_Device::TranslateTime: have to have either sample time or host time valid on the input");

	//	compute the host ticks pere frame
    Float64 theActualHostTicksPerFrame = CAHostTimeBase::GetFrequency() / fSampleRate;

	//	calculate the sample time
	Float64 theOffset = 0.0;
	if((outTime.mFlags & kAudioTimeStampSampleTimeValid) != 0)
	{
		if((inTime.mFlags & kAudioTimeStampSampleTimeValid) != 0)
		{
			//	no calculations necessary
			outTime.mSampleTime = inTime.mSampleTime;
		}
		else if((inTime.mFlags & kAudioTimeStampHostTimeValid) != 0)
		{
			//	calculate how many host ticks away from the current 0 time stamp the input host time is
			if(inTime.mHostTime >= mAnchorHostTime)
			{
				theOffset = inTime.mHostTime - mAnchorHostTime;
			}
			else
			{
				//	do it this way to avoid overflow problems with the unsigned numbers
				theOffset = mAnchorHostTime - inTime.mHostTime;
				theOffset *= -1.0;
			}
			
			//	convert it to a number of samples
			theOffset /= theActualHostTicksPerFrame;
			
			//	lop off the fractional sample
			outTime.mSampleTime = floor(theOffset);
		}
		else
		{
			//	no basis for projection, so put in a 0
			outTime.mSampleTime = 0;
		}
	}
	
	//	calculate the host time
	if((outTime.mFlags & kAudioTimeStampHostTimeValid) != 0)
	{
		if((inTime.mFlags & kAudioTimeStampHostTimeValid) != 0)
		{
			//	no calculations necessary
			outTime.mHostTime = inTime.mHostTime;
		}
		else if((inTime.mFlags & kAudioTimeStampSampleTimeValid) != 0)
		{
			//	calculate how many samples away from the current 0 time stamp the input sample time is
			theOffset = inTime.mSampleTime;
			
			//	convert it to a number of host ticks
			theOffset *= theActualHostTicksPerFrame;
			
			//	lop off the fractional host tick
			theOffset = floor(theOffset);
            
            // printf("kAudioTimeStampHostTimeValid theOffset %f\n", theOffset);
			
			//	put in the host time as an offset from the 0 time stamp's host time
			outTime.mHostTime = mAnchorHostTime + static_cast<UInt64>(theOffset);
		}
		else
		{
			//	no basis for projection, so put in a 0
			outTime.mHostTime = 0;
		}
	}
	
	//	calculate the rate scalar
	if(outTime.mFlags & kAudioTimeStampRateScalarValid)
	{
		//	the sample device has perfect timing
		//outTime.mRateScalar = 1.0;
        //outTime.mRateScalar = (outTime.mSampleTime /(float(CAHostTimeBase::ConvertToNanos(outTime.mHostTime - mAnchorHostTime)) / 1000000000)) / fSampleRate;
        outTime.mRateScalar = inTime.mRateScalar;
	}
}

UInt32 JackRouterDevice::GetMinimumIOBufferFrameSize() const
{
	return fBufferSize;
}

UInt32 JackRouterDevice::GetMaximumIOBufferFrameSize() const
{
	return fBufferSize;
}

void JackRouterDevice::GetNearestStartTime(AudioTimeStamp& ioRequestedStartTime, UInt32 inFlags)
{
	JARLog("JackRouterDevice::GetNearestStartTime\n");

    // Copied from SampleHardwarePlugIn
    bool isConsultingHAL = (inFlags & kAudioDeviceStartTimeDontConsultHALFlag) == 0;
	bool isConsultingDevice = (inFlags & kAudioDeviceStartTimeDontConsultDeviceFlag) == 0;

	//ThrowIf(!IsIOEngineRunning(), CAException(kAudioHardwareNotRunningError), "JackRouterDevice::GetNearestStartTime: can't because there isn't anything running yet");
	ThrowIf(!isConsultingHAL && !isConsultingDevice, CAException(kAudioHardwareNotRunningError), "JackRouterDevice::GetNearestStartTime: can't because the start time flags are conflicting");

	UInt32 theIOBufferFrameSize = GetIOBufferFrameSize();
	bool isInput = (inFlags & kAudioDeviceStartTimeIsInputFlag) != 0;
	UInt32 theSafetyOffset = GetSafetyOffset(isInput);
	
	//	fix up the requested time so we have everything we need
	AudioTimeStamp theRequestedStartTime;
	theRequestedStartTime.mFlags = ioRequestedStartTime.mFlags | kAudioTimeStampSampleTimeValid | kAudioTimeStampHostTimeValid;
	TranslateTime(ioRequestedStartTime, theRequestedStartTime);
	
	//	figure out the requested position in terms of the IO thread position
	AudioTimeStamp theTrueRequestedStartTime = theRequestedStartTime;

	//  only do this math if we are supposed to consult the HAL
	if(isConsultingHAL)
	{
		theTrueRequestedStartTime.mFlags = kAudioTimeStampSampleTimeValid;
		if(isInput)
		{
			theTrueRequestedStartTime.mSampleTime += theIOBufferFrameSize;
			theTrueRequestedStartTime.mSampleTime += theSafetyOffset;
		}
		else
		{
			theTrueRequestedStartTime.mSampleTime -= theIOBufferFrameSize;
			theTrueRequestedStartTime.mSampleTime -= theSafetyOffset;
		}
			
		AudioTimeStamp theMinimumStartSampleTime;
		AudioTimeStamp theMinimumStartTime;
		if(mIOProcList->IsOnlyNULLEnabled())
		{
			//	no IOProcs are enabled, so we can start whenever
			
			//	the minimum starting time is the current time
			GetCurrentTime(theMinimumStartSampleTime);
			
			//	plus some slop
			theMinimumStartSampleTime.mSampleTime += theSafetyOffset + (2 * theIOBufferFrameSize);
			theMinimumStartTime.mFlags = kAudioTimeStampSampleTimeValid;
			
			if(theTrueRequestedStartTime.mSampleTime < theMinimumStartSampleTime.mSampleTime)
			{
				//	clamp it to the minimum
				theTrueRequestedStartTime = theMinimumStartSampleTime;
			}
		}
		else if(mIOProcList->IsAnythingEnabled())
		{
			//	an IOProc is already running, so the next start time is two buffers
			//	from wherever the IO thread is currently
            
            GetCurrentTime(theMinimumStartSampleTime);
            theMinimumStartSampleTime.mSampleTime += (2 * theIOBufferFrameSize);
   			
			if(theTrueRequestedStartTime.mSampleTime < theMinimumStartSampleTime.mSampleTime)
			{
				//	clamp it to the minimum
				theTrueRequestedStartTime = theMinimumStartSampleTime;
			}
			else if(theTrueRequestedStartTime.mSampleTime > theMinimumStartSampleTime.mSampleTime)
			{
				//	clamp it to an even IO cycle
				UInt32 theNumberBuffers = static_cast<UInt32>(theTrueRequestedStartTime.mSampleTime - theMinimumStartSampleTime.mSampleTime);
				theNumberBuffers /= theIOBufferFrameSize;
				theNumberBuffers += 2;
				
				theTrueRequestedStartTime.mSampleTime = theMinimumStartSampleTime.mSampleTime + (theNumberBuffers * theIOBufferFrameSize);
			}
		}
		
		//	bump the sample time in the right direction
		if(isInput)
		{
			theTrueRequestedStartTime.mSampleTime -= theIOBufferFrameSize;
			theTrueRequestedStartTime.mSampleTime -= theSafetyOffset;
		}
		else
		{
			theTrueRequestedStartTime.mSampleTime += theIOBufferFrameSize;
			theTrueRequestedStartTime.mSampleTime += theSafetyOffset;
		}
	}
		
	//	convert it back if neccessary
	if(theTrueRequestedStartTime.mSampleTime != theRequestedStartTime.mSampleTime)
	{
		TranslateTime(theTrueRequestedStartTime, theRequestedStartTime);
	}
	
	//	now filter it through the hardware, unless told not to
	if(mIOProcList->IsOnlyNULLEnabled() && isConsultingDevice)
	{
	}
	
	//	assign the return value
	ioRequestedStartTime = theRequestedStartTime;
}

void JackRouterDevice::CreateStreams()
{
	//  common variables
	OSStatus		theError = 0;
	AudioObjectID   theNewStreamID = 0;
	JackRouterStream*		theStream = NULL;

	//  create a vector of AudioStreamIDs to hold the stream ids we are creating
	std::vector<AudioStreamID> theStreamIDs;
	
	for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
	
		//  instantiate an AudioStream
	#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		theError = AudioHardwareClaimAudioStreamID(mSHPPlugIn->GetInterface(), GetObjectID(), &theNewStreamID);
	#else
		theError = AudioObjectCreate(mSHPPlugIn->GetInterface(), GetObjectID(), kAudioStreamClassID, &theNewStreamID);
	#endif
		if(theError == 0)
		{
			//  create the stream
			theStream = new JackRouterStream(theNewStreamID, mSHPPlugIn, this, true, i+1, fSampleRate);
			theStream->Initialize();
			
			//	add to the list of streams in this device
			AddStream(theStream);
			
			//  store the new stream ID
			theStreamIDs.push_back(theNewStreamID);
		}
	}
	
	for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {

		//  claim a stream ID for the stream
	#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		theError = AudioHardwareClaimAudioStreamID(mSHPPlugIn->GetInterface(), GetObjectID(), &theNewStreamID);
	#else
		theError = AudioObjectCreate(mSHPPlugIn->GetInterface(), GetObjectID(), kAudioStreamClassID, &theNewStreamID);
	#endif
		if(theError == 0)
		{
			//  create the stream
			theStream = new JackRouterStream(theNewStreamID, mSHPPlugIn, this, false, i+1, fSampleRate);
			theStream->Initialize();
			
			//	add to the list of streams in this device
			AddStream(theStream);
			
			//  store the new stream ID
			theStreamIDs.push_back(theNewStreamID);
		}
	}

	//  now tell the HAL about the new stream IDs
	if(theStreamIDs.size() != 0)
	{
		//	set the object state mutexes
		for(std::vector<AudioStreamID>::iterator theIterator = theStreamIDs.begin(); theIterator != theStreamIDs.end(); std::advance(theIterator, 1))
		{
			HP_Object* theObject = HP_Object::GetObjectByID(*theIterator);
			if(theObject != NULL)
			{
				HP_Object::SetObjectStateMutexForID(*theIterator, theObject->GetObjectStateMutex());
			}
		}
		
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		theError = AudioHardwareStreamsCreated(mSHPPlugIn->GetInterface(), GetObjectID(), theStreamIDs.size(), &(theStreamIDs.front()));
#else
		theError = AudioObjectsPublishedAndDied(mSHPPlugIn->GetInterface(), GetObjectID(), theStreamIDs.size(), &(theStreamIDs.front()), 0, NULL);
#endif
		ThrowIfError(theError, CAException(theError), "JackRouterDevice::CreateStreams: couldn't tell the HAL about the streams");
	}
}

/*
void JackRouterDevice::CreateForHAL(AudioDeviceID theNewDeviceID)
{
	JARLog("CreateForHAL\n");
	SetObjectID(theNewDeviceID);  // setup the new deviceID
	CreateStreams();
}
 */

void JackRouterDevice::ReleaseStreams()
{
	//	This method is only called when tearing down, so there isn't any need to inform the HAL about changes
	//	since the HAL has long since released it's internal representation of these stream objects. Note that
	//	if this method needs to be called outside of teardown, it would need to be modified to call
	//	AudioObjectsPublishedAndDied (or AudioHardwareStreamsDied on pre-Tiger systems) to notify the HAL about
	//	the state change.
	while (GetNumberStreams(true) > 0)
	{
		//	get the stream
		JackRouterStream* theStream = static_cast<JackRouterStream*>(GetStreamByIndex(true, 0));
		
		//	remove the object state mutex
		HP_Object::SetObjectStateMutexForID(theStream->GetObjectID(), NULL);

		//	remove it from the lists
		RemoveStream(theStream);
		
		//	toss it
		theStream->Teardown();
		delete theStream;
	}
	
	while (GetNumberStreams(false) > 0)
	{
		//	get the stream
		JackRouterStream* theStream = static_cast<JackRouterStream*>(GetStreamByIndex(false, 0));
		
		//	remove the object state mutex
		HP_Object::SetObjectStateMutexForID(theStream->GetObjectID(), NULL);

		//	remove it from the lists
		RemoveStream(theStream);
		
		//	toss it
		theStream->Teardown();
		delete theStream;
	}
}

void JackRouterDevice::ReleaseFromHAL()
{
    JARLog("JackRouterDevice::ReleaseFromHAL\n");
    AudioObjectID theObjectID = GetObjectID();
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
    OSStatus theError = AudioHardwareDevicesDied(mSHPPlugIn->GetInterface(), 1, &theObjectID);
#else
    OSStatus theError = AudioObjectsPublishedAndDied(mSHPPlugIn->GetInterface(), kAudioObjectSystemObject, 0, NULL, 1, &theObjectID);
#endif
    AssertNoError(theError, "JackRouterPlugIn::Teardown: got an error telling the HAL a device died");
    Destroy();
}

// JACK

void JackRouterDevice::SaveConnections()
{
	if (!fClient)
        return;

    JARLog("--------------------------------------------------------\n");
    JARLog("SaveConnections\n");

	const char** connections;
    fConnections.clear();

    for (int i = 0; i < fInputChannels; ++i) {
        if (fInputPortList[i] && (connections = jack_port_get_connections(fInputPortList[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(connections[j], jack_port_name(fInputPortList[i])));
            }
            free(connections);
        }
    }

    for (int i = 0; i < fOutputChannels; ++i) {
        if (fOutputPortList[i] && (connections = jack_port_get_connections(fOutputPortList[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(jack_port_name(fOutputPortList[i]), connections[j]));
            }
            free(connections);
        }
    }

    if (JAR_fDebug) {
	
        list<pair<string, string> >::const_iterator it;
		for (it = fConnections.begin(); it != fConnections.end(); it++) {
            pair<string, string> connection = *it;
            JARLog("connections : %s %s\n", connection.first.c_str(), connection.second.c_str());
        }
    }
}

void JackRouterDevice::RestoreConnections()
{
    JARLog("--------------------------------------------------------\n");
    JARLog("RestoreConnections size = %ld\n", fConnections.size());

    list<pair<string, string> >::const_iterator it;

    for (it = fConnections.begin(); it != fConnections.end(); it++) {
        pair<string, string> connection = *it;
        JARLog("connections : %s %s\n", connection.first.c_str(), connection.second.c_str());
        jack_connect(fClient, connection.first.c_str(), connection.second.c_str());
    }
}

bool JackRouterDevice::AutoConnect()
{
    const char** ports;

    if (fAutoConnect) {

        if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsOutput)) == NULL) {
            JARLog("cannot find any physical capture ports\n");
        } else {

            for (int i = 0; i < fInputChannels; i++) {
                if (JAR_fDebug) {
                    if (ports[i])
                        JARLog("ports[i] %s\n", ports[i]);
                    if (fInputPortList[i] && jack_port_name(fInputPortList[i]))
                        JARLog("jack_port_name(fInputPortList[i]) = %s\n", jack_port_name(fInputPortList[i]));
                }

                // Stop condition
                if (ports[i] == 0)
                    break;

                if (fInputPortList[i] && jack_port_name(fInputPortList[i])) {
                    if (jack_connect(fClient, ports[i], jack_port_name(fInputPortList[i]))) {
                        JARLog("cannot connect input ports\n");
                    }
                } 
            }
            free(ports);
        }

        if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsInput)) == NULL) {
            JARLog("cannot find any physical playback ports\n");
        } else {

            for (int i = 0; i < fOutputChannels; i++) {
                if (JAR_fDebug) {
                    if (ports[i])
                        JARLog("ports[i] %s\n", ports[i]);
                    if (fOutputPortList[i] && jack_port_name(fOutputPortList[i]))
                        JARLog("jack_port_name(fOutputPortList[i]) %s\n", jack_port_name(fOutputPortList[i]));
                }

                // Stop condition
                if (ports[i] == 0)
                    break;

                if (fOutputPortList[i] && jack_port_name(fOutputPortList[i])) {
                    if (jack_connect(fClient, jack_port_name(fOutputPortList[i]), ports[i])) {
                        JARLog("cannot connect ouput ports\n");
                    }
                }
            }
            free(ports);
        }
    }

    return true;
}

bool JackRouterDevice::AllocatePorts()
{
    char in_port_name[JACK_PORT_NAME_LEN];

    JARLog("AllocatePorts fInputChannels = %ld fOutputChannels = %ld \n", fInputChannels, fOutputChannels);

    for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
        sprintf(in_port_name, "in%d", i + 1);
        if ((fInputPortList[i] = jack_port_register(fClient, in_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL)
            goto error;
        fInputList->mBuffers[i].mNumberChannels = 1;
        fInputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
    }

    char out_port_name[JACK_PORT_NAME_LEN];

    for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
        sprintf(out_port_name, "out%d", i + 1);
        if ((fOutputPortList[i] = jack_port_register(fClient, out_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL)
            goto error;
        fOutputList->mBuffers[i].mNumberChannels = 1;
        fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
    }

    return true;

error:

    JARLog("Cannot register ports\n");
    DisposePorts();
    return false;
}

void JackRouterDevice::DisposePorts()
{
    JARLog("DisposePorts\n");

    for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
        if (fInputPortList[i]) {
			JARLog("DisposePorts input %ld\n",i);
            jack_port_unregister(fClient, fInputPortList[i]);
            fInputPortList[i] = 0;
        }
    }

    for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
        if (fOutputPortList[i]) {
			JARLog("DisposePorts output %ld\n",i);
            jack_port_unregister(fClient, fOutputPortList[i]);
            fOutputPortList[i] = 0;
        }
    }
}

static void PrintTime(const char* name, const AudioTimeStamp& time)
{
     printf("%s Time mSampleTime = %f mHostTime = %lld mRateScalar = %f mWordClockTime = %lld SMPTE %d mFlags = %ld\n", name, 
        time.mSampleTime,  time.mHostTime,  time.mRateScalar, time.mWordClockTime, time.mSMPTETime.mSeconds, time.mFlags);
}

static void SetTime(AudioTimeStamp* timeVal, long curTime, UInt64 time)
{
    timeVal->mSampleTime = curTime;
    timeVal->mHostTime = time;
    timeVal->mRateScalar = 1.0;
    timeVal->mFlags = kAudioTimeFlags;
}

int JackRouterDevice::Process(jack_nframes_t nframes, void* arg)
{
    AudioTimeStamp inNow = CAAudioTimeStamp::kZero;
    AudioTimeStamp inInputTime = CAAudioTimeStamp::kZero;
    AudioTimeStamp inOutputTime = CAAudioTimeStamp::kZero;
	bool wasLocked;
	OSStatus err;
	JackRouterDevice* client = (JackRouterDevice*)arg;
	
	// Lock is on, so drop this cycle.
	if (!client->mIOGuard.Try(wasLocked)) {
		JARLog("JackRouterDevice::Process LOCK ON\n"); 
		return 0;
	}
    
   	//JARLog("Process \n");
    
    fSampleCount += JackRouterDevice::fBufferSize;
    // Move sample time buffer by buffer (otherwise QuickTime player fails...)
    client->GetCallbackCurrentTime(inNow, Float64(fSampleCount));
    //client->GetCurrentTime(inNow);
    
    AudioTimeStamp theInputFrameTime;
    theInputFrameTime = inNow;
    theInputFrameTime.mFlags = kAudioTimeStampSampleTimeValid;
    theInputFrameTime.mSampleTime -= JackRouterDevice::fBufferSize;
    
    //	use that to figure the corresponding host time
    inInputTime.mFlags = kAudioTimeFlags;
    client->TranslateTime(theInputFrameTime, inInputTime);
    
    //	calculate the head position in frames
    AudioTimeStamp theOutputFrameTime;
    theOutputFrameTime = inNow;
    theOutputFrameTime.mFlags = kAudioTimeStampSampleTimeValid;
    theOutputFrameTime.mSampleTime += JackRouterDevice::fBufferSize;
    
    //	use that to figure the corresponding host time
    inOutputTime.mFlags = kAudioTimeFlags;
    client->TranslateTime(theOutputFrameTime, inOutputTime);
   
     
    if (getenv("JACK_ROUTER_DEBUG") && strcmp(getenv("JACK_ROUTER_DEBUG"), "on") == 0) {
        printf("-------------\n");
        PrintTime("now", inNow);
        PrintTime("in", inInputTime);
        PrintTime("out", inOutputTime);
    }

    
    /*
    UInt64 time = CAHostTimeBase::GetTheCurrentTime();
    //fSampleCount += JackRouterDevice::fBufferSize;
  	SetTime(&inNow, fSampleCount, time);
	SetTime(&inInputTime, fSampleCount - JackRouterDevice::fBufferSize, time);
    SetTime(&inOutputTime, fSampleCount + JackRouterDevice::fBufferSize, time);
    */
    
    /*
    printf("------------- 2\n");
    PrintTime("now", inNow);
    PrintTime("in", inInputTime);
    PrintTime("out", inOutputTime);
    */
      
   	// One IOProc
  	if (client->mIOProcList->GetNumberIOProcs() == 1) {
		//JARLog("GetNumberIOProcs == 1 \n");

    	HP_IOProc* proc = client->mIOProcList->GetIOProcByIndex(0);
    
    	if (proc->IsEnabled()) {
		
			if (proc->GetStreamUsage(true).size() > 0 || proc->GetStreamUsage(false).size() > 0) {  // A VERIFIER
			
				//JARLog("Process GetStreamUsage\n");
			
				// Only set up buffers that are really needed
                for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
                    if (proc->IsStreamEnabled(true, i) && client->fInputPortList[i]) {
                        client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
						client->fInputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
                    } else {
                        client->fInputList->mBuffers[i].mData = NULL;
                    }
                }
      
			#ifdef OPTIMIZE_PROCESS
                for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
                    if (proc->IsStreamEnabled(false, i) && client->fOutputPortList[i]) {
                        //JARLog("Process GetStreamUsage %i\n", i);
                        float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
						memset(output, 0, nframes * sizeof(float));
						client->fOutputList->mBuffers[i].mData = output;
						client->fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
                    } else {
                        client->fOutputList->mBuffers[i].mData = NULL;
                    }
                }
      			
			#else
         		for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
                    if (proc->IsStreamEnabled(false, i)) {
                        memset(client->fOutputListTemp[i], 0, nframes * sizeof(float));
						client->fOutputList->mBuffers[i].mData = client->fOutputListTemp[i];
						client->fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
                    } else {
                        client->fOutputList->mBuffers[i].mData = NULL;
                    }
                }
			#endif

            } else {
			
				//JARLog("Process NO GetStreamUsage\n");
			
				// Non Interleaved
                for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
					if (client->fInputPortList[i]) {
                        client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
						client->fInputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
					} else {
						//JARLog("JackRouterDevice::Process client->fInputPortList[i] %d is null\n",i);
						client->fInputList->mBuffers[i].mData = NULL;
						client->fInputList->mBuffers[i].mDataByteSize = 0;  // MUST be set when NO GetStreamUsage (iMovie HD crash...)
					}
                }
	            
			#ifdef OPTIMIZE_PROCESS
                for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
					if (client->fOutputPortList[i]) {
         				float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
						memset(output, 0, nframes * sizeof(float));
						client->fOutputList->mBuffers[i].mData = output;
						client->fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
					} else {
						client->fOutputList->mBuffers[i].mData = NULL;
						client->fOutputList->mBuffers[i].mDataByteSize = 0; // MUST be set when NO GetStreamUsage (iMovie HD crash...)
					}
                }
			#else
				for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
					if (client->fOutputPortList[i]) {
    					memset(client->fOutputListTemp[i], 0, nframes * sizeof(float));
						client->fOutputList->mBuffers[i].mData = client->fOutputListTemp[i];
						client->fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
					} else {
						client->fOutputList->mBuffers[i].mData = NULL;
						client->fOutputList->mBuffers[i].mDataByteSize = 0; // MUST be set when NO GetStreamUsage (iMovie HD crash...)
					}
                }
			#endif
            }
			
			// Unlock before calling IO proc
			client->mIOGuard.Unlock();
			
            err = (proc->GetIOProc()) (client->mObjectID,
										&inNow,
										client->fInputList,
										&inInputTime,
										client->fOutputList,
										&inOutputTime,
										proc->GetClientData());

            if (JAR_fDebug && err != kAudioHardwareNoError) {
				JARLog("Process error = %ld\n", err);
            }

		#ifndef OPTIMIZE_PROCESS
		
            // Copy intermediate buffer in client buffer
            for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
				if (client->fOutputPortList[i]) {
					float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
					memcpy(output, client->fOutputListTemp[i], nframes * sizeof(float));
				}
            }
			
		#endif
			
        } else {
		
			for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
				if (client->fOutputPortList[i]) {
					float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
					memset(output, 0, nframes * sizeof(float));
				}
			}
			
			// Final Unlock
			client->mIOGuard.Unlock();
		}

    } else if (client->mIOProcList->GetNumberIOProcs() > 1) { // Several IOProc : need mixing

		//JARLog("GetNumberIOProcs > 1 size = %d\n", client->mIOProcList->GetNumberIOProcs());
        
        // If no active IO proc, then clear output buffers
        if (client->mIOProcList->GetNumberEnabledIOProcs() == 0) {
            //JARLog("GetNumberEnabledIOProcs = 0\n"); 
            
            for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
                if (client->fOutputPortList[i]) {
                    float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
                    memset(output, 0, nframes * sizeof(float));
                }
            }
            
        } else {  // At least one active IO proc
    		
            bool firstproc[JackRouterDevice::fOutputChannels];
            for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) 
                firstproc[i] = true;
                
            for (UInt32 k = 0; k < client->mIOProcList->GetNumberIOProcs(); k++) {

                HP_IOProc* proc = client->mIOProcList->GetIOProcByIndex(k);
            
                if (proc && proc->IsEnabled()) { // If proc is started

                    if (proc->GetStreamUsage(true).size() > 0 || proc->GetStreamUsage(false).size() > 0) {
                    
                        //JARLog("Process GetStreamUsage input YES k = %d proc = %ld\n", k, proc->GetIOProc());

                        // Only set up buffers that are really needed
                        for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
                            if (proc->IsStreamEnabled(true, i) && client->fInputPortList[i]) {
                                client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
                                client->fInputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
                            } else {
                                client->fInputList->mBuffers[i].mData = NULL;
                            }
                        }

                        for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
                            // Use an intermediate mixing buffer
                            if (proc->IsStreamEnabled(false, i)) {
                                memset(client->fOutputListTemp[i], 0, nframes * sizeof(float));
                                client->fOutputList->mBuffers[i].mData = client->fOutputListTemp[i];
                                client->fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
                            } else {
                                client->fOutputList->mBuffers[i].mData = NULL;
                            }
                        }

                    } else {
                    
                        //JARLog("Process GetStreamUsage input NO k = %d proc = %ld\n", k, proc->GetIOProc());
                    
                        for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
                            if (client->fInputPortList[i]) {
                                client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
                                client->fInputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
                            } else {
                                //JARLog("JackRouterDevice::Process client->fInputPortList[i] %d is null\n",i);
                                client->fInputList->mBuffers[i].mData = NULL;
                                client->fInputList->mBuffers[i].mDataByteSize = 0;	// MUST be set when NO GetStreamUsage (iMovie HD crash...)
                            }
                        }

                        for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
                            // Use an intermediate mixing buffer
                            if (client->fOutputPortList[i]) {
                                memset(client->fOutputListTemp[i], 0, nframes * sizeof(float));
                                client->fOutputList->mBuffers[i].mData = client->fOutputListTemp[i];
                                client->fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
                            } else {
                                client->fOutputList->mBuffers[i].mData = NULL;
                                client->fOutputList->mBuffers[i].mDataByteSize = 0;	 // MUST be set when NO GetStreamUsage (iMovie HD crash...)
                            }
                        }
                    }

                    // Unlock before calling IO proc
                    client->mIOGuard.Unlock();
                    
                    //JARLog("Process GetIOProc() = %ld\n", proc->GetIOProc());

                    err = (proc->GetIOProc()) (client->mObjectID,
                                                &inNow,
                                                client->fInputList,
                                                &inInputTime,
                                                client->fOutputList,
                                                &inOutputTime,
                                                proc->GetClientData());
                    
                    
                    if (JAR_fDebug && err != kAudioHardwareNoError) {
                        JARLog("Process error = %ld\n", err);
                    }
                    
                    // Lock is on, so drop this cycle.
                    if (!client->mIOGuard.Try(wasLocked)) {
                        JARLog("JackRouterDevice::Process LOCK ON\n"); 
                        return 0;
                    }
                    
                    // Only mix buffers that are really needed
                    if (proc->GetStreamUsage(true).size() > 0 || proc->GetStreamUsage(false).size() > 0) {
                    
                        //JARLog("Process GetStreamUsage output YES k = %d proc = %ld\n", k, proc->GetIOProc());
                                    
                        for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
                        
                            //JARLog("Process GetStreamUsage YES i = %ld\n", i);
                            
                            if (proc->IsStreamEnabled(false, i) && client->fOutputPortList[i]) {
                                float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
                                if (firstproc[i]) {	// first proc : copy
                                    //JARLog("Process GetStreamUsage YES first proc : copy = %ld\n", proc->GetIOProc());
                                    memcpy(output, (float*)client->fOutputList->mBuffers[i].mData, nframes * sizeof(float));
                                    firstproc[i] = false;
                                } else {			// other proc : mix
                                    //JARLog("Process GetStreamUsage YES other proc : mix = %ld\n", proc->GetIOProc());
                                    float gain = 1.0f;
                                    vDSP_vsma((float*)client->fOutputList->mBuffers[i].mData, 1, &gain, output, 1, output, 1, nframes);
                                     
                                    /*
                                    for (UInt32 j = 0; j < nframes; j++) {
                                        output[j] += ((float*)client->fOutputList->mBuffers[i].mData)[j];
                                    }
                                    */
                                }
                            } else {
                                if (client->fOutputPortList[i] && firstproc[i]) {
                                    float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
                                    memset(output, 0, nframes * sizeof(float));
                                    firstproc[i] = false;
                                }
                            }
                        }

                    } else {
                    
                        //JARLog("Process GetStreamUsage output NO k = %d proc = %ld\n", k, proc->GetIOProc());
                        
                        for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
                            if (client->fOutputPortList[i]) {
                                float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
                                if (firstproc[i]) {	// first proc : copy
                                    //JARLog("Process GetStreamUsage NO first proc : copy = %ld\n", proc->GetIOProc());
                                    memcpy(output, (float*)client->fOutputList->mBuffers[i].mData, nframes * sizeof(float));
                                    firstproc[i] = false;
                                } else {			// other proc : mix
                                    //JARLog("Process GetStreamUsage NO other proc : mix = %ld\n", proc->GetIOProc());
                                    float gain = 1.0f;
                                    vDSP_vsma((float*)client->fOutputList->mBuffers[i].mData, 1, &gain, output, 1, output, 1, nframes);
                                    /*
                                    for (UInt32 j = 0; j < nframes; j++) {
                                        output[j] += ((float*)client->fOutputList->mBuffers[i].mData)[j];
                                    }
                                    */
                                }
                            } else {
                                //JARLog("JackRouterDevice::Process client->fOutputPortList[i] %d is null\n",i); 
                            }
                            
                        }
                    }
                }
            }
        }
		 
		// Final Unlock
		client->mIOGuard.Unlock();
    } else {
	
		// No IOProc so clear buffers
		
		//JARLog("No IOProc \n");
		for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
			if (client->fOutputPortList[i]) {
				float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
				memset(output, 0, nframes * sizeof(float));
			}
		}
		
		// Final Unlock
		client->mIOGuard.Unlock();
	}

    map<int, pair<float*, jack_port_t*> >::const_iterator it;

    // Copy temp buffers from VST plug-ins into the Jack buffers
    for (it = client->fPlugInPortsVST.begin(); it != client->fPlugInPortsVST.end(); it++) {
        pair<float*, jack_port_t*> obj = it->second;
        memcpy((float*)jack_port_get_buffer(obj.second, nframes), obj.first, nframes * sizeof(float));
        memset(obj.first, 0, nframes * sizeof(float));
    }
	
    // Copy temp buffers from AU plug-ins into the Jack buffers
    for (it = client->fPlugInPortsAU.begin(); it != client->fPlugInPortsAU.end(); it++) {
        pair<float*, jack_port_t*> obj = it->second;
        memcpy((float*)jack_port_get_buffer(obj.second, nframes), obj.first, nframes * sizeof(float));
        memset(obj.first, 0, nframes * sizeof(float));
    }

	return 0;
}

bool JackRouterDevice::Open()
{
	pid_t pid = getpid();
	jack_status_t status;
    char* id_name = bequite_getNameFromPid(pid);
    assert(id_name != NULL);
    JARLog("JackRouterDevice::Open id %ld name %s\n", pid, id_name);
 	
	// From previous state
	if (fClient) {
		JARLog("Close old client\n");
		jack_client_close(fClient);
        fClient = NULL;
		mIOProcList->RemoveAllIOProcs();
	}

    if ((fClient = jack_client_open(id_name, JackNoStartServer, &status)) == NULL) {
        JARLog("JackRouterDevice::Open jack server not running?\n");
        return false;
    } else {
        jack_set_thread_init_callback(fClient, Init, this);
		jack_set_process_callback(fClient, Process, this);
		jack_on_shutdown(fClient, Shutdown, this);
		jack_set_buffer_size_callback(fClient, BufferSize, this);
		jack_set_xrun_callback(fClient, XRun, this);
		
        /*
		char logfile[128];
		snprintf(logfile, sizeof(logfile) - 1, "%s_%s", "/tmp/JackRouter_latency", id_name);
		mLogFile = new CALatencyLog(logfile, ".txt");
        */
		return true;
	}
}

void JackRouterDevice::Close()
{
    JARLog("JackRouterDevice::Close\n");

    if (fClient) {
        if (jack_client_close(fClient) != 0) {
            JARLog("JackRouterDevice::Close cannot close client\n");
        }
		fClient = NULL;
		delete mLogFile;
    }
}

void JackRouterDevice::Destroy()
{
    JARLog("JackRouterDevice::Destroy\n");
    
    CAPropertyAddress theIsAliveAddress(kAudioDevicePropertyDeviceIsAlive);
	PropertiesChanged(1, &theIsAliveAddress);

    if (fClient) {
        if (jack_client_close(fClient) != 0) {
            JARLog("JackRouterDevice::Close cannot close client\n");
        }
		fClient = NULL;
    }
	
	mIOProcList->RemoveAllIOProcs();
	fFirstActivate = true;
		
	StopHardware();
}

 void JackRouterDevice::Init(void* arg)
 {
    JackRouterDevice* client = (JackRouterDevice*)arg;
    client->StartIOCycleTimingServices();
 }

int JackRouterDevice::BufferSize(jack_nframes_t nframes, void* arg)
{
    JackRouterDevice* client = (JackRouterDevice*)arg;
 	
	if (nframes != JackRouterDevice::fBufferSize) {
    
        JARLog("New BufferSize %ld\n", nframes);
		JackRouterDevice::fBufferSize = nframes;
		client->mIOBufferFrameSize = nframes;

		for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
			client->fInputList->mBuffers[i].mNumberChannels = 1;
			client->fInputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
		}

		for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
			client->fOutputList->mBuffers[i].mNumberChannels = 1;
			client->fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
			if (client->fOutputListTemp[i])
				free(client->fOutputListTemp[i]);
			client->fOutputListTemp[i] = (float*)malloc(sizeof(float) * JackRouterDevice::fBufferSize);
		}
		
		CAPropertyAddress thePropertyBufferFrameSize(kAudioDevicePropertyBufferFrameSize);
		client->PropertiesChanged(1, &thePropertyBufferFrameSize);
		CAPropertyAddress thePropertyBufferFrameSizeAddress(kAudioDevicePropertyBufferFrameSize);
		client->PropertiesChanged(1, &thePropertyBufferFrameSizeAddress);
	}
	
    return 0;
}

int JackRouterDevice::GetBufferSize()
{
	if (fClient) {
		int nframes = jack_get_buffer_size(fClient);
		JARLog("GetBufferSize %ld \n", nframes);
	
		if (nframes != JackRouterDevice::fBufferSize) {
		
			JackRouterDevice::fBufferSize = nframes;
			mIOBufferFrameSize = nframes;

			for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
				fInputList->mBuffers[i].mNumberChannels = 1;
				fInputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
			}

			for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
				fOutputList->mBuffers[i].mNumberChannels = 1;
				fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
				if (fOutputListTemp[i])
					free(fOutputListTemp[i]);
				fOutputListTemp[i] = (float*)malloc(sizeof(float) * JackRouterDevice::fBufferSize);
			}
		}
	}
		
	return JackRouterDevice::fBufferSize;
}

void JackRouterDevice::Shutdown(void* /*arg */)
{}

int JackRouterDevice::XRun(void* arg)
{
    JARLog("XRun\n");
    printf("XRun\n");
	JackRouterDevice* client = (JackRouterDevice*)arg;
    client->StartIOCycleTimingServices();
	//client->mLogFile->Capture(AudioGetCurrentHostTime() - AudioConvertNanosToHostTime(LOG_SAMPLE_DURATION * 1000000), AudioGetCurrentHostTime(), true, "Captured Latency Log for I/O Cycle Overload\n");
	CAPropertyAddress theProcessorOverloadAddress(kAudioDeviceProcessorOverload);
	client->PropertiesChanged(1, &theProcessorOverloadAddress);
    return 0;
}

bool JackRouterDevice::Activate()
{
    JARLog("Activate\n");

    if (jack_activate(fClient)) {
        JARLog("cannot activate client");
        return false;
    } else {
        // Autoconnect is only done for the first activation
        if (JackRouterDevice::fFirstActivate) {
            JARLog("First activate\n");
            AutoConnect();
            JackRouterDevice::fFirstActivate = false;
        } else {
            RestoreConnections();
        }
        //StartIOCycleTimingServices();
        return true;
    }
}

bool JackRouterDevice::Desactivate()
{
    JARLog("Desactivate\n");
	SaveConnections();

    if (fClient != NULL && jack_deactivate(fClient)) {
        JARLog("cannot deactivate client\n");
        return false;
    }
    StopIOCycleTimingServices();
    return true;
}

bool JackRouterDevice::AllocatePlugInPortVST(int num)
{
    JARLog("AllocatePlugInPortVST %ld\n", num + 1);
    char name[256];
    sprintf(name, "VSTsend%d", num + 1);
    jack_port_t* port = jack_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;

    float* buffer = (float*)malloc(sizeof(float) * fBufferSize);
    if (!buffer)
        return false;

    memset(buffer, 0, fBufferSize * sizeof(float));
    fPlugInPortsVST[num] = make_pair(buffer, port);
    return true;
}

bool JackRouterDevice::AllocatePlugInPortAU(int num)
{
    JARLog("AllocatePlugInPortAU %ld\n", num + 1);
    char name[256];
    sprintf(name, "AUsend%d", num + 1);
    jack_port_t* port = jack_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;

    float* buffer = (float*)malloc(sizeof(float) * fBufferSize);
    if (!buffer)
        return false;

    memset(buffer, 0, fBufferSize * sizeof(float));
    fPlugInPortsAU[num] = make_pair(buffer, port);
    return true;
}

float* JackRouterDevice::GetPlugInPortVST(int num)
{
    JARLog("GetPlugInPortVST %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsVST[num];
    return obj.first;
}

float* JackRouterDevice::GetPlugInPortAU(int num)
{
    JARLog("GetPlugInPortAU %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsAU[num];
    return obj.first;
}

void JackRouterDevice::ReleasePlugInPortVST(int num)
{
    JARLog("ReleasePlugInPortVST %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsVST[num];
    assert(obj.first);
    assert(obj.second);
    free(obj.first);
    jack_port_unregister(fClient, obj.second);
    fPlugInPortsVST.erase(num); /// TO CHECK : RT access ??
}

void JackRouterDevice::ReleasePlugInPortAU(int num)
{
    JARLog("ReleasePlugInPortAU %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsAU[num];
    assert(obj.first);
    assert(obj.second);
    free(obj.first);
    jack_port_unregister(fClient, obj.second);
    fPlugInPortsAU.erase(num); /// TO CHECK : RT access ??
}



