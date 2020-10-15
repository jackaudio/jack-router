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
	SHP_Control.h

==================================================================================================*/
#if !defined(__SHP_Control_h__)
#define __SHP_Control_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	Super Class Includes
#include "HP_Control.h"

//	PublicUtility Includes
#include "CAVolumeCurve.h"

//==================================================================================================
//	Types
//==================================================================================================

class	JackRouterDevice;
class	JackRouterPlugIn;

//==================================================================================================
//	JackRouterLevelControl
//==================================================================================================

class JackRouterLevelControl
:
	public HP_LevelControl
{

//	Construction/Destruction
public:
										JackRouterLevelControl(AudioObjectID inObjectID, AudioClassID inClassID, AudioObjectPropertyScope inDevicePropertyScope, AudioObjectPropertyElement inDevicePropertyElement, JackRouterPlugIn* inPlugIn, JackRouterDevice* inOwningDevice);
	virtual								~JackRouterLevelControl();
	
	virtual void						Initialize();
	virtual void						Teardown();

//	Attributes
public:
	virtual AudioObjectPropertyScope	GetPropertyScope() const;
	virtual AudioObjectPropertyElement	GetPropertyElement() const;

	virtual Float32						GetMinimumDBValue() const;
	virtual Float32						GetMaximumDBValue() const;

	virtual Float32						GetDBValue() const;
	virtual void						SetDBValue(Float32 inDBValue);

	virtual Float32						GetScalarValue() const;
	virtual void						SetScalarValue(Float32 inScalarValue);

	virtual Float32						ConverScalarValueToDBValue(Float32 inScalarValue) const;
	virtual Float32						ConverDBValueToScalarValue(Float32 inDBValue) const;
	
//	Implementation
private:
	SInt32								GetRawValue() const;
	void								SetRawValue(SInt32 inRawValue);
	void								CacheRawValue();

	AudioObjectPropertyScope			mDevicePropertyScope;
	AudioObjectPropertyElement			mDevicePropertyElement;
	CAVolumeCurve						mVolumeCurve;
	SInt32								mCurrentRawValue;

};

//==================================================================================================
//	JackRouterBooleanControl
//==================================================================================================

class JackRouterBooleanControl
:
	public HP_BooleanControl
{

//	Construction/Destruction
public:
										JackRouterBooleanControl(AudioObjectID inObjectID, AudioClassID inClassID, AudioObjectPropertyScope inDevicePropertyScope, AudioObjectPropertyElement inDevicePropertyElement, JackRouterPlugIn* inPlugIn, JackRouterDevice* inOwningDevice);
	virtual								~JackRouterBooleanControl();

	virtual void						Initialize();
	virtual void						Teardown();

//	Attributes
public:
	virtual AudioObjectPropertyScope	GetPropertyScope() const;
	virtual AudioObjectPropertyElement	GetPropertyElement() const;

	virtual bool						GetValue() const;
	virtual void						SetValue(bool inValue);

//	Implementation
private:
	virtual void						CacheValue();

	AudioObjectPropertyScope			mDevicePropertyScope;
	AudioObjectPropertyElement			mDevicePropertyElement;
	bool								mCurrentValue;

};

//==================================================================================================
//	JackRouterSelectorControl
//==================================================================================================

