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

// JackVST.cpp

#include <JackVST.hpp>

//-------------------------------------------------------------------------------------------------------
int JackVST::fInstances = 0;
jack_client_t* JackVST::fJackClient = NULL;
list<JackVST*> JackVST::fPlugInList;

//-------------------------------------------------------------------------------------------------------
bool JackVST::CheckClient() 
{
	if (JackVST::fJackClient) {
		return true;
	}else{
		JackVST::fJackClient = jack_client_new("JACK-ASinsert");
		if (JackVST::fJackClient){
			jack_set_process_callback(JackVST::fJackClient,JackProcess,NULL);
			return true;
		}else{
			return false;
		}
	}
}

//-------------------------------------------------------------------------------------------------------
JackVST::JackVST (audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, 1, 1)	// 1 program, 1 parameter only
{
	fGain = 1.;				// default to 0 dB
	setNumInputs (2);		// stereo in
	setNumOutputs (2);		// stereo out
	setUniqueID ('JACK-ASinsert');	// identify
	canMono ();				// makes sense to feed both inputs with the same signal
	canProcessReplacing ();	// supports both accumulating and replacing output
	strcpy (fProgramName, "Default");	// default program name
	fStatus = kIsOff;
}

//-------------------------------------------------------------------------------------------------------
bool JackVST::Open()
{
	if (CheckClient()) {
	
		fInPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*MAX_PORTS);
		fOutPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*MAX_PORTS);

		fBufferSize = jack_get_buffer_size (fJackClient);
		fInPortsNum = fOutPortsNum = MAX_PORTS;
		
		// steph : are 32 buffers really needed??
		int numBuff = 4;

		for(int i = 0; i < fInPortsNum; i++) {
			char newName[256];
			sprintf(newName,"VSTreturn%d",fInstances+i+1);
			fInPorts[i] = jack_port_register(JackVST::fJackClient,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);
			printf("Port: %s created\n",newName);
			fRBufferIn[i] = (float*)malloc(sizeof(float)*fBufferSize*numBuff);
			if (RingBuffer_Init(&fRingBufferIn[i],fBufferSize*numBuff*sizeof(float),fRBufferIn[i])==-1) printf("error while creating ring buffer.\n");
		}

		for(int i = 0; i < fOutPortsNum; i++) {
			char newName[256];
			sprintf(newName,"VSTsend%d",fInstances+i+1);
			fOutPorts[i] = jack_port_register(JackVST::fJackClient,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);
			printf("Port: %s created\n",newName);
			fRBufferOut[i] = (float*)malloc(sizeof(float)*fBufferSize*numBuff);
			if (RingBuffer_Init(&fRingBufferOut[i],fBufferSize*numBuff*sizeof(float),fRBufferOut[i])==-1) printf("error while creating ring buffer.\n");
		}
			
		fStatus = kIsOn;
		fInstances += MAX_PORTS;
		JackVST::fPlugInList.push_front(this);
		jack_activate(JackVST::fJackClient); 
		return true;
	}else{
		return false;
	}
}

//-------------------------------------------------------------------------------------------------------
JackVST::~JackVST ()
{
	if(fStatus == kIsOn) Close();
}

//-------------------------------------------------------------------------------------------------------
void JackVST::setProgramName (char *name)
{
	strcpy (fProgramName, name);
}

//-----------------------------------------------------------------------------------------
void JackVST::getProgramName (char *name)
{
	strcpy (name, fProgramName);
}

//-----------------------------------------------------------------------------------------
void JackVST::setParameter (long index, float value)
{
	fGain = value;
}

//-----------------------------------------------------------------------------------------
float JackVST::getParameter (long index)
{
	return fGain;
}

//-----------------------------------------------------------------------------------------
void JackVST::getParameterName (long index, char *label)
{
    if(fJackIsOn)
		strcpy (label, "ONLINE");
    else 
        strcpy (label, "OFFLINE");
}

//-----------------------------------------------------------------------------------------
void JackVST::getParameterDisplay (long index, char *text)
{
	dB2string (fGain, text);
}

