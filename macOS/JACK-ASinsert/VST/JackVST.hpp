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

// JackVST.hpp

#include "audioeffectx.h"
#include <Jack/jack.h>
#include <stdio.h>
#include <memory.h>
#include <ringbuffer.h>
#include <Carbon/Carbon.h>
#include <list>
using namespace std;

#define MAX_PORTS 2


//-------------------------------------------------------------------------------------------------------

enum {
    kIsOn,kIsOff
};


class GuiComm {
    public:
        char fileName[256];
        char * getFileName() { return fileName; }
        void setFileName(char *name) { strcpy(fileName,name); }
};



class JackVST : public AudioEffectX
{
	public:
	
		JackVST (audioMasterCallback audioMaster);
		~JackVST ();
		// Processes
			GuiComm laGui;
		virtual void process (float **inputs, float **outputs, long sampleFrames);
		virtual void processReplacing (float **inputs, float **outputs, long sampleFrames);

		// Program
		virtual void setProgramName (char *name);
		virtual void getProgramName (char *name);

		// Parameters
		virtual void setParameter (long index, float value);
		virtual float getParameter (long index);
		virtual void getParameterLabel (long index, char *label);
		virtual void getParameterDisplay (long index, char *text);
		virtual void getParameterName (long index, char *text);

		virtual bool getEffectName (char* name);
		virtual bool getVendorString (char* text);
		virtual bool getProductString (char* text);
		virtual long getVendorVersion () { return 1000; }
		
		virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }
		
		bool Open();
		void Close();
	
	    
	private:
	
		bool CheckClient();
		
		// Global state
		static list<JackVST*>   fPlugInList;
        static jack_client_t*   fJackClient;
		static int				fInstances;
      	
		float* fRBufferIn[MAX_PORTS];
        float* fRBufferOut[MAX_PORTS];
		
		RingBuffer fRingBufferIn[MAX_PORTS];
        RingBuffer fRingBufferOut[MAX_PORTS];
		
        long fBufferSize;
		
        jack_port_t **  fInPorts;
        jack_port_t **  fOutPorts;
        int				fInPortsNum;
		int				fOutPortsNum;
        bool			fJackIsOn;
		int				fStatus;
		float			fGain;
		char			fProgramName[32];
		
		static int JackProcess(jack_nframes_t nframes, void *arg);
  };


