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
/*==================================================================================================
	SHP_Control.cpp

==================================================================================================*/

//==================================================================================================
//	Includes
//==================================================================================================

//	Self Include
#include "JackRouterControl.h"

//  Local Includes
#include "JackRouterDevice.h"
#include "JackRouterPlugIn.h"

//  PublicUtility Includes
#include "CACFArray.h"
#include "CACFDictionary.h"
#include "CACFNumber.h"
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"

//==================================================================================================
//	JackRouterLevelControl
//==================================================================================================

JackRouterLevelControl::JackRouterLevelControl(AudioObjectID inObjectID, AudioClassID inClassID, AudioObjectPropertyScope inDevicePropertyScope, AudioObjectPropertyElement inDevicePropertyElement, JackRouterPlugIn* inPlugIn, JackRouterDevice* inOwningDevice)
:
	HP_LevelControl(inObjectID, inClassID, inPlugIn, inOwningDevice),
	mDevicePropertyScope(inDevicePropertyScope),
	mDevicePropertyElement(inDevicePropertyElement),
	mVolumeCurve(),
	mCurrentRawValue(0)
{
}

JackRouterLevelControl::~JackRouterLevelControl()
{
}

void	JackRouterLevelControl::Initialize()
{
	//	cache the info about the control
	SInt32 theMinRaw = 0;
	SInt32 theMaxRaw = 1024;
	Float32 theMinDB = -90;
	Float32 theMaxDB = 0;
	
	//	set up the volume curve
	mVolumeCurve.ResetRange();
	mVolumeCurve.AddRange(theMinRaw, theMaxRaw, theMinDB, theMaxDB);
	
	//	cache the raw value
	CacheRawValue();
}

void	JackRouterLevelControl::Teardown()
{}

AudioObjectPropertyScope	JackRouterLevelControl::GetPropertyScope() const
{
	return mDevicePropertyScope;
}

AudioObjectPropertyElement	JackRouterLevelControl::GetPropertyElement() const
{
	return mDevicePropertyElement;
}

Float32	JackRouterLevelControl::GetMinimumDBValue() const
{
	return mVolumeCurve.GetMinimumDB();
}

Float32	JackRouterLevelControl::GetMaximumDBValue() const
{
	return mVolumeCurve.GetMaximumDB();
}

Float32	JackRouterLevelControl::GetDBValue() const
{
	SInt32 theRawValue = GetRawValue();
	Float32 thDBValue = mVolumeCurve.ConvertRawToDB(theRawValue);
	return thDBValue;
}

void	JackRouterLevelControl::SetDBValue(Float32 inDBValue)
{
	SInt32 theNewRawValue = mVolumeCurve.ConvertDBToRaw(inDBValue);
	SetRawValue(theNewRawValue);
}

Float32	JackRouterLevelControl::GetScalarValue() const
{
	SInt32 theRawValue = GetRawValue();
	Float32 theScalarValue = mVolumeCurve.ConvertRawToScalar(theRawValue);
	return theScalarValue;
}

void	JackRouterLevelControl::SetScalarValue(Float32 inScalarValue)
{
	SInt32 theNewRawValue = mVolumeCurve.ConvertScalarToRaw(inScalarValue);
	SetRawValue(theNewRawValue);
}

Float32	JackRouterLevelControl::ConverScalarValueToDBValue(Float32 inScalarValue) const
{
	Float32 theDBValue = mVolumeCurve.ConvertScalarToDB(inScalarValue);
	return theDBValue;
}

Float32	JackRouterLevelControl::ConverDBValueToScalarValue(Float32 inDBValue) const
{
	Float32 theScalarValue = mVolumeCurve.ConvertDBToScalar(inDBValue);
	return theScalarValue;
}

SInt32	JackRouterLevelControl::GetRawValue() const
{
	//	Always get the value from the hardware and cache it in mCurrentRawValue. Note that if
	//	getting the value from the hardware fails for any reason, we just return mCurrentRawValue.
	//	We always just return mCurrentRawValue here because there is no hardware to talk to.
	return mCurrentRawValue;
}

void	JackRouterLevelControl::SetRawValue(SInt32 inRawValue)
{
	//	Set the value in hardware. Note that mCurrentRawValue should be updated only if setting the
	//	hardware value is synchronous. Otherwise, mCurrentRawValue will be updated when the hardware
	//	notifies us that the value of the control changed. Here, we just directly set
	//	mCurrentRawValue because there is no hardware.
	if(inRawValue != mCurrentRawValue)
	{
		mCurrentRawValue = inRawValue;
	
		//	we also have to send the change notification
		ValueChanged();
	}
}

