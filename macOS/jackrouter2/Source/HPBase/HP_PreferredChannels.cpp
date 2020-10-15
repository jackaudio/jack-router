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
	HP_PreferredChannels.cpp

==================================================================================================*/

//==================================================================================================
//	Includes
//==================================================================================================

//	Self Include
#include "HP_PreferredChannels.h"

//	Local Includes
#include "HP_Device.h"

//	PublicUtility Includes
#include "CAAudioChannelLayout.h"
#include "CACFArray.h"
#include "CACFDictionary.h"
#include "CACFPreferences.h"
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"

//==================================================================================================
//	HP_PreferredChannels
//==================================================================================================

static void	HP_PreferredChannels_ConstructDictionaryFromLayout(const AudioChannelLayout& inLayout, CACFDictionary& outLayoutDictionary)
{
	//	stick in the tag
	outLayoutDictionary.AddUInt32(CFSTR("channel layout tag"), inLayout.mChannelLayoutTag);
	
	//	stick in the bitmap
	outLayoutDictionary.AddUInt32(CFSTR("channel bitmap"), inLayout.mChannelBitmap);
	
	//	stick in the number channels
	outLayoutDictionary.AddUInt32(CFSTR("number channels"), inLayout.mNumberChannelDescriptions);
	
	//	add the channel descriptions, if necessary
	if(inLayout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
	{
		//	create an array to hold the channel descriptions
		CACFArray theChannelDescriptions(true);
		if(theChannelDescriptions.IsValid())
		{
			//	iterate through the descriptions and stick them in the array
			for(UInt32 theChannelIndex = 0; theChannelIndex < inLayout.mNumberChannelDescriptions; ++theChannelIndex)
			{
				//	create a dictionary to hold the description
				CACFDictionary theDescription(true);
				if(theDescription.IsValid())
				{
					//	stick in the easy values
					theDescription.AddUInt32(CFSTR("channel label"), inLayout.mChannelDescriptions[theChannelIndex].mChannelLabel);
					theDescription.AddUInt32(CFSTR("channel flags"), inLayout.mChannelDescriptions[theChannelIndex].mChannelFlags);
				
					//	create an array to hold the coordinates
					CACFArray theCoordinates(true);
					if(theCoordinates.IsValid())
					{
						//	add the coordinates to the array
						for(UInt32 theCoordinateIndex = 0; theCoordinateIndex < 3; ++theCoordinateIndex)
						{
							theCoordinates.AppendFloat32(inLayout.mChannelDescriptions[theChannelIndex].mCoordinates[theCoordinateIndex]);
						}
						
						//	add the array of coordinates to the description
						theDescription.AddArray(CFSTR("coordinates"), theCoordinates.GetCFArray());
					}
					
					//	add the description to the array of descriptions
					theChannelDescriptions.AppendDictionary(theDescription.GetCFDictionary());
				}
			}
			
			//	add the array of descriptions to the layout dictionary
			outLayoutDictionary.AddArray(CFSTR("channel descriptions"), theChannelDescriptions.GetCFArray());
		}
	}
}

static void	HP_PreferredChannels_ConstructLayoutFromDictionary(const CACFDictionary& inLayoutDictionary, AudioChannelLayout& outLayout)
{
	//	get the tag, bitmap
	inLayoutDictionary.GetUInt32(CFSTR("channel layout tag"), outLayout.mChannelLayoutTag);
	inLayoutDictionary.GetUInt32(CFSTR("channel bitmap"), outLayout.mChannelBitmap);
	
	//	get the number of channels specified in the dictionary
	UInt32 theNumberChannelsInDictionary = 0;
	inLayoutDictionary.GetUInt32(CFSTR("number channels"), theNumberChannelsInDictionary);
	
	//	get the descriptions if they are present and required
	CFArrayRef __theDescriptions;
	if((outLayout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions) && inLayoutDictionary.GetArray(CFSTR("channel descriptions"), __theDescriptions))
	{
		//	don't release this array because it came straight out of the dictionary
		CACFArray theDescriptions(__theDescriptions, false);
		
		//	get the number of items in the array
		UInt32 theNumberItems = theDescriptions.GetNumberItems();
		
		//	iterate through the array and fill out the struct
		for(UInt32 theItemIndex = 0; (theItemIndex < theNumberItems) && (theItemIndex < outLayout.mNumberChannelDescriptions); ++theItemIndex)
		{
			//	get the description
			CFDictionaryRef __theDescription;
			if(theDescriptions.GetDictionary(theItemIndex, __theDescription))
			{
				//	don't release this dictionary because it came straight out of the array
				CACFDictionary theDescription(__theDescription, false);
				
				//	get the channel label and flags
				theDescription.GetUInt32(CFSTR("channel label"), outLayout.mChannelDescriptions[theItemIndex].mChannelLabel);
				theDescription.GetUInt32(CFSTR("channel flags"), outLayout.mChannelDescriptions[theItemIndex].mChannelFlags);
				
				//	get the coordinates
				CFArrayRef __theCoordinates;
				if(theDescription.GetArray(CFSTR("coordinates"), __theCoordinates))
				{
					//	don't release this array because it came straight out of the dictionary
					CACFArray theCoordinates(__theCoordinates, false);
					
					//	iterate through the array and get the coordinates
					UInt32 theNumberCoordinates = theCoordinates.GetNumberItems();
					for(UInt32 theCoordinateIndex = 0; (theCoordinateIndex < 3) && (theCoordinateIndex < theNumberCoordinates); ++theCoordinateIndex)
					{
						theCoordinates.GetFloat32(theCoordinateIndex, outLayout.mChannelDescriptions[theItemIndex].mCoordinates[theCoordinateIndex]);
					}
				}
			}
		}
	}
}

HP_PreferredChannels::HP_PreferredChannels(HP_Device* inDevice)
:
	HP_Property(),
	mDevice(inDevice),
	mToken(0),
	mInputStereoPrefsKey(NULL),
	mOutputStereoPrefsKey(NULL),
	mInputChannelLayoutPrefsKey(NULL),
	mOutputChannelLayoutPrefsKey(NULL)
{
	//	get our token
	if(sTokenMap == NULL)
	{
		sTokenMap = new CATokenMap<HP_PreferredChannels>();
	}
	mToken = sTokenMap->MapObject(this);
}

HP_PreferredChannels::~HP_PreferredChannels()
{
	sTokenMap->RemoveMapping(mToken, this);
}

void	HP_PreferredChannels::Initialize()
{
	//	construct the name of the preferences
	CACFString theUID(mDevice->CopyDeviceUID());
	
	mInputStereoPrefsKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.PreferredStereoChannels.%s.%@"), "Input", theUID.GetCFString());
	ThrowIfNULL(mInputStereoPrefsKey, CAException(kAudioHardwareIllegalOperationError), "HP_PreferredChannels::Initialize: couldn't create the input stereo prefs key");
	
	mOutputStereoPrefsKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.PreferredStereoChannels.%s.%@"), "Output", theUID.GetCFString());
	ThrowIfNULL(mOutputStereoPrefsKey, CAException(kAudioHardwareIllegalOperationError), "HP_PreferredChannels::Initialize: couldn't create the output stereo prefs key");
	
	mInputChannelLayoutPrefsKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.PreferredChannelLayout.%s.%@"), "Input", theUID.GetCFString());
	ThrowIfNULL(mInputChannelLayoutPrefsKey, CAException(kAudioHardwareIllegalOperationError), "HP_PreferredChannels::Initialize: couldn't create the input channel layout prefs key");
	
	mOutputChannelLayoutPrefsKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.PreferredChannelLayout.%s.%@"), "Output", theUID.GetCFString());
	ThrowIfNULL(mOutputChannelLayoutPrefsKey, CAException(kAudioHardwareIllegalOperationError), "HP_PreferredChannels::Initialize: couldn't create the output channel layout prefs key");
	
	//	sign up for notifications
	CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(), (const void*)mToken, (CFNotificationCallback)ChangeNotification, mInputStereoPrefsKey, NULL, CFNotificationSuspensionBehaviorCoalesce);
	CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(), (const void*)mToken, (CFNotificationCallback)ChangeNotification, mOutputStereoPrefsKey, NULL, CFNotificationSuspensionBehaviorCoalesce);
	CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(), (const void*)mToken, (CFNotificationCallback)ChangeNotification, mInputChannelLayoutPrefsKey, NULL, CFNotificationSuspensionBehaviorCoalesce);
	CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(), (const void*)mToken, (CFNotificationCallback)ChangeNotification, mOutputChannelLayoutPrefsKey, NULL, CFNotificationSuspensionBehaviorCoalesce);
}

