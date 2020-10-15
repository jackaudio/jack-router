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
	JackRouterPlugIn.cpp
=============================================================================*/

/*
History
 
08-11-07 : Version 0.86 : S Letz
22-11-07 : Fix a locking issue in Process, cleanup dirty buffers, implement JackRouterDevice::GetNearestStartTime 
	       Use a unique UUID for JackRouter. Throw an exception for JackAU or JackVST access is plugin not running.
26-11-07 : Fix a bug in JackRouterDevice::Process. Use blacklist also in JackRouterPlugIn::AddForHAL. Lock taken in CommandThread::WorkLoop() only if commands are waiting.
27-11-07 : Fix a buffer size initialisation bug in JackRouterDevice::Initialize(). Use AutoConnect/RestoreConnections also in kAudioDevicePropertyIOProcStreamUsage handling.
28-11-07 : Fix DVD-Player issue: it works only at 1024 frames. kAudioDevicePropertyBufferFrameSize and kAudioDevicePropertyBufferSize properties are now Settable. 
		   Correct JackRouterDevice::BufferSize.
29-11-07 : New JackRouterDevice::GetBufferSize method for dynamic buffer size changes. Optimization in Process.
30-11-07 : Remove usleep in command thread: use a Wait/Notify mechanism.
04-12-07 : Another dirty buffers fix in JackRouterDevice::Process.
06-12-07 : Another dirty buffers fix in JackRouterDevice::Process.
01-03-08 : Version 0.87 : S Letz: correct JackRouterDevice::Process when null buffers are used. Correct timing information in the Process callback: mSampleTime is now 
           incremented a whole buffer each callback.
31-07-08 : Version 0.88 : S Letz: remove MAX_JACK_PORTS. Dynamic allocation of fInputPortList and fOutputPortList.
28-10-08 : Version 0.89 : S Letz: correct JackRouterDevice::Process for cases when kAudioDevicePropertyIOProcStreamUsage is not used.
07-01-09 : Version 0.90 : S Letz: JackFakeRouterDevice device to be used by "coreaudiod" process (do not need to access JACK server).
22-01-09 : Version 0.91 : S Letz: Fix "dirty buffer issue" with Max/MSP: in JackRouterDevice::Process, output buffers are cleared if GetNumberIOProcs > 0 but GetNumberEnabledIOProcs == 0
24-03-10 : Version 0.93 : S Letz: Use of vDSP_vsma for mixing. Emit kAudioDevicePropertyDeviceIsAlive in JackRouterDevice::Destroy(). 
05-11-10 : Version 0.94 : S Letz: Correct channel numbering in JackRouterDevice::CreateStreams. 
15-07-11 : Version 0.95 : S Letz: Correct timing given to IO: solves Flash and ProTools 9.0 incompatibility. Correct SetPropertyData for kAudioDevicePropertyIOProcStreamUsage. 
           Device notification from the server only on pre Lion systems.

*/

//=============================================================================
//	Includes
//=============================================================================

//	Self Include
#include "JackRouterPlugIn.h"

#include "bequite.h"
#include "JARLog.h"

//	Internal Includes
#include "JackRouterDevice.h"
#include "JackFakeRouterDevice.h"

//	HPBase Includes
#include "HP_DeviceSettings.h"

//	PublicUtility Includes
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAPropertyAddress.h"

#include <Carbon/Carbon.h>
#include <CoreFoundation/CFNotificationCenter.h>

using namespace std;

// JACK
set<string>*		JackRouterPlugIn::fBlackList = NULL;
JackRouterPlugIn*   JackRouterPlugIn::fInstance = NULL;

//=============================================================================
//	JackRouterPlugIn
//=============================================================================

static const char* DefaultServerName()
{
    const char* server_name;
    if ((server_name = getenv("JACK_DEFAULT_SERVER")) == NULL)
        server_name = "default";
    return server_name;
}

static void silent_jack_error_callback(const char*)
{}

static void startCallback(CFNotificationCenterRef /*center*/,
                          void*	/*observer*/,
                          CFStringRef /*name*/,
                          const void* /*object*/,
                          CFDictionaryRef /*userInfo*/)
{
	//printf("com.grame.jackserver.start notification\n");
    JackRouterPlugIn::fInstance->AddForHAL();
}

