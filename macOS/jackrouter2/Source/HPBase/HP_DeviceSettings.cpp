/*	Copyright © 2007 Apple Inc. All Rights Reserved.
	
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
/*==================================================================================================
	HP_DeviceSettings.cpp

==================================================================================================*/

//==================================================================================================
//	Includes
//==================================================================================================

//	Self Include
#include "HP_DeviceSettings.h"

//	Local Includes
#include "HP_Device.h"
#include "HP_Stream.h"

//	PublicUtility Includes
#include "CAAutoDisposer.h"
#include "CACFData.h"
#include "CACFDictionary.h"
#include "CACFPreferences.h"
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAPropertyAddress.h"
#include "CAStreamBasicDescription.h"

#if	CoreAudio_Debug
//	#define	Log_SaveRestoreFromPrefs	1
#endif

#define	kHALDeviceSettingsFilePath	"/Library/Preferences/com.apple.audio.DeviceSettings.plist"

//==================================================================================================
//	HP_DeviceSettings
//==================================================================================================

static CFStringRef	HP_DeviceSettings_ConstructControlKey(UInt32 inID, UInt32 inChannel, bool inIsInput, bool inIsStream)
{
	char theID[5];
	*((UInt32*)theID) = inID;
	theID[4] = 0;
	return CFStringCreateWithFormat(NULL, NULL, CFSTR("%s '%s' control on %s channel %ld"), inIsStream ? "stream" : "device", theID, inIsInput ? "input" : "output", inChannel);
}

static CFStringRef	HP_DeviceSettings_ConstructFormatKey(UInt32 inStreamIndex, bool inIsInput)
{
	return CFStringCreateWithFormat(NULL, NULL, CFSTR("physical format for %s stream %ld"), inIsInput ? "input" : "output", inStreamIndex);
}

static void	HP_DeviceSettings_ConstructDictionaryFromFormat(const AudioStreamBasicDescription& inFormat, CACFDictionary& outFormatDictionary)
{
	outFormatDictionary.AddFloat64(CFSTR("sample rate"), inFormat.mSampleRate);
	outFormatDictionary.AddUInt32(CFSTR("format id"), inFormat.mFormatID);
	outFormatDictionary.AddUInt32(CFSTR("format flags"), inFormat.mFormatFlags);
	outFormatDictionary.AddUInt32(CFSTR("bytes per packet"), inFormat.mBytesPerPacket);
	outFormatDictionary.AddUInt32(CFSTR("frames per packet"), inFormat.mFramesPerPacket);
	outFormatDictionary.AddUInt32(CFSTR("bytes per frame"), inFormat.mBytesPerFrame);
	outFormatDictionary.AddUInt32(CFSTR("channels per frame"), inFormat.mChannelsPerFrame);
	outFormatDictionary.AddUInt32(CFSTR("bits per channel"), inFormat.mBitsPerChannel);
}

static bool	HP_DeviceSettings_ConstructFormatFromDictionary(const CACFDictionary& inFormatDictionary, AudioStreamBasicDescription& outFormat)
{
	memset(&outFormat, 0, sizeof(AudioStreamBasicDescription));
	bool theAnswer = inFormatDictionary.GetFloat64(CFSTR("sample rate"), outFormat.mSampleRate);
	theAnswer = theAnswer && inFormatDictionary.GetUInt32(CFSTR("format id"), outFormat.mFormatID);
	theAnswer = theAnswer && inFormatDictionary.GetUInt32(CFSTR("format flags"), outFormat.mFormatFlags);
	theAnswer = theAnswer && inFormatDictionary.GetUInt32(CFSTR("bytes per packet"), outFormat.mBytesPerPacket);
	theAnswer = theAnswer && inFormatDictionary.GetUInt32(CFSTR("frames per packet"), outFormat.mFramesPerPacket);
	theAnswer = theAnswer && inFormatDictionary.GetUInt32(CFSTR("bytes per frame"), outFormat.mBytesPerFrame);
	theAnswer = theAnswer && inFormatDictionary.GetUInt32(CFSTR("channels per frame"), outFormat.mChannelsPerFrame);
	theAnswer = theAnswer && inFormatDictionary.GetUInt32(CFSTR("bits per channel"), outFormat.mBitsPerChannel);
	return theAnswer;
}