void	HP_PreferredChannels::Teardown()
{
	CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), (const void*)mToken, mOutputChannelLayoutPrefsKey, NULL);
	CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), (const void*)mToken, mInputChannelLayoutPrefsKey, NULL);
	CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), (const void*)mToken, mOutputStereoPrefsKey, NULL);
	CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), (const void*)mToken, mInputStereoPrefsKey, NULL);
	CFRelease(mOutputChannelLayoutPrefsKey);
	CFRelease(mInputChannelLayoutPrefsKey);
	CFRelease(mOutputStereoPrefsKey);
	CFRelease(mInputStereoPrefsKey);
}

bool	HP_PreferredChannels::IsActive(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			theAnswer = ((inAddress.mScope == kAudioDevicePropertyScopeInput) && (mDevice->GetTotalNumberChannels(true) > 1)) || ((inAddress.mScope == kAudioDevicePropertyScopeOutput) && (mDevice->GetTotalNumberChannels(false) > 1));
			break;
		
		case kAudioDevicePropertyPreferredChannelLayout:
			theAnswer = ((inAddress.mScope == kAudioDevicePropertyScopeInput) && mDevice->HasInputStreams()) || ((inAddress.mScope == kAudioDevicePropertyScopeOutput) && mDevice->HasOutputStreams());
			break;
	};
	
	return theAnswer;
}