class JackRouterSelectorControl
:
	public HP_SelectorControl
{

//	Construction/Destruction
public:
										JackRouterSelectorControl(AudioObjectID inObjectID, AudioClassID inClassID, AudioObjectPropertyScope inDevicePropertyScope, AudioObjectPropertyElement inDevicePropertyElement, JackRouterPlugIn* inPlugIn, JackRouterDevice* inOwningDevice);
	virtual								~JackRouterSelectorControl();
	
	virtual void						Initialize();
	virtual void						Teardown();

//	Attributes
public:
	virtual AudioObjectPropertyScope	GetPropertyScope() const;
	virtual AudioObjectPropertyElement	GetPropertyElement() const;

	virtual UInt32						GetNumberItems() const;

	virtual UInt32						GetCurrentItemID() const;
	virtual UInt32						GetCurrentItemIndex() const;
	
	virtual void						SetCurrentItemByID(UInt32 inItemID);
	virtual void						SetCurrentItemByIndex(UInt32 inItemIndex);
	
	virtual UInt32						GetItemIDForIndex(UInt32 inItemIndex) const;
	virtual UInt32						GetItemIndexForID(UInt32 inItemID) const;
	
	virtual CFStringRef					CopyItemNameByID(UInt32 inItemID) const;
	virtual CFStringRef					CopyItemNameByIndex(UInt32 inItemIndex) const;

	virtual CFStringRef					CopyItemNameByIDWithoutLocalizing(UInt32 inItemID) const;
	virtual CFStringRef					CopyItemNameByIndexWithoutLocalizing(UInt32 inItemIndex) const;

	virtual UInt32						GetItemKindByID(UInt32 inItemID) const;
	virtual UInt32						GetItemKindByIndex(UInt32 inItemIndex) const;

//	Implementation
private:
	void								CacheCurrentItemID();
	
	struct SelectorItem
	{
		CFStringRef	mItemName;
		UInt32		mItemKind;
		
		SelectorItem() : mItemName(NULL), mItemKind(0) {}
		SelectorItem(CFStringRef inItemName, UInt32 inItemKind) : mItemName(inItemName), mItemKind(inItemKind) {}
		SelectorItem(const SelectorItem& inItem) : mItemName(inItem.mItemName), mItemKind(inItem.mItemKind) { if(mItemName != NULL) { CFRetain(mItemName); } }
		SelectorItem&	operator=(const SelectorItem& inItem) { if(mItemName != NULL) { CFRelease(mItemName); } mItemName = inItem.mItemName; if(mItemName != NULL) { CFRetain(mItemName); } mItemKind = inItem.mItemKind; return *this; }
		~SelectorItem() { if(mItemName != NULL) { CFRelease(mItemName); } }
	};
	typedef std::map<UInt32, SelectorItem>	SelectorMap;
	
	AudioObjectPropertyScope			mDevicePropertyScope;
	AudioObjectPropertyElement			mDevicePropertyElement;
	SelectorMap							mSelectorMap;
	UInt32								mCurrentItemID;

};

//==================================================================================================
//	JackRouterStereoPanControl
//==================================================================================================

class JackRouterStereoPanControl
:
	public HP_StereoPanControl
{

//	Construction/Destruction
public:
										JackRouterStereoPanControl(AudioObjectID inObjectID, AudioClassID inClassID, AudioObjectPropertyScope inDevicePropertyScope, AudioObjectPropertyElement inDevicePropertyElement, UInt32 inLeftChannel, UInt32 inRightChannel, JackRouterPlugIn* inPlugIn, JackRouterDevice* inOwningDevice);
	virtual								~JackRouterStereoPanControl();

	virtual void						Initialize();
	virtual void						Teardown();

//	Attributes
public:
	virtual AudioObjectPropertyScope	GetPropertyScope() const;
	virtual AudioObjectPropertyElement	GetPropertyElement() const;

	virtual Float32						GetValue() const;
	virtual void						SetValue(Float32 inValue);
	virtual void						GetChannels(UInt32& outLeftChannel, UInt32& outRightChannel) const;

//	Implementation
private:
	virtual SInt32						GetRawValue() const;
	virtual void						SetRawValue(SInt32 inValue);
	virtual void						CacheRawValue();
	
	AudioObjectPropertyScope			mDevicePropertyScope;
	AudioObjectPropertyElement			mDevicePropertyElement;
	UInt32								mLeftChannel;
	UInt32								mRightChannel;
	SInt32								mFullLeftRawValue;
	SInt32								mCenterRawValue;
	SInt32								mFullRightRawValue;
	SInt32								mCurrentRawValue;

};

#endif