const HP_DeviceSettings::ControlInfo HP_DeviceSettings::sStandardControlsToSave[] = 
{
	{ kAudioDevicePropertyVolumeScalar,				HP_DeviceSettings::kControlValueTypeFloat },
	{ kAudioDevicePropertyStereoPan,				HP_DeviceSettings::kControlValueTypeFloat },
	{ kAudioDevicePropertyMute,						HP_DeviceSettings::kControlValueTypeBool },
	{ kAudioDevicePropertySolo,						HP_DeviceSettings::kControlValueTypeBool },
	{ kAudioDevicePropertyDataSource,				HP_DeviceSettings::kControlValueType4CC },
	{ kAudioDevicePropertyClockSource,				HP_DeviceSettings::kControlValueType4CC },
	{ kAudioDevicePropertyPlayThru,					HP_DeviceSettings::kControlValueTypeBool },
	{ kAudioDevicePropertyPlayThruSolo,				HP_DeviceSettings::kControlValueTypeBool },
	{ kAudioDevicePropertyPlayThruVolumeScalar,		HP_DeviceSettings::kControlValueTypeFloat },
	{ kAudioDevicePropertyPlayThruStereoPan,		HP_DeviceSettings::kControlValueTypeFloat },
	{ kAudioDevicePropertyPlayThruDestination,		HP_DeviceSettings::kControlValueType4CC },
	{ kAudioDevicePropertyChannelNominalLineLevel,	HP_DeviceSettings::kControlValueType4CC },
	{ kAudioDevicePropertyDriverShouldOwniSub,		HP_DeviceSettings::kControlValueTypeBool },
	{ kAudioDevicePropertySubVolumeDecibels,		HP_DeviceSettings::kControlValueTypeFloat },
	{ kAudioDevicePropertySubMute,					HP_DeviceSettings::kControlValueTypeBool }
};

static void	HP_DeviceSettings_SaveControlsFromObject(const HP_Object& inObject, bool inIsStream, UInt32 inNumberChannels, CACFDictionary& outSettings, bool inIsInput, const HP_DeviceSettings::ControlInfo* inControlsToSave, UInt32 inNumberControlsToSave)
{
	CAPropertyAddress theAddress(0, inIsInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput);
	for(UInt32 theControlIndex = 0; theControlIndex < inNumberControlsToSave; ++theControlIndex)
	{
		theAddress.mSelector = inControlsToSave[theControlIndex].mSelector;
		
		//	note that 0 is the master channel, making the actual channels start at 1
		for(theAddress.mElement = 0; theAddress.mElement <= inNumberChannels; ++theAddress.mElement)
		{
			try
			{
				//	only do something if the property is present
				if(inObject.HasProperty(theAddress))
				{
					//	create the key
					CACFString theControlKey(HP_DeviceSettings_ConstructControlKey(theAddress.mSelector, theAddress.mElement, inIsInput, inIsStream));
					
					#if Log_SaveRestoreFromPrefs
						char theControlKeyString[1024];
						UInt32 theControlKeyStringSize = 1024;
						theControlKey.GetCString(theControlKeyString, theControlKeyStringSize);
					#endif
				
					//	all control values are the same size
					UInt32 theSize = 4;
				
					//	but not the same type.
					//	get the value and stuff it in the dictionary
					UInt32 theIntValue;
					Float32 theFloatValue;
					switch(inControlsToSave[theControlIndex].mValueType)
					{
						case HP_DeviceSettings::kControlValueTypeBool:
							inObject.GetPropertyData(theAddress, 0, NULL, theSize, &theIntValue);
							outSettings.AddUInt32(theControlKey.GetCFString(), theIntValue);
							#if Log_SaveRestoreFromPrefs
								DebugMessageN2("Saving %s: %lu", theControlKeyString, theIntValue);
							#endif
							break;
						
						case HP_DeviceSettings::kControlValueTypeFloat:
							inObject.GetPropertyData(theAddress, 0, NULL, theSize, &theFloatValue);
							outSettings.AddFloat32(theControlKey.GetCFString(), theFloatValue);
							#if Log_SaveRestoreFromPrefs
								DebugMessageN2("Saving %s: %f", theControlKeyString, theFloatValue);
							#endif
							break;
						
						case HP_DeviceSettings::kControlValueType4CC:
							inObject.GetPropertyData(theAddress, 0, NULL, theSize, &theIntValue);
							outSettings.AddUInt32(theControlKey.GetCFString(), theIntValue);
							#if Log_SaveRestoreFromPrefs
								DebugMessageN2("Saving %s: %lu", theControlKeyString, theIntValue);
							#endif
							break;
					};
				}
			}
			catch(...)
			{
			}
		}
	}
}