static void stopCallback(CFNotificationCenterRef /*center*/,
                         void*	/*observer*/,
                         CFStringRef /*name*/,
                         const void* /*object*/,
                         CFDictionaryRef /*userInfo*/)
{
	//printf("com.grame.jackserver.stop notification\n");
 	JackRouterPlugIn::fInstance->ReleaseFromHAL();
}

static void StartNotification()
{
    SInt32 system;
    Gestalt(gestaltSystemVersion, &system);
    
    // Notify only on pre Lion...
    if (system < 0x00001070) { 
        printf("StartNotification name = %s \n", DefaultServerName());
        CFStringRef ref = CFStringCreateWithCString(NULL, DefaultServerName(), kCFStringEncodingMacRoman);
        CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                        NULL, startCallback, CFSTR("com.grame.jackserver.start"),
                                        ref, CFNotificationSuspensionBehaviorDeliverImmediately);
        CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                        NULL, stopCallback, CFSTR("com.grame.jackserver.stop"),
                                        ref, CFNotificationSuspensionBehaviorDeliverImmediately);
        CFRelease(ref);
    }
}

static void StopNotification()
{
    SInt32 system;
    Gestalt(gestaltSystemVersion, &system);
    
    // Notify only on pre Lion...
    if (system < 0x00001070) { 
        printf("StopNotification name = %s \n", DefaultServerName());
        CFStringRef ref = CFStringCreateWithCString(NULL, DefaultServerName(), kCFStringEncodingMacRoman);
        CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), NULL,
                                            CFSTR("com.grame.jackserver.start"), ref);
        CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), NULL,
                                            CFSTR("com.grame.jackserver.stop"), ref);
        CFRelease(ref);
    }
}

JackRouterPlugIn::JackRouterPlugIn(CFUUIDRef inFactoryUUID)
:
	HP_HardwarePlugIn(inFactoryUUID),
	mDevice(NULL)
{
	JackRouterPlugIn::fInstance = this;
	// Start getting notification from the jack server
	StartNotification();
	
	// Set of always "blacklisted" clients
    fBlackList = new set<string>();
    fBlackList->insert("jackd");
    fBlackList->insert("jackdmp");
}

JackRouterPlugIn::~JackRouterPlugIn()
{
	// Stop getting notification from the jack server
	StopNotification();
}

bool JackRouterPlugIn::GetServerParameters(AudioObjectID inObjectID)
{
    jack_client_t* client;
    const char** ports;
    int i;
    
    bool prefOK = ReadPref();
    char* id_name = bequite_getNameFromPid((int)getpid());
    
	// Reject "blacklisted" clients
    if (fBlackList->find(id_name) != fBlackList->end()) {
        JARLog("Rejected client = %s\n", id_name);
        return false;
    }

    if ((client = CheckServer(inObjectID))) {
        
        if (!prefOK) {
            
            // Input ports
            i = 0;
            if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput)) != NULL) {
                while (ports[i]) i++;
            }
            JackRouterDevice::fInputChannels = max(2, i); // At least 2 ports
            
            // Output ports
            i = 0;
            if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsInput)) != NULL) {
                while (ports[i]) i++;
            }
            JackRouterDevice::fOutputChannels = max(2, i); // At least 2 ports
        }
        
        JARLog("fInputChannels = %ld \n", JackRouterDevice::fInputChannels);
        JARLog("fOutputChannels = %ld \n", JackRouterDevice::fOutputChannels);
        jack_client_close(client);
        return true;
        
    } else {
        return false;
    }
}

