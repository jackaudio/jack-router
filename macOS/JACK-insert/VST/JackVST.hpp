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
#include <CoreAudio/CoreAudio.h>
#include <jack/jack.h>
#include <stdio.h>
#include <memory.h>
#include <JARInsert.h>
#include <Carbon/Carbon.h>


//-------------------------------------------------------------------------------------------------------

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
	virtual void process (float **inputs, float **outputs, VstInt32 sampleFrames);
	virtual void processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames);

	// Program
	virtual void setProgramName (char *name);
	virtual void getProgramName (char *name);

	// Parameters
	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char *label);
	virtual void getParameterDisplay (VstInt32 index, char *text);
	virtual void getParameterName (VstInt32 index, char *text);

	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion () { return 1000; }
	
	virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }        
private:
	JARInsert *c_jar;
	int c_error;
	float fGain;
	char programName[32];
};