static void	HP_DeviceSettings_SaveDeviceSettings(const HP_Device& inDevice, CACFDictionary& outSettings, bool inIsInput, const HP_DeviceSettings::ControlInfo* inControlsToSave, UInt32 inNumberControlsToSave)
{
	HP_DeviceSettings_SaveControlsFromObject(inDevice, false, inDevice.GetTotalNumberChannels(inIsInput), outSettings, inIsInput, inControlsToSave, inNumberControlsToSave);
}

static void	HP_DeviceSettings_SaveStreamSettings(const HP_Device& inDevice, CACFDictionary& outSettings, bool inIsInput, const HP_DeviceSettings::ControlInfo* inControlsToSave, UInt32 inNumberControlsToSave)
{
	//	iterate through the streams in the given direction
	UInt32 theNumberStreams = inDevice.GetNumberStreams(inIsInput);
	for(UInt32 theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
	{
		//	get the stream
		HP_Stream* theStream = inDevice.GetStreamByIndex(inIsInput, theStreamIndex);
		
		//	go through the controls and save the master control
		HP_DeviceSettings_SaveControlsFromObject(*theStream, true, 0, outSettings, inIsInput, inControlsToSave, inNumberControlsToSave);
		
		try
		{
			//	get the stream's physical format
			AudioStreamBasicDescription theFormat;
			theStream->GetCurrentPhysicalFormat(theFormat);
			
			#if Log_SaveRestoreFromPrefs
				DebugMessageN2("Saving %s stream %lu format", inIsInput ? "input" : "output", theStreamIndex);
				CAStreamBasicDescription::PrintToLog(theFormat);
			#endif
			
			//	only save it if it is mixable
			if(CAStreamBasicDescription::IsMixable(theFormat))
			{
				//	make a dictionary out of it
				CACFDictionary theFormatDictionary(true);
				HP_DeviceSettings_ConstructDictionaryFromFormat(theFormat, theFormatDictionary);
				
				//	create the format key
				CACFString theFormatKey(HP_DeviceSettings_ConstructFormatKey(theStreamIndex, inIsInput));
				
				//	add it to the settings
				outSettings.AddDictionary(theFormatKey.GetCFString(), theFormatDictionary.GetCFDictionary());
			}
		}
		catch(...)
		{
		}
	}
}

void	HP_DeviceSettings::SaveToPrefs(const HP_Device& inDevice, const HP_DeviceSettings::ControlInfo* inControlsToSave, UInt32 inNumberControlsToSave)
{
	//	The settings for each device are stored in CFPreferences in the system
	//	domain on a per-host basis. This code saves the value of all the controls
	//	the device implements and the physical format of each stream. The key for
	//	the value of the settings pref ought to include the UID of the device.
	
	#if	Log_SaveRestoreFromPrefs
		CACFString theDeviceName(inDevice.CopyDeviceName());
		char theName[256];
		UInt32 theNameSize = 256;
		theDeviceName.GetCString(theName, theNameSize);
		DebugMessageN1("HP_DeviceSettings::SaveToPrefs: %s", theName);
	#endif
	
	//	get the current prefs Data
	CFMutableDictionaryRef theCurrentPrefsCFDictionary = NULL;

	//	open the prefs file for reading
	FILE* thePrefsFile = fopen(kHALDeviceSettingsFilePath, "r");
	if(thePrefsFile != NULL)
	{
		//	get the length of the file
		fseek(thePrefsFile, 0, SEEK_END);
		UInt32 theFileLength = ftell(thePrefsFile);
		fseek(thePrefsFile, 0, SEEK_SET);
		
		if(theFileLength > 0)
		{
			//	allocate a block of memory to hold the data in the file
			CAAutoFree<Byte> theRawPrefsData(theFileLength);
			
			//	read all the data in
			fread(static_cast<Byte*>(theRawPrefsData), theFileLength, 1, thePrefsFile);
			
			//	close the file
			fclose(thePrefsFile);
			
			//	put it into a CFData object
			CACFData theRawPrefsCFData(static_cast<Byte*>(theRawPrefsData), theFileLength);
			
			//	parse the data as a property list
			theCurrentPrefsCFDictionary = (CFMutableDictionaryRef)CFPropertyListCreateFromXMLData(NULL, theRawPrefsCFData.GetCFData(), kCFPropertyListMutableContainersAndLeaves, NULL);
		}
		else
		{
			//	no data, so close the file
			fclose(thePrefsFile);
			
			//	create a new, mutable dictionary
			theCurrentPrefsCFDictionary = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		}
	}
	else
	{
		//	no file, or a bad file, so just create a new, mutable dictionary
		theCurrentPrefsCFDictionary = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	}
	
	//	just skip things if we still don't have a current prefs dictionary
	if(theCurrentPrefsCFDictionary != NULL)
	{
		//	make a CACFDictionary for convenience and to be sure it gets released
		CACFDictionary theCurrentPrefsDictionary(theCurrentPrefsCFDictionary, true);
		
		//	make the dictionary key for this device
		CACFString theUID(inDevice.CopyDeviceUID());
		CACFString thePrefsKey(CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.DeviceSettings.%@"), theUID.GetCFString()));
		
		//	save the device's settings into a dictionary
		CACFDictionary theSettingsDictionary(SaveToDictionary(inDevice, inControlsToSave, inNumberControlsToSave), true);
		
		//	put the settings into the prefs
		theCurrentPrefsDictionary.AddDictionary(thePrefsKey.GetCFString(), theSettingsDictionary.GetCFDictionary());
		
		//	make a CFData that contains the new prefs
		CACFData theNewRawPrefsCFData(CFPropertyListCreateXMLData(NULL, (CFPropertyListRef)theCurrentPrefsDictionary.GetCFMutableDictionary()), true);
		
		if(theNewRawPrefsCFData.IsValid())
		{
			//	open the prefs file for writing
			thePrefsFile = fopen(kHALDeviceSettingsFilePath, "w+");
			if(thePrefsFile != NULL)
			{
				//	write the data
				fwrite(theNewRawPrefsCFData.GetDataPtr(), theNewRawPrefsCFData.GetSize(), 1, thePrefsFile);
				
				//	close the file
				fclose(thePrefsFile);
			}
		}
	}
	
	#if	Log_SaveRestoreFromPrefs
		DebugMessageN1("HP_DeviceSettings::SaveToPrefs: %s finished", theName);
	#endif
}

CFDictionaryRef	HP_DeviceSettings::SaveToDictionary(const HP_Device& inDevice, const HP_DeviceSettings::ControlInfo* inControlsToSave, UInt32 inNumberControlsToSave)
{
	//	create a dictionary to hold the settings
	CACFDictionary theSettings(false);
	
	//	Save the device settings
	HP_DeviceSettings_SaveDeviceSettings(inDevice, theSettings, true, inControlsToSave, inNumberControlsToSave);
	HP_DeviceSettings_SaveDeviceSettings(inDevice, theSettings, false, inControlsToSave, inNumberControlsToSave);
	
	//	Save the stream settings
	HP_DeviceSettings_SaveStreamSettings(inDevice, theSettings, true, inControlsToSave, inNumberControlsToSave);
	HP_DeviceSettings_SaveStreamSettings(inDevice, theSettings, false, inControlsToSave, inNumberControlsToSave);
	
	//	return the dictionary
	CFDictionaryRef theDictionary = theSettings.GetDict();
	return theDictionary;
}

static void	HP_DeviceSettings_RestoreControlsToObject(HP_Object& inObject, bool inIsStream, UInt32 inNumberChannels, const CACFDictionary& inSettings, bool inIsInput, const HP_DeviceSettings::ControlInfo* inControlsToRestore, UInt32 inNumberControlsToRestore)
{
	CAPropertyAddress theAddress(0, inIsInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput);
	for(UInt32 theControlIndex = 0; theControlIndex < inNumberControlsToRestore; ++theControlIndex)
	{
		theAddress.mSelector = inControlsToRestore[theControlIndex].mSelector;
		
		//	note that 0 is the master channel, making the actual channels start at 1
		for(theAddress.mElement = 0; theAddress.mElement <= inNumberChannels; ++theAddress.mElement)
		{
			try
			{
				//	only do something if the property is present
				if(inObject.HasProperty(theAddress))
				{
					//	create the key
					CACFString theControlKey(HP_DeviceSettings_ConstructControlKey(theAddress.mSelector, theAddress.mElement, inIsInput, inIsStream));
				
					#if Log_SaveRestoreFromPrefs
						char theControlKeyString[1024];
						UInt32 theControlKeyStringSize = 1024;
						theControlKey.GetCString(theControlKeyString, theControlKeyStringSize);
					#endif
				
					//	all control values are the same size
					UInt32 theSize = 4;
				
					//	but not the same type.
					//	get the value and tell the object
					UInt32 theIntValue;
					Float32 theFloatValue;
					switch(inControlsToRestore[theControlIndex].mValueType)
					{
						case HP_DeviceSettings::kControlValueTypeBool:
							if(inSettings.GetUInt32(theControlKey.GetCFString(), theIntValue))
							{
								#if Log_SaveRestoreFromPrefs
									DebugMessageN2("Restoring %s: %lu", theControlKeyString, theIntValue);
								#endif
								inObject.SetPropertyData(theAddress, 0, NULL, theSize, &theIntValue, NULL);
							}
							break;
						
						case HP_DeviceSettings::kControlValueTypeFloat:
							if(inSettings.GetFloat32(theControlKey.GetCFString(), theFloatValue))
							{
								#if Log_SaveRestoreFromPrefs
									DebugMessageN2("Restoring %s: %f", theControlKeyString, theFloatValue);
								#endif
								inObject.SetPropertyData(theAddress, 0, NULL, theSize, &theFloatValue, NULL);
							}
							break;
						
						case HP_DeviceSettings::kControlValueType4CC:
							if(inSettings.GetUInt32(theControlKey.GetCFString(), theIntValue))
							{
								#if Log_SaveRestoreFromPrefs
									DebugMessageN2("Restoring %s: %lu", theControlKeyString, theIntValue);
								#endif
								inObject.SetPropertyData(theAddress, 0, NULL, theSize, &theIntValue, NULL);
							}
							break;
					};
				}
			}
			catch(...)
			{
			}
		}
	}
}

static void	HP_DeviceSettings_RestoreDeviceSettings(HP_Device& inDevice, const CACFDictionary& inSettings, bool inIsInput, const HP_DeviceSettings::ControlInfo* inControlsToRestore, UInt32 inNumberControlsToRestore)
{
	HP_DeviceSettings_RestoreControlsToObject(inDevice, false, inDevice.GetTotalNumberChannels(inIsInput), inSettings, inIsInput, inControlsToRestore, inNumberControlsToRestore);
}

static void	HP_DeviceSettings_RestoreStreamSettings(HP_Device& inDevice, const CACFDictionary& inSettings, bool inIsInput, const HP_DeviceSettings::ControlInfo* inControlsToRestore, UInt32 inNumberControlsToRestore)
{
	//	iterate through the streams in the given direction
	UInt32 theNumberStreams = inDevice.GetNumberStreams(inIsInput);
	for(UInt32 theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
	{
		//	get the stream
		HP_Stream* theStream = inDevice.GetStreamByIndex(inIsInput, theStreamIndex);
		
		//	go through the controls and restore the master control
		HP_DeviceSettings_RestoreControlsToObject(*theStream, true, 0, inSettings, inIsInput, inControlsToRestore, inNumberControlsToRestore);
		
		//	create the format key
		CACFString theFormatKey(HP_DeviceSettings_ConstructFormatKey(theStreamIndex, inIsInput));

		//	get the stream's physical format dictionary
		CFDictionaryRef theRawFormatDictionary;
		if(inSettings.GetDictionary(theFormatKey.GetCFString(), theRawFormatDictionary))
		{
			//	since this CFObject came directly out of a CFDictionary without
			//	an additional CFRetain, we don't have to release it here
			CACFDictionary theFormatDictionary(theRawFormatDictionary, false);
			
			//	make an ABSD out of the format
			AudioStreamBasicDescription theFormat;
			if(HP_DeviceSettings_ConstructFormatFromDictionary(theFormatDictionary, theFormat))
			{
				#if Log_SaveRestoreFromPrefs
					DebugMessageN2("Restoring %s stream %lu format", inIsInput ? "input" : "output", theStreamIndex);
					CAStreamBasicDescription::PrintToLog(theFormat);
				#endif
				
				//	tell the stream about the format
				CAPropertyAddress theAddress(kAudioStreamPropertyPhysicalFormat);
				UInt32 theSize = sizeof(AudioStreamBasicDescription);
				try
				{
					if(theStream->HasProperty(theAddress))
					{
						theStream->SetPropertyData(theAddress, 0, NULL, theSize, &theFormat, NULL);
					}
				}
				catch(...)
				{
				}
			}
		}
	}
}

void	HP_DeviceSettings::RestoreFromPrefs(HP_Device& inDevice, const HP_DeviceSettings::ControlInfo* inControlsToRestore, UInt32 inNumberControlsToRestore)
{
	//	Take and hold the device's state guard while we do this to prevent notifications from
	//	interupting the full completion of this routine.
	CAMutex::Locker theDeviceStateMutex(inDevice.GetStateMutex());
	
	#if	Log_SaveRestoreFromPrefs
		CACFString theDeviceName(inDevice.CopyDeviceName());
		char theName[256];
		UInt32 theNameSize = 256;
		theDeviceName.GetCString(theName, theNameSize);
	#endif

	//	look for this device's settings in the global preferences domain
	CACFString theUID(inDevice.CopyDeviceUID());
	CACFString thePrefsKey(CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.DeviceSettings.%@"), theUID.GetCFString()));
	CFDictionaryRef thePrefValue = CACFPreferences::CopyDictionaryValue(thePrefsKey.GetCFString(), false, true);
	
	//	delete it if we found it, since we aren't using this location any longer
	if(thePrefValue != NULL)
	{
		#if	Log_SaveRestoreFromPrefs
			DebugMessageN1("HP_DeviceSettings::RestoreFromPrefs: %s (old-skool)", theName);
		#endif
		CACFPreferences::DeleteValue(thePrefsKey.GetCFString(), false, true, true);
		CACFPreferences::Synchronize(false, true, true);
	}
	
	//	if we didn't find it, look in the prefs file
	if(thePrefValue == NULL)
	{
		//	get the current prefs Data
		CFMutableDictionaryRef theCurrentPrefsCFDictionary = NULL;

		//	open the prefs file for reading
		FILE* thePrefsFile = fopen(kHALDeviceSettingsFilePath, "r");
		if(thePrefsFile != NULL)
		{
			//	get the length of the file
			fseek(thePrefsFile, 0, SEEK_END);
			UInt32 theFileLength = ftell(thePrefsFile);
			fseek(thePrefsFile, 0, SEEK_SET);
			
			if(theFileLength > 0)
			{
				//	allocate a block of memory to hold the data in the file
				CAAutoFree<Byte> theRawPrefsData(theFileLength);
				
				//	read all the data in
				fread(static_cast<Byte*>(theRawPrefsData), theFileLength, 1, thePrefsFile);
				
				//	close the file
				fclose(thePrefsFile);
				
				//	put it into a CFData object
				CACFData theRawPrefsCFData(static_cast<Byte*>(theRawPrefsData), theFileLength);
				
				//	parse the data as a property list
				theCurrentPrefsCFDictionary = (CFMutableDictionaryRef)CFPropertyListCreateFromXMLData(NULL, theRawPrefsCFData.GetCFData(), kCFPropertyListMutableContainersAndLeaves, NULL);
			}
			else
			{
				//	no data in the file, so just close it
				fclose(thePrefsFile);
			}
		}
		
		if(theCurrentPrefsCFDictionary != NULL)
		{
			//	there are some prefs, so make a CACFDictionary for convenience and to make sure it is released
			CACFDictionary theCurrentPrefsDictionary(theCurrentPrefsCFDictionary, true);
			
			//	look for the settings for this device
			theCurrentPrefsDictionary.GetDictionary(thePrefsKey.GetCFString(), thePrefValue);
			
			//	if we found it, retain it because we will release it later
			if(thePrefValue != NULL)
			{
				CFRetain(thePrefValue);
			}
		}
		
		#if	Log_SaveRestoreFromPrefs
			if(thePrefValue != NULL)
			{
				DebugMessageN1("HP_DeviceSettings::RestoreFromPrefs: %s (nu-skool)", theName);
			}
			else
			{
				DebugMessageN1("HP_DeviceSettings::RestoreFromPrefs: %s (nada)", theName);
			}
		#endif
	}
	
	//	restore the settings
	if(thePrefValue != NULL)
	{
		RestoreFromDictionary(inDevice, thePrefValue, inControlsToRestore, inNumberControlsToRestore);
		CFRelease(thePrefValue);
	}
	
	#if	Log_SaveRestoreFromPrefs
		DebugMessageN1("HP_DeviceSettings::RestoreFromPrefs: %s finished", theName);
	#endif
}

void	HP_DeviceSettings::RestoreFromDictionary(HP_Device& inDevice, const CFDictionaryRef inDictionary, const ControlInfo* inControlsToRestore, UInt32 inNumberControlsToRestore)
{
	if(inDictionary != NULL)
	{
		//	Take and hold the device's state guard while we do this to prevent notifications from
		//	interupting the full completion of this routine.
		CAMutex::Locker theDeviceStateMutex(inDevice.GetStateMutex());
	
		CACFDictionary theSettings(inDictionary, false);
		
		//	restore the device settings
		HP_DeviceSettings_RestoreDeviceSettings(inDevice, theSettings, true, inControlsToRestore, inNumberControlsToRestore);
		HP_DeviceSettings_RestoreDeviceSettings(inDevice, theSettings, false, inControlsToRestore, inNumberControlsToRestore);
	
		//	restore the stream settings
		HP_DeviceSettings_RestoreStreamSettings(inDevice, theSettings, true, inControlsToRestore, inNumberControlsToRestore);
		HP_DeviceSettings_RestoreStreamSettings(inDevice, theSettings, false, inControlsToRestore, inNumberControlsToRestore);
	}
}