void JackRouterPlugIn::InitializeWithObjectID(AudioObjectID inObjectID)
{
	// initialize the super class
	HP_HardwarePlugIn::InitializeWithObjectID(inObjectID);
    
    if (!GetServerParameters(inObjectID)) {
        JARLog("jack server not running or rejected client\n");
		throw CAException(kAudioHardwareIllegalOperationError);
    }
	
	// instantiate a new AudioDevice object in the HAL
	AudioDeviceID theNewDeviceID = 0;
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
	OSStatus theError = AudioHardwareClaimAudioDeviceID(GetInterface(), &theNewDeviceID);
#else
	OSStatus theError = AudioObjectCreate(GetInterface(), kAudioObjectSystemObject, kAudioDeviceClassID, &theNewDeviceID);
#endif

	if (theError != 0)
		JARLog("JackRouterPlugIn::InitializeWithObjectID: couldn't instantiate the AudioDevice object theError = %ld\n", theError);
	
	ThrowIfError(theError, CAException(theError), "JackRouterPlugIn::InitializeWithObjectID: couldn't instantiate the AudioDevice object");
	
	// make a device object
	mDevice = new JackRouterDevice(theNewDeviceID, this);
	mDevice->Initialize();

	// restore it's settings if necessary
	UInt32 isMaster = 0;
	UInt32 theSize = sizeof(UInt32);
	AudioHardwareGetProperty(kAudioHardwarePropertyProcessIsMaster, &theSize, &isMaster);
	if(isMaster != 0) {
		HP_DeviceSettings::RestoreFromPrefs(*mDevice, HP_DeviceSettings::sStandardControlsToSave, HP_DeviceSettings::kStandardNumberControlsToSave);
	}

	// set the object state mutex
	HP_Object::SetObjectStateMutexForID(theNewDeviceID, mDevice->GetObjectStateMutex());

	// tell the HAL about the device
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
	theError = AudioHardwareDevicesCreated(GetInterface(), 1, &theNewDeviceID);
#else
	theError = AudioObjectsPublishedAndDied(GetInterface(), kAudioObjectSystemObject, 1, &theNewDeviceID, 0, NULL);
#endif
	AssertNoError(theError, "JackRouterPlugIn::InitializeWithObjectID: got an error telling the HAL a device died");
}

void JackRouterPlugIn::Teardown()
{
   JARLog("JackRouterPlugIn::Teardown \n");
    
	//  first figure out if this is being done as part of the process being torn down
	UInt32 isInitingOrExiting = 0;
	UInt32 theSize = sizeof(UInt32);
	AudioHardwareGetProperty(kAudioHardwarePropertyIsInitingOrExiting, &theSize, &isInitingOrExiting);

	//  next figure out if this is the master process
	UInt32 isMaster = 0;
	theSize = sizeof(UInt32);
	AudioHardwareGetProperty(kAudioHardwarePropertyProcessIsMaster, &theSize, &isMaster);
	
	// JACK
	if (!mDevice)
		throw CAException(kAudioHardwareUnspecifiedError);

	//  do the full teardown if this is outside of the process being torn down or this is the master process
	if ((isInitingOrExiting == 0) || (isMaster != 0)) {
		//	stop all IO on the device
		mDevice->Do_StopAllIOProcs();
		
		//	send the necessary IsAlive notifications
		CAPropertyAddress theIsAliveAddress(kAudioDevicePropertyDeviceIsAlive);
		mDevice->PropertiesChanged(1, &theIsAliveAddress);
        
        JARLog("JackRouterPlugIn::Teardown kAudioDevicePropertyDeviceIsAlive \n");
		
		//	save it's settings if necessary
		if (isMaster != 0) {
			HP_DeviceSettings::SaveToPrefs(*mDevice, HP_DeviceSettings::sStandardControlsToSave, HP_DeviceSettings::kStandardNumberControlsToSave);
		}
	
		//	tell the HAL that the device has gone away
		AudioObjectID theObjectID = mDevice->GetObjectID();
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		OSStatus theError = AudioHardwareDevicesDied(GetInterface(), 1, &theObjectID);
#else
		OSStatus theError = AudioObjectsPublishedAndDied(GetInterface(), kAudioObjectSystemObject, 0, NULL, 1, &theObjectID);
#endif
		AssertNoError(theError, "JackRouterPlugIn::Teardown: got an error telling the HAL a device died");
			
		//	remove the object state mutex
		HP_Object::SetObjectStateMutexForID(theObjectID, NULL);

		//  toss it
		mDevice->Teardown();
		delete mDevice;
		mDevice = NULL;

		//	teardown the super class
		HP_HardwarePlugIn::Teardown();
		JARLog("JackRouterPlugIn::Teardown 0\n");
	} else {
		//  otherwise, only stop the IOProcs
		mDevice->Do_StopAllIOProcs();
		
		//	finalize (rather than tear down) the devices
		mDevice->Finalize();
		
		//	and leave the rest to die with the process
		JARLog("JackRouterPlugIn::Teardown 1\n");
	}
}

