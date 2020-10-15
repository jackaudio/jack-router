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
	JackFakeRouterDevice.cpp
=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

//	Self Include
#include "JackFakeRouterDevice.h"
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

#define BUFFER_SIZE 512
#define SAMPLE_RATE 44100
#define INPUT_CHAN 2
#define OUTPUT_CHAN 2

using namespace std;

//=============================================================================
//	JackFakeRouterDevice
//=============================================================================

JackFakeRouterDevice::JackFakeRouterDevice(AudioDeviceID inAudioDeviceID, JackRouterPlugIn* inPlugIn)
:JackRouterDeviceInterface(inAudioDeviceID, kAudioDeviceClassID, inPlugIn, 1, false),
	mSHPPlugIn(inPlugIn),
	mIOGuard("IOGuard")
 {}

JackFakeRouterDevice::~JackFakeRouterDevice()
{}

void JackFakeRouterDevice::Initialize()
{
	JARLog("JackFakeRouterDevice Initialize\n");
	
	HP_Device::Initialize();
	
 	//	create the streams
	CreateStreams();
	
	//  set the default buffer size before we go any further
	mIOBufferFrameSize = BUFFER_SIZE;
	JARLog("JackFakeRouterDevice Initialize OK\n");
}

void JackFakeRouterDevice::Teardown()
{
	JARLog("JackFakeRouterDevice Teardown\n");
		
	//	stop things
	Do_StopAllIOProcs();
	ReleaseStreams();	
	
	HP_Device::Teardown();
}

void JackFakeRouterDevice::Finalize()
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

CFStringRef	JackFakeRouterDevice::CopyDeviceName() const
{
	CFStringRef theAnswer = CFSTR("JackRouter");
	CFRetain(theAnswer);
	return theAnswer;
}

CFStringRef	JackFakeRouterDevice::CopyDeviceManufacturerName() const
{
	CFStringRef theAnswer = CFSTR("Grame");
	CFRetain(theAnswer);
	return theAnswer;
}

CFStringRef	JackFakeRouterDevice::CopyDeviceUID() const
{
	CFStringRef theAnswer = CFSTR("JackRouter:0");
	CFRetain(theAnswer);
	return theAnswer;
}

bool JackFakeRouterDevice::CanBeDefaultDevice(bool /*inIsInput */, bool inIsSystem) const
{
	return (inIsSystem) ? false : true;
}

bool JackFakeRouterDevice::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	JARPrint4CharCode("JackFakeRouterDevice::HasProperty ", inAddress.mSelector);
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackFakeRouterDevice*>(this)->GetStateMutex());
	
	switch(inAddress.mSelector)
	{
            JARLog("JackFakeRouterDevice::HasProperty JACK special\n");
			theAnswer = true;
			break;
		
		default:
			theAnswer = HP_Device::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool JackFakeRouterDevice::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	JARPrint4CharCode("JackFakeRouterDevice::IsPropertySettable ", inAddress.mSelector);
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackFakeRouterDevice*>(this)->GetStateMutex());
	
	switch(inAddress.mSelector)
	{
    
		case kAudioDevicePropertyBufferFrameSize:
		case kAudioDevicePropertyBufferSize:
			theAnswer = true;
			break;
	
    	case kAudioDevicePropertyIOProcStreamUsage:
			theAnswer = true;
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

void JackFakeRouterDevice::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackFakeRouterDevice*>(this)->GetStateMutex());
	
	JARPrint4CharCode("JackFakeRouterDevice::GetPropertyData ", inAddress.mSelector);
	
	switch(inAddress.mSelector)
	{
		
		case kAudioDevicePropertyBufferFrameSize:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "JackFakeRouterDevice::GetPropertyData: wrong data size for kAudioDevicePropertyLatency");
			*static_cast<UInt32*>(outData) = BUFFER_SIZE;
			break;
			
        default:
			HP_Device::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void JackFakeRouterDevice::StartIOEngine()
{
	JARLog("JackFakeRouterDevice::StartIOEngine\n");
	if (!IsIOEngineRunning()) {
		StartHardware();
	}
}

void JackFakeRouterDevice::StartIOEngineAtTime(const AudioTimeStamp&  /*inStartTime*/, UInt32 /*inStartTimeFlags*/)
{
	JARLog("JackFakeRouterDevice::StartIOEngineAtTime\n");
	if (!IsIOEngineRunning()) {
		StartHardware();
	}
}

void JackFakeRouterDevice::StopIOEngine()
{
	JARLog("JackFakeRouterDevice::StopIOEngine\n");
	if (IsIOEngineRunning()) {
		StopHardware();
	}
}

void JackFakeRouterDevice::StartHardware()
{
	//	the calling thread must have already locked the Guard prior to calling this method
	JARLog("JackFakeRouterDevice::StartHardware\n");

	//	set the device state to know the engine is running
	IOEngineStarted();
	
	//	notify clients that the engine is running
	CAPropertyAddress theIsRunningAddress(kAudioDevicePropertyDeviceIsRunning);
	PropertiesChanged(1, &theIsRunningAddress);	
}