bool	HP_PreferredChannels::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			theAnswer = true;
			break;
		
		case kAudioDevicePropertyPreferredChannelLayout:
			theAnswer = true;
			break;
	};
	
	return theAnswer;
}

UInt32	HP_PreferredChannels::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 /*inQualifierDataSize*/, const void* /*inQualifierData*/) const
{
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			theAnswer = 2 * sizeof(UInt32);
			break;
			
		case kAudioDevicePropertyPreferredChannelLayout:
			theAnswer = CAAudioChannelLayout::CalculateByteSize(mDevice->GetTotalNumberChannels(inAddress.mScope == kAudioDevicePropertyScopeInput));
			break;
	};
	
	return theAnswer;
}

void	HP_PreferredChannels::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	bool isInput = inAddress.mScope == kAudioDevicePropertyScopeInput;
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			{
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_PreferredChannels::GetPropertyData: wrong data size for kAudioDevicePropertyPreferredChannelsForStereo");
				UInt32* theStereoChannels = static_cast<UInt32*>(outData);
				
				//	initialize the output
				theStereoChannels[0] = 1;
				theStereoChannels[1] = 2;
				
				//	get the preference
				CACFArray thePrefStereoChannels(CACFPreferences::CopyArrayValue((isInput ? mInputStereoPrefsKey : mOutputStereoPrefsKey), false, true), true);
				if(thePrefStereoChannels.IsValid())
				{
					//	get the values from the array
					thePrefStereoChannels.GetUInt32(0, theStereoChannels[0]);
					thePrefStereoChannels.GetUInt32(1, theStereoChannels[1]);
				}
			}
			break;
			
		case kAudioDevicePropertyPreferredChannelLayout:
			{
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_PreferredChannels::GetPropertyData: wrong data size for kAudioDevicePropertyPreferredChannelLayout");
				AudioChannelLayout* theChannelLayout = static_cast<AudioChannelLayout*>(outData);
				
				//	initialize the output
				CAAudioChannelLayout::SetAllToUnknown(*theChannelLayout, mDevice->GetTotalNumberChannels(isInput));
				
				//	get the pref
				CFDictionaryRef __thePrefChannelLayout = CACFPreferences::CopyDictionaryValue((isInput ? mInputChannelLayoutPrefsKey : mOutputChannelLayoutPrefsKey), false, true);
				CACFDictionary thePrefChannelLayout(__thePrefChannelLayout, true);
				if(thePrefChannelLayout.IsValid())
				{
					HP_PreferredChannels_ConstructLayoutFromDictionary(thePrefChannelLayout, *theChannelLayout);
				}
			}
			break;
	};
}

