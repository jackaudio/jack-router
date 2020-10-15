/*
Copyright © Stefan Werner stefan@keindesign.de, Grame 2003-2006

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

Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
grame@rd.grame.fr

*/

/*
History
 
14-08-03 : First version : S Letz
 
15-08-03 : Version 0.05 : S Letz
        DeviceAddIOProc : check if a client is already setup, 
        Improve running management. Implement kAudioDevicePropertyPreferredChannelsForStereo.
        Cleanup error codes.
        
16-08-03 : Version 0.06 : S Letz
        Experimental : special Property "kAudioDevicePropertyJackClient" to access Jack client from application code.
        fInputChannels are now allocated for Input, fOutputChannels are now allocated for Ouput ==> this JAS can be selected
        as default input/output in Audio/Midi setup.
 
21-08-03 : Version 0.07 : S Letz
        Completly implement DeviceGetPropertyInfo and StreamGetPropertyInfo : to be checked
    
02-09-03 : Version 0.08 : S Letz
        Correct kAudioDevicePropertyStreamFormatSupported implementation, add Johnny preferences file management code.
 
03-09-03 : Version 0.09 : S Letz
        Improves stream management, now Audio/Midi setup and Daisy correctly display streams settings.
        Vokator works correctly at launch time, but still does not work when changing audio settings dynamically.
        
03-09-03 : Version 0.10 : Johnny Petrantoni
        Now the names of the clients are the names of the applications that is using JAS. (procinfo folder).
        
03-09-03 : Version 0.11 : S Letz
        Change the way the Jack client is activated. Now QuickTime Player runs correctly.
        
04-09-03 : Version 0.12 : S Letz
        Add kLinearPCMFormatFlagIsFloat in stream format. Implement kAudioDevicePropertyStreamFormatMatch.
        
04-09-03 : Version 0.13 : S Letz
        Correct kAudioDevicePropertyPreferredChannelsForStereo management. Now QuickTime Player seems to play on the correct channels.
 
05-09-03 : Version 0.14 : S Letz
        Redirect some calls to the Jack CoreAudio driver. Warning : hard coded CoreAudio driver (Built-in).
        Still problems with clock sources....
 
08-09-03 : Version 0.15 : S Letz
        Redirect some calls to the Jack CoreAudio driver. Now get the Jack CoreAudio driver name dynamically.
        Still problems with clock sources....   
      
09-09-03 : Version 0.16 : S Letz
        Implement AudioTimeStamp on Process, implement DeviceGetCurrentTime, call listener in Start and Stop (for Reason) 
 
10-09-03 : Version 0.17 : S Letz
        Redirect some calls to the Jack CoreAudio driver : the same calls are redirected in DeviceGetPropertyInfo and DeviceGetProperty. 
		Clock sources problems seem to be solved.
 
10-09-03 : Version 0.18 : Johnny Petrantoni
        Correct ReadPref.
 
10-09-03 : Version 0.19 : S Letz
        Check QueryDevice return code in Initialize. Add streams and device desallocation in Teardown.
 
11-09-03 : Version 0.20 : S Letz
        Redirect all still unhandled calls to the Jack CoreAudio driver.
 
11-09-03 : Version 0.21 : S Letz
        Implements kAudioDevicePropertyDeviceName and all in StreamGetPropertry (DP4 use it...), Implements kAudioStreamPropertyOwningDevice. 	
		Correct some extra property management issues.
 
12-09-03 : Version 0.22 : S Letz
        QueryDevices : correct bug in Jack CoreAudio driver name extraction.
 
12-09-03 : Version 0.23 : S Letz
        Support for multiple IOProc, implements mixing.
 
13-09-03 : Version 0.24 : S Letz
        More explicit debug messages.
 
14-09-03 : Version 0.25 : S Letz
        Implement kAudioDevicePropertyUsesVariableBufferFrameSizes in DeviceGetProperty.
       
15-09-03 : Version 0.26 : S Letz
        Correct bug in multiple IOProc management.
       
16-09-03 : Version 0.27 : S Letz
        Return kAudioHardwareUnknownPropertyError in case of unknown property.
       
16-09-03 : Version 0.28 : S Letz
        Correct DeviceGetProperty for kAudioDevicePropertyDeviceIsAlive (back to old behaviour).
       
22-09-03 : Version 0.29 : S Letz
        Improves kAudioStreamPropertyTerminalType management.
        Correct stream flags configuration. Should not use kAudioFormatFlagIsNonInterleaved !!
        Now iTunes, Reason and DLSynth are working...
       
22-09-03 : Version 0.30 : S Letz
        Implements GetProperty for kAudioDevicePropertyDataSource, kAudioDevicePropertyDataSources,
        kAudioDevicePropertyDataSourceNameForID and kAudioDevicePropertyDataSourceNameForIDCFString.
               
23-09-03 : Version 0.31 : S Letz
        Change again JackFlags value... kAudioFormatFlagIsBigEndian was the lacking one that made iTunes and other not work..
        kAudioFormatFlagIsNonInterleaved is back again.
       
23-09-03 : Version 0.32 : S Letz
        Client side more robust error handling when the Jackd server quits. Use the jack_on_shutdown callback to cleany deactivate
        the JAS client. Quitting the server, changing it's settings, reactivating the clients should work.
        
26-09-03 : Version 0.33 : Johnny Petrantoni
        New ReadPref.
 
29-09-03 : Version 0.34 : S Letz
        Better version management.
 
01-10-03 : Version 0.35 : S Letz
        Code cleanup. kAudioDevicePropertyJackClient now opens the Jack client if it is not already running (needed for VST plugins)
 
13-10-03 : Version 0.36 : S Letz
        Major code cleanup. kAudioDevicePropertyJackClient renamed to kAudioDevicePropertyGetJackClient. New kAudioDevicePropertyReleaseJackClient
        to release the jack client.
 
14-10-03 : Version 0.37 : S Letz, Johnny Petrantoni
        Correct kAudioDevicePropertyStreamFormatMatch property. Remove the fixed size length for CoreAudio driver names.
        Correct driver name and manufacturer string length bug. Correct kAudioDevicePropertyStreamConfiguration bug.
        Cleanup error codes. Correct a bug in Activate method when connecting jack ports.
 
18-10-03 : Version 0.38 : S Letz
        Removes unused code.
        
19-10-03 : Version 0.39 : S Letz
        Improve DeviceStart/DeviceStop : now each proc use a running status. Check for multiple start/stop. Check for multiple same IOProc. 	
		Correct bugs in DeviceGetProperty, ioPropertyDataSize was not correctly set.
 
19-10-03 : Version 0.40 : S Letz
        Check HAL connection in Teardown. More debug messages. Cleanup ReadPref.
        
22-10-03 : Version 0.41 : S Letz
        Checking of parameter size value in DeviceGetProperty. Management of outWritable in DeviceGetPropertyInfo and
        StreamGetPropertyInfo.
        
23-10-03 : Version 0.42 : S Letz
        Correct a bug that cause crash when JASBus is used without JAS activated. Prepare the code for compilation with QT 6.4.
 
28-10-03 : Version 0.43 : S Letz
        Cleanup.
 
03-11-03 : Version 0.44 : Johnny Petrantoni. Implement kAudioDevicePropertyPreferredChannelLayout on Panther.
 
08-11-03 : Version 0.45 : S Letz 
		kAudioDevicePropertyUsesVariableBufferFrameSizes not supported in DeviceGetPropertyInfo. 
		Return a result in DeviceGetPropertyInfo even when outSize is null.
		Change management of kAudioDevicePropertyDataSource and kAudioDevicePropertyDataSources.
		Idea for Panther default system driver problem. Update ReadPref.         
 
09-01-04 : Version 0.46 : S Letz 
		Ugly hack to solve the iTunes deconnection problem.
		  
11-01-04 : Version 0.47 : S Letz 
		Removes ugly hack, implement SaveConnections and RestoreConnections (to solve the iTunes deconnection problem),
		Unregister jack port in Close. Check jack_port_register result.	Return kAudioHardwareUnsupportedOperationError
		for unsupported DeviceRead, DeviceGetNearestStartTime and DeviceTranslateTime.	
		
15-01-04 : Version 0.48 : S Letz 
		Implement null proc behaviour in DeviceStop (to be checked)
		Implement (outPropertyData == NULL) && (ioPropertyDataSize != NULL) case in DeviceGetPropertyInfo and StreamGetPropertyInfo
        In process, clean the ouput buffers in all cases to avoid looping an old buffer in some situations.
		Implement null proc behaviour in DeviceStart and DeviceStop.
 
16-01-04 : Version 0.49 : S Letz
        Rename flags kJackStreamFormat and kAudioTimeFlags.
		
22-01-04 : Version 0.50 : S Letz
        Rename Jack Audio Server to Jack Router.
 
24-01-04 : Version 0.51 : S Letz  Johnny Petrantoni
        Implement kAudioDevicePropertyIOProcStreamUsage. Implement kAudioDevicePropertyUsesVariableBufferFrameSizes in in DeviceGetProperty.
        This solve the iMovie 3.03 crash. Improve debug code using Johnny's code.  Reject "jackd" as a possible client.
		Fixed ReadPref bug introduced when improving Debug code. Add kAudioHardwarePropertyBootChimeVolumeScalar in DeviceGetPropertyInfo.
		Correct bug in CheckServer.
		
12-07-04 : Version 0.52 : S Letz
		Check TJackClient::fJackClient in SetProperty.
 
13-07-04 : Version 0.53 : S Letz
		Correct bug in kAudioDevicePropertyIOProcStreamUsage management.
	
1-09-04 : Version 0.54 : S Letz
		Distinguish kAudioDevicePropertyActualSampleRate and kAudioDevicePropertyNominalSampleRate.
		kAudioDevicePropertyActualSampleRate is returned only when the driver is running.
 
1-09-04 : Version 0.55 : S Letz
		Correct use of AudioHardwareStreamsCreated and AudioHardwareStreamsDied functions (they are using an array of streamIDs)
 
10-09-04 : Version 0.56 : S Letz
		Correct DeviceSetProperty: some properties can be "set" even there is no Jack client running other like kAudioDevicePropertyIOProcStreamUsage not.
 
14-09-04 : Version 0.57 : S Letz
		Improve the external (Jack plugins) management : now internal and external "clients" are distinguished. Correct Save/Restore connections bug.
		
03-10-04 : Version 0.58 : S Letz
		Correct bug in bequite_getNameFromPid. This solve the Band-in-a Box bug. 
		New KillJackClient method to be called in TearDown when clients do not correctly quit (like iMovie).
		
15-10-04 : Version 0.59 : S Letz
		Redirect kAudioDevicePropertySafetyOffset and kAudioDevicePropertyLatency properties on the real driver.
 
26-10-04 : Version 0.60 : S Letz
		Previous connection state takes precedence over auto-connection state.
 
15-11-04 : Version 0.61 : S Letz
		Correct kAudioStreamPropertyPhysicalFormat setting problems. This finally solve the NI applications issue...
		Correct DeviceGetPropertyInfo and StreamGetPropertyInfo for properties that can be set. Debug activated with pref file.
 
08-12-04 : Version 0.62 : S Letz
		Correct internal output/input port connections bug : output ports are no more cleared. CoreAudio and PortAudio drivers are corrected also.
 
29-11-04 : Version 0.63 : S Letz
		Remove QueryDevices. The CoreAudio driver AudioDeviceID is now read in the JAS.pil preference file.
		
10-12-04 : Version 0.64 : S Letz
		Implement XRun and BufferSize change notifications.
		
15-12-04 : Version 0.65 : : S Letz
		Notification sent by JackPilot when the jack server start and stops : allows applications to dynamically see when jack server is available.
 
13-01-05 : Version 0.66 : S Letz
		Correct RestoreConnections issue : RestoreConnections is called only once for the first Activate. Correct bug in Process in the "several proc" case :
		output buffer need to be cleared.
		
24-01-05 : Version 0.67 : S Letz
		Correct Final Cut Pro crash by changing the way DeviceGetProperty for kAudioDevicePropertyStreams behaves.
 
03-02-05 : Version 0.68 : S Letz
		Ouput buffer are produced in fOuputListTemp buffer before being copied to jack port buffers (to solve a dirty buffer looping problem in QT player).
 
03-02-05 : Version 0.69 : S Letz
		Correct (again...) RestoreConnections: AutoConnect if called only for the *first* activation, otherwise RestoreConnections is used.
 
15-02-05 : Version 0.70 : S Letz
		To solve dirty buffer problems : new management of audio buffers for plug-ins: they are now allocated and managed on JAR side. 
 
16-02-05 : Version 0.71 : S Letz
		Independant management of VST and AU plug-ins so that both types can be used at the same time. 
		
16-02-05 : Version 0.72 : S Letz
		Management of a set of "blacklisted" clients. Fix plug-in port naming bug.
		
22-02-05 : Version 0.73 : S Letz
		Correct a crash with some applications (like Skype) that cause static variables (like fBlackList) not to be allocated properly : allocation with new.
 
22-02-05 : Version 0.74 : S Letz 
		Correct DeviceSetProperty for kAudioDevicePropertyStreamFormat. It returns kAudioHardwareNoError if the given description is the exact format 
		supported by Jack Router otherwise kAudioDeviceUnsupportedFormatError. Change error code for kAudioHardwareIllegalOperationError in DeviceSetProperty buffer size.
		DeviceStartAtTime returns kAudioHardwareUnsupportedOperationError. Add kAudioDevicePropertyDeviceManufacturerCFString in DeviceGetPropertyInfo.
		Add AU and VST plug-in properties in DeviceGetPropertyInfo.
		
25-05-05 : Version 0.75 : S Letz
		Unregister Jack port if no more needed in DeviceSetProperty kAudioDevicePropertyIOProcStreamUsage. Fix a bug in process.

06-06-05 : Version 0.76 : S Letz
		Use the PropertyDeviceUID as the identifier for the used CoreAudio driver.
		
29-06-05 : Version 0.77 : S Letz
		Correct DeviceGetProperty for kAudioDevicePropertyStreamFormatMatch. Fix a bug in Process (Rax using 2 IOProc with stream usage was not working anymore).

08-12-05 : Version 0.78 : S Letz
		Fixes for 10.4 compilation. Correct kAudioDevicePropertyPreferredChannelLayout management. Correct ProcContext intialization.

19-12-05 : Version 0.79 : S Letz
		Correct desactivation: ports were unregistered before deactivate.... change SaveConnections call : now done in Deactivate. Test CheckRunning result in methods.

18-01-06 : Version 0.80 : S Letz
		Implements DeviceTranslateTime: simply copy inTime ==> outTime for now.

18-01-06 : Version 0.81 : S Letz
		Remove default input/output management. Deactivate JackRouter as DefaultSystemDevice.

21-03-06 : Version 0.82 : S Letz
		Endianaess issue for Intel version. Fix fscan bug.

11-04-06 : Version 0.83 : S Letz
		JackRouter as default input/output "again"...
		
11-04-06 : Version 0.84 : S Letz
		Fix an Autoconnect/Restore connection issue found with AudioFinder (in kAudioDevicePropertyIOProcStreamUsage).

07-11-07 : Version 0.85 : S Letz
		Implement kAudioDevicePropertyDataSource and kAudioDevicePropertyDataSources. Use of "jack_client_open" instead of "jack_client_new".
		Do not check jack server when loaded in "coreaudiod" process.

TODO :
    
        - solve zombification problem of Jack (remove time-out check or use -T option)
        - check kAudioDevicePropertyPreferredChannelLayout parameters.
        ....
 
*/

#include <Carbon/Carbon.h>
#include "TJackClient.h"
#include <notify.h>

#include <CoreFoundation/CFNotificationCenter.h>
#include <IOKit/audio/IOAudioTypes.h>

#include <stdio.h>
#include <assert.h>

//#define JACK_NOTIFICATION 1

// Static variables declaration

long TJackClient::fBufferSize;
float TJackClient::fSampleRate;

long TJackClient::fInputChannels = 0;
long TJackClient::fOutputChannels = 0;

bool TJackClient::fAutoConnect = true;
bool TJackClient::fDeviceRunning = false;
bool TJackClient::fConnected2HAL = false;
bool TJackClient::fDefaultInput = true;	 
bool TJackClient::fDefaultOutput = true;	 
bool TJackClient::fDefaultSystem = true;
bool TJackClient::fDebug = false;
list<pair<string, string> > TJackClient::fConnections;

string TJackClient::fDeviceName = "JackRouter";
string TJackClient::fStreamName = "Float32";
string TJackClient::fDeviceManufacturer = "Grame";

string TJackClient::fInputDataSource = "JackRouter Input";
string TJackClient::fOutputDataSource = "JackRouter Output";