void	JackRouterLevelControl::CacheRawValue()
{
	//	Set mCurrentRawValue to the value of the hardware. We do nothing here because there is no
	//	hardware.
}

//==================================================================================================
//	JackRouterBooleanControl
//==================================================================================================

JackRouterBooleanControl::JackRouterBooleanControl(AudioObjectID inObjectID, AudioClassID inClassID, AudioObjectPropertyScope inDevicePropertyScope, AudioObjectPropertyElement inDevicePropertyElement, JackRouterPlugIn* inPlugIn, JackRouterDevice* inOwningDevice)
:
	HP_BooleanControl(inObjectID, inClassID, inPlugIn, inOwningDevice),
	mDevicePropertyScope(inDevicePropertyScope),
	mDevicePropertyElement(inDevicePropertyElement),
	mCurrentValue(false)
{}

JackRouterBooleanControl::~JackRouterBooleanControl()
{
}

void	JackRouterBooleanControl::Initialize()
{
	//	cache the value
	CacheValue();
}

void	JackRouterBooleanControl::Teardown()
{}

AudioObjectPropertyScope	JackRouterBooleanControl::GetPropertyScope() const
{
	return mDevicePropertyScope;
}

AudioObjectPropertyElement	JackRouterBooleanControl::GetPropertyElement() const
{
	return mDevicePropertyElement;
}

bool	JackRouterBooleanControl::GetValue() const
{
	//	Always get the value from the hardware and cache it in mCurrentValue. Note that if
	//	getting the value from the hardware fails for any reason, we just return mCurrentValue.
	//	We always just return mCurrentValue here because there is no hardware to talk to.
	return mCurrentValue;
}

void	JackRouterBooleanControl::SetValue(bool inValue)
{
	//	Set the value in hardware. Note that mCurrentValue should be updated only if setting the
	//	hardware value is synchronous. Otherwise, mCurrentValue will be updated when the hardware
	//	notifies us that the value of the control changed. Here, we just directly set
	//	mCurrentValue because there is no hardware.
	if(inValue != mCurrentValue)
	{
		mCurrentValue = inValue;
	
		//	we also have to send the change notification
		ValueChanged();
	}
}

void	JackRouterBooleanControl::CacheValue()
{
	//	Set mCurrentValue to the value of the hardware. We do nothing here because there is no hardware.
}

//==================================================================================================
//	JackRouterSelectorControl
//==================================================================================================

JackRouterSelectorControl::JackRouterSelectorControl(AudioObjectID inObjectID, AudioClassID inClassID, AudioObjectPropertyScope inDevicePropertyScope, AudioObjectPropertyElement inDevicePropertyElement, JackRouterPlugIn* inPlugIn, JackRouterDevice* inOwningDevice)
:
	HP_SelectorControl(inObjectID, inClassID, inPlugIn, inOwningDevice),
	mDevicePropertyScope(inDevicePropertyScope),
	mDevicePropertyElement(inDevicePropertyElement),
	mSelectorMap(),
	mCurrentItemID(0)
{
}

JackRouterSelectorControl::~JackRouterSelectorControl()
{
}

void	JackRouterSelectorControl::Initialize()
{
	//	clear the current items
	mSelectorMap.clear();
	
	//	Insert items into mSelectorMap for all the items in this control. Here, we just stick in a
	//	few fake items.
	for(UInt32 theItemIndex = 0; theItemIndex < 4; ++theItemIndex)
	{
		//	make a name for the item
		CACFString theName(CFStringCreateWithFormat(NULL, NULL, CFSTR("Item %u"), theItemIndex));
		
		//	insert it into the map, using the item index as the item ID
		mSelectorMap.insert(SelectorMap::value_type(theItemIndex, SelectorItem(theName.CopyCFString(), 0)));
	}
	
	//	cache the current item ID
	CacheCurrentItemID();
}

void	JackRouterSelectorControl::Teardown()
{
	mSelectorMap.clear();
}

AudioObjectPropertyScope	JackRouterSelectorControl::GetPropertyScope() const
{
	return mDevicePropertyScope;
}

AudioObjectPropertyElement	JackRouterSelectorControl::GetPropertyElement() const
{
	return mDevicePropertyElement;
}

UInt32	JackRouterSelectorControl::GetNumberItems() const
{
	return mSelectorMap.size();
}

UInt32	JackRouterSelectorControl::GetCurrentItemID() const
{
	//	Always get the value from the hardware and cache it in mCurrentItemID. Note that if
	//	getting the value from the hardware fails for any reason, we just return mCurrentItemID.
	//	We always just return mCurrentItemID here because there is no hardware to talk to.
	return mCurrentItemID;
}

UInt32	JackRouterSelectorControl::GetCurrentItemIndex() const
{
	UInt32 theItemID = GetCurrentItemID();
	return GetItemIndexForID(theItemID);
}