void	HP_PreferredChannels::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* /*inWhen*/)
{
	CFStringRef thePrefsKey = NULL;
	bool isInput = inAddress.mScope == kAudioDevicePropertyScopeInput;
	UInt32 theTotalNumberChannels = mDevice->GetTotalNumberChannels(isInput);
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			{
				ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_PreferredChannels::SetPropertyData: wrong data size for kAudioDevicePropertyPreferredChannelLayout");
				const UInt32* theStereoChannels = static_cast<const UInt32*>(inData);
				
				//	create an array to hold the prefs value
				CACFArray thePrefStereoChannels(true);
				
				//	put in the left channel
				thePrefStereoChannels.AppendUInt32(std::min(std::max(theStereoChannels[0], (UInt32)1), theTotalNumberChannels));
				
				//	put in the right channel
				thePrefStereoChannels.AppendUInt32(std::min(std::max(theStereoChannels[1], (UInt32)1), theTotalNumberChannels));
				
				//	set the value in the prefs
				thePrefsKey = (isInput ? mInputStereoPrefsKey : mOutputStereoPrefsKey);
				CACFPreferences::SetValue(thePrefsKey, thePrefStereoChannels.GetCFArray(), false, true, true);
				
				//	send the notification
				SendChangeNotification(thePrefsKey);
			}
			break;
			
		case kAudioDevicePropertyPreferredChannelLayout:
			{
				ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_PreferredChannels::SetPropertyData: wrong data size for kAudioDevicePropertyPreferredChannelLayout");
				const AudioChannelLayout* theChannelLayout = static_cast<const AudioChannelLayout*>(inData);
				
				//	create a dictionary to hold the prefs value
				CACFDictionary thePrefChannelLayout(true);
				
				//	fill out the dictionary
				HP_PreferredChannels_ConstructDictionaryFromLayout(*theChannelLayout, thePrefChannelLayout);
				
				//	set the value in the prefs
				thePrefsKey = (isInput ? mInputChannelLayoutPrefsKey : mOutputChannelLayoutPrefsKey);
				CACFPreferences::SetValue(thePrefsKey, thePrefChannelLayout.GetDict(), false, true, true);
				
				//	send the notification
				SendChangeNotification(thePrefsKey);
			}
			break;
	};
}

UInt32	HP_PreferredChannels::GetNumberAddressesImplemented() const
{
	return 2;
}

void	HP_PreferredChannels::GetImplementedAddressByIndex(UInt32 inIndex, AudioObjectPropertyAddress& outAddress) const
{
	switch(inIndex)
	{
		case 0:
			outAddress.mSelector = kAudioDevicePropertyPreferredChannelsForStereo;
			break;
			
		case 1:
			outAddress.mSelector = kAudioDevicePropertyPreferredChannelLayout;
			break;
	};
	outAddress.mScope = kAudioObjectPropertyScopeWildcard;
	outAddress.mElement = kAudioObjectPropertyElementWildcard;
}

void	HP_PreferredChannels::SendChangeNotification(CFStringRef inNotificationName) const
{
	CACFPreferences::SendNotification(inNotificationName);
}

void	HP_PreferredChannels::ChangeNotification(CFNotificationCenterRef /*inCenter*/, const void* inToken, CFStringRef inNotificationName, const void* /*inObject*/, CFDictionaryRef /*inUserInfo*/)
{
	try
	{
		if(sTokenMap != NULL)
		{
			HP_PreferredChannels* thePreferredChannelProperty = sTokenMap->GetObject(inToken);
			if(thePreferredChannelProperty != NULL)
			{
				AudioObjectPropertyAddress theAddress;
				theAddress.mElement = kAudioObjectPropertyElementMaster;
				
				//	figure out what changed
				if(CFStringCompare(inNotificationName, thePreferredChannelProperty->mInputStereoPrefsKey, 0) == kCFCompareEqualTo)
				{
					theAddress.mSelector = kAudioDevicePropertyPreferredChannelsForStereo;
					theAddress.mScope = kAudioDevicePropertyScopeInput;
				}
				else if(CFStringCompare(inNotificationName, thePreferredChannelProperty->mOutputStereoPrefsKey, 0) == kCFCompareEqualTo)
				{
					theAddress.mSelector = kAudioDevicePropertyPreferredChannelsForStereo;
					theAddress.mScope = kAudioDevicePropertyScopeOutput;
				}
				else if(CFStringCompare(inNotificationName, thePreferredChannelProperty->mInputChannelLayoutPrefsKey, 0) == kCFCompareEqualTo)
				{
					theAddress.mSelector = kAudioDevicePropertyPreferredChannelLayout;
					theAddress.mScope = kAudioDevicePropertyScopeInput;
				}
				else if(CFStringCompare(inNotificationName, thePreferredChannelProperty->mOutputChannelLayoutPrefsKey, 0) == kCFCompareEqualTo)
				{
					theAddress.mSelector = kAudioDevicePropertyPreferredChannelLayout;
					theAddress.mScope = kAudioDevicePropertyScopeOutput;
				}
				
				//	mark the prefs as dirty
				CACFPreferences::MarkPrefsOutOfDate(false, true);
				
				//	send the notification
				if(theAddress.mSelector != 0)
				{
					thePreferredChannelProperty->mDevice->PropertiesChanged(1, &theAddress);
				}
			}
		}
	}
	catch(...)
	{
	}
}

CATokenMap<HP_PreferredChannels>*	HP_PreferredChannels::sTokenMap = NULL;