void JackRouterPlugIn::AddForHAL()
{
	char* id_name = bequite_getNameFromPid((int)getpid());
  	JARLog("AddForHAL name = %s\n", id_name);
   
	// reject "blacklisted" clients
    if (fBlackList->find(id_name) != fBlackList->end()) {
        JARLog("Rejected client = %s\n", id_name);
        return;
    }
    
    /*
    JARLog("JackRouterPlugIn::AddForHAL\n");
    
    if (!GetServerParameters(DEFAULT_ID)) {
        JARLog("jack server not running or rejected client\n");
        return;
    }
    */
	
	// instantiate a new AudioDevice object in the HAL
	AudioDeviceID theNewDeviceID = 0;
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
	OSStatus theError = AudioHardwareClaimAudioDeviceID(GetInterface(), &theNewDeviceID);
#else
	OSStatus theError = AudioObjectCreate(GetInterface(), kAudioObjectSystemObject, kAudioDeviceClassID, &theNewDeviceID);
#endif
	ThrowIfError(theError, CAException(theError), "JackRouterPlugIn::InitializeWithObjectID: couldn't instantiate the AudioDevice object");
	
	// device already allocated
    if (mDevice) {
        mDevice->ReleaseFromHAL();
        delete mDevice;
    }
    
    // check loading process...
    //char* id_name = bequite_getNameFromPid((int)getpid());
    if (strcmp("coreaudiod", id_name) == 0) {
        mDevice = new JackFakeRouterDevice(theNewDeviceID, this);
    } else {
        mDevice = new JackRouterDevice(theNewDeviceID, this);
    }
    
    /// start the plug-in
    mDevice->Initialize();

	// restore it's settings if necessary
	UInt32 isMaster = 0;
	UInt32 theSize = sizeof(UInt32);
	AudioHardwareGetProperty(kAudioHardwarePropertyProcessIsMaster, &theSize, &isMaster);
	if (isMaster != 0) {
		HP_DeviceSettings::RestoreFromPrefs(*mDevice, HP_DeviceSettings::sStandardControlsToSave, HP_DeviceSettings::kStandardNumberControlsToSave);
	}

	// set the object state mutex
	HP_Object::SetObjectStateMutexForID(theNewDeviceID, mDevice->GetObjectStateMutex());

	// tell the HAL about the device
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
	theError = AudioHardwareDevicesCreated(GetInterface(), 1, &theNewDeviceID);
#else
	theError = AudioObjectsPublishedAndDied(GetInterface(), kAudioObjectSystemObject, 1, &theNewDeviceID, 0, NULL);
#endif
	AssertNoError(theError, "JackRouterPlugIn::InitializeWithObjectID: got an error telling the HAL a device died");
}

void JackRouterPlugIn::ReleaseFromHAL()
{
    JARLog("JackRouterPlugIn::ReleaseFromHAL\n");
	if (mDevice) 
		mDevice->ReleaseFromHAL();
}