void	JackRouterSelectorControl::SetCurrentItemByID(UInt32 inItemID)
{
	//	Set the value in hardware. Note that mCurrentItemID should be updated only if setting the
	//	hardware value is synchronous. Otherwise, mCurrentItemID will be updated when the hardware
	//	notifies us that the value of the control changed. Here, we just directly set
	//	mCurrentItemID because there is no hardware.
	if(inItemID != mCurrentItemID)
	{
		mCurrentItemID = inItemID;
	
		//	we also have to send the change notification
		ValueChanged();
	}
}

void	JackRouterSelectorControl::SetCurrentItemByIndex(UInt32 inItemIndex)
{
	UInt32 theItemID = GetItemIDForIndex(inItemIndex);
	SetCurrentItemByID(theItemID);
}

UInt32	JackRouterSelectorControl::GetItemIDForIndex(UInt32 inItemIndex) const
{
	ThrowIf(inItemIndex >= mSelectorMap.size(), CAException(kAudioHardwareIllegalOperationError), "JackRouterSelectorControl::GetItemIDForIndex: index out of range");
	SelectorMap::const_iterator theIterator = mSelectorMap.begin();
	std::advance(theIterator, inItemIndex);
	return theIterator->first;
}

UInt32	JackRouterSelectorControl::GetItemIndexForID(UInt32 inItemID) const
{
	UInt32 theIndex = 0;
	bool wasFound = false;
	SelectorMap::const_iterator theIterator = mSelectorMap.begin();
	while(!wasFound && (theIterator != mSelectorMap.end()))
	{
		if(theIterator->first == inItemID)
		{
			wasFound = true;
		}
		else
		{
			++theIndex;
			std::advance(theIterator, 1);
		}
	}
	ThrowIf(!wasFound, CAException(kAudioHardwareIllegalOperationError), "JackRouterSelectorControl::GetItemIndexForID: ID not in selector map");
	return theIndex;
}

CFStringRef	JackRouterSelectorControl::CopyItemNameByID(UInt32 inItemID) const
{
	SelectorMap::const_iterator theIterator = mSelectorMap.find(inItemID);
	ThrowIf(theIterator == mSelectorMap.end(), CAException(kAudioHardwareIllegalOperationError), "JackRouterSelectorControl::CopyItemNameByID: ID not in selector map");
	
	return (CFStringRef)CFRetain(theIterator->second.mItemName);
}

CFStringRef	JackRouterSelectorControl::CopyItemNameByIndex(UInt32 inItemIndex) const
{
	CFStringRef theAnswer = NULL;
	
	if(inItemIndex < mSelectorMap.size())
	{
		SelectorMap::const_iterator theIterator = mSelectorMap.begin();
		std::advance(theIterator, inItemIndex);
		ThrowIf(theIterator == mSelectorMap.end(), CAException(kAudioHardwareIllegalOperationError), "JackRouterSelectorControl::CopyItemNameByIndex: index out of range");
		
		theAnswer = (CFStringRef)CFRetain(theIterator->second.mItemName);
	}
		
	return theAnswer;
}

CFStringRef	JackRouterSelectorControl::CopyItemNameByIDWithoutLocalizing(UInt32 inItemID) const
{
	return CopyItemNameByID(inItemID);
}

CFStringRef	JackRouterSelectorControl::CopyItemNameByIndexWithoutLocalizing(UInt32 inItemIndex) const
{
	return CopyItemNameByIndex(inItemIndex);
}

UInt32	JackRouterSelectorControl::GetItemKindByID(UInt32 inItemID) const
{
	SelectorMap::const_iterator theIterator = mSelectorMap.find(inItemID);
	ThrowIf(theIterator == mSelectorMap.end(), CAException(kAudioHardwareIllegalOperationError), "JackRouterSelectorControl::GetItemKindByID: ID not in selector map");
	
	return theIterator->second.mItemKind;
}

UInt32	JackRouterSelectorControl::GetItemKindByIndex(UInt32 inItemIndex) const
{
	UInt32 theAnswer = 0;
	
	if(inItemIndex < mSelectorMap.size())
	{
		SelectorMap::const_iterator theIterator = mSelectorMap.begin();
		std::advance(theIterator, inItemIndex);
		ThrowIf(theIterator == mSelectorMap.end(), CAException(kAudioHardwareIllegalOperationError), "JackRouterSelectorControl::GetItemKindByIndex: index out of range");
		theAnswer = theIterator->second.mItemKind;
	}
	
	return theAnswer;
}

void	JackRouterSelectorControl::CacheCurrentItemID()
{
	//	Set mCurrentItemID to the value of the hardware. We do nothing here because there is no hardware.
}