void JackFakeRouterDevice::StopHardware()
{
	JARLog("JackFakeRouterDevice::StopHardware\n");
	
	//	set the device state to know the engine has stopped
	IOEngineStopped();
	
	//	Notify clients that the IO callback is stopping
	CAPropertyAddress theIsRunningAddress(kAudioDevicePropertyDeviceIsRunning);
	PropertiesChanged(1, &theIsRunningAddress);
}

bool JackFakeRouterDevice::IsSafeToExecuteCommand()
{
    return true;
}

bool JackFakeRouterDevice::StartCommandExecution(void** outSavedCommandState)
{
	JARLog("JackFakeRouterDevice::StartCommandExecution \n");
	*outSavedCommandState = mIOGuard.Lock() ? (void*)1 : (void*)0;
	return true;
}

void JackFakeRouterDevice::FinishCommandExecution(void* inSavedCommandState)
{
	JARLog("JackFakeRouterDevice::FinishCommandExecution \n");
	if (inSavedCommandState != 0) {
		mIOGuard.Unlock();
	}
}

UInt32 JackFakeRouterDevice::GetMinimumIOBufferFrameSize() const
{
	return BUFFER_SIZE;
}

UInt32 JackFakeRouterDevice::GetMaximumIOBufferFrameSize() const
{
	return BUFFER_SIZE;
}

void JackFakeRouterDevice::CreateStreams()
{
	//  common variables
	OSStatus		theError = 0;
	AudioObjectID   theNewStreamID = 0;
	JackRouterStream*		theStream = NULL;

	//  create a vector of AudioStreamIDs to hold the stream ids we are creating
	std::vector<AudioStreamID> theStreamIDs;
	
	for (int i = 0; i < INPUT_CHAN; i++) {
	
		//  instantiate an AudioStream
	#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		theError = AudioHardwareClaimAudioStreamID(mSHPPlugIn->GetInterface(), GetObjectID(), &theNewStreamID);
	#else
		theError = AudioObjectCreate(mSHPPlugIn->GetInterface(), GetObjectID(), kAudioStreamClassID, &theNewStreamID);
	#endif
		if (theError == 0) {
			//  create the stream
			theStream = new JackRouterStream(theNewStreamID, mSHPPlugIn, this, true, i+1, SAMPLE_RATE);
			theStream->Initialize();
			
			//	add to the list of streams in this device
			AddStream(theStream);
			
			//  store the new stream ID
			theStreamIDs.push_back(theNewStreamID);
		}
	}
	
	for (int i = 0; i < OUTPUT_CHAN; i++) {

		//  claim a stream ID for the stream
	#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		theError = AudioHardwareClaimAudioStreamID(mSHPPlugIn->GetInterface(), GetObjectID(), &theNewStreamID);
	#else
		theError = AudioObjectCreate(mSHPPlugIn->GetInterface(), GetObjectID(), kAudioStreamClassID, &theNewStreamID);
	#endif
		if (theError == 0) {
			//  create the stream
			theStream = new JackRouterStream(theNewStreamID, mSHPPlugIn, this, false, i+1, SAMPLE_RATE);
			theStream->Initialize();
			
			//	add to the list of streams in this device
			AddStream(theStream);
			
			//  store the new stream ID
			theStreamIDs.push_back(theNewStreamID);
		}
	}

	//  now tell the HAL about the new stream IDs
	if (theStreamIDs.size() != 0) {
		//	set the object state mutexes
		for (std::vector<AudioStreamID>::iterator theIterator = theStreamIDs.begin(); theIterator != theStreamIDs.end(); std::advance(theIterator, 1))
		{
			HP_Object* theObject = HP_Object::GetObjectByID(*theIterator);
			if (theObject != NULL) {
				HP_Object::SetObjectStateMutexForID(*theIterator, theObject->GetObjectStateMutex());
			}
		}
		
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		theError = AudioHardwareStreamsCreated(mSHPPlugIn->GetInterface(), GetObjectID(), theStreamIDs.size(), &(theStreamIDs.front()));
#else
		theError = AudioObjectsPublishedAndDied(mSHPPlugIn->GetInterface(), GetObjectID(), theStreamIDs.size(), &(theStreamIDs.front()), 0, NULL);
#endif
		ThrowIfError(theError, CAException(theError), "JackFakeRouterDevice::CreateStreams: couldn't tell the HAL about the streams");
	}
}

void JackFakeRouterDevice::CreateForHAL(AudioDeviceID theNewDeviceID)
{
	JARLog("CreateForHAL\n");
	SetObjectID(theNewDeviceID);  // setup the new deviceID
	CreateStreams();
}

void JackFakeRouterDevice::ReleaseStreams()
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

void JackFakeRouterDevice::ReleaseFromHAL()
{
		AudioObjectID theObjectID = GetObjectID();
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		OSStatus theError = AudioHardwareDevicesDied(mSHPPlugIn->GetInterface(), 1, &theObjectID);
#else
		OSStatus theError = AudioObjectsPublishedAndDied(mSHPPlugIn->GetInterface(), kAudioObjectSystemObject, 0, NULL, 1, &theObjectID);
#endif
		AssertNoError(theError, "JackRouterPlugIn::Teardown: got an error telling the HAL a device died");
}





