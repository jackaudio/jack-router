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
	JackRouterDevice.h
=============================================================================*/
#if !defined(__JackRouterDevice_h__)
#define __JackRouterDevice_h__

// JACK include
#include <jack/jack.h>

//=============================================================================
//	Types
//=============================================================================

class	HP_DeviceControlProperty;
class	HP_HogMode;
class	HP_IOProc;
class   HP_IOThread;
class	JackRouterPlugIn;
class   JackRouterStream;
class	JackRouterDevice;

//=============================================================================
//	JackRouterDevice
//=============================================================================

#define JACK_PORT_NAME_LEN 256
#define LOG_SAMPLE_DURATION 3	// in millisecond

#include "CALatencyLog.h"
#include "JackRouterDeviceInterface.h"

class CommandThread
{

//	Constants
public:

	JackRouterDevice*			mDevice;

//	Construction/Destruction
public:
						CommandThread(JackRouterDevice* inDevice);
	virtual				~CommandThread();
	
	CAPThread			mCommandThread;
	CAGuard				mCommandGuard;
	void				WorkLoop();
	
	void				Start();
	
protected:	
	
	static void*		ThreadEntry(CommandThread* inIOThread);
};

class JackRouterDevice
:
	public JackRouterDeviceInterface
{

	// Special property fo access from Jack plug-ins (AU, VST)
    enum {
        kAudioDevicePropertyGetJackClient = 'jasg',
        kAudioDevicePropertyReleaseJackClient = 'jasr',
        kAudioDevicePropertyAllocateJackPortVST = 'jpav',
        kAudioDevicePropertyAllocateJackPortAU = 'jpaa',
        kAudioDevicePropertyGetJackPortVST = 'jpgv',
        kAudioDevicePropertyGetJackPortAU = 'jpga',
        kAudioDevicePropertyReleaseJackPortVST = 'jprv',
        kAudioDevicePropertyReleaseJackPortAU = 'jpra',
        kAudioDevicePropertyDeactivateJack = 'daja',
        kAudioDevicePropertyActivateJack = 'aaja'
    };

	// Construction/Destruction
public:
								JackRouterDevice(AudioDeviceID inAudioDeviceID, JackRouterPlugIn* inPlugIn);
	virtual						~JackRouterDevice();

	virtual void				Initialize();
	virtual void				Teardown();
	virtual void				Finalize();
	
	//virtual void				CreateForHAL(AudioDeviceID theNewDeviceID);
	virtual void				ReleaseFromHAL();

protected:
	JackRouterPlugIn*			mSHPPlugIn;
	
//	Attributes
public:
	JackRouterPlugIn*			GetSHPPlugIn() const { return mSHPPlugIn; }
	virtual CFStringRef			CopyDeviceName() const;
	virtual CFStringRef			CopyDeviceManufacturerName() const;
	virtual CFStringRef			CopyDeviceUID() const;
	
	bool CanBeDefaultDevice(bool /*inIsInput*/, bool /*inIsSystem*/) const;

private:

    UInt64 mAnchorHostTime;  // From SHP example
    Float64 mAnchorSampleTime;
 
	jack_client_t* fClient;				// Jack client
  
    jack_port_t** fInputPortList;		// Jack input ports
	jack_port_t** fOutputPortList;		// Jack output ports

	std::map<int, std::pair<float*, jack_port_t*> > fPlugInPortsVST;	// Map of temp buffers and associated Jack ports to be used by AU plug-ins
	std::map<int, std::pair<float*, jack_port_t*> > fPlugInPortsAU;		// Map of temp buffers and associated Jack ports to be used by VST plug-ins

	AudioBufferList* fInputList;										// CoreAudio input buffers
	AudioBufferList* fOutputList;										// CoreAudio output buffers
	float** fOutputListTemp;												// Intermediate output buffers
	
	std::list<std::pair<std::string, std::string> > fConnections;		// Connections list
	
	bool fFirstActivate;
	
	CAGuard	mIOGuard;
	CommandThread* mCommandThread;
	
	CALatencyLog* mLogFile;

public:
	
	// Static 
	static int fInputChannels;
	static int fOutputChannels;
	
	static bool fAutoConnect;
	
	static bool fDefaultInput;
	static bool fDefaultOutput;
	static bool fDefaultSystem;
	
	static int fBufferSize;
	static float fSampleRate;
    
    static UInt64 fSampleCount; 
	
	static char fCoreAudioDriverUID[128];		// The CoreAudio driver currently loaded by Jack

//	Property Access
public:
	virtual bool				HasProperty(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool				IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32				GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void				GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void				SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);

protected:
	virtual void				PropertyListenerAdded(const AudioObjectPropertyAddress& inAddress);

//	Command Management
protected:

	bool						IsSafeToExecuteCommand();
	virtual bool				StartCommandExecution(void** outSavedCommandState);
	virtual void				FinishCommandExecution(void* inSavedCommandState);

//	IOProc Management
public:
	virtual void				Do_StartIOProc(AudioDeviceIOProc inProc);
	virtual void				Do_StartIOProcAtTime(AudioDeviceIOProc inProc, AudioTimeStamp& ioStartTime, UInt32 inStartTimeFlags);
	virtual void				Do_StopIOProc(AudioDeviceIOProcID inProcID);
	virtual	AudioDeviceIOProcID	Do_CreateIOProcID(AudioDeviceIOProc inProc, void* inClientData);
	
	virtual void				AddIOProc(AudioDeviceIOProc inProc, void* inClientData);
	virtual void				RemoveIOProc(AudioDeviceIOProc inProc);
	virtual void				StopAllIOProcs();
	
	CAGuard*					GetIOGuard() {return &mIOGuard;}
	
	int							GetCommands() {return mCommandList.size();}
	
//  IO Management
public:
	
protected:
	virtual void				StartIOEngine();
	virtual void				StartIOEngineAtTime(const AudioTimeStamp& inStartTime, UInt32 inStartTimeFlags);
	virtual void				StopIOEngine();
	
	virtual void				StartHardware();
	virtual void				StopHardware();
	
	UInt32						GetMinimumIOBufferFrameSize() const;
	UInt32						GetMaximumIOBufferFrameSize() const;
	
//	Time Management
public:
	virtual void				GetCurrentTime(AudioTimeStamp& outTime);
    virtual void				GetCallbackCurrentTime(AudioTimeStamp& outTime, Float64 callback_sample_time);
    
	virtual void				SafeGetCurrentTime(AudioTimeStamp& outTime);
	virtual void				TranslateTime(const AudioTimeStamp& inTime, AudioTimeStamp& outTime);
	virtual void				GetNearestStartTime(AudioTimeStamp& ioRequestedStartTime, UInt32 inFlags);
    void                        StartIOCycleTimingServices();
    void                        StopIOCycleTimingServices();
    virtual Float64             GetCurrentActualSampleRate() const;
 
private:
		
//  Stream Management
private:
	void						CreateStreams();
 	void						ReleaseStreams();

private:
	
	// JACK
	void						SaveConnections();
	void						RestoreConnections();
	bool						AutoConnect();
	
	bool						Open();
	void						Close();
	void						Destroy();
	
	bool						AllocatePorts();
	void						DisposePorts();
	bool						Activate();
	bool						Desactivate();
	
	bool						AllocatePlugInPortVST(int num);
	bool						AllocatePlugInPortAU(int num);
	float*						GetPlugInPortVST(int num);
	float*						GetPlugInPortAU(int num);
	void						ReleasePlugInPortVST(int num);
	void						ReleasePlugInPortAU(int num);
	
	int							GetBufferSize();
  	
	// JACK callbacks
    static void					Init(void* arg);
	static int					Process(jack_nframes_t nframes, void* arg);
	static int					BufferSize(jack_nframes_t nframes, void* arg);
	static void					Shutdown(void* arg);
	static int					XRun(void* arg);
};

#endif