AudioDeviceID TJackClient::fDeviceID = 0;
TJackClient* TJackClient::fJackClient = NULL;

AudioStreamID TJackClient::fStreamIDList[MAX_JACK_PORTS];
AudioStreamID TJackClient::fCoreAudioDriver = 0;
char TJackClient::fCoreAudioDriverUID[128];
AudioHardwarePlugInRef TJackClient::fPlugInRef = 0;

bool TJackClient::fNotification = false;
bool TJackClient::fFirstActivate = true;

set<string>* TJackClient::fBlackList = NULL;

#define kJackStreamFormat  kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved

#define kAudioTimeFlags kAudioTimeStampSampleTimeValid|kAudioTimeStampHostTimeValid|kAudioTimeStampRateScalarValid

#define JackInputDataSource 0
#define JackOutputDataSource 1

struct stereoList
{
    UInt32	channel[2];
};
typedef struct stereoList stereoList;

//------------------------------------------------------------------------
void JARLog(char *fmt, ...)
{
    if (TJackClient::fDebug) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "JAR: ");
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
}

//------------------------------------------------------------------------
static void Print4CharCode(char* msg, long c)
{
    if (TJackClient::fDebug) {
        UInt32 __4CC_number = (c);
        char __4CC_string[5];
     	*((SInt32*)__4CC_string) = EndianU32_NtoB(__4CC_number);		
        __4CC_string[4] = 0;
        JARLog("%s'%s'\n", (msg), __4CC_string);
    }
}

//------------------------------------------------------------------------
static void printError(OSStatus err)
{
    if (TJackClient::fDebug) {
        switch (err) {
            case kAudioHardwareNoError:
                JARLog("error code : kAudioHardwareNoError\n");
                break;
            case kAudioHardwareNotRunningError:
                JARLog("error code : kAudioHardwareNotRunningError\n");
                break;
            case kAudioHardwareUnspecifiedError:
                printf("error code : kAudioHardwareUnspecifiedError\n");
            case kAudioHardwareUnknownPropertyError:
                JARLog("error code : kAudioHardwareUnknownPropertyError\n");
                break;
            case kAudioHardwareBadPropertySizeError:
                JARLog("error code : kAudioHardwareBadPropertySizeError\n");
                break;
            case kAudioHardwareIllegalOperationError:
                JARLog("error code : kAudioHardwareIllegalOperationError\n");
                break;
            case kAudioHardwareBadDeviceError:
                JARLog("error code : kAudioHardwareBadDeviceError\n");
                break;
            case kAudioHardwareBadStreamError:
                JARLog("error code : kAudioHardwareBadStreamError\n");
                break;
            case kAudioDeviceUnsupportedFormatError:
                JARLog("error code : kAudioDeviceUnsupportedFormatError\n");
                break;
            case kAudioDevicePermissionsError:
                JARLog("error code : kAudioDevicePermissionsError\n");
                break;
            default:
                JARLog("error code : unknown\n");
                break;
        }
    }
}

#ifdef JACK_NOTIFICATION

//---------------------------------------------------------------------------------------------------------------------------------
static void startCallback(CFNotificationCenterRef center,
                          void*	observer,
                          CFStringRef name,
                          const void* object,
                          CFDictionaryRef userInfo)
{
    OSStatus err = TJackClient::Initialize(TJackClient::fPlugInRef);
    JARLog("com.grame.jackserver.start notification %ld\n", err);
}

//---------------------------------------------------------------------------------------------------------------------------------
static void stopCallback(CFNotificationCenterRef center,
                         void*	observer,
                         CFStringRef name,
                         const void* object,
                         CFDictionaryRef userInfo)
{
    OSStatus err = TJackClient::Teardown(TJackClient::fPlugInRef);
    JARLog("com.grame.jackserver.stop notification %ld\n", err);
}

//---------------------------------------------------------------------------------------------------------------------------------
static void StartNotification()
{
    if (!TJackClient::fNotification) {
        CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                        NULL, startCallback, CFSTR("com.grame.jackserver.start"),
                                        CFSTR("com.grame.jackserver"), CFNotificationSuspensionBehaviorDeliverImmediately);
        CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                        NULL, stopCallback, CFSTR("com.grame.jackserver.stop"),
                                        CFSTR("com.grame.jackserver"), CFNotificationSuspensionBehaviorDeliverImmediately);
        TJackClient::fNotification = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------
static void StopNotification()
{
    if (TJackClient::fNotification) {
        CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), NULL,
                                           CFSTR("com.grame.jackserver.start"), CFSTR("com.grame.jackserver"));
        CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), NULL,
                                           CFSTR("com.grame.jackserver.stop"), CFSTR("com.grame.jackserver"));
        TJackClient::fNotification = false;
    }
}

#endif

//------------------------------------------------------------------------
void TJackClient::SaveConnections()
{
    const char** connections;

    if (!fClient)
        return;

    JARLog("--------------------------------------------------------\n");
    JARLog("SaveConnections\n");

    fConnections.clear();

    for (int i = 0; i < TJackClient::fInputChannels; ++i) {
        if (fInputPortList[i] && (connections = jack_port_get_connections(fInputPortList[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(connections[j], jack_port_name(fInputPortList[i])));
            }
            free(connections);
        }
    }

    for (int i = 0; i < TJackClient::fOutputChannels; ++i) {
        if (fOutputPortList[i] && (connections = jack_port_get_connections(fOutputPortList[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(jack_port_name(fOutputPortList[i]), connections[j]));
            }
            free(connections);
        }
    }

    if (TJackClient::fDebug) {
        list<pair<string, string> >::const_iterator it;

        for (it = fConnections.begin(); it != fConnections.end(); it++) {
            pair<string, string> connection = *it;
            JARLog("connections : %s %s\n", connection.first.c_str(), connection.second.c_str());
        }
    }
}

//------------------------------------------------------------------------
void TJackClient::RestoreConnections()
{
    JARLog("--------------------------------------------------------\n");
    JARLog("RestoreConnections size : %ld\n", fConnections.size());

    list<pair<string, string> >::const_iterator it;

    for (it = fConnections.begin(); it != fConnections.end(); it++) {
        pair<string, string> connection = *it;
        JARLog("connections : %s %s\n", connection.first.c_str(), connection.second.c_str());
        jack_connect(fClient, connection.first.c_str(), connection.second.c_str());
    }

    fConnections.clear();
}

//------------------------------------------------------------------------
TJackClient* TJackClient::GetJackClient()
{
    JARLog("--------------------------------------------------------\n");
    JARLog("GetJackClient\n");

    if (TJackClient::fJackClient) {
        return TJackClient::fJackClient;
    } else {
        TJackClient::fJackClient = new TJackClient();

        if (TJackClient::fJackClient->Open()) {
            return TJackClient::fJackClient;
        } else {
            delete TJackClient::fJackClient;
            TJackClient::fJackClient = NULL;
            return NULL;
        }
    }
}

//------------------------------------------------------------------------
void TJackClient::ClearJackClient()
{
    JARLog("--------------------------------------------------------\n");
    JARLog("ClearJackClient\n");

    if (TJackClient::fJackClient) {
        TJackClient::fJackClient->ClearIOProc();
        delete TJackClient::fJackClient;
        TJackClient::fJackClient = NULL;
    }
}

//------------------------------------------------------------------------
void TJackClient::KillJackClient()
{
    JARLog("--------------------------------------------------------\n");
    JARLog("KillJackClient\n");

    if (TJackClient::fJackClient) {
        TJackClient::fJackClient->Desactivate();
        TJackClient::fJackClient->Close();
        delete TJackClient::fJackClient;
		
        TJackClient::fInputChannels = 0;
		TJackClient::fOutputChannels = 0;

		TJackClient::fAutoConnect = true;
		TJackClient::fDeviceRunning = false;
		TJackClient::fConnected2HAL = false;
		TJackClient::fDebug = false;
		TJackClient::fDeviceID = 0;
		TJackClient::fJackClient = NULL;
		TJackClient::fCoreAudioDriver = 0;
		TJackClient::fPlugInRef = 0;
		TJackClient::fNotification = false;
		TJackClient::fFirstActivate = true;
    }
}

//------------------------------------------------------------------------
void TJackClient::CheckFirstRef()
{
    assert(TJackClient::fJackClient);

    if (TJackClient::fJackClient->fExternalClientNum + TJackClient::fJackClient->fInternalClientNum == 1) {
        TJackClient::fJackClient->Activate();
    }
}

//------------------------------------------------------------------------
void TJackClient::CheckLastRef()
{
    assert(TJackClient::fJackClient);

    if (TJackClient::fJackClient->fExternalClientNum + TJackClient::fJackClient->fInternalClientNum == 0) {
		TJackClient::fJackClient->Desactivate();
    }
}

//------------------------------------------------------------------------
void TJackClient::CloseLastRef()
{
    assert(TJackClient::fJackClient);

    if (TJackClient::fJackClient->fExternalClientNum + TJackClient::fJackClient->fInternalClientNum == 0) {
 		TJackClient::fJackClient->Close();
        delete TJackClient::fJackClient;
        TJackClient::fJackClient = NULL;
    }
}

//------------------------------------------------------------------------
void TJackClient::IncRefInternal()
{
    assert(TJackClient::fJackClient);
    TJackClient::fJackClient->fInternalClientNum++;
    JARLog("IncRefInternal : %ld\n", TJackClient::fJackClient->fInternalClientNum);
    if (TJackClient::fJackClient->fInternalClientNum == 1) {
        TJackClient::fJackClient->AllocatePorts();
		CheckFirstRef();   
    }
    //CheckFirstRef();
}

//------------------------------------------------------------------------
void TJackClient::DecRefInternal()
{
    assert(TJackClient::fJackClient);
    TJackClient::fJackClient->fInternalClientNum--;
    JARLog("DecRefInternal : %ld\n", TJackClient::fJackClient->fInternalClientNum);
    if (TJackClient::fJackClient->fInternalClientNum == 0) {
		CheckLastRef(); 
		TJackClient::fJackClient->DisposePorts();
	}
    //CheckLastRef();
	CloseLastRef();
}

//------------------------------------------------------------------------
void TJackClient::IncRefExternal()
{
    assert(TJackClient::fJackClient);
    TJackClient::fJackClient->fExternalClientNum++;
    JARLog("IncRefExternal : %ld\n", TJackClient::fJackClient->fExternalClientNum);
    CheckFirstRef();
}

//------------------------------------------------------------------------
void TJackClient::DecRefExternal()
{
    assert(TJackClient::fJackClient);
    TJackClient::fJackClient->fExternalClientNum--;
    JARLog("DecRefExternal : %ld\n", TJackClient::fJackClient->fExternalClientNum);
    CheckLastRef();
	CloseLastRef();
}

//------------------------------------------------------------------------
void TJackClient::Shutdown(void* arg)
{
    TJackClient::fDeviceRunning = false;
    KillJackClient();
    OSStatus err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                   TJackClient::fDeviceID,
                   0,
                   0,
                   kAudioDevicePropertyDeviceIsAlive);
    JARLog("Shutdown err %ld\n", err);
}

//---------------------------------------------------------------------------
void TJackClient::SetTime(AudioTimeStamp* timeVal, long curTime, UInt64 time)
{
    timeVal->mSampleTime = curTime;
    timeVal->mHostTime = time;
    timeVal->mRateScalar = 1.0;
    timeVal->mWordClockTime = 0;
    timeVal->mFlags = kAudioTimeFlags;
}

//------------------------------------------------------------------------
int TJackClient::XRun(void* arg)
{
    JARLog("XRun\n");
    AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                                       TJackClient::fDeviceID,
                                       0,
                                       0,
                                       kAudioDeviceProcessorOverload);
    return 0;
}

//------------------------------------------------------------------------
int TJackClient::BufferSize(jack_nframes_t nframes, void* arg)
{
    TJackClient* client = (TJackClient*)arg;
    JARLog("New BufferSize %ld\n", nframes);

    TJackClient::fBufferSize = nframes;

    for (long i = 0; i < TJackClient::fInputChannels; i++) {
        client->fInputList->mBuffers[i].mNumberChannels = 1;
        client->fInputList->mBuffers[i].mDataByteSize = TJackClient::fBufferSize * sizeof(float);
    }

    for (long i = 0; i < TJackClient::fOutputChannels; i++) {
        client->fOutputList->mBuffers[i].mNumberChannels = 1;
        client->fOutputList->mBuffers[i].mDataByteSize = TJackClient::fBufferSize * sizeof(float);
        free(client->fOuputListTemp[i]);
        client->fOuputListTemp[i] = (float *)malloc(sizeof(float) * TJackClient::fBufferSize);
    }

    AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                                       TJackClient::fDeviceID,
                                       0,
                                       true,
                                       kAudioDevicePropertyBufferFrameSize);

    AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                                       TJackClient::fDeviceID,
                                       0,
                                       false,
                                       kAudioDevicePropertyBufferFrameSize);

    AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                                       TJackClient::fDeviceID,
                                       0,
                                       true,
                                       kAudioDevicePropertyBufferSize);

    AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                                       TJackClient::fDeviceID,
                                       0,
                                       false,
                                       kAudioDevicePropertyBufferSize);

    return 0;
}

