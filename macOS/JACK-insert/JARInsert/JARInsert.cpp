/*
JARInsert.cpp

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

#include "JARInsert.h"

int JARInsert::c_instances = 0;
int JARInsert::c_instances_count = 0;
bool JARInsert::c_printDebug = false;

extern "C" void JARILog(char *fmt, ...)
{
    if (JARInsert::c_printDebug) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stdout, "JARInsert Log: ");
        vfprintf(stdout, fmt, ap);
        va_end(ap);
    }
}

JARInsert::JARInsert(long host_buffer_size, int hostType)
        : c_error(kNoErr), 
		c_client(NULL), 
		c_isRunning(false), 
		c_rBufOn(false), 
		c_needsDeactivate(false), 
		c_hBufferSize(host_buffer_size), 
		c_hostType(hostType)
{
    ReadPrefs();
	
	UInt32 outSize;
    Boolean isWritable;
	
    if (!OpenAudioClient()) {
        JARILog("Cannot find jack client.\n");
        SHOWALERT("Cannot find jack client for this application, check if Jack server is running.");
        return;
    }
	
	// Deactivate Jack callback
	//AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyDeactivateJack, &outSize, &isWritable);
	//AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyDeactivateJack, 0, NULL);

    int nPorts = 2;
	
	c_inPorts = (jack_port_t**)malloc(sizeof(jack_port_t*) * nPorts);
    c_outPorts = (float**)malloc(sizeof(float*) * nPorts);

    c_nInPorts = c_nOutPorts = nPorts;
    c_jBufferSize = jack_get_buffer_size(c_client);

    char name[256];

    for (int i = 0;i < c_nInPorts;i++) {
        if (hostType == 'vst ')
            sprintf(name, "VSTreturn%d", JARInsert::c_instances + i + 1);
        else
            sprintf(name, "AUreturn%d", JARInsert::c_instances + i + 1);
		c_inPorts[i] = jack_port_register(c_client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        JARILog("Port: %s created\n", name);
    }
	
	c_instance = JARInsert::c_instances;
	
	for (int i = 0; i < c_nOutPorts; i++) {
		UInt32 portNum = c_instance + i;
        if (hostType == 'vst ') {
			AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyAllocateJackPortVST, &outSize, &isWritable);
            AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyAllocateJackPortVST, portNum, NULL);
			AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyGetJackPortVST, &outSize, &isWritable);
			AudioDeviceGetProperty(c_jackDevID, 0, true, kAudioDevicePropertyGetJackPortVST, &portNum, &c_outPorts[i]);
		} else {
			AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyAllocateJackPortAU, &outSize, &isWritable);
			AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyAllocateJackPortAU, portNum, NULL);
			AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyGetJackPortAU, &outSize, &isWritable);
			AudioDeviceGetProperty(c_jackDevID, 0, true, kAudioDevicePropertyGetJackPortAU, &portNum, &c_outPorts[i]);
		}
 		
		JARILog("Port: %s created\n", name);
    }

#if 0
    if (!c_isRunning) {
        JARILog("Jack client activated\n");
        jack_activate(c_client);
        c_needsDeactivate = true;
    } else
        c_needsDeactivate = false;
#endif
    
    if (c_jBufferSize > c_hBufferSize) {
        c_bsAI1 = new BSizeAlign(c_hBufferSize, c_jBufferSize);
        c_bsAI2 = new BSizeAlign(c_hBufferSize, c_jBufferSize);
        c_bsAO1 = new BSizeAlign(c_jBufferSize, c_hBufferSize);
        c_bsAO2 = new BSizeAlign(c_jBufferSize, c_hBufferSize);
        if (c_bsAI1->Ready() && c_bsAI2->Ready() && c_bsAO1->Ready() && c_bsAO2->Ready()) {
            c_rBufOn = true;
        } else {
            c_error = kErrInvalidBSize;
            Flush();
            return ;
        }
    }

    JARInsert::c_instances += 2;
	JARInsert::c_instances_count++;
    c_canProcess = true;
	
	// (Possible) reactivate Jack callback
	//AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyActivateJack, &outSize, &isWritable);
	//AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyActivateJack, 0, NULL);
}

JARInsert::JARInsert(int hostType)
        : c_error(kNoErr), 
		c_client(NULL), 
		c_isRunning(false), 
		c_rBufOn(false), 
		c_needsDeactivate(false), 
		c_hBufferSize(0),  
		c_hostType(hostType)
{
    ReadPrefs();
	
	UInt32 outSize;
    Boolean isWritable;

    if (!OpenAudioClient()) {
        JARILog("Cannot find jack client.\n");
        SHOWALERT("Cannot find jack client for this application, check if Jack server is running.");
        return ;
    }
	
	// Deactivate Jack callback
	//AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyDeactivateJack, &outSize, &isWritable);
	//AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyDeactivateJack, 0, NULL);

    int nPorts = 2;

	c_inPorts = (jack_port_t**)malloc(sizeof(jack_port_t*) * nPorts);
    c_outPorts = (float**)malloc(sizeof(float*) * nPorts);
	
    c_nInPorts = c_nOutPorts = nPorts;
    c_jBufferSize = jack_get_buffer_size(c_client);

    char name[256];

    for (int i = 0; i < c_nInPorts; i++) {
        if (hostType == 'vst ')
            sprintf(name, "VSTreturn%d", JARInsert::c_instances + i + 1);
        else
            sprintf(name, "AUreturn%d", JARInsert::c_instances + i + 1);
        c_inPorts[i] = jack_port_register(c_client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        JARILog("Port: %s created\n", name);
    }

	c_instance = JARInsert::c_instances;
	
	for (int i = 0; i < c_nOutPorts; i++) {
		UInt32 portNum = c_instance + i;
        if (hostType == 'vst ') {
			AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyAllocateJackPortVST, &outSize, &isWritable);
            AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyAllocateJackPortVST, portNum, NULL);
			AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyGetJackPortVST, &outSize, &isWritable);
			AudioDeviceGetProperty(c_jackDevID, 0, true, kAudioDevicePropertyGetJackPortVST, &portNum, &c_outPorts[i]);
		} else {
			AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyAllocateJackPortAU, &outSize, &isWritable);
			AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyAllocateJackPortAU, portNum, NULL);
			AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyGetJackPortAU, &outSize, &isWritable);
			AudioDeviceGetProperty(c_jackDevID, 0, true, kAudioDevicePropertyGetJackPortAU, &portNum, &c_outPorts[i]);
		}
 		
		JARILog("Port: %s created\n", name);
    }

#if 0
    if (!c_isRunning) {
        JARILog("Jack client activated\n");
        jack_activate(c_client);
        c_needsDeactivate = true;
    } else
        c_needsDeactivate = false;
#endif

    JARInsert::c_instances += 2;
    JARInsert::c_instances_count++;
    c_canProcess = false;
	
	// (Possible) reactivate Jack callback
	//AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyActivateJack, &outSize, &isWritable);
	//AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyActivateJack, 0, NULL);
}

JARInsert::~JARInsert()
{
    if (c_error == kNoErr)
        Flush();
}

bool JARInsert::AllocBSizeAlign(long host_buffer_size)
{
	JARILog("AllocBSizeAlign host_buffer_size = %ld \n", host_buffer_size);
	
    c_hBufferSize = host_buffer_size;
    if (c_jBufferSize > c_hBufferSize) {  
		
        if (((host_buffer_size - 1) & host_buffer_size) != 0) {
			JARILog("Bad buffer size for BSizeAlign host_buffer_size %ld \n", host_buffer_size);
            c_error = kErrInvalidBSize;
            Flush();
            return false;
        }
		
		JARILog("AllocBSizeAlign c_jBufferSize = %ld c_hBufferSize = %ld\n", c_jBufferSize, c_hBufferSize);
		
        c_bsAI1 = new BSizeAlign(c_hBufferSize, c_jBufferSize);
        c_bsAI2 = new BSizeAlign(c_hBufferSize, c_jBufferSize);
        c_bsAO1 = new BSizeAlign(c_jBufferSize, c_hBufferSize);
        c_bsAO2 = new BSizeAlign(c_jBufferSize, c_hBufferSize);
        if (c_bsAI1->Ready() && c_bsAI2->Ready() && c_bsAO1->Ready() && c_bsAO2->Ready()) {
            c_rBufOn = true;
        } else {
            JARILog("Bad buffer size for BSizeAlign c_hBufferSize %ld c_jBufferSize %ld \n",c_hBufferSize, c_jBufferSize);
            c_error = kErrInvalidBSize;
            Flush();
            return false;
        }
    } else if (c_jBufferSize < c_hBufferSize) {
        JARILog("Bad buffer size jack<host, must be jack>host || jack==host %ld %ld \n", c_jBufferSize, c_hBufferSize);
        c_error = kErrInvalidBSize;
        Flush();
        return false;
    }
    c_canProcess = true;
    if (c_rBufOn)
        JARILog("Using BSizeAlign.\n");
    else
        JARILog("Not Using BSizeAlign.\n");
    return true;
}

int JARInsert::Process(float** in_buffer, float** out_buffer, long host_nframes)
{
	if (c_hBufferSize != host_nframes) {
        JARILog("CRITICAL ERROR: Host Buffer Size mismatch, NOT PROCESSING!! %ld c_hBufferSize  %ld host_nframes \n", c_hBufferSize, host_nframes);
        return 1;
    }
	
    if (c_rBufOn) {
		float* out1 = c_outPorts[0];
        float* out2 = c_outPorts[1];
        float* in1 = (float*) jack_port_get_buffer(c_inPorts[0], (jack_nframes_t)c_jBufferSize);
        float* in2 = (float*) jack_port_get_buffer(c_inPorts[1], (jack_nframes_t)c_jBufferSize);
		
		c_bsAI1->AddBuffer(in_buffer[0]);
        c_bsAI2->AddBuffer(in_buffer[1]);

        if (!c_bsAO1->CanGet())
            c_bsAO1->AddBuffer(in1);
        if (!c_bsAO2->CanGet())
            c_bsAO2->AddBuffer(in2);

        if (c_bsAO1->CanGet())
            c_bsAO1->GetBuffer(out_buffer[0]);
        if (c_bsAO2->CanGet())
            c_bsAO2->GetBuffer(out_buffer[1]);

        if (c_bsAI1->CanGet())
            c_bsAI1->GetBuffer(out1);
        if (c_bsAI2->CanGet())
            c_bsAI2->GetBuffer(out2);
    } else {
        if (c_jBufferSize != host_nframes) {
            JARILog("CRITICAL ERROR: Host Buffer Size mismatch, NOT PROCESSING!! %ld c_hBufferSize  %ld host_nframes \n", c_hBufferSize, host_nframes);
            return 1;
        }
		float* out1 = c_outPorts[0];
        float* out2 = c_outPorts[1];
        float* in1 = (float*) jack_port_get_buffer(c_inPorts[0], (jack_nframes_t)c_jBufferSize);
        float* in2 = (float*) jack_port_get_buffer(c_inPorts[1], (jack_nframes_t)c_jBufferSize);
		
		memcpy(out1, in_buffer[0], sizeof(float) * c_jBufferSize);
        memcpy(out2, in_buffer[1], sizeof(float) * c_jBufferSize);
        memcpy(out_buffer[0], in1, sizeof(float) * c_jBufferSize);
        memcpy(out_buffer[1], in2, sizeof(float) * c_jBufferSize);
    }
    return 0;
}

bool JARInsert::OpenAudioClient()
{
    OSStatus err;
    UInt32 size;
    Boolean isWritable;

    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
    if (err != noErr) {
        c_error = kErrCoreAudio;
        return false;
    }

    int nDevices = size / sizeof(AudioDeviceID);
	JARILog("There are %d audio devices\n", nDevices);

    AudioDeviceID *device = (AudioDeviceID*)calloc(nDevices, sizeof(AudioDeviceID));
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, device);
    if (err != noErr) {
        c_error = kErrCoreAudio;
        return false;
    }

    for (int i = 0; i < nDevices; i++) {

        JARILog("ID: %ld\n", device[i]);

        char name[256];
        size = 256;
        err = AudioDeviceGetProperty(device[i], 0, true, kAudioDevicePropertyDeviceName, &size, &name);
        if (err == noErr) {
            JARILog("Name: %s\n", name);

            if (strcmp(&name[0], "JackRouter") == 0) {
                c_jackDevID = device[i];
                if (device != NULL)
                    free(device);
					
				JARILog("Get Jack client\n");
				size = sizeof(UInt32); 
                err = AudioDeviceGetProperty(c_jackDevID, 0, true, kAudioDevicePropertyGetJackClient, &size, &c_client);
                if (err != noErr) {
					JARILog("Get Jack client error = %d\n", err);
                    c_error = kErrNoClient;
                    return false;
                }
				
				JARILog("Get Jack client OK\n");
#if 0
				size = sizeof(UInt32);
                err = AudioDeviceGetProperty(c_jackDevID, 0, true, kAudioDevicePropertyDeviceIsRunning, &size, &c_isRunning);
                if (err != noErr) {
                    c_error = kErrNoClient;
                    return false;
                }
#endif
                if (c_client != NULL) {
                    c_error = kNoErr;
                    return true;
                } else
                    return false;
            }
        }
    }
    c_error = kErrNoClient;
    return false;
}

void JARInsert::Flush()
{
    JARILog("Running flush\n");
	UInt32 outSize;
    Boolean isWritable;
    OSStatus err;
	
    if (c_client != NULL) {
	
		// Deactivate Jack callback
		//AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyDeactivateJack, &outSize, &isWritable);
		//AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyDeactivateJack, 0, NULL);
	
        if (c_rBufOn) {
            delete c_bsAO1;
            delete c_bsAO2;
            delete c_bsAI1;
            delete c_bsAI2;
        }

#if 0
        if (c_needsDeactivate) {
            JARILog("Needs Deactivate client\n");
            jack_deactivate(c_client);
        }

#endif
        
        // Check if client is still opened
        JARILog("Get Jack client\n");
        outSize = sizeof(UInt32); 
        err = AudioDeviceGetProperty(c_jackDevID, 0, true, kAudioDevicePropertyGetJackClient, &outSize, &c_client);
        if (err != noErr) {
            JARILog("Get Jack client error = %d\n", err);
            return;
        }
        
        if (!c_client) {
            JARILog("Jack client already desallocated...\n", err);
            return;
        }
            
        for (int i = 0; i < c_nInPorts; i++) {
            jack_port_unregister(c_client, c_inPorts[i]);
        }
        free(c_inPorts);
		for (int i = 0; i < c_nOutPorts; i++) {
			UInt32 portNum = c_instance + i;
			if (c_hostType == 'vst ') {
				AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyReleaseJackPortVST, &outSize, &isWritable);
				AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyReleaseJackPortVST, portNum, NULL);
			} else {
				AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyReleaseJackPortAU, &outSize, &isWritable);
				AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyReleaseJackPortAU, portNum, NULL);
			}
        }
        free(c_outPorts);
      
		// (Possible) reactivate Jack callback
		//AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyActivateJack, &outSize, &isWritable);
		//AudioDeviceSetProperty(c_jackDevID, NULL, 0, true, kAudioDevicePropertyActivateJack, 0, NULL);
		AudioDeviceGetPropertyInfo(c_jackDevID, 0, true, kAudioDevicePropertyReleaseJackClient, &outSize, &isWritable);
        AudioDeviceGetProperty(c_jackDevID, 0, true, kAudioDevicePropertyReleaseJackClient, &outSize, &c_client);

        JARInsert::c_instances_count--;
        if (JARInsert::c_instances_count == 0)
            JARInsert::c_instances = 0;
    }
}

bool JARInsert::ReadPrefs()
{
    CFURLRef prefURL;
    FSRef prefFolderRef;
    OSErr err;
    char buf[256];
    char path[256];

    err = FSFindFolder(kUserDomain, kPreferencesFolderType, kDontCreateFolder, &prefFolderRef);
    if (err == noErr) {
        prefURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &prefFolderRef);
        if (prefURL) {
            CFURLGetFileSystemRepresentation(prefURL, FALSE, (UInt8*)buf, 256);
            sprintf(path, "%s/JAS.jpil", buf);
            FILE* prefFile;
            if ((prefFile = fopen(path, "rt"))) {
                int nullo;
                fscanf(
                    prefFile, "\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
                    &nullo,
                    &nullo,
                    &nullo,
                    &nullo,
                    &nullo,
                    &nullo,
                    &nullo,
                    &nullo,
                    &nullo,
                    &nullo,
                    &nullo,
                    &nullo,
                    (int*)&JARInsert::c_printDebug
                );
                fclose(prefFile);
                return true;
            }
        }
    }
    return false;
}

