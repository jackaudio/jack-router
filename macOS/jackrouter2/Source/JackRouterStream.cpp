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
	JackRouterStream.cpp
=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

//	Self Include
#include "JackRouterStream.h"

//	Internal Includes
#include "JackRouterDevice.h"
#include "JackRouterPlugIn.h"

//	PublicUtility Includes
#include "CACFArray.h"
#include "CACFDictionary.h"
#include "CACFNumber.h"
#include "CADebugMacros.h"
#include "CAException.h"

#define kJackStreamFormat  kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved

//=============================================================================
//	JackRouterStream
//=============================================================================

JackRouterStream::JackRouterStream(AudioStreamID inAudioStreamID, 
									HP_HardwarePlugIn* inPlugIn, 
									HP_Device* inOwningDevice, 
									bool inIsInput, 
									UInt32 inStartingDeviceChannelNumber, 
									Float64 sampleRate)
:
	HP_Stream(inAudioStreamID, inPlugIn, inOwningDevice, inIsInput, inStartingDeviceChannelNumber),
	mSHPPlugIn(inPlugIn),
	mOwningSHPDevice(inOwningDevice),
	mSampleRate(sampleRate)
{}

JackRouterStream::~JackRouterStream()
{}

void	JackRouterStream::Initialize()
{
	//	initialize the super class
	HP_Stream::Initialize();
	
	//	add the available physical formats
	AddAvailablePhysicalFormats();
	
	//	set the initial format, which is 16 bit stereo
	AudioStreamBasicDescription thePhysicalFormat;
	thePhysicalFormat.mSampleRate = mSampleRate;
	thePhysicalFormat.mFormatID = kAudioFormatLinearPCM;
	thePhysicalFormat.mFormatFlags = kJackStreamFormat;
	thePhysicalFormat.mBytesPerPacket = 4;
	thePhysicalFormat.mFramesPerPacket = 1;
	thePhysicalFormat.mBytesPerFrame = 4;
	thePhysicalFormat.mChannelsPerFrame = 1;
	thePhysicalFormat.mBitsPerChannel = 32;
	mFormatList->SetCurrentPhysicalFormat(thePhysicalFormat, false);
}

void	JackRouterStream::Teardown()
{
		HP_Stream::Teardown();
}

void	JackRouterStream::Finalize()
{}

bool	JackRouterStream::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(GetOwningDevice()->GetStateMutex());
	
	//  do the work if we still have to
	switch(inAddress.mSelector)
	{
		default:
			theAnswer = HP_Stream::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool	JackRouterStream::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(GetOwningDevice()->GetStateMutex());
	
	//  do the work if we still have to
	switch(inAddress.mSelector)
	{
		
		default:
			theAnswer = HP_Stream::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	JackRouterStream::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32	theAnswer = 0;
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(GetOwningDevice()->GetStateMutex());
	
	//  do the work if we still have to
	switch(inAddress.mSelector)
	{
		default:
			theAnswer = HP_Stream::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	JackRouterStream::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(GetOwningDevice()->GetStateMutex());
	
	//  do the work if we still have to
	switch(inAddress.mSelector)
	{
		default:
			HP_Stream::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	JackRouterStream::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(GetOwningDevice()->GetStateMutex());
	
	switch(inAddress.mSelector)
	{
					
		default:
			HP_Stream::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

void	JackRouterStream::AddAvailablePhysicalFormats()
{
	//	basically, for this sample device, we're only going add two formats
	AudioStreamRangedDescription thePhysicalFormat;
	
	//	the first is 32 bit stereo
	thePhysicalFormat.mFormat.mSampleRate = mSampleRate;
	thePhysicalFormat.mSampleRateRange.mMinimum = mSampleRate;
	thePhysicalFormat.mSampleRateRange.mMaximum = mSampleRate;
	thePhysicalFormat.mFormat.mFormatID = kAudioFormatLinearPCM;
	thePhysicalFormat.mFormat.mFormatFlags = kJackStreamFormat;
	thePhysicalFormat.mFormat.mBytesPerPacket = 4;
	thePhysicalFormat.mFormat.mFramesPerPacket = 1;
	thePhysicalFormat.mFormat.mBytesPerFrame = 4;
	thePhysicalFormat.mFormat.mChannelsPerFrame = 1;
	thePhysicalFormat.mFormat.mBitsPerChannel = 32;
	mFormatList->AddPhysicalFormat(thePhysicalFormat);
}