/* Jack Process callback */
//------------------------------------------------------------------------
int TJackClient::Process(jack_nframes_t nframes, void* arg)
{
    OSStatus err;
    AudioTimeStamp inNow;
    AudioTimeStamp inInputTime;
    AudioTimeStamp inOutputTime;

    TJackClient* client = (TJackClient*)arg;
    UInt64 time = AudioGetCurrentHostTime();
    UInt64 curTime = jack_frame_time(client->fClient);

    SetTime(&inNow, curTime, time);
    SetTime(&inInputTime, curTime - TJackClient::fBufferSize, time);
    SetTime(&inOutputTime, curTime + TJackClient::fBufferSize, time);

    // One IOProc
    if (client->GetProcNum() == 1) {

        pair<AudioDeviceIOProc, TProcContext> val = *client->fAudioIOProcList.begin();
        TProcContext context = val.second;

        if (context.fStatus) { // If proc is started

            for (int i = 0; i < TJackClient::fOutputChannels; i++) {
                // Use an intermediate buffer
                memset(client->fOuputListTemp[i], 0, nframes * sizeof(float));
            }

            if (context.fStreamUsage) {

                // Only set up buffers that are really needed
                for (int i = 0; i < TJackClient::fInputChannels; i++) {
                    if (context.fInput[i] && client->fInputPortList[i]) {
                        client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
                    } else {
                        client->fInputList->mBuffers[i].mData = NULL;
                    }
                }

                for (int i = 0; i < TJackClient::fOutputChannels; i++) {
                    if (context.fOutput[i]) {
                        client->fOutputList->mBuffers[i].mData = client->fOuputListTemp[i];
                    } else {
                        client->fOutputList->mBuffers[i].mData = NULL;
                    }
                }

            } else {
                // Non Interleaved
                for (int i = 0; i < TJackClient::fInputChannels; i++) {
                    client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
                }

                for (int i = 0; i < TJackClient::fOutputChannels; i++) {
                    client->fOutputList->mBuffers[i].mData = client->fOuputListTemp[i];
                }
            }

            err = (val.first) (client->fDeviceID,
                               &inNow,
                               client->fInputList,
                               &inInputTime,
                               client->fOutputList,
                               &inOutputTime,
                               context.fContext);

            if (TJackClient::fDebug) {
                if (err != kAudioHardwareNoError)
                    JARLog("Process error %ld\n", err);
            }

            // Copy intermediate buffer in client buffer
            for (int i = 0; i < TJackClient::fOutputChannels; i++) {
				if (client->fOutputPortList[i])
					memcpy((float*)jack_port_get_buffer(client->fOutputPortList[i], nframes), client->fOuputListTemp[i], nframes * sizeof(float));
            }
        }

    } else if (client->GetProcNum() > 1) { // Several IOProc : need mixing

        for (int i = 0; i < TJackClient::fOutputChannels; i++) {
            // Use a intermediate mixing buffer
            memset(client->fOuputListTemp[i], 0, nframes * sizeof(float));
        }
		
	    map<AudioDeviceIOProc, TProcContext>::iterator iter;
        int k;
        for (k = 1, iter = client->fAudioIOProcList.begin(); iter != client->fAudioIOProcList.end(); iter++, k++) {

            pair<AudioDeviceIOProc, TProcContext> val = *iter;
            TProcContext context = val.second;

            if (context.fStatus) { // If proc is started

                if (context.fStreamUsage) {

                    // Only set up buffers that are really needed
                    for (int i = 0; i < TJackClient::fInputChannels; i++) {
                        if (context.fInput[i]) {
                            client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
                        } else {
                            client->fInputList->mBuffers[i].mData = NULL;
                        }
                    }

                    for (int i = 0; i < TJackClient::fOutputChannels; i++) {
                        // Use an intermediate mixing buffer
                        if (context.fOutput[i]) {
							client->fOutputList->mBuffers[i].mData = client->fOuputListTemp[i];
                        } else {
                            client->fOutputList->mBuffers[i].mData = NULL;
                        }
                    }

                } else {

                    for (int i = 0; i < TJackClient::fInputChannels; i++) {
                        client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
                    }

                    for (int i = 0; i < TJackClient::fOutputChannels; i++) {
                        // Use an intermediate mixing buffer
                        client->fOutputList->mBuffers[i].mData = client->fOuputListTemp[i];
                    }
                }

                err = (val.first) (client->fDeviceID,
                                   &inNow,
                                   client->fInputList,
                                   &inInputTime,
                                   client->fOutputList,
                                   &inOutputTime,
                                   context.fContext);

                if (TJackClient::fDebug) {
                    if (err != kAudioHardwareNoError)
                        JARLog("Process error %ld\n", err);
                }

                // Only mix buffers that are really needed
                if (context.fStreamUsage) {
                    for (int i = 0; i < TJackClient::fOutputChannels; i++) {
                        
                        if (context.fOutput[i]) {
							float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
							if (k == 1) {	// first proc : copy
                                memcpy(output, (float*)client->fOutputList->mBuffers[i].mData, nframes * sizeof(float));
                            } else { // other proc : mix
                                for (unsigned int j = 0; j < nframes; j++) {
                                    output[j] += ((float*)client->fOutputList->mBuffers[i].mData)[j];
                                }
                            }
                        } else {
                			if (client->fOutputPortList[i] && k == 1) {
								float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
								memset(output, 0, nframes * sizeof(float));
							}
                        }
                    }

                } else {
                    for (int i = 0; i < TJackClient::fOutputChannels; i++) {
                        float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
                        if (k == 1) {	// first proc : copy
                            memcpy(output, (float*)client->fOutputList->mBuffers[i].mData, nframes * sizeof(float));
                        } else { // other proc : mix
                            for (unsigned int j = 0; j < nframes; j++) {
                                output[j] += ((float*)client->fOutputList->mBuffers[i].mData)[j];
                            }
                        }
                    }
                }
            }
        }
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

//------------------------------------------------------------------------
TJackClient::TJackClient()
{
    JARLog("TJackClient constructor\n");

    fInputList = (AudioBufferList*)malloc(sizeof(UInt32) + sizeof(AudioBuffer) * TJackClient::fInputChannels);
    assert(fInputList);
    fOutputList = (AudioBufferList*)malloc(sizeof(UInt32) + sizeof(AudioBuffer) * TJackClient::fOutputChannels);
    assert(fOutputList);

    fInputList->mNumberBuffers = TJackClient::fInputChannels;
    fOutputList->mNumberBuffers = TJackClient::fOutputChannels;

    fOuputListTemp = (float**)malloc(sizeof(float*) * TJackClient::fOutputChannels);
    assert(fOuputListTemp);

    for (int i = 0; i < TJackClient::fOutputChannels; i++) {
        fOuputListTemp[i] = (float*)malloc(sizeof(float) * TJackClient::fBufferSize);
		assert(fOuputListTemp[i]);
        memset(fOuputListTemp[i], 0, TJackClient::fBufferSize * sizeof(float));
     }

    for (int i = 0; i < MAX_JACK_PORTS; i++) {
        fInputPortList[i] = NULL;
        fOutputPortList[i] = NULL;
    }

    fProcRunning = 0;
    fExternalClientNum = 0;
    fInternalClientNum = 0;
    fActivated = false;
}

//------------------------------------------------------------------------
TJackClient::~TJackClient()
{
    JARLog("TJackClient destructor\n");

    free(fInputList);
    free(fOutputList);

    for (int i = 0; i < TJackClient::fOutputChannels; i++)
        free(fOuputListTemp[i]);
    free(fOuputListTemp);
}

//------------------------------------------------------------------------
bool TJackClient::AllocatePlugInPortVST(int num)
{
    JARLog("AllocatePlugInPortVST %ld\n", num + 1);
    char name[256];
    sprintf(name, "VSTsend%d", num + 1);
    jack_port_t* port = jack_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;

    float* buffer = (float*)malloc(sizeof(float) * TJackClient::fBufferSize);
    if (!buffer)
        return false;

    memset(buffer, 0, TJackClient::fBufferSize * sizeof(float));
    fPlugInPortsVST[num] = make_pair(buffer, port);
    return true;
}

//------------------------------------------------------------------------
bool TJackClient::AllocatePlugInPortAU(int num)
{
    JARLog("AllocatePlugInPortAU %ld\n", num + 1);
    char name[256];
    sprintf(name, "AUsend%d", num + 1);
    jack_port_t* port = jack_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;

    float* buffer = (float*)malloc(sizeof(float) * TJackClient::fBufferSize);
    if (!buffer)
        return false;

    memset(buffer, 0, TJackClient::fBufferSize * sizeof(float));
    fPlugInPortsAU[num] = make_pair(buffer, port);
    return true;
}

//------------------------------------------------------------------------
float* TJackClient::GetPlugInPortVST(int num)
{
    JARLog("GetPlugInPortVST %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsVST[num];
    return obj.first;
}

//------------------------------------------------------------------------
float* TJackClient::GetPlugInPortAU(int num)
{
    JARLog("GetPlugInPortAU %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsAU[num];
    return obj.first;
}

//------------------------------------------------------------------------
void TJackClient::ReleasePlugInPortVST(int num)
{
    JARLog("ReleasePlugInPortVST %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsVST[num];
    assert(obj.first);
    assert(obj.second);
    free(obj.first);
    jack_port_unregister(fClient, obj.second);
    fPlugInPortsVST.erase(num); /// TO CHECK : RT access ??
}

//------------------------------------------------------------------------
void TJackClient::ReleasePlugInPortAU(int num)
{
    JARLog("ReleasePlugInPortAU %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsAU[num];
    assert(obj.first);
    assert(obj.second);
    free(obj.first);
    jack_port_unregister(fClient, obj.second);
    fPlugInPortsAU.erase(num); /// TO CHECK : RT access ??
}

//------------------------------------------------------------------------
bool TJackClient::AllocatePorts()
{
    char in_port_name [JACK_PORT_NAME_LEN];

    JARLog("AllocatePorts fInputChannels %ld fOutputChannels %ld \n", fInputChannels, fOutputChannels);

    for (long i = 0; i < TJackClient::fInputChannels; i++) {
        sprintf(in_port_name, "in%ld", i + 1);
        if ((fInputPortList[i] = jack_port_register(fClient, in_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL)
            goto error;
        fInputList->mBuffers[i].mNumberChannels = 1;
        fInputList->mBuffers[i].mDataByteSize = TJackClient::fBufferSize * sizeof(float);
    }

    char out_port_name [JACK_PORT_NAME_LEN];

    for (long i = 0; i < TJackClient::fOutputChannels; i++) {
        sprintf(out_port_name, "out%ld", i + 1);
        if ((fOutputPortList[i] = jack_port_register(fClient, out_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL)
            goto error;
        fOutputList->mBuffers[i].mNumberChannels = 1;
        fOutputList->mBuffers[i].mDataByteSize = TJackClient::fBufferSize * sizeof(float);
    }

    return true;

error:

    JARLog("Cannot register ports\n");
    DisposePorts();
    return false;
}

//------------------------------------------------------------------------
void TJackClient::DisposePorts()
{
    JARLog("DisposePorts\n");
    //SaveConnections();

    for (long i = 0; i < TJackClient::fInputChannels; i++) {
        if (fInputPortList[i]) {
			JARLog("DisposePorts input %ld\n",i);
            jack_port_unregister(fClient, fInputPortList[i]);
            fInputPortList[i] = 0;
        }
    }

    for (long i = 0; i < TJackClient::fOutputChannels; i++) {
        if (fOutputPortList[i]) {
			JARLog("DisposePorts output %ld\n",i);
            jack_port_unregister(fClient, fOutputPortList[i]);
            fOutputPortList[i] = 0;
        }
    }
}

//------------------------------------------------------------------------
void TJackClient::ClearIOProc()
{
    fAudioIOProcList.clear();
}

//------------------------------------------------------------------------
long TJackClient::GetProcNum()
{
    return fAudioIOProcList.size();
}

//------------------------------------------------------------------------
bool TJackClient::IsRunning()
{
    return fProcRunning > 0;
}

//------------------------------------------------------------------------
void TJackClient::IncRunning()
{
    fProcRunning++;
}

//------------------------------------------------------------------------
void TJackClient::DecRunning()
{
    fProcRunning--;
    if (fProcRunning < 0)
        fProcRunning = 0;
}

//------------------------------------------------------------------------
void TJackClient::StopRunning()
{
    fProcRunning = 0;
}

//------------------------------------------------------------------------
bool TJackClient::Open()
{
    pid_t pid = getpid();
	jack_options_t options = JackNullOption;
	jack_status_t status;
    char* id_name = bequite_getNameFromPid(pid);
    JARLog("JackClient::Open id %ld name %s\n", pid, id_name);
    assert(id_name != NULL);

    if ((fClient = jack_client_open(id_name, options, &status)) == NULL) {
        JARLog("TJackClient::Open jack server not running?\n");
        return false;
    } else {
		jack_set_process_callback(fClient, Process, this);
		jack_on_shutdown(fClient, Shutdown, NULL);
		jack_set_buffer_size_callback(fClient, BufferSize, this);
		jack_set_xrun_callback(fClient, XRun, this);
		return true;
	}
}

//------------------------------------------------------------------------
void TJackClient::Close()
{
    JARLog("Close\n");

    if (fClient) {
        if (jack_client_close(fClient) != 0) {
            JARLog("Cannot close client\n");
        }
    }
}

//------------------------------------------------------------------------
bool TJackClient::AddIOProc(AudioDeviceIOProc proc, void* context)
{
    if (fAudioIOProcList.find(proc) == fAudioIOProcList.end()) {
	    fAudioIOProcList.insert(make_pair(proc, TProcContext(context)));
        JARLog("AddIOProc fAudioIOProcList.size %ld \n", fAudioIOProcList.size());
        return true;
    } else {
        JARLog("AddIOProc proc already added %x \n", proc);
        return false;
    }
}

//------------------------------------------------------------------------
bool TJackClient::RemoveIOProc(AudioDeviceIOProc proc)
{
    if (fAudioIOProcList.find(proc) != fAudioIOProcList.end()) {
        fAudioIOProcList.erase(proc);
        JARLog("fAudioIOProcList size %ld \n", fAudioIOProcList.size());
        return true;
    } else {
        JARLog("RemoveIOProc proc not present %x \n", proc);
        return false;
    }
}

//------------------------------------------------------------------------
void TJackClient::Start(AudioDeviceIOProc proc)
{
    if (proc == NULL) { // is supposed to start the hardware
        IncRunning();
        AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                                           TJackClient::fDeviceID,
                                           0,
                                           0,
                                           kAudioDevicePropertyDeviceIsRunning);
    } else {
        map<AudioDeviceIOProc, TProcContext>::iterator iter = fAudioIOProcList.find(proc);
        if (iter != fAudioIOProcList.end()) {
            if (!iter->second.fStatus) { // check multiple start of the proc
                iter->second.fStatus = true;
                IncRunning();
                AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                                                   TJackClient::fDeviceID,
                                                   0,
                                                   0,
                                                   kAudioDevicePropertyDeviceIsRunning);
            } else {
                JARLog("TJackClient::Start proc already started %x\n", proc);
            }
        } else {
            JARLog("Start proc %x not found\n", proc);
        }
    }
}

//------------------------------------------------------------------------
void TJackClient::Stop(AudioDeviceIOProc proc)
{
    // clear ouput buffers
    for (int i = 0; i < TJackClient::fOutputChannels; i++) {
		if (fOutputPortList[i])
			memset((float *)jack_port_get_buffer(fOutputPortList[i], TJackClient::fBufferSize), 0, TJackClient::fBufferSize * sizeof(float));
    }

    if (proc == NULL) { // is supposed to stop the hardware
        StopRunning();
        AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                                           TJackClient::fDeviceID,
                                           0,
                                           0,
                                           kAudioDevicePropertyDeviceIsRunning);
    } else {
        map<AudioDeviceIOProc, TProcContext>::iterator iter = fAudioIOProcList.find(proc);
        if (iter != fAudioIOProcList.end()) {
            if (iter->second.fStatus) { // check multiple stop of the proc
                iter->second.fStatus = false;
                DecRunning();
                AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                                                   TJackClient::fDeviceID,
                                                   0,
                                                   0,
                                                   kAudioDevicePropertyDeviceIsRunning);
            } else {
                JARLog("TJackClient::Stop proc already stopped %x\n", proc);
            }
        } else {
            JARLog("Stop : proc %x not found\n", proc);
        }
    }
}

//------------------------------------------------------------------------
bool TJackClient::Activate()
{
    JARLog("Activate\n");

    if (jack_activate(fClient)) {
        JARLog("cannot activate client");
        return false;
    } else {
        // Autoconnect is only done for the first activation
        if (TJackClient::fFirstActivate) {
            JARLog("First activate\n");
            AutoConnect();
            TJackClient::fFirstActivate = false;
        } else {
            RestoreConnections();
        }
        fActivated = true;
        return true;
    }
}

//------------------------------------------------------------------------
bool TJackClient::Desactivate()
{
    JARLog("Desactivate\n");
	SaveConnections();

    if (jack_deactivate(fClient)) {
        JARLog("cannot deactivate client\n");
        return false;
    }
    fActivated = false;
    return true;
}

//------------------------------------------------------------------------
void TJackClient::SaveActivate()
{
    JARLog("SaveActivate\n");
    if (fActivated) {
        SaveConnections();
        if (jack_deactivate(fClient))
            JARLog("cannot deactivate client\n");
    }
}

//------------------------------------------------------------------------
void TJackClient::RestoreActivate()
{
    JARLog("RestoreActivate\n");
    if (fActivated) {
        if (jack_activate(fClient))
            JARLog("cannot activate client\n");
        else
            RestoreConnections();
    }
}

//------------------------------------------------------------------------
bool TJackClient::AutoConnect()
{
    const char **ports;

    if (TJackClient::fAutoConnect) {

        if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsOutput)) == NULL) {
            JARLog("cannot find any physical capture ports\n");
        } else {

            for (int i = 0; i < TJackClient::fInputChannels; i++) {
                if (TJackClient::fDebug) {
                    if (ports[i])
                        JARLog("ports[i] %s\n", ports[i]);
                    if (fInputPortList[i] && jack_port_name(fInputPortList[i]))
                        JARLog("jack_port_name(fInputPortList[i]) %s\n", jack_port_name(fInputPortList[i]));
                }

                // Stop condition
                if (ports[i] == 0)
                    break;

                if (fInputPortList[i] && jack_port_name(fInputPortList[i])) {
                    if (jack_connect(fClient, ports[i], jack_port_name(fInputPortList[i]))) {
                        JARLog("cannot connect input ports\n");
                    }
                } else {
                    JARLog("AutoConnect input: i %ld fInputPortList[i] %x\n", i, fInputPortList[i]);
                    goto error;
                }

            }
            free (ports);
        }

        if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsInput)) == NULL) {
            JARLog("cannot find any physical playback ports\n");
        } else {

            for (int i = 0; i < TJackClient::fOutputChannels; i++) {
                if (TJackClient::fDebug) {
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
                } else {
                    JARLog("AutoConnect output: i %ld fOutputPortList[i] %x\n", i, fOutputPortList[i]);
                    goto error;
                }
            }
            free (ports);
        }
    }

    return true;

error:
    JARLog("AutoConnect error\n");
    return false;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceAddIOProc(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, AudioDeviceIOProc proc, void* context)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceAddIOProc called inSelf, proc %ld %x \n", (long)inSelf, proc);

	if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;
    TJackClient* client = TJackClient::GetJackClient();

    if (client) {
        JARLog("DeviceAddIOProc : add a new proc\n");
        if (client->AddIOProc(proc, context))
            IncRefInternal();
        return kAudioHardwareNoError;
    } else {
        JARLog("DeviceAddIOProc : no client \n");
        return kAudioHardwareBadDeviceError;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceRemoveIOProc(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, AudioDeviceIOProc proc)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceRemoveIOProc called inSelf, proc %ld %x \n", (long)inSelf, proc);

    if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;
	TJackClient* client = TJackClient::fJackClient;

    JARLog("DeviceRemoveIOProc %x client\n", client);

    if (client) {
        if (client->RemoveIOProc(proc))
            DecRefInternal();
        return kAudioHardwareNoError;
    } else {
        JARLog("DeviceRemoveIOProc : no client \n");
        return kAudioHardwareBadDeviceError;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceStart(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, AudioDeviceIOProc proc)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceStart called inSelf, proc %ld %x \n", (long)inSelf, proc);

    if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;
    TJackClient* client = TJackClient::fJackClient;

    if (client) {
        client->Start(proc);
        JARLog("DeviceStart fProcRunning %ld \n", client->fProcRunning);
    } else {
        JARLog("DeviceStart error : null client\n");
        return kAudioHardwareBadDeviceError;
    }

    return kAudioHardwareNoError;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceStop(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, AudioDeviceIOProc proc)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceStop called inSelf, proc %ld %x \n", (long)inSelf, proc);

     if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;
	TJackClient* client = TJackClient::fJackClient;

    if (client) {
        client->Stop(proc);
        JARLog("DeviceStop fProcRunning %ld \n", client->fProcRunning);
    } else {
        JARLog("DeviceStop error : null client\n");
        return kAudioHardwareBadDeviceError;
    }

    return kAudioHardwareNoError;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceRead(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, const AudioTimeStamp* inStartTime, AudioBufferList* outData)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceRead : not yet implemented\n");
    return kAudioHardwareUnsupportedOperationError;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceGetCurrentTime(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, AudioTimeStamp* outTime)
{
    if (TJackClient::fJackClient) {
        outTime->mSampleTime = jack_frame_time(TJackClient::fJackClient->fClient);
        outTime->mHostTime = AudioGetCurrentHostTime();
        outTime->mRateScalar = 1.0;
        outTime->mWordClockTime = 0;
        outTime->mFlags = kAudioTimeFlags;
        return kAudioHardwareNoError;
    } else
        return kAudioHardwareNotRunningError;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceTranslateTime(AudioHardwarePlugInRef inSelf,
        AudioDeviceID inDevice,
        const AudioTimeStamp* inTime,
        AudioTimeStamp* outTime)
{
	// Simply copy inTime ==> outTime for now
	memcpy(outTime, inTime, sizeof(AudioTimeStamp));
	return kAudioHardwareNoError;
}

//---------------------------------------------------------------------------------------------------------------------------------
// Device Property Management
//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceGetPropertyInfo(AudioHardwarePlugInRef inSelf,
        AudioDeviceID inDevice,
        UInt32 inChannel,
        Boolean isInput,
        AudioDevicePropertyID inPropertyID,
        UInt32* outSize,
        Boolean* outWritable)
{
    OSStatus err = kAudioHardwareNoError;

    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceGetPropertyInfo inSelf inDevice inChannel isInput  %ld %ld %ld %ld %ld %ld \n", (long)inSelf, inDevice, inChannel, isInput, outSize, outWritable);

	if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;
		
    Print4CharCode("DeviceGetPropertyInfo ", inPropertyID);

    if (inDevice != TJackClient::fDeviceID) {
        JARLog("DeviceGetPropertyInfo called for invalid device ID\n");
        return kAudioHardwareBadDeviceError;
    }

    if (outSize == NULL) {
        JARLog("DeviceGetPropertyInfo received NULL outSize pointer\n");
    }

    if (outWritable != NULL) {
        *outWritable = false;
    } else {
        JARLog("DeviceGetPropertyInfo received NULL outWritable pointer\n");
    }

    switch (inPropertyID) {
#ifdef kAudioHardwarePlugInInterface2ID
            // For applications that needs a output channels map, and for audio midi setup (that will not crash)
        case kAudioDevicePropertyPreferredChannelLayout:
     		if (outSize)
				if (isInput)
					*outSize = offsetof(AudioChannelLayout, mChannelDescriptions) + sizeof(AudioChannelDescription) * TJackClient::fInputChannels;
				else
					*outSize = offsetof(AudioChannelLayout, mChannelDescriptions) + sizeof(AudioChannelDescription) * TJackClient::fOutputChannels;
            break;
#endif

        case kAudioDevicePropertyDeviceName:
            if (outSize)
                *outSize = TJackClient::fDeviceName.size() + 1;
            break;

        case kAudioDevicePropertyDeviceManufacturer:
            if (outSize)
                *outSize = TJackClient::fDeviceManufacturer.size() + 1;
            break;

        case kAudioDevicePropertyDeviceNameCFString:
        case kAudioDevicePropertyDeviceUID:
        case kAudioDevicePropertyDeviceManufacturerCFString:
            if (outSize)
                *outSize = sizeof(CFStringRef); // TO BE CHECKED
            break;

        case kAudioDevicePropertyPlugIn:
            if (outSize)
                *outSize = sizeof(OSStatus);
            break;

        case kAudioDevicePropertyTransportType:
        case kAudioDevicePropertyDeviceIsAlive:
        case kAudioDevicePropertyDeviceIsRunning:
        case kAudioDevicePropertyDeviceIsRunningSomewhere:
        case kAudioDevicePropertyDeviceCanBeDefaultDevice:
        case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
        case kAudioDeviceProcessorOverload:
            if (outSize)
                *outSize = sizeof(UInt32);
            break;

        case kAudioDevicePropertyHogMode:
            if (outSize)
                *outSize = sizeof(pid_t);
            break;

        case kAudioDevicePropertyRegisterBufferList:
            if (outSize)
                *outSize = 4 + sizeof(AudioBuffer) * (TJackClient::fOutputChannels + TJackClient::fInputChannels);
            break;

        case kAudioDevicePropertyLatency:
            if (outSize)
                *outSize = sizeof(UInt32);
            break;

        case kAudioDevicePropertyBufferSize:
        case kAudioDevicePropertyBufferFrameSize:
            if (outSize)
                *outSize = sizeof(UInt32);
            if (outWritable)
                *outWritable = true;
            break;

        case kAudioDevicePropertyUsesVariableBufferFrameSizes:
			if (outSize)
                *outSize = *outSize = sizeof(UInt32);
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioDevicePropertyJackIsConnected:
			if (outSize)
                *outSize = *outSize = sizeof(UInt32);
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioDevicePropertyBufferSizeRange:
        case kAudioDevicePropertyBufferFrameSizeRange:
            if (outSize)
                *outSize = sizeof(AudioValueRange);
            if (outWritable)
                *outWritable = true;
            break;

        case kAudioDevicePropertyStreams:
            if (outSize) {
                if (isInput)
                    *outSize = sizeof(AudioStreamID) * TJackClient::fInputChannels;
                else
                    *outSize = sizeof(AudioStreamID) * TJackClient::fOutputChannels;
            }
            break;

        case kAudioDevicePropertySafetyOffset:
        case kAudioDevicePropertySupportsMixing:
            if (outSize)
                *outSize = sizeof(UInt32);
            break;

        case kAudioDevicePropertyStreamConfiguration:
            if (outSize) {
                if (isInput)
                    *outSize = sizeof(UInt32) + sizeof(AudioBuffer) * TJackClient::fInputChannels;
                else
                    *outSize = sizeof(UInt32) + sizeof(AudioBuffer) * TJackClient::fOutputChannels;
                JARLog("DeviceGetPropertyInfo::kAudioDevicePropertyStreamConfiguration %ld\n", *outSize);
            }
            break;

        case kAudioDevicePropertyIOProcStreamUsage:
            if (outSize) {
                if (isInput)
                    *outSize = sizeof(void*) + sizeof(UInt32) + sizeof(UInt32) * TJackClient::fInputChannels;
                else
                    *outSize = sizeof(void*) + sizeof(UInt32) + sizeof(UInt32) * TJackClient::fOutputChannels;
                JARLog("DeviceGetPropertyInfo::kAudioDevicePropertyIOProcStreamUsage %ld %ld\n", isInput, *outSize);
            }
            if (outWritable)
                *outWritable = true;
            break;

        case kAudioDevicePropertyPreferredChannelsForStereo:
            if (outSize)
                *outSize = sizeof(stereoList);
            break;

        case kAudioDevicePropertyNominalSampleRate:
            if (outSize)
                *outSize = sizeof(Float64);
            if (outWritable)
                *outWritable = true;
            break;

        case kAudioDevicePropertyActualSampleRate:
            if (outSize)
                *outSize = sizeof(Float64);
            if (outWritable)
                *outWritable = true;
            break;

        case kAudioDevicePropertyAvailableNominalSampleRates:
            if (outSize)
                *outSize = sizeof(AudioValueRange);
            break;

        case kAudioStreamPropertyPhysicalFormat:
        case kAudioDevicePropertyStreamFormatSupported:
        case kAudioDevicePropertyStreamFormatMatch:
            if (outSize)
                *outSize = sizeof(AudioStreamBasicDescription);
            break;

        case kAudioDevicePropertyStreamFormat:
            if (outSize)
                *outSize = sizeof(AudioStreamBasicDescription);
            if (outWritable)
                *outWritable = true;
            break;

        case kAudioDevicePropertyStreamFormats:          // TO BE CHECKED
        case kAudioStreamPropertyPhysicalFormats:
            if (outSize)
                *outSize = sizeof(AudioStreamBasicDescription);
            break;

        case kAudioDevicePropertyDataSource:
        case kAudioDevicePropertyDataSources:
			if (outSize)
                *outSize = sizeof(UInt32);
            break;

        case kAudioDevicePropertyDataSourceNameForID:
        case kAudioDevicePropertyDataSourceNameForIDCFString:
			if (outSize)
                *outSize = sizeof(AudioValueTranslation);
			break;

        case kAudioDevicePropertyConfigurationApplication:
            if (outSize)
                *outSize = sizeof(CFStringRef);
            break;

            // Special Property to access Jack client from application code
        case kAudioDevicePropertyGetJackClient:
        case kAudioDevicePropertyReleaseJackClient:
            if (outSize)
                *outSize = sizeof(jack_client_t*);
            break;

        case kAudioDevicePropertyAllocateJackPortVST:
        case kAudioDevicePropertyAllocateJackPortAU:
        case kAudioDevicePropertyReleaseJackPortVST:
        case kAudioDevicePropertyReleaseJackPortAU:
        case kAudioDevicePropertyDeactivateJack:
        case kAudioDevicePropertyActivateJack:
            if (outSize)
                *outSize = sizeof(UInt32);
            if (outWritable)
                *outWritable = true;
            break;

        case kAudioDevicePropertyGetJackPortVST:
        case kAudioDevicePropertyGetJackPortAU:
            if (outSize)
                *outSize = sizeof(float*);
            break;

            // Redirect call on used CoreAudio driver
        case kAudioDevicePropertyClockSource:
        case kAudioDevicePropertyClockSources:
        case kAudioDevicePropertyClockSourceNameForID:
        case kAudioDevicePropertyClockSourceNameForIDCFString:
        case kAudioHardwarePropertyBootChimeVolumeScalar:
        case kAudioDevicePropertyDriverShouldOwniSub:
        case kAudioDevicePropertyVolumeScalar:
        case kAudioDevicePropertyVolumeDecibels:
        case kAudioDevicePropertyVolumeRangeDecibels:
        case kAudioDevicePropertyVolumeScalarToDecibels:
        case kAudioDevicePropertyVolumeDecibelsToScalar:
        case kAudioDevicePropertyMute:
        case kAudioDevicePropertyPlayThru:
        case kAudioDevicePropertySubVolumeScalar:
        case kAudioDevicePropertySubVolumeDecibels:
        case kAudioDevicePropertySubVolumeRangeDecibels:
        case kAudioDevicePropertySubVolumeScalarToDecibels:
        case kAudioDevicePropertySubVolumeDecibelsToScalar:
        case kAudioDevicePropertySubMute: {
                JARLog("Redirect call on used CoreAudio driver ID %ld \n", TJackClient::fCoreAudioDriver);
                Print4CharCode("property ", inPropertyID);
                err = AudioDeviceGetPropertyInfo(TJackClient::fCoreAudioDriver, inChannel, isInput, inPropertyID, outSize, outWritable);
                JARLog("Redirect call on used CoreAudio driver err %ld \n", err);
                printError(err);
                break;
            }

        default:
            Print4CharCode("DeviceGetPropertyInfo unkown request:", inPropertyID);
            err = kAudioHardwareUnknownPropertyError;
            break;
    }

    return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceGetProperty(AudioHardwarePlugInRef inSelf,
                                        AudioDeviceID inDevice,
                                        UInt32 inChannel,
                                        Boolean isInput,
                                        AudioDevicePropertyID inPropertyID,
                                        UInt32* ioPropertyDataSize,
                                        void* outPropertyData)
{
    OSStatus err = kAudioHardwareNoError;

	if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;

    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceGetProperty inSelf isInput inDevice %ld %ld %ld\n", (long)inSelf, isInput, inDevice);
    Print4CharCode("DeviceGetProperty ", inPropertyID);

    if (inDevice != TJackClient::fDeviceID) {
        JARLog("DeviceGetProperty called for invalid device ID\n");
        return kAudioHardwareBadDeviceError;
    }

    switch (inPropertyID) {

#ifdef kAudioHardwarePlugInInterface2ID
            // For applications that needs an output channels map, and for audio midi setup (that will not crash)
        case kAudioDevicePropertyPreferredChannelLayout: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioChannelLayout);
                } else if (*ioPropertyDataSize < sizeof(AudioChannelLayout)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    AudioChannelLayout *res = (AudioChannelLayout*)outPropertyData;
                    res->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
                    res->mChannelBitmap = 0;
                    res->mNumberChannelDescriptions = TJackClient::fOutputChannels;

                    int firstType = 1; //see line 325 of CoreAudioTypes.h (panther)

                    for (int i = 0;i < TJackClient::fOutputChannels;i++) {
                        res->mChannelDescriptions[i].mChannelLabel = firstType;
                        firstType++;
                    }

                    *ioPropertyDataSize = sizeof(AudioChannelLayout);
                }
                break;
            }
#endif

        case kAudioDevicePropertyDeviceName: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = TJackClient::fDeviceName.size() + 1;
                } else if (*ioPropertyDataSize < TJackClient::fDeviceName.size() + 1) {
                    err = kAudioHardwareBadPropertySizeError;
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                } else {
                    char* data = (char*) outPropertyData;
                    strcpy(data, TJackClient::fDeviceName.c_str());
                    *ioPropertyDataSize = TJackClient::fDeviceName.size() + 1;
                }
                break;
            }

        case kAudioDevicePropertyDeviceNameCFString: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(CFStringRef);
                } else if (*ioPropertyDataSize < sizeof (CFStringRef)) {
                    err = kAudioHardwareBadPropertySizeError;
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                } else {
                    CFStringRef* outString = (CFStringRef*) outPropertyData;
                    *outString = CFStringCreateWithCString(NULL, TJackClient::fDeviceName.c_str(), CFStringGetSystemEncoding());
                    *ioPropertyDataSize = sizeof(CFStringRef);
                }
                break;
            }

        case kAudioDevicePropertyDeviceManufacturer: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = TJackClient::fDeviceManufacturer.size() + 1;
                } else if (*ioPropertyDataSize < TJackClient::fDeviceManufacturer.size() + 1) {
                    err = kAudioHardwareBadPropertySizeError;
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                } else {
                    char* data = (char*) outPropertyData;
                    strcpy(data, TJackClient::fDeviceManufacturer.c_str());
                    *ioPropertyDataSize = TJackClient::fDeviceManufacturer.size() + 1;
                    err = kAudioHardwareNoError;
                }
                break;
            }

        case kAudioDevicePropertyDeviceManufacturerCFString: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(CFStringRef);
                } else if (*ioPropertyDataSize < sizeof(CFStringRef)) {
                    err = kAudioHardwareBadPropertySizeError;
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                } else {
                    CFStringRef* outString = (CFStringRef*) outPropertyData;
                    *outString = CFStringCreateWithCString(NULL, TJackClient::fDeviceManufacturer.c_str(), CFStringGetSystemEncoding());
                    *ioPropertyDataSize = sizeof(CFStringRef);
                }
                break;
            }

        case kAudioDevicePropertyDeviceUID: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(CFStringRef);
                } else if (*ioPropertyDataSize < sizeof(CFStringRef)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    CFStringRef* outString = (CFStringRef*) outPropertyData;
                    *outString = CFStringCreateWithCString(kCFAllocatorDefault, "JackRouter:0", CFStringGetSystemEncoding());
                    *ioPropertyDataSize = sizeof(CFStringRef);
                }
                break;
            }

        case kAudioDevicePropertyTransportType: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    //*(UInt32*) outPropertyData = kIOAudioDeviceTransportTypeBuiltIn; // steph
                    *(UInt32*) outPropertyData = kIOAudioDeviceTransportTypeOther;
                    *ioPropertyDataSize = sizeof(UInt32);
                }
                break;
            }

        case kAudioDevicePropertyDeviceIsAlive: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(UInt32*) outPropertyData = TJackClient::fDeviceRunning;
                    *ioPropertyDataSize = sizeof(UInt32);
                }
                break;
            }

        case kAudioDevicePropertyBufferFrameSize: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(UInt32*) outPropertyData = TJackClient::fBufferSize;
                    *ioPropertyDataSize = sizeof(UInt32);
                }
                break;
            }

        case kAudioDevicePropertyBufferFrameSizeRange: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioValueRange);
                } else if (*ioPropertyDataSize < sizeof(AudioValueRange)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    AudioValueRange* range = (AudioValueRange*) outPropertyData;
                    range->mMinimum = TJackClient::fBufferSize;
                    range->mMaximum = TJackClient::fBufferSize;
                    *ioPropertyDataSize = sizeof(AudioValueRange);
                }
                break;
            }

        case kAudioDevicePropertyBufferSize: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(UInt32*) outPropertyData = TJackClient::fBufferSize * sizeof(float);
                    *ioPropertyDataSize = sizeof(UInt32);
                }
                break;
            }

        case kAudioDevicePropertyBufferSizeRange: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioValueRange);
                } else if (*ioPropertyDataSize < sizeof(AudioValueRange)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    AudioValueRange* range = (AudioValueRange*) outPropertyData;
                    range->mMinimum = TJackClient::fBufferSize * sizeof(float);
                    range->mMaximum = TJackClient::fBufferSize * sizeof(float);
                    *ioPropertyDataSize = sizeof(AudioValueRange);
                }
                break;
            }

        case kAudioDevicePropertyStreamConfiguration: {
                long channels = (isInput) ? TJackClient::fInputChannels : TJackClient::fOutputChannels;

                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32) + sizeof(AudioBuffer) * channels;
                } else if (*ioPropertyDataSize < (sizeof(UInt32) + sizeof(AudioBuffer)*channels)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {

                    AudioBufferList* bList = (AudioBufferList*) outPropertyData;
                    bList->mNumberBuffers = channels;

                    for (unsigned int i = 0; i < bList->mNumberBuffers; i++) {
                        bList->mBuffers[i].mNumberChannels = 1;
                        bList->mBuffers[i].mDataByteSize = TJackClient::fBufferSize * sizeof(float);
                        bList->mBuffers[i].mData = NULL;
                    }

                    *ioPropertyDataSize = sizeof(UInt32) + sizeof(AudioBuffer) * channels;
                    JARLog("DeviceGetProperty::kAudioDevicePropertyStreamConfiguration %ld\n", *ioPropertyDataSize);
                }
                break;
            }

        case kAudioDevicePropertyStreams: {
                long channels = (isInput) ? TJackClient::fInputChannels : TJackClient::fOutputChannels;

                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioStreamID) * channels;
                } else if (*ioPropertyDataSize < (sizeof(AudioStreamID)*channels)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    /*
                    FINAL Cut pro *incorrectly* ask for this property with a two small "ioPropertyDataSize". 
                    The kAudioHardwareBadPropertySizeError value was thus returned but then cause a crash in FCP
                    Now the date size that can be safely written is returned without any error.....
                    */
                    AudioStreamID* streamIDList = (AudioStreamID*)outPropertyData;
                    int streams = *ioPropertyDataSize / sizeof(AudioStreamID);

                    JARLog("DeviceGetProperty : kAudioDevicePropertyStreams copy %ld stream\n", streams);

                    if (isInput) {
                        for (int i = 0; i < streams; i++) {
                            streamIDList[i] = TJackClient::fStreamIDList[i];
                        }
                    } else {
                        for (int i = 0; i < streams; i++) {
                            streamIDList[i] = TJackClient::fStreamIDList[i + TJackClient::fInputChannels];
                        }
                    }

                    *ioPropertyDataSize = sizeof(AudioStreamID) * streams;

                } else {

                    AudioStreamID* streamIDList = (AudioStreamID*)outPropertyData;

                    if (isInput) {
                        for (int i = 0; i < channels; i++) {
                            streamIDList[i] = TJackClient::fStreamIDList[i];
                        }
                    } else {
                        for (int i = 0; i < channels; i++) {
                            streamIDList[i] = TJackClient::fStreamIDList[i + TJackClient::fInputChannels];
                        }
                    }

                    *ioPropertyDataSize = sizeof(AudioStreamID) * channels;
                }
                break;
            }

        case kAudioDevicePropertyStreamFormatSupported: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
                } else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    JARLog("DeviceGetProperty::kAudioDevicePropertyStreamFormatSupported \n");
                    AudioStreamBasicDescription* desc = (AudioStreamBasicDescription*) outPropertyData;
                    bool res = true;

                    JARLog("Sample Rate:        %f\n", desc->mSampleRate);
                    JARLog("Encoding:           %d\n", (int)desc->mFormatID);
                    JARLog("FormatFlags:        %d\n", (int)desc->mFormatFlags);

                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);

                    res &= (desc->mSampleRate != 0) ? (desc->mSampleRate == TJackClient::fSampleRate) : true;
                    res &= (desc->mFormatID != 0) ? (desc->mFormatID == kIOAudioStreamSampleFormatLinearPCM) : true;
                    res &= (desc->mFormatFlags != 0) ? (desc->mFormatFlags == kJackStreamFormat) : true;
                    res &= (desc->mBytesPerPacket != 0) ? (desc->mBytesPerPacket == 4) : true;
                    res &= (desc->mFramesPerPacket != 0) ? (desc->mFramesPerPacket == 1) : true;
                    res &= (desc->mBytesPerFrame != 0) ? (desc->mBytesPerFrame == 4) : true;
                    res &= (desc->mChannelsPerFrame != 0) ? (desc->mChannelsPerFrame == 1) : true;
                    res &= (desc->mBitsPerChannel != 0) ? (desc->mBitsPerChannel == 32) : true;

                    if (res) {
                        err = kAudioHardwareNoError;
                        JARLog("DeviceGetProperty::kAudioDevicePropertyStreamFormatSupported : format SUPPORTED\n");
                    } else {
                        JARLog("DeviceGetProperty::kAudioDevicePropertyStreamFormatSupported : format UNSUPPORTED\n");
                        err = kAudioDeviceUnsupportedFormatError;
                    }
                }
                break;
            }

        case kAudioDevicePropertyStreamFormatMatch: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
                } else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    JARLog("DeviceGetProperty::kAudioDevicePropertyStreamFormatMatch \n");
                    AudioStreamBasicDescription* desc = (AudioStreamBasicDescription*) outPropertyData;

                    JARLog("Sample Rate:        %f\n", desc->mSampleRate);
                    JARLog("Encoding:           %d\n", (int)desc->mFormatID);
                    JARLog("FormatFlags:        %d\n", (int)desc->mFormatFlags);

                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
					desc->mSampleRate = TJackClient::fSampleRate;
                    desc->mFormatID = kIOAudioStreamSampleFormatLinearPCM;
                    desc->mFormatFlags = kJackStreamFormat;
					desc->mBytesPerPacket = 4;
                    desc->mFramesPerPacket = 1;
                    desc->mBytesPerFrame = 4;
                    desc->mChannelsPerFrame = 1;
                    desc->mBitsPerChannel = 32;
                }
                break;
            }

        case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(UInt32*) outPropertyData = 0; // no....
					 *ioPropertyDataSize = sizeof(UInt32);
                    JARLog("DeviceGetProperty::kAudioDevicePropertyDeviceCanBeDefaultDevice %ld\n", (long)isInput);
                }
                break;
            }
			
		case kAudioDevicePropertyDeviceCanBeDefaultDevice: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(UInt32*) outPropertyData = 1; // yes...
                    *ioPropertyDataSize = sizeof(UInt32);
                    JARLog("DeviceGetProperty::kAudioDevicePropertyDeviceCanBeDefaultDevice %ld\n", (long)isInput);
                }
                break;
            }

        case kAudioDevicePropertyStreamFormat:
        case kAudioDevicePropertyStreamFormats:
        case kAudioStreamPropertyPhysicalFormat:
        case kAudioStreamPropertyPhysicalFormats: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
                } else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    AudioStreamBasicDescription* streamDescList = (AudioStreamBasicDescription*) outPropertyData;
                    streamDescList[0].mSampleRate = TJackClient::fSampleRate;
                    streamDescList[0].mFormatID = kIOAudioStreamSampleFormatLinearPCM;
                    streamDescList[0].mFormatFlags = kJackStreamFormat;
                    streamDescList[0].mBytesPerPacket = 4;
                    streamDescList[0].mFramesPerPacket = 1;
                    streamDescList[0].mBytesPerFrame = 4;
                    streamDescList[0].mChannelsPerFrame = 1;
                    streamDescList[0].mBitsPerChannel = 32;
                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
                }
                break;
            }

        case kAudioDevicePropertyHogMode: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(pid_t);
                } else if (*ioPropertyDataSize < sizeof(pid_t)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    // TO BE CHECKED
                    pid_t* pid = (pid_t*)outPropertyData;
                    *pid = -1;
                    //*pid = getpid();
                    *ioPropertyDataSize = sizeof(pid_t);
				}
                break;
            }

        case kAudioDevicePropertyNominalSampleRate: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(Float64);
                } else if (*ioPropertyDataSize < sizeof(Float64)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    Float64* valRange = (Float64*)outPropertyData;
                    *valRange = TJackClient::fSampleRate;
                    *ioPropertyDataSize = sizeof(Float64);
                }
                break;
            }

        case kAudioDevicePropertyActualSampleRate: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(Float64);
                } else if (*ioPropertyDataSize < sizeof(Float64)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    Float64* valRange = (Float64*)outPropertyData;
                    bool running = (TJackClient::fJackClient) ? TJackClient::fJackClient->IsRunning() : false;
                    *valRange = (running) ? TJackClient::fSampleRate : 0.0f;
                    *ioPropertyDataSize = sizeof(Float64);
                }
                break;
            }

        case kAudioDevicePropertyAvailableNominalSampleRates: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioValueRange);
                } else if (*ioPropertyDataSize < sizeof(AudioValueRange)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    AudioValueRange* valRange = (AudioValueRange*)outPropertyData;
                    valRange->mMinimum = TJackClient::fSampleRate;
                    valRange->mMaximum = TJackClient::fSampleRate;
                    *ioPropertyDataSize = sizeof(AudioValueRange);
                }
                break;
            }

        case kAudioDevicePropertyDeviceIsRunning: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(UInt32*) outPropertyData = (TJackClient::fJackClient) ? TJackClient::fJackClient->IsRunning() : false;
                    *ioPropertyDataSize = sizeof(UInt32);
                    JARLog("DeviceGetProperty::kAudioDevicePropertyDeviceIsRunning %ld \n", *(UInt32*) outPropertyData);
                }
                break;
            }

        case kAudioDevicePropertyDeviceIsRunningSomewhere: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(UInt32*) outPropertyData = true;
                    *ioPropertyDataSize = sizeof(UInt32);
                    JARLog("DeviceGetProperty::kAudioDevicePropertyDeviceIsRunning %ld \n", *(UInt32*) outPropertyData);
                }
                break;
            }

        case kAudioDevicePropertyPreferredChannelsForStereo: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(stereoList);
                } else if (*ioPropertyDataSize < sizeof(stereoList)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    stereoList* list = (stereoList*) outPropertyData;
                    list->channel[0] = 1;
                    list->channel[1] = 2;
                    *ioPropertyDataSize = sizeof(stereoList);
                }
                break;
            }

        case kAudioDevicePropertyDataSource:
        case kAudioDevicePropertyDataSources: {
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
					*ioPropertyDataSize = sizeof(UInt32);
				} else if (*ioPropertyDataSize < sizeof(UInt32)) {
					JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
					err = kAudioHardwareBadPropertySizeError;
				} else {
					*(UInt32*) outPropertyData = (isInput) ? JackInputDataSource : JackOutputDataSource; 
					*ioPropertyDataSize = sizeof(UInt32);
				}
				JARLog("DeviceGetProperty : kAudioDevicePropertyDataSource ok %ld\n");
				break;
		}

        case kAudioDevicePropertyPlugIn: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(OSStatus);
                } else if (*ioPropertyDataSize < sizeof(OSStatus)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(OSStatus*) outPropertyData = kAudioHardwareNoError; // TO BE CHECKED
                    *ioPropertyDataSize = sizeof(OSStatus);
                }
                break;
            }

        case kAudioDevicePropertyJackIsConnected:
            err = kAudioHardwareUnknownPropertyError;
            break;

            // Special Property to access Jack client from application (plug-in) code
        case kAudioDevicePropertyGetJackClient: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(jack_client_t*);
                } else if (*ioPropertyDataSize < sizeof(jack_client_t*)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *ioPropertyDataSize = sizeof(jack_client_t*);
                    JARLog("DeviceGetProperty::kAudioDevicePropertyGetJackClient\n");
                    if (TJackClient* client = GetJackClient()) {
                        IncRefExternal();
                        *(jack_client_t **) outPropertyData = client->fClient;
                        err = kAudioHardwareNoError;
                    } else {
                        *(jack_client_t **) outPropertyData = NULL;
                        err = kAudioHardwareBadDeviceError;
                    }
                }
                break;
            }

            // Special Property to release Jack client from plug-in code
        case kAudioDevicePropertyReleaseJackClient: {
                DecRefExternal();
                break;
            }

            // Special Property to allocate Jack port from plug-in code
        case kAudioDevicePropertyGetJackPortVST: {
                *(float**)outPropertyData = GetJackClient()->GetPlugInPortVST(*ioPropertyDataSize);
                break;
            }

        case kAudioDevicePropertyGetJackPortAU: {
                *(float**)outPropertyData = GetJackClient()->GetPlugInPortAU(*ioPropertyDataSize);
                break;
            }

        case kAudioDevicePropertySupportsMixing: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(UInt32*)outPropertyData = 1; // support Mixing
                    *ioPropertyDataSize = sizeof(UInt32);
                }
                break;
            }

        case kAudioDevicePropertyUsesVariableBufferFrameSizes:
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioDevicePropertyDataSourceNameForID: {
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = (isInput) ? TJackClient::fInputDataSource.size() + 1 : TJackClient::fOutputDataSource.size() + 1;
                } else if (isInput && *ioPropertyDataSize < sizeof(TJackClient::fInputDataSource.size() + 1)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
				} else if (!isInput && *ioPropertyDataSize < sizeof(fOutputDataSource)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else if (outPropertyData) {
					AudioValueTranslation* data = (AudioValueTranslation*)outPropertyData;
					if (isInput && (*(UInt32*)data->mInputData) == JackInputDataSource) {
						data->mOutputDataSize = TJackClient::fInputDataSource.size() + 1; 
						strcpy((char*)data->mOutputData, TJackClient::fInputDataSource.c_str());
					} else if (!isInput && (*(UInt32*)data->mInputData) == JackOutputDataSource) {
						data->mOutputDataSize = TJackClient::fOutputDataSource.size() + 1; 
						strcpy((char*)data->mOutputData, TJackClient::fOutputDataSource.c_str());
					}
				} else {
					err = kAudioHardwareBadPropertySizeError;
				}
				break;
            }

        case kAudioDevicePropertyDataSourceNameForIDCFString: {
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(CFStringRef);
                } else if (*ioPropertyDataSize < sizeof (CFStringRef)) {
                    err = kAudioHardwareBadPropertySizeError;
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
				} else if (outPropertyData) {
					AudioValueTranslation* data = (AudioValueTranslation*)outPropertyData;
					if (isInput && (*(UInt32*)data->mInputData) == JackInputDataSource) {
						data->mOutputDataSize = TJackClient::fInputDataSource.size() + 1; 
						CFStringRef* outString = (CFStringRef*)data->mOutputData;
						*outString = CFStringCreateWithCString(NULL, TJackClient::fInputDataSource.c_str(), CFStringGetSystemEncoding());
					} else if (!isInput && (*(UInt32*)data->mInputData) == JackOutputDataSource) {
						data->mOutputDataSize = TJackClient::fOutputDataSource.size() + 1; 
						CFStringRef* outString = (CFStringRef*)data->mOutputData;
						*outString = CFStringCreateWithCString(NULL, TJackClient::fOutputDataSource.c_str(), CFStringGetSystemEncoding());
					}
				} else {
					err = kAudioHardwareBadPropertySizeError;
				}
                break;
            }

        case kAudioDevicePropertyIOProcStreamUsage: {
                unsigned int size;
                if (isInput)
                    size = sizeof(void*) + sizeof(UInt32) + sizeof(UInt32) * TJackClient::fInputChannels;
                else
                    size = sizeof(void*) + sizeof(UInt32) + sizeof(UInt32) * TJackClient::fOutputChannels;

                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = size;
                } else if (*ioPropertyDataSize < size) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    JARLog("DeviceGetProperty : kAudioDevicePropertyIOProcStreamUsage %ld \n", *ioPropertyDataSize);
                    AudioHardwareIOProcStreamUsage* outData = (AudioHardwareIOProcStreamUsage*)outPropertyData;
                    JARLog("DeviceGetProperty : kAudioDevicePropertyIOProcStreamUsage mIOProc %x \n", (AudioDeviceIOProc)outData->mIOProc);

                    if (isInput) {
                        outData->mNumberStreams = TJackClient::fInputChannels;
                        for (int i = 0; i < TJackClient::fInputChannels; i++) {
							outData->mStreamIsOn[i] = (TJackClient::fJackClient->fInputPortList[i] != NULL);
							JARLog("DeviceGetProperty : kAudioDevicePropertyIOProcStreamUsage input mStreamIsOn %ld \n", outData->mStreamIsOn[i]);
                        }
                    } else {
                        outData->mNumberStreams = TJackClient::fOutputChannels;
                        for (int i = 0; i < TJackClient::fOutputChannels; i++) {
                            outData->mStreamIsOn[i] = (TJackClient::fJackClient->fOutputPortList[i] != NULL);
                            JARLog("DeviceGetProperty : kAudioDevicePropertyIOProcStreamUsage output mStreamIsOn %ld \n", outData->mStreamIsOn[i]);
                        }
                    }
                    *ioPropertyDataSize = size;
                }
                break;
            }

        case kAudioDevicePropertyConfigurationApplication: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(CFStringRef);
                } else if (*ioPropertyDataSize < sizeof(CFStringRef)) {
                    JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    CFStringRef* outString = (CFStringRef*) outPropertyData;
                    *outString = CFStringCreateWithCString(NULL, "com.apple.audio.AudioMIDISetup", CFStringGetSystemEncoding());
                    *ioPropertyDataSize = sizeof(CFStringRef);
                }
                break;
            }

            // Redirect call on used CoreAudio driver
        case kAudioDevicePropertyClockSource:
        case kAudioDevicePropertyClockSources:
        case kAudioDevicePropertyClockSourceNameForID:
        case kAudioDevicePropertyClockSourceNameForIDCFString:
        case kAudioHardwarePropertyBootChimeVolumeScalar:
        case kAudioDevicePropertyDriverShouldOwniSub:
        case kAudioDevicePropertyLatency:
        case kAudioDevicePropertyVolumeScalar:
        case kAudioDevicePropertyVolumeDecibels:
        case kAudioDevicePropertyVolumeRangeDecibels:
        case kAudioDevicePropertyVolumeScalarToDecibels:
        case kAudioDevicePropertyVolumeDecibelsToScalar:
        case kAudioDevicePropertyMute:
        case kAudioDevicePropertyPlayThru:
        case kAudioDevicePropertySafetyOffset:
        case kAudioDevicePropertySubVolumeScalar:
        case kAudioDevicePropertySubVolumeDecibels:
        case kAudioDevicePropertySubVolumeRangeDecibels:
        case kAudioDevicePropertySubVolumeScalarToDecibels:
        case kAudioDevicePropertySubVolumeDecibelsToScalar:
        case kAudioDevicePropertySubMute: {
                JARLog("Redirect call on used CoreAudio driver ID %ld \n", TJackClient::fCoreAudioDriver);
                Print4CharCode("property ", inPropertyID);
                err = AudioDeviceGetProperty(TJackClient::fCoreAudioDriver,
                                             inChannel,
                                             isInput,
                                             inPropertyID,
                                             ioPropertyDataSize,
                                             outPropertyData);

                JARLog("Redirect call on used CoreAudio driver err %ld \n", err);
                printError(err);
                break;
            }

        default:
            Print4CharCode("DeviceGetProperty unkown request:", inPropertyID);
            err = kAudioHardwareUnknownPropertyError;
            break;
    }
    return err;
}