bool JackRouterPlugIn::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	JARLog("JackRouterPlugIn::HasProperty\n");
	//	initialize the return value
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theAnswer = true;
			break;
			
		default:
			theAnswer = HP_HardwarePlugIn::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool JackRouterPlugIn::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	JARLog("JackRouterPlugIn::IsPropertySettable\n");
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theAnswer = false;
			break;
			
		default:
			theAnswer = HP_HardwarePlugIn::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32 JackRouterPlugIn::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	JARLog("JackRouterPlugIn::GetPropertyDataSize\n");
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theAnswer = sizeof(CFStringRef);
			break;
			
		default:
			theAnswer = HP_HardwarePlugIn::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void JackRouterPlugIn::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	JARLog("JackRouterPlugIn::GetPropertyData\n");
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "JackRouterPlugIn::GetPropertyData: wrong data size for kAudioObjectPropertyName");
			*static_cast<CFStringRef*>(outData) = CFSTR("com.apple.audio.JackRouter");
			CFRetain(*static_cast<CFStringRef*>(outData));
			break;
			
		default:
			HP_HardwarePlugIn::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void JackRouterPlugIn::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	JARLog("JackRouterPlugIn::SetPropertyData\n");
	switch(inAddress.mSelector)
	{
		default:
			HP_HardwarePlugIn::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

// JACK
bool JackRouterPlugIn::ReadPref()
{
    CFURLRef prefURL;
    FSRef prefFolderRef;
    OSErr err;
    char buf[256];
    char path[256];
    bool res = false;
	
	//printf("JackRouterPlugIn::ReadPref\n");

    err = FSFindFolder(kUserDomain, kPreferencesFolderType, kDontCreateFolder, &prefFolderRef);
    if (err == noErr) {
        prefURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &prefFolderRef);
        if (prefURL) {
            CFURLGetFileSystemRepresentation(prefURL, FALSE, (UInt8*)buf, 256);
            sprintf(path, "%s/JAS.jpil", buf);
            FILE *prefFile;
            if ((prefFile = fopen(path, "rt"))) {
                int nullo;
				int input, output, autoconnect, debug, default_input, default_output, default_system;
                fscanf(prefFile, "\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s",
                        &input,
                        &nullo,
                        &output,
                        &nullo,
                        &autoconnect,
                        &nullo,
                        &default_input, 
                        &nullo,
                        &default_output, 
                        &nullo,
                        &default_system,
                        &nullo,
                        &debug,
                        &nullo,
                        JackRouterDevice::fCoreAudioDriverUID);
                
                //printf("fCoreAudioDriverUID %s\n", JackRouterDevice::fCoreAudioDriverUID);

                fclose(prefFile);
				JackRouterDevice::fInputChannels = input;
				JackRouterDevice::fOutputChannels = output;
				JackRouterDevice::fAutoConnect = autoconnect;
				JackRouterDevice::fDefaultInput = default_input;
				JackRouterDevice::fDefaultOutput = default_output;
				JackRouterDevice::fDefaultSystem = default_system;
				JAR_fDebug = debug;
                
                // Print messages in console only in debug mode...
                if (!JAR_fDebug) {
                    jack_set_error_function(silent_jack_error_callback);
                    jack_set_info_function(silent_jack_error_callback);
                }
                    
                res = true;
            }
			CFRelease(prefURL);
        }
    }

    const char* path1 = "/Library/Audio/Plug-Ins/HAL/JackRouter.plugin/Contents/BlackList.txt";
    FILE* blackListFile;
	
    if ((blackListFile = fopen(path1, "rt"))) {
        char client_name[64];
        char line[500];
        while (fgets(line, 500, blackListFile)) {
            sscanf(line, "%s", client_name);
            fBlackList->insert(client_name);
            JARLog("Blacklisted client %s\n", client_name);
        }
		fclose(blackListFile);
    }

    return res;
}

jack_client_t* JackRouterPlugIn::CheckServer(AudioObjectID inSelf)
{
    jack_client_t * client;
    char name[JACK_CLIENT_NAME_LEN];
 	jack_status_t status;
    sprintf(name, "JAR::%ld", inSelf);

    if ((client = jack_client_open(name, JackNoStartServer, &status))) {
        JackRouterDevice::fBufferSize = jack_get_buffer_size(client);
        JackRouterDevice::fSampleRate = jack_get_sample_rate(client);

        JARLog("CheckServer JackRouterDevice::fBufferSize = %ld\n", JackRouterDevice::fBufferSize);
        JARLog("CheckServer JackRouterDevice::fSampleRate = %f\n", JackRouterDevice::fSampleRate);

		return client;
    } else {
        return NULL;
	}
}

extern "C" void*
New_JackRouterPlugIn(CFAllocatorRef /*inAllocator*/, CFUUIDRef inRequestedTypeUUID) 
{
	void* theAnswer = NULL;
	
	if (CFEqual(inRequestedTypeUUID, kAudioHardwarePlugInTypeID)) {
		JackRouterPlugIn* thePlugIn = new JackRouterPlugIn(inRequestedTypeUUID);
		thePlugIn->Retain();
		theAnswer = thePlugIn->GetInterface();
	}
	
	return theAnswer;
}