//-----------------------------------------------------------------------------------------
void JackVST::getParameterLabel(long index, char *label)
{
	strcpy (label, "");
}

//------------------------------------------------------------------------
bool JackVST::getEffectName (char* name)
{
	strcpy (name, "JACK-ASinsert");
	return true;
}

//------------------------------------------------------------------------
bool JackVST::getProductString (char* text)
{
	strcpy (text, "JACK-ASinsert");
	return true;
}

//------------------------------------------------------------------------
bool JackVST::getVendorString (char* text)
{
	strcpy (text, "(c) 2004, Johnny Petrantoni.");
	return true;
}

//-----------------------------------------------------------------------------------------
int JackVST::JackProcess(jack_nframes_t nframes, void *arg) 
{
	list<JackVST*>::iterator it;
	
	for(it = JackVST::fPlugInList.begin(); it != JackVST::fPlugInList.end(); ++it) {
		JackVST *c = *it;
		if (c->fStatus == kIsOn) {
			for(int i = 0; i < c->fInPortsNum; i++) {
				RingBuffer_Write(&c->fRingBufferIn[i],jack_port_get_buffer(c->fInPorts[i],nframes),nframes*sizeof(float));
			}
			for(int i = 0; i < c->fOutPortsNum; i++) {
				RingBuffer_Read(&c->fRingBufferOut[i],jack_port_get_buffer(c->fOutPorts[i],nframes),nframes*sizeof(float));
			}
		}
	}
	
	return 0;
}

//-----------------------------------------------------------------------------------------
void JackVST::process (float **inputs, float **outputs, long sampleFrames)
{
	if(fStatus == kIsOn) {
		for(int i = 0; i < fOutPortsNum; i++) {
			RingBuffer_Write(&fRingBufferOut[i],inputs[i],sizeof(float)*sampleFrames);
		}
		for(int i = 0; i < fInPortsNum; i++) {
			RingBuffer_Read(&fRingBufferIn[i],outputs[i],sizeof(float)*sampleFrames);
		}
    }
}

//-----------------------------------------------------------------------------------------
void JackVST::processReplacing (float **inputs, float **outputs, long sampleFrames)
{	
    if(fStatus == kIsOn) {
		for(int i = 0; i < fOutPortsNum;i++) {
			RingBuffer_Write(&fRingBufferOut[i],inputs[i],sizeof(float)*sampleFrames);
		}
		for(int i = 0; i < fInPortsNum;i++) {
			RingBuffer_Read(&fRingBufferIn[i],outputs[i],sizeof(float)*sampleFrames);
		}
    }
}

//-----------------------------------------------------------------------------------------
void JackVST::Close()
{
	fStatus = kIsOff;
	list<JackVST*>::iterator it;
	
	printf("actually there are %ld instances.\n",JackVST::fPlugInList.size());
	JackVST::fPlugInList.remove(this);
	printf("now there are %ld instances.\n",JackVST::fPlugInList.size());
	
	for(int i = 0; i < fInPortsNum; i++) {
		RingBuffer_Flush(&fRingBufferIn[i]);	
		free(fRBufferIn[i]);
		jack_port_unregister(JackVST::fJackClient,fInPorts[i]);
		printf("unregistering in port %d.\n",i);
	}
	free(fInPorts);
	
	for(int i = 0; i < fOutPortsNum; i++) {
		RingBuffer_Flush(&fRingBufferIn[i]);	
		free(fRBufferOut[i]);
		jack_port_unregister(JackVST::fJackClient,fOutPorts[i]);
		printf("unregistering out port %d.\n",i);
	}
	free(fOutPorts);
				
	if(JackVST::fPlugInList.size() == 0) { 
		printf("closing client.\n"); 
		jack_deactivate(JackVST::fJackClient); 
		jack_client_close(JackVST::fJackClient);
		JackVST::fJackClient = NULL;
		JackVST::fInstances = 0;
	} 
}