/*
Some properties like kAudioDevicePropertyBufferSize, kAudioDevicePropertyNominalSampleRate can be "set", even is there is no Jack client running
*/

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceSetProperty(AudioHardwarePlugInRef inSelf,
                                        AudioDeviceID inDevice,
                                        const AudioTimeStamp* inWhen,
                                        UInt32 inChannel,
                                        Boolean isInput,
                                        AudioDevicePropertyID inPropertyID,
                                        UInt32 inPropertyDataSize,
                                        const void* inPropertyData)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceSetProperty inSelf %ld\n", (long)inSelf);
    if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;

    Print4CharCode ("DeviceSetProperty ", inPropertyID);
    OSStatus err = kAudioHardwareNoError;

    if (inDevice != TJackClient::fDeviceID) {
        JARLog("DeviceSetProperty called for invalid device ID\n");
        return kAudioHardwareBadDeviceError;
    }

    switch (inPropertyID) {

#ifdef kAudioHardwarePlugInInterface2ID
        case kAudioDevicePropertyPreferredChannelLayout:
            JARLog("kAudioDevicePropertyPreferredChannelLayout\n");
            err = kAudioHardwareNoError;
            break;
#endif
            // Special Property to allocate Jack port from VST plug-in code
        case kAudioDevicePropertyAllocateJackPortVST: {
                err = (GetJackClient()->AllocatePlugInPortVST(inPropertyDataSize)) ? kAudioHardwareNoError : kAudioHardwareBadPropertySizeError;
                break;
            }

            // Special Property to allocate Jack port from AU plug-in code
        case kAudioDevicePropertyAllocateJackPortAU: {
                err = (GetJackClient()->AllocatePlugInPortAU(inPropertyDataSize)) ? kAudioHardwareNoError : kAudioHardwareBadPropertySizeError;
                break;
            }

            // Special Property to release Jack port from VST plug-in code
        case kAudioDevicePropertyReleaseJackPortVST: {
                GetJackClient()->ReleasePlugInPortVST(inPropertyDataSize);
                break;
            }

            // Special Property to release Jack port from AU plug-in code
        case kAudioDevicePropertyReleaseJackPortAU: {
                GetJackClient()->ReleasePlugInPortAU(inPropertyDataSize);
                break;
            }

            // Special Property to deactivate jack from plug-in code
        case kAudioDevicePropertyDeactivateJack: {
                GetJackClient()->SaveActivate();
                break;
            }

        case kAudioDevicePropertyActivateJack: {
                GetJackClient()->RestoreActivate();
                break;
            }

        case kAudioDevicePropertyBufferSize:
            JARLog("kAudioDevicePropertyBufferSize %ld \n", *(UInt32*) inPropertyData);
            err = (*(UInt32*) inPropertyData == TJackClient::fBufferSize * sizeof(float)) ? kAudioHardwareNoError
                  : kAudioHardwareIllegalOperationError;
            break;

        case kAudioDevicePropertyBufferFrameSize:
            JARLog("kAudioDevicePropertyBufferFrameSize %ld \n", *(UInt32*) inPropertyData);
            err = (*(UInt32*) inPropertyData == TJackClient::fBufferSize) ? kAudioHardwareNoError
                  : kAudioHardwareIllegalOperationError;
            break;

        case kAudioDevicePropertyNominalSampleRate: {
                Float64* value = (Float64*)inPropertyData;
                JARLog("kAudioDevicePropertyNominalSampleRate %f \n", *value);
                err = (*value == TJackClient::fSampleRate) ? kAudioHardwareNoError
                      : kAudioHardwareIllegalOperationError;
                JARLog("kAudioDevicePropertyNominalSampleRate err %ld \n", err);
                break;
            }

        case kAudioDevicePropertyStreamFormat: {
                AudioStreamBasicDescription* desc = (AudioStreamBasicDescription*) inPropertyData;
                bool res = true;
                JARLog("Sample Rate:        %f \n", desc->mSampleRate);
                JARLog("Encoding:           %d \n", (int)desc->mFormatID);
                JARLog("FormatFlags:        %d \n", desc->mFormatFlags);
                JARLog("Bytes per Packet:   %d \n", (int)desc->mBytesPerPacket);
                JARLog("Frames per Packet:  %d \n", (int)desc->mFramesPerPacket);
                JARLog("Bytes per Frame:    %d \n", (int)desc->mBytesPerFrame);
                JARLog("Channels per Frame: %d \n", (int)desc->mChannelsPerFrame);
                JARLog("Bits per Channel:   %d \n", (int)desc->mBitsPerChannel);

                res &= (desc->mSampleRate == TJackClient::fSampleRate);
                res &= (desc->mFormatID == kIOAudioStreamSampleFormatLinearPCM);
                res &= (desc->mFormatFlags == kJackStreamFormat);
                res &= (desc->mBytesPerPacket == 4);
                res &= (desc->mFramesPerPacket == 1);
                res &= (desc->mBytesPerFrame == 4);
                res &= (desc->mChannelsPerFrame == 1);
                res &= (desc->mBitsPerChannel == 32);

                err = (res) ? kAudioHardwareNoError : kAudioDeviceUnsupportedFormatError;
                if (res)
                    JARLog("kAudioDevicePropertyStreamFormat format supported\n");
                else
                    JARLog("kAudioDevicePropertyStreamFormat format not supported\n");
                break;
            }

        case kAudioDevicePropertyDeviceCanBeDefaultDevice:
        case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
            break;

        case kAudioDevicePropertyPlugIn:
        case kAudioDevicePropertyDeviceUID:
        case kAudioDevicePropertyTransportType:
        case kAudioDevicePropertyDeviceIsAlive:
        case kAudioDevicePropertyDeviceIsRunning:
        case kAudioDevicePropertyDeviceIsRunningSomewhere:
        case kAudioDevicePropertyJackIsConnected:
        case kAudioDeviceProcessorOverload:
        case kAudioDevicePropertyHogMode:
        case kAudioDevicePropertyRegisterBufferList:
        case kAudioDevicePropertyLatency:
        case kAudioDevicePropertyBufferSizeRange:
        case kAudioDevicePropertyBufferFrameSizeRange:
        case kAudioDevicePropertyUsesVariableBufferFrameSizes:
        case kAudioDevicePropertyStreams:
        case kAudioDevicePropertySafetyOffset:
        case kAudioDevicePropertySupportsMixing:
        case kAudioDevicePropertyStreamConfiguration:
            err = kAudioHardwareUnknownPropertyError; // steph
            break;
			
		case kAudioDevicePropertyIOProcStreamUsage: {
                // Can only be set when a client is running
                if (TJackClient::fJackClient == NULL) {
                    JARLog("DeviceSetProperty kAudioDevicePropertyIOProcStreamUsage : called when then Jack server is not running\n");
                    err = kAudioHardwareBadDeviceError;
                } else {

                    AudioHardwareIOProcStreamUsage* inData = (AudioHardwareIOProcStreamUsage*)inPropertyData;

                    JARLog("DeviceSetProperty : kAudioDevicePropertyIOProcStreamUsage size %ld\n", inPropertyDataSize);
                    JARLog("DeviceSetProperty : kAudioDevicePropertyIOProcStreamUsage proc %x\n", (AudioDeviceIOProc)inData->mIOProc);
                    JARLog("DeviceSetProperty : kAudioDevicePropertyIOProcStreamUsage mNumberStreams %ld\n", inData->mNumberStreams);

                    map<AudioDeviceIOProc, TProcContext>::iterator iter = TJackClient::fJackClient->fAudioIOProcList.find((AudioDeviceIOProc)inData->mIOProc);

                    if (iter == TJackClient::fJackClient->fAudioIOProcList.end()) {
                        JARLog("DeviceSetProperty kAudioDevicePropertyIOProcStreamUsage : Proc not found %x \n", (AudioDeviceIOProc)inData->mIOProc);
                        err = kAudioHardwareUnknownPropertyError;
                    } else {
                        JARLog("DeviceSetProperty kAudioDevicePropertyIOProcStreamUsage : Proc found %x \n", (AudioDeviceIOProc)inData->mIOProc);
                        iter->second.fStreamUsage = true; // We need to take care of stream usage in Process

                        if (isInput) {
                            for (unsigned int i = 0; i < inData->mNumberStreams; i++) {

                                // Register the Jack port if needed again
                                iter->second.fInput[i] = inData->mStreamIsOn[i];

                                if (inData->mStreamIsOn[i]) {
                                    if (TJackClient::fJackClient) {
                                        if (TJackClient::fJackClient->fInputPortList[i] == 0) {
                                            char in_port_name [JACK_PORT_NAME_LEN];
											sprintf(in_port_name, "in%u", i + 1);
                                            TJackClient::fJackClient->fInputPortList[i] = jack_port_register(TJackClient::fJackClient->fClient, in_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                                            JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_register %ld \n", i);
                                        }
                                    }
                                } else {
                                    if (TJackClient::fJackClient) {
                                        if (TJackClient::fJackClient->fInputPortList[i] && !TJackClient::fJackClient->IsUsedInput(i)) {
                                            jack_port_unregister(TJackClient::fJackClient->fClient, TJackClient::fJackClient->fInputPortList[i]);
                                            TJackClient::fJackClient->fInputPortList[i] = 0;
                                            JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_unregister %ld \n", i);
                                        }
                                    }
                                }

                                JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage input inData->mStreamIsOn %ld \n", inData->mStreamIsOn[i]);
                            }
						} else {
                            for (unsigned int i = 0; i < inData->mNumberStreams; i++) {

                                // Unregister the Jack port if no more needed
                                iter->second.fOutput[i] = inData->mStreamIsOn[i];

                                if (inData->mStreamIsOn[i]) {
                                    if (TJackClient::fJackClient) {
                                        if (TJackClient::fJackClient->fOutputPortList[i] == 0) {
                                            char out_port_name [JACK_PORT_NAME_LEN];
											sprintf(out_port_name, "out%u", i + 1);
                                            TJackClient::fJackClient->fOutputPortList[i] = jack_port_register(TJackClient::fJackClient->fClient, out_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                                            JARLog("DeviceSetProperty : output kAudioDevicePropertyIOProcStreamUsage jack_port_register %ld \n", i);
                                        }
                                    }
                                } else {
                                    if (TJackClient::fJackClient) {
                                        if (TJackClient::fJackClient->fOutputPortList[i] && !TJackClient::fJackClient->IsUsedOutput(i)) {
                                            jack_port_unregister(TJackClient::fJackClient->fClient, TJackClient::fJackClient->fOutputPortList[i]);
                                            TJackClient::fJackClient->fOutputPortList[i] = 0;
                                            JARLog("DeviceSetProperty : output kAudioDevicePropertyIOProcStreamUsage jack_port_unregister %ld \n", i);
                                        }
                                    }
                                }

                                JARLog("DeviceSetProperty : output kAudioDevicePropertyIOProcStreamUsage output inData->mStreamIsOn %ld \n", inData->mStreamIsOn[i]);
                            }
                        }
						// Autoconnect is only done for the first activation
						if (TJackClient::fFirstActivate) {
							TJackClient::fJackClient->AutoConnect();
							TJackClient::fFirstActivate = false;
						} else {
							TJackClient::fJackClient->RestoreConnections();
						}
                        err = kAudioHardwareNoError;
                    }
                }
                break;
            }
	
        case kAudioDevicePropertyPreferredChannelsForStereo:
        case kAudioDevicePropertyAvailableNominalSampleRates:
        case kAudioDevicePropertyActualSampleRate:
        case kAudioDevicePropertyStreamFormats:
        case kAudioDevicePropertyStreamFormatSupported:
        case kAudioDevicePropertyStreamFormatMatch:
        case kAudioDevicePropertyDataSource:
        case kAudioDevicePropertyDataSources:
        case kAudioDevicePropertyDataSourceNameForID:
        case kAudioDevicePropertyDataSourceNameForIDCFString:
        case kAudioDevicePropertyClockSource:
        case kAudioDevicePropertyClockSources:
        case kAudioDevicePropertyClockSourceNameForID:
        case kAudioDevicePropertyClockSourceNameForIDCFString:
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioDevicePropertyVolumeScalar:
        case kAudioDevicePropertyVolumeDecibels:
        case kAudioDevicePropertyVolumeRangeDecibels:
        case kAudioDevicePropertyVolumeScalarToDecibels:
        case kAudioDevicePropertyVolumeDecibelsToScalar:
        case kAudioDevicePropertyMute:
        case kAudioDevicePropertyPlayThru:
        case kAudioDevicePropertyDriverShouldOwniSub:
        case kAudioDevicePropertySubVolumeScalar:
        case kAudioDevicePropertySubVolumeDecibels:
        case kAudioDevicePropertySubVolumeRangeDecibels:
        case kAudioDevicePropertySubVolumeScalarToDecibels:
        case kAudioDevicePropertySubVolumeDecibelsToScalar:
        case kAudioDevicePropertySubMute: {
                JARLog("Redirect call on used CoreAudio driver ID %ld \n", TJackClient::fCoreAudioDriver);
                Print4CharCode("property ", inPropertyID);
                err = AudioDeviceSetProperty(TJackClient::fCoreAudioDriver,
                                             inWhen,
                                             inChannel,
                                             isInput,
                                             inPropertyID,
                                             inPropertyDataSize,
                                             inPropertyData);

                JARLog("Redirect call on used CoreAudio driver err %ld \n", err);
                printError(err);
                break;
            }

        default:
            Print4CharCode("DeviceSetProperty unkown request:", inPropertyID);
            err = kAudioHardwareUnknownPropertyError;
            break;
    }

    return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
// Utilities
//---------------------------------------------------------------------------------------------------------------------------------
bool TJackClient::IsUsedInput(int port)
{
    map<AudioDeviceIOProc, TProcContext>::iterator it;
    for (it = fAudioIOProcList.begin(); it != fAudioIOProcList.end(); it++) {
		JARLog("IsUsedInput %x %ld \n", it->first, it->second.fInput[port]);
        if (it->second.fInput[port])
            return true;
    }
    return false;
}

bool TJackClient::IsUsedOutput(int port)
{
    map<AudioDeviceIOProc, TProcContext>::iterator it;
    for (it = fAudioIOProcList.begin(); it != fAudioIOProcList.end(); it++) {
        if (it->second.fOutput[port])
            return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------------------
// Stream Property Management
//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::StreamGetPropertyInfo(AudioHardwarePlugInRef inSelf,
        AudioStreamID inStream,
        UInt32 inChannel,
        AudioDevicePropertyID inPropertyID,
        UInt32* outSize,
        Boolean* outWritable)
{
    OSStatus err = kAudioHardwareNoError;
    JARLog("--------------------------------------------------------\n");
    JARLog("StreamGetPropertyInfo inSelf inPropertyID %ld \n", (long)inSelf);
    if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;
		
    Print4CharCode("StreamGetPropertyInfo ", inPropertyID);

    if (outSize == NULL) {
        JARLog("StreamGetPropertyInfo received NULL outSize pointer\n");
    }

    if (outWritable != NULL) {
        *outWritable = false;
    } else {
        JARLog("StreamGetPropertyInfo received NULL outWritable pointer\n");
    }

    switch (inPropertyID) {
        case kAudioStreamPropertyOwningDevice:
            if (outSize)
                *outSize = sizeof(AudioDeviceID);
            break;

        case kAudioStreamPropertyDirection:
        case kAudioStreamPropertyTerminalType:
        case kAudioStreamPropertyStartingChannel:
            if (outSize)
                *outSize = sizeof(UInt32);
            break;

        case kAudioDevicePropertyStreamFormat:
        case kAudioDevicePropertyStreamFormats:
        case kAudioStreamPropertyPhysicalFormat:
        case kAudioStreamPropertyPhysicalFormats:
            if (outSize)
                *outSize = sizeof(AudioStreamBasicDescription);
            if (outWritable)
                *outWritable = true;
            break;

        case kAudioStreamPropertyPhysicalFormatSupported:
        case kAudioStreamPropertyPhysicalFormatMatch:
            if (outSize)
                *outSize = sizeof(AudioStreamBasicDescription);
            break;

        case kAudioDevicePropertyDataSource:
        case kAudioDevicePropertyDataSources:
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioDevicePropertyJackIsConnected:
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioDevicePropertyDeviceName:
        case kAudioDevicePropertyDeviceNameCFString:
        case kAudioDevicePropertyDeviceManufacturer:
        case kAudioDevicePropertyDeviceManufacturerCFString:
        case kAudioDevicePropertyPlugIn:
        case kAudioDevicePropertyDeviceUID:
        case kAudioDevicePropertyTransportType:
        case kAudioDevicePropertyDeviceIsAlive:
        case kAudioDevicePropertyDeviceIsRunning:
        case kAudioDevicePropertyDeviceIsRunningSomewhere:
        case kAudioDevicePropertyDeviceCanBeDefaultDevice:
        case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
        case kAudioDeviceProcessorOverload:
        case kAudioDevicePropertyHogMode:
        case kAudioDevicePropertyRegisterBufferList:
        case kAudioDevicePropertyLatency:
        case kAudioDevicePropertyBufferSize:
        case kAudioDevicePropertyBufferSizeRange:
        case kAudioDevicePropertyBufferFrameSize:
        case kAudioDevicePropertyBufferFrameSizeRange:
        case kAudioDevicePropertyUsesVariableBufferFrameSizes:
        case kAudioDevicePropertyStreams:
        case kAudioDevicePropertySafetyOffset:
        case kAudioDevicePropertySupportsMixing:
        case kAudioDevicePropertyStreamConfiguration:
        case kAudioDevicePropertyIOProcStreamUsage:
        case kAudioDevicePropertyPreferredChannelsForStereo:
        case kAudioDevicePropertyNominalSampleRate:
        case kAudioDevicePropertyAvailableNominalSampleRates:
        case kAudioDevicePropertyActualSampleRate:
        case kAudioDevicePropertyStreamFormatSupported:
        case kAudioDevicePropertyStreamFormatMatch:
        case kAudioDevicePropertyVolumeScalar:
        case kAudioDevicePropertyVolumeDecibels:
        case kAudioDevicePropertyVolumeRangeDecibels:
        case kAudioDevicePropertyVolumeScalarToDecibels:
        case kAudioDevicePropertyVolumeDecibelsToScalar:
        case kAudioDevicePropertyMute:
        case kAudioDevicePropertyPlayThru:
        case kAudioDevicePropertyDataSourceNameForID:
        case kAudioDevicePropertyDataSourceNameForIDCFString:
        case kAudioDevicePropertyClockSource:
        case kAudioDevicePropertyClockSources:
        case kAudioDevicePropertyClockSourceNameForID:
        case kAudioDevicePropertyClockSourceNameForIDCFString:
        case kAudioDevicePropertyDriverShouldOwniSub:
        case kAudioDevicePropertySubVolumeScalar:
        case kAudioDevicePropertySubVolumeDecibels:
        case kAudioDevicePropertySubVolumeRangeDecibels:
        case kAudioDevicePropertySubVolumeScalarToDecibels:
        case kAudioDevicePropertySubVolumeDecibelsToScalar:
        case kAudioDevicePropertySubMute: {
                JARLog("Error : StreamGetPropertyInfo called for a stream\n");
                Print4CharCode("property ", inPropertyID);
                err = kAudioHardwareUnknownPropertyError;
                break;
            }

        default:
            Print4CharCode("StreamGetPropertyInfo unkown request:", inPropertyID);
            err = kAudioHardwareUnknownPropertyError;
            break;
    }
    return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::StreamGetProperty(AudioHardwarePlugInRef inSelf,
                                        AudioStreamID inStream,
                                        UInt32 inChannel,
                                        AudioDevicePropertyID inPropertyID,
                                        UInt32* ioPropertyDataSize,
                                        void* outPropertyData)
{
    OSStatus err = kAudioHardwareNoError;
    JARLog("--------------------------------------------------------\n");
    JARLog("StreamGetProperty inSelf %ld\n", (long)inSelf);
	if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;
		
    Print4CharCode("StreamGetProperty ", inPropertyID);

    switch (inPropertyID) {
        case kAudioStreamPropertyDirection: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    for (int i = 0; i < TJackClient::fInputChannels; i++) {
                        if (inStream == fStreamIDList[i]) {
                            *(UInt32*) outPropertyData = 1;
                            JARLog("StreamGetProperty FOUND INPUT %ld\n", inStream);
                            break;
                        }
                    }

                    for (int i = TJackClient::fInputChannels; i < (TJackClient::fOutputChannels + TJackClient::fInputChannels); i++) {
                        if (inStream == fStreamIDList[i]) {
                            *(UInt32*) outPropertyData = 0;
                            JARLog("StreamGetProperty FOUND OUTPUT %ld\n", inStream);
                            break;
                        }
                    }
                    *ioPropertyDataSize = sizeof(UInt32);
                }
                break;
            }

        case kAudioStreamPropertyStartingChannel: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    *(UInt32*) outPropertyData = 1; // TO BE CHECKED
                    *ioPropertyDataSize = sizeof(UInt32);
                }
                break;
            }

        case kAudioStreamPropertyTerminalType: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(UInt32);
                } else if (*ioPropertyDataSize < sizeof(UInt32)) {
                    JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    for (int i = 0; i < TJackClient::fInputChannels; i++) {
                        if (inStream == fStreamIDList[i]) {
                            *(UInt32*) outPropertyData = INPUT_MICROPHONE;
                            //*(UInt32*) outPropertyData = 0;
							JARLog("StreamGetProperty FOUND INPUT %ld\n", inStream);
                            break;
                        }
                    }

                    for (int i = TJackClient::fInputChannels; i < (TJackClient::fOutputChannels + TJackClient::fInputChannels); i++) {
                        if (inStream == fStreamIDList[i]) {
                            *(UInt32*) outPropertyData = OUTPUT_SPEAKER;
                            //*(UInt32*) outPropertyData = 0;
							JARLog("StreamGetProperty FOUND OUTPUT %ld\n", inStream);
                            break;
                        }
                    }
                    *ioPropertyDataSize = sizeof(UInt32);
                }
                break;
            }

        case kAudioDevicePropertyStreamFormat:
        case kAudioStreamPropertyPhysicalFormat: {

                if (inPropertyID == kAudioDevicePropertyStreamFormat) {
                    JARLog("StreamGetProperty : GET kAudioDevicePropertyStreamFormat %ld\n", *ioPropertyDataSize);
                } else {
                    JARLog("StreamGetProperty : GET kAudioStreamPropertyPhysicalFormat %ld\n", *ioPropertyDataSize);
                }

                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
                } else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)) {
                    JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    AudioStreamBasicDescription* streamDesc = (AudioStreamBasicDescription*) outPropertyData;
                    streamDesc->mSampleRate = TJackClient::fSampleRate;
                    streamDesc->mFormatID = kIOAudioStreamSampleFormatLinearPCM;
                    streamDesc->mFormatFlags = kJackStreamFormat;
                    streamDesc->mBytesPerPacket = 4;
                    streamDesc->mFramesPerPacket = 1;
                    streamDesc->mBytesPerFrame = 4;
                    streamDesc->mChannelsPerFrame = 1;
                    streamDesc->mBitsPerChannel = 32;
                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
                }
                break;
            }

        case kAudioDevicePropertyStreamFormats:
        case kAudioStreamPropertyPhysicalFormats: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
                } else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)) {
                    JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    AudioStreamBasicDescription* streamDescList = (AudioStreamBasicDescription*) outPropertyData;
                    streamDescList[0].mSampleRate = TJackClient::fSampleRate;
                    streamDescList[0].mFormatID = kIOAudioStreamSampleFormatLinearPCM;
                    streamDescList[0].mFormatFlags = kJackStreamFormat;
                    streamDescList[0].mBytesPerPacket = 4;
                    streamDescList[0].mFramesPerPacket = 1;
                    streamDescList[0].mBytesPerFrame = 4;
                    streamDescList[0].mChannelsPerFrame = 1;
                    streamDescList[0].mBitsPerChannel = 32;
                    *ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
                }
                break;
            }

        case kAudioStreamPropertyOwningDevice: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = sizeof(AudioDeviceID);
                } else if (*ioPropertyDataSize < sizeof(AudioDeviceID)) {
                    JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    err = kAudioHardwareUnknownPropertyError;
                    *ioPropertyDataSize = sizeof(AudioDeviceID);
                    for (int i = 0; i < TJackClient::fOutputChannels + TJackClient::fInputChannels; i++) {
                        if (fStreamIDList[i] == inStream) {
                            *(AudioDeviceID*)outPropertyData = TJackClient::fDeviceID;
                            err = kAudioHardwareNoError;
                            break;
                        }
                    }
                }
            }
            break;

        case kAudioStreamPropertyPhysicalFormatSupported:
        case kAudioStreamPropertyPhysicalFormatMatch:
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioDevicePropertyDeviceName: {
                if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)) {
                    *ioPropertyDataSize = TJackClient::fStreamName.size() + 1;
                } else if (*ioPropertyDataSize < TJackClient::fStreamName.size() + 1) {
                    err = kAudioHardwareBadPropertySizeError;
                    JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n", *ioPropertyDataSize);
                } else {
                    char* data = (char*) outPropertyData;
                    strcpy(data, TJackClient::fStreamName.c_str());
                    *ioPropertyDataSize = TJackClient::fStreamName.size() + 1;
                }
                break;
            }
            break;

        case kAudioDevicePropertyDataSource:
        case kAudioDevicePropertyDataSources:
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioDevicePropertyJackIsConnected:
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioDevicePropertyDeviceNameCFString:
        case kAudioDevicePropertyDeviceManufacturer:
        case kAudioDevicePropertyDeviceManufacturerCFString:
        case kAudioDevicePropertyPlugIn:
        case kAudioDevicePropertyDeviceUID:
        case kAudioDevicePropertyTransportType:
        case kAudioDevicePropertyDeviceIsAlive:
        case kAudioDevicePropertyDeviceIsRunning:
        case kAudioDevicePropertyDeviceIsRunningSomewhere:
        case kAudioDevicePropertyDeviceCanBeDefaultDevice:
        case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
        case kAudioDeviceProcessorOverload:
        case kAudioDevicePropertyHogMode:
        case kAudioDevicePropertyRegisterBufferList:
        case kAudioDevicePropertyLatency:
        case kAudioDevicePropertyBufferSize:
        case kAudioDevicePropertyBufferSizeRange:
        case kAudioDevicePropertyBufferFrameSize:
        case kAudioDevicePropertyBufferFrameSizeRange:
        case kAudioDevicePropertyUsesVariableBufferFrameSizes:
        case kAudioDevicePropertyStreams:
        case kAudioDevicePropertySafetyOffset:
        case kAudioDevicePropertySupportsMixing:
        case kAudioDevicePropertyStreamConfiguration:
        case kAudioDevicePropertyIOProcStreamUsage:
        case kAudioDevicePropertyPreferredChannelsForStereo:
        case kAudioDevicePropertyNominalSampleRate:
        case kAudioDevicePropertyAvailableNominalSampleRates:
        case kAudioDevicePropertyActualSampleRate:
        case kAudioDevicePropertyStreamFormatSupported:
        case kAudioDevicePropertyStreamFormatMatch:
        case kAudioDevicePropertyVolumeScalar:
        case kAudioDevicePropertyVolumeDecibels:
        case kAudioDevicePropertyVolumeRangeDecibels:
        case kAudioDevicePropertyVolumeScalarToDecibels:
        case kAudioDevicePropertyVolumeDecibelsToScalar:
        case kAudioDevicePropertyMute:
        case kAudioDevicePropertyPlayThru:

        case kAudioDevicePropertyDataSourceNameForID:
        case kAudioDevicePropertyDataSourceNameForIDCFString:
        case kAudioDevicePropertyClockSource:
        case kAudioDevicePropertyClockSources:
        case kAudioDevicePropertyClockSourceNameForID:
        case kAudioDevicePropertyClockSourceNameForIDCFString:
        case kAudioDevicePropertyDriverShouldOwniSub:
        case kAudioDevicePropertySubVolumeScalar:
        case kAudioDevicePropertySubVolumeDecibels:
        case kAudioDevicePropertySubVolumeRangeDecibels:
        case kAudioDevicePropertySubVolumeScalarToDecibels:
        case kAudioDevicePropertySubVolumeDecibelsToScalar:
        case kAudioDevicePropertySubMute:
            JARLog("Error : StreamGetProperty called for a stream %ld \n", inPropertyID);
            err = kAudioHardwareUnknownPropertyError;
            break;

        default:
            Print4CharCode("StreamGetProperty unkown request:", inPropertyID);
            err = kAudioHardwareUnknownPropertyError;
            break;
    }
    return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::StreamSetProperty(AudioHardwarePlugInRef inSelf,
                                        AudioStreamID inStream,
                                        const AudioTimeStamp* inWhen,
                                        UInt32 inChannel,
                                        AudioDevicePropertyID inPropertyID,
                                        UInt32 inPropertyDataSize,
                                        const void* inPropertyData)
{
    OSStatus err = kAudioHardwareNoError;
    JARLog("--------------------------------------------------------\n");
    JARLog("StreamSetProperty inSelf %ld\n", (long)inSelf);
	if (!CheckRunning(inSelf))
		return kAudioHardwareNotRunningError;
		
    JARLog("StreamSetProperty\n");
    Print4CharCode("StreamSetProperty request:", inPropertyID);

    switch (inPropertyID) {

        case kAudioStreamPropertyOwningDevice:
        case kAudioStreamPropertyDirection:
        case kAudioStreamPropertyTerminalType:
        case kAudioStreamPropertyStartingChannel:

        case kAudioStreamPropertyPhysicalFormatSupported:
        case kAudioStreamPropertyPhysicalFormatMatch:
            err = kAudioHardwareUnknownPropertyError;
            break;

        case kAudioStreamPropertyPhysicalFormat:
        case kAudioStreamPropertyPhysicalFormats: {
                if (inPropertyDataSize < sizeof(AudioStreamBasicDescription)) {
                    JARLog("StreamSetProperty : kAudioHardwareBadPropertySizeError %ld\n", inPropertyDataSize);
                    err = kAudioHardwareBadPropertySizeError;
                } else {
                    AudioStreamBasicDescription* streamDesc = (AudioStreamBasicDescription*) inPropertyData;
                    err = (streamDesc->mSampleRate == TJackClient::fSampleRate
                           && streamDesc->mFormatID == kIOAudioStreamSampleFormatLinearPCM
                           && streamDesc->mFormatFlags == kJackStreamFormat
                           && streamDesc->mBytesPerPacket == 4
                           && streamDesc->mFramesPerPacket == 1
                           && streamDesc->mBytesPerFrame == 4
                           && streamDesc->mChannelsPerFrame == 1
                           && streamDesc->mBitsPerChannel == 32) ? kAudioHardwareNoError : kAudioDeviceUnsupportedFormatError;
                }
                break;
            }

        case kAudioDevicePropertyDeviceName:
        case kAudioDevicePropertyDeviceNameCFString:
        case kAudioDevicePropertyDeviceManufacturer:
        case kAudioDevicePropertyDeviceManufacturerCFString:
        case kAudioDevicePropertyPlugIn:
        case kAudioDevicePropertyDeviceUID:
        case kAudioDevicePropertyTransportType:
        case kAudioDevicePropertyDeviceIsAlive:
        case kAudioDevicePropertyDeviceIsRunning:
        case kAudioDevicePropertyDeviceIsRunningSomewhere:
        case kAudioDevicePropertyDeviceCanBeDefaultDevice:
        case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
        case kAudioDevicePropertyJackIsConnected:
        case kAudioDeviceProcessorOverload:
        case kAudioDevicePropertyHogMode:
        case kAudioDevicePropertyRegisterBufferList:
        case kAudioDevicePropertyLatency:
        case kAudioDevicePropertyBufferSize:
        case kAudioDevicePropertyBufferSizeRange:
        case kAudioDevicePropertyBufferFrameSize:
        case kAudioDevicePropertyBufferFrameSizeRange:
        case kAudioDevicePropertyUsesVariableBufferFrameSizes:
        case kAudioDevicePropertyStreams:
        case kAudioDevicePropertySafetyOffset:
        case kAudioDevicePropertySupportsMixing:
        case kAudioDevicePropertyStreamConfiguration:
        case kAudioDevicePropertyIOProcStreamUsage:
        case kAudioDevicePropertyPreferredChannelsForStereo:
        case kAudioDevicePropertyNominalSampleRate:
        case kAudioDevicePropertyAvailableNominalSampleRates:
        case kAudioDevicePropertyActualSampleRate:
        case kAudioDevicePropertyStreamFormat:
        case kAudioDevicePropertyStreamFormats:
        case kAudioDevicePropertyStreamFormatSupported:
        case kAudioDevicePropertyStreamFormatMatch:
        case kAudioDevicePropertyVolumeScalar:
        case kAudioDevicePropertyVolumeDecibels:
        case kAudioDevicePropertyVolumeRangeDecibels:
        case kAudioDevicePropertyVolumeScalarToDecibels:
        case kAudioDevicePropertyVolumeDecibelsToScalar:
        case kAudioDevicePropertyMute:
        case kAudioDevicePropertyPlayThru:
        case kAudioDevicePropertyDataSource:
        case kAudioDevicePropertyDataSources:
        case kAudioDevicePropertyDataSourceNameForID:
        case kAudioDevicePropertyDataSourceNameForIDCFString:
        case kAudioDevicePropertyClockSource:
        case kAudioDevicePropertyClockSources:
        case kAudioDevicePropertyClockSourceNameForID:
        case kAudioDevicePropertyClockSourceNameForIDCFString:
        case kAudioDevicePropertyDriverShouldOwniSub:
        case kAudioDevicePropertySubVolumeScalar:
        case kAudioDevicePropertySubVolumeDecibels:
        case kAudioDevicePropertySubVolumeRangeDecibels:
        case kAudioDevicePropertySubVolumeScalarToDecibels:
        case kAudioDevicePropertySubVolumeDecibelsToScalar:
        case kAudioDevicePropertySubMute:
            JARLog("Error : DeviceProperty called for a stream %ld \n", inPropertyID);
            err = kAudioHardwareUnknownPropertyError;
            break;

        default:
            Print4CharCode("StreamSetProperty unkown request:", inPropertyID);
            err = kAudioHardwareUnknownPropertyError;
            break;
    }

    return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
#ifdef kAudioHardwarePlugInInterface2ID

OSStatus TJackClient::DeviceStartAtTime(AudioHardwarePlugInRef inSelf,
                                        AudioDeviceID inDevice,
                                        AudioDeviceIOProc inProc,
                                        AudioTimeStamp* ioRequestedStartTime,
                                        UInt32 inFlags)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceStartAtTime : not yet implemented\n");
    return kAudioHardwareUnsupportedOperationError;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceGetNearestStartTime (AudioHardwarePlugInRef inSelf,
        AudioDeviceID inDevice,
        AudioTimeStamp* ioRequestedStartTime,
        UInt32 inFlags)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceGetNearestStartTime : not yet implemented\n");
    return kAudioHardwareUnsupportedOperationError;
}