//==================================================================================================
//	JackRouterStereoPanControl
//==================================================================================================

JackRouterStereoPanControl::JackRouterStereoPanControl(AudioObjectID inObjectID, AudioClassID inClassID, AudioObjectPropertyScope inDevicePropertyScope, AudioObjectPropertyElement inDevicePropertyElement, UInt32 inLeftChannel, UInt32 inRightChannel, JackRouterPlugIn* inPlugIn, JackRouterDevice* inOwningDevice)
:
	HP_StereoPanControl(inObjectID, inClassID, inPlugIn, inOwningDevice),
	mDevicePropertyScope(inDevicePropertyScope),
	mDevicePropertyElement(inDevicePropertyElement),
	mLeftChannel(inLeftChannel),
	mRightChannel(inRightChannel),
	mFullLeftRawValue(0),
	mCenterRawValue(0),
	mFullRightRawValue(0),
	mCurrentRawValue(0)
{
}

JackRouterStereoPanControl::~JackRouterStereoPanControl()
{
}

void	JackRouterStereoPanControl::Initialize()
{
	//	cache the info about the control
	mFullLeftRawValue = 0;
	mCenterRawValue = 512;
	mFullRightRawValue = 1024;
	
	//	set the value to center, since we don't have any hardware
	mCurrentRawValue = mCenterRawValue;
	
	//	cache the current raw value
	CacheRawValue();
}

void	JackRouterStereoPanControl::Teardown()
{
}

AudioObjectPropertyScope	JackRouterStereoPanControl::GetPropertyScope() const
{
	return mDevicePropertyScope;
}

AudioObjectPropertyElement	JackRouterStereoPanControl::GetPropertyElement() const
{
	return mDevicePropertyElement;
}

Float32	JackRouterStereoPanControl::GetValue() const
{
	Float32	theAnswer = 0.0;
	SInt32	theRawValue = GetRawValue();
	Float32	theSpan;
	
	if(theRawValue == mCenterRawValue)
	{
		theAnswer = 0.5;
	}
	else if(theRawValue > mCenterRawValue)
	{
		theSpan = mFullRightRawValue - mCenterRawValue;
		theAnswer = theRawValue - mCenterRawValue;
		theAnswer *= 0.5;
		theAnswer /= theSpan;
		theAnswer += 0.5;
	}
	else
	{
		theSpan = mCenterRawValue - mFullLeftRawValue;
		theAnswer = theRawValue - mFullLeftRawValue;
		theAnswer *= 0.5;
		theAnswer /= theSpan;
	}
	
	return theAnswer;
}

void	JackRouterStereoPanControl::SetValue(Float32 inValue)
{
	SInt32 theRawValue = 0;
	Float32 theSpan;
	
	if(inValue == 0.5)
	{
		theRawValue = mCenterRawValue;
	}
	else if(inValue > 0.5)
	{
		theSpan = mFullRightRawValue - mCenterRawValue;
		inValue -= 0.5;
		inValue *= theSpan;
		inValue *= 2.0;
		theRawValue = static_cast<SInt32>(inValue);
		theRawValue += mCenterRawValue;
	}
	else
	{
		theSpan = mCenterRawValue - mFullLeftRawValue;
		inValue *= theSpan;
		inValue *= 2.0;
		theRawValue = static_cast<SInt32>(inValue);
	}
	
	SetRawValue(theRawValue);
}

void	JackRouterStereoPanControl::GetChannels(UInt32& outLeftChannel, UInt32& outRightChannel) const
{
	outLeftChannel = mLeftChannel;
	outRightChannel = mRightChannel;
}

SInt32	JackRouterStereoPanControl::GetRawValue() const
{
	//	Always get the value from the hardware and cache it in mCurrentRawValue. Note that if
	//	getting the value from the hardware fails for any reason, we just return mCurrentRawValue.
	//	We always just return mCurrentRawValue here because there is no hardware to talk to.
	return mCurrentRawValue;
}

void	JackRouterStereoPanControl::SetRawValue(SInt32 inValue)
{
	//	Set the value in hardware. Note that mCurrentRawValue should be updated only if setting the
	//	hardware value is synchronous. Otherwise, mCurrentRawValue will be updated when the hardware
	//	notifies us that the value of the control changed. Here, we just directly set
	//	mCurrentRawValue because there is no hardware.
	if(inValue != mCurrentRawValue)
	{
		mCurrentRawValue = inValue;
	
		//	we also have to send the change notification
		ValueChanged();
	}
}

void	JackRouterStereoPanControl::CacheRawValue()
{
	//	Set mCurrentRawValue to the value of the hardware. We do nothing here because there is no
	//	hardware.
}
