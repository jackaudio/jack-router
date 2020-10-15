/*
  Copyright �  Johnny Petrantoni 2003
 
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
JackVST::JackVST (audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, 1, 1),c_jar(NULL),c_error(JARInsert::kNoErr)// 1 program, 1 parameter only
{
	fGain = 1.;				// default to 0 dB
	setNumInputs(2);		// stereo in
	setNumOutputs(2);		// stereo out
	setUniqueID('JACK');	// identify
	canProcessReplacing();	// supports both accumulating and replacing output
	strcpy(programName, "Default");	// default program name
	
	c_jar = new JARInsert('vst ');
	c_error = c_jar->GetError();
}

//-------------------------------------------------------------------------------------------------------
JackVST::~JackVST ()
{
	if(c_jar) delete c_jar;
}

//-------------------------------------------------------------------------------------------------------
void JackVST::setProgramName (char *name)
{
	strcpy (programName, name);
}

//-----------------------------------------------------------------------------------------
void JackVST::getProgramName (char *name)
{
	strcpy (name, programName);
}

//-----------------------------------------------------------------------------------------
void JackVST::setParameter (VstInt32 index, float value)
{
	fGain = value;
}

//-----------------------------------------------------------------------------------------
float JackVST::getParameter (VstInt32 index)
{
	return fGain;
}

//-----------------------------------------------------------------------------------------
void JackVST::getParameterName (VstInt32 index, char *label)
{
    if(c_error == JARInsert::kNoErr) strcpy (label, "ONLINE");
    else strcpy (label, "OFFLINE");
}

//-----------------------------------------------------------------------------------------
void JackVST::getParameterDisplay (VstInt32 index, char *text)
{
	dB2string (fGain, text, strlen(text));
}

//-----------------------------------------------------------------------------------------
void JackVST::getParameterLabel(VstInt32 index, char *label)
{
	strcpy (label, "");
}

//------------------------------------------------------------------------
bool JackVST::getEffectName (char* name)
{
	strcpy(name, "JACK-insert");
	return true;
}

//------------------------------------------------------------------------
bool JackVST::getProductString (char* text)
{
	strcpy(text, "JACK-insert");
	return true;
}

//------------------------------------------------------------------------
bool JackVST::getVendorString (char* text)
{
	strcpy(text, "(c) 2003-2009, J.Petrantoni, Grame");
	return true;
}

//-----------------------------------------------------------------------------------------
void JackVST::process (float **inputs, float **outputs, VstInt32 sampleFrames)
{
    processReplacing(inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------------------
void JackVST::processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames)
{
	if ((c_jar->GetError() == JARInsert::kNoErr) && c_jar) {
		if (!c_jar->CanProcess()) 
			c_jar->AllocBSizeAlign(sampleFrames);
		c_jar->Process(inputs, outputs, sampleFrames);
	} else {
		for(int i = 0; i < 2; i++) memset(outputs[i], 0x0, sizeof(float) * sampleFrames);
	}
}