#endif

//---------------------------------------------------------------------------------------------------------------------------------
bool TJackClient::ReadPref()
{
    CFURLRef prefURL;
    FSRef prefFolderRef;
    OSErr err;
    char buf[256];
    char path[256];
    bool res = false;

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
				fscanf(
					prefFile, "\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s",
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
 					&TJackClient::fCoreAudioDriverUID
                );

                fclose(prefFile);
				TJackClient::fInputChannels = input;
				TJackClient::fOutputChannels = output;
				TJackClient::fAutoConnect = autoconnect;
				TJackClient::fDefaultInput = default_input;
				TJackClient::fDefaultOutput = default_output;
				TJackClient::fDefaultSystem = default_system;
				TJackClient::fDebug = debug;
                JARLog("Reading Preferences fInputChannels: %ld fOutputChannels: %ld fAutoConnect: %ld\n",
                       TJackClient::fInputChannels, TJackClient::fOutputChannels, TJackClient::fAutoConnect);
                JARLog("Reading Preferences fDefaultInput: %ld fDefaultOutput: %ld fDefaultSystem: %ld fDeviceID: %ld\n",
                       TJackClient::fInputChannels, TJackClient::fOutputChannels, TJackClient::fAutoConnect, TJackClient::fCoreAudioDriver);
                res = true;
            }
			CFRelease(prefURL);
        }
    }

    char* path1 = "/Library/Audio/Plug-Ins/HAL/JackRouter.plugin/Contents/Resources/BlackList.txt";
    FILE* blackListFile;
	
    if ((blackListFile = fopen(path1, "rt"))) {
        char client_name[64];
        char line[500];
        while (fgets(line, 500, blackListFile)) {
            sscanf(line, "%s", client_name);
            TJackClient::fBlackList->insert(client_name);
            JARLog("Blacklisted client %s\n", client_name);
        }
    }
	
	path1 = "/Library/Audio/Plug-Ins/HAL/JackRouterMP.plugin/Contents/Resources/BlackList.txt";
	if ((blackListFile = fopen(path1, "rt"))) {
        char client_name[64];
        char line[500];
        while (fgets(line, 500, blackListFile)) {
            sscanf(line, "%s", client_name);
            TJackClient::fBlackList->insert(client_name);
            JARLog("Blacklisted client %s\n", client_name);
        }
    }

    return res;
}

//---------------------------------------------------------------------------------------------------------------------------------
jack_client_t* TJackClient::CheckServer(AudioHardwarePlugInRef inSelf)
{
    jack_client_t * client;
    char name [JACK_CLIENT_NAME_LEN];
    sprintf(name, "CA::%ld", (long)inSelf);

    if (client = jack_client_new(name)) {
        TJackClient::fBufferSize = jack_get_buffer_size(client);
        TJackClient::fSampleRate = jack_get_sample_rate(client);
        TJackClient::fDeviceRunning = true;

        JARLog("CheckServer TJackClient::fBufferSizer %ld\n", TJackClient::fBufferSize);
        JARLog("CheckServer TJackClient::fSampleRate  %f\n", TJackClient::fSampleRate);

        OSStatus err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                       TJackClient::fDeviceID,
                       0,
                       0,
                       kAudioDevicePropertyBufferSize);
        JARLog("CheckServer kAudioDevicePropertyBufferSize err %ld\n", err);

        err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                TJackClient::fDeviceID,
                0,
                0,
                kAudioDevicePropertyBufferFrameSize);
        JARLog("CheckServer kAudioDevicePropertyBufferFrameSize err %ld\n", err);

        err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                TJackClient::fDeviceID,
                0,
                0,
                kAudioDevicePropertyNominalSampleRate);
        JARLog("CheckServer kAudioDevicePropertyNominalSampleRate err %ld\n", err);

        err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                TJackClient::fDeviceID,
                0,
                0,
                kAudioDevicePropertyActualSampleRate);
        JARLog("CheckServer kAudioDevicePropertyActualSampleRate err %ld\n", err);

        err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef,
                TJackClient::fDeviceID,
                0,
                0,
                kAudioDevicePropertyDeviceIsAlive);
        JARLog("CheckServer kAudioDevicePropertyIsRunning err %ld\n", err);

        return client;
    } else
        return NULL;
}

//---------------------------------------------------------------------------------------------------------------------------------
bool TJackClient::CheckRunning(AudioHardwarePlugInRef inSelf)
{
    if (TJackClient::fDeviceRunning) {
		// Retrieve CoreAudio driver AudioDeviceID 
		if (fCoreAudioDriver == 0) {
			UInt32 size = sizeof(AudioValueTranslation);
			CFStringRef inIUD = CFStringCreateWithCString(NULL, fCoreAudioDriverUID, CFStringGetSystemEncoding());
			AudioValueTranslation value = { &inIUD, sizeof(CFStringRef), &fCoreAudioDriver, sizeof(AudioDeviceID) };
			if (inIUD != NULL)  {
				OSStatus err = AudioHardwareGetProperty(kAudioHardwarePropertyDeviceForUID, &size, &value);
				JARLog("CheckRunning fCoreAudioDriverUID %s err =  %d fCoreAudioDriver = %ld\n", fCoreAudioDriverUID, err, fCoreAudioDriver);
				printError(err);
				CFRelease(inIUD);
			}
		}
	    return true;
    } else {
        jack_client_t * client = TJackClient::CheckServer(inSelf);
        if (client) {
            jack_client_close(client);
            return true;
        } else {
            return false;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::Initialize(AudioHardwarePlugInRef inSelf)
{
    OSStatus err = kAudioHardwareNoError;
    char* id_name = bequite_getNameFromPid((int)getpid());

    // Set of always "blacklisted" clients
    fBlackList = new set<string>();
    TJackClient::fBlackList->insert("jackd");
    TJackClient::fBlackList->insert("jackdmp");

    bool prefOK = ReadPref();

    JARLog("Initialize [inSelf, name] : %ld %s \n", (long)inSelf, id_name);
    TJackClient::fPlugInRef = inSelf;

#ifdef JACK_NOTIFICATION
    // Start notifications (but not for JackPilot)
    if (strcmp(id_name, "JackPilot") != 0) {
        StartNotification();
    }
#endif

    // Reject "blacklisted" clients
    if (TJackClient::fBlackList->find(id_name) != TJackClient::fBlackList->end()) {
        JARLog("Rejected client : %s\n", id_name);
        return noErr;
    }

#ifdef kAudioHardwarePlugInInterface2ID
    UInt32 theSize = 0;
    UInt32 outData = 0;
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyProcessIsMaster, &theSize, NULL);
    JARLog("kAudioHardwarePropertyProcessIsMaster err theSize %ld %ld\n", err, theSize);
    err = AudioHardwareGetProperty(kAudioHardwarePropertyProcessIsMaster, &theSize, &outData);
    JARLog("kAudioHardwarePropertyProcessIsMaster err outData %ld %ld\n", err, outData);
#endif

    jack_client_t* client;

	if (strcmp("coreaudiod", id_name) != 0) {
	
		if (client = TJackClient::CheckServer(inSelf)) {

			const char** ports;
			
			if (!prefOK) {

				int i = 0;
				if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput)) != NULL) {
					while (ports[i])
						i++;
				}
				TJackClient::fInputChannels = max(2, i); // At least 2 channels

				i = 0;
				if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsInput)) != NULL) {
					while (ports[i])
						i++;
				}
				TJackClient::fOutputChannels = max(2, i); // At least 2 channels
			}

			assert(TJackClient::fInputChannels < MAX_JACK_PORTS);
			assert(TJackClient::fOutputChannels < MAX_JACK_PORTS);
		
			JARLog("fInputChannels %ld \n", TJackClient::fInputChannels);
			JARLog("fOutputChannels %ld \n", TJackClient::fOutputChannels);
			jack_client_close(client);

		} else {
			JARLog("jack server not running?\n");
			return kAudioHardwareNotRunningError;
		}
	}

    err = AudioHardwareClaimAudioDeviceID(inSelf, &TJackClient::fDeviceID);
    if (err == kAudioHardwareNoError) {
        JARLog("AudioHardwareClaimAudioDeviceID %ld\n", TJackClient::fDeviceID);
        err = AudioHardwareDevicesCreated(inSelf, 1, &TJackClient::fDeviceID);
    }
    if (err == kAudioHardwareNoError) {
        for (int i = 0; i < TJackClient::fOutputChannels + TJackClient::fInputChannels; i++) {
            err = AudioHardwareClaimAudioStreamID(inSelf, TJackClient::fDeviceID, &fStreamIDList[i]);
            JARLog("AudioHardwareClaimAudioStreamID %ld\n", (long)TJackClient::fStreamIDList[i]);
            if (err != kAudioHardwareNoError)
                return err;
        }

        err = AudioHardwareStreamsCreated(inSelf, TJackClient::fDeviceID, TJackClient::fOutputChannels + TJackClient::fInputChannels, &TJackClient::fStreamIDList[0]);
        if (err != kAudioHardwareNoError)
            return err;
        TJackClient::fConnected2HAL = true;
    }
	
	if (TJackClient::fDefaultInput) {	 
		err = AudioHardwareSetProperty(kAudioHardwarePropertyDefaultInputDevice, sizeof(UInt32), &TJackClient::fDeviceID);	 
		if (err != kAudioHardwareNoError)	 
			return err;	 
	}	 
 	 
	if (TJackClient::fDefaultOutput) {	 
		err = AudioHardwareSetProperty(kAudioHardwarePropertyDefaultOutputDevice, sizeof(UInt32), &TJackClient::fDeviceID);	 
		if (err != kAudioHardwareNoError)	 
			return err;	 
	}	 
 	 
	if (TJackClient::fDefaultSystem) {	 
		err = AudioHardwareSetProperty(kAudioHardwarePropertyDefaultSystemOutputDevice, sizeof(UInt32), &TJackClient::fDeviceID);	 
		if (err != kAudioHardwareNoError)	 
			return err;	 
	}	 

    return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::Teardown(AudioHardwarePlugInRef inSelf)
{
    char* id_name = bequite_getNameFromPid((int)getpid());
	
	JARLog("Teardown [inSelf, name] : %ld %s \n", (long)inSelf, id_name);
    KillJackClient(); // In case the client did not correctly quit itself (like iMovie...)
#ifdef JACK_NOTIFICATION
    StopNotification();
#endif

    OSStatus err = AudioHardwareStreamsDied(inSelf, TJackClient::fDeviceID, TJackClient::fOutputChannels + TJackClient::fInputChannels, &TJackClient::fStreamIDList[0]);
    JARLog("Teardown : AudioHardwareStreamsDied\n");
    printError(err);

    if (TJackClient::fConnected2HAL) {
        err = AudioHardwareDevicesDied(inSelf, 1, &TJackClient::fDeviceID);
        JARLog("Teardown : AudioHardwareDevicesDied\n");
        printError(err);
        JARLog("Teardown : connection to HAL\n");
    } else {
        JARLog("Teardown : no connection to HAL\n");
    }

    delete fBlackList;
	fBlackList = NULL;
    return kAudioHardwareNoError;
}

