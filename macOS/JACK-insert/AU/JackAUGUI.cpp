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

// JackAUGUI.h

#include <JackAU.h>
#include "AUCarbonViewBase.h"
#include "AUCarbonViewControl.h"
#include "AUControlGroup.h"


class ElCAJASView : public AUCarbonViewBase {
public:
    ElCAJASView(AudioUnitCarbonView auv) : AUCarbonViewBase(auv) {}
    virtual OSStatus			CreateUI(Float32 xoffset, Float32 yoffset);
};

COMPONENT_ENTRY(ElCAJASView);
COMPONENT_REGISTER(ElCAJASView, kAudioUnitCarbonViewComponentType , 'JASb', 'ElCA');

OSStatus ElCAJASView::CreateUI(Float32 xoffset, Float32 yoffset) {

    int xoff = (int)xoffset;
    int yoff = (int)yoffset;
    
        	
#define kLabelWidth 80
#define kLabelHeight 18
#define kEditTextWidth 40
#define kMinMaxWidth 45
	ControlRef newControl;
	ControlFontStyleRec fontStyle;
	fontStyle.flags = kControlUseFontMask | kControlUseJustMask;
	fontStyle.font = kControlFontSmallSystemFont;
	fontStyle.just = teFlushRight;
	
	Rect r;
	Point labelSize, textSize;
	labelSize.v = textSize.v = kLabelHeight;
	labelSize.h = kMinMaxWidth;
	textSize.h = kEditTextWidth;
	
	
        
	
	{
		AUVParameter auvp(mEditAudioUnit, kDryLevel, kAudioUnitScope_Global, 0);
		
		r.top = 10 + yoff;
        r.bottom = r.top + kLabelHeight;
		r.left = 10;
        r.right = r.left + kLabelWidth;
		verify_noerr(CreateStaticTextControl(mCarbonWindow, &r, auvp.GetName(), &fontStyle, &newControl));
                
	}

         {
		
		r.top = 50 + yoff;
        r.bottom = r.top + 80;
		r.left = xoff;
        r.right = r.left + 350;
		verify_noerr(CreateStaticTextControl(mCarbonWindow, &r, CFSTR("Johnny Petrantoni JACK-insert, (c) 2003 - 2004.    ") , &fontStyle, &newControl));
		verify_noerr(EmbedControl(newControl));
		r.top = 75 + yoff;
        r.bottom = r.top + 80;
		r.left = xoff;
        r.right = r.left + 350;
		verify_noerr(CreateStaticTextControl(mCarbonWindow, &r, CFSTR("johnny@lato-b.com    ") , &fontStyle, &newControl));
		verify_noerr(EmbedControl(newControl));
		r.top = 100 + yoff;
        r.bottom = r.top + 80;
		r.left = xoff;
        r.right = r.left + 350;
		verify_noerr(CreateStaticTextControl(mCarbonWindow, &r, CFSTR("http://www.jackosx.com/    ") , &fontStyle, &newControl));
		verify_noerr(EmbedControl(newControl));

		
	}
        
            
        
	
	
	SizeControl(mCarbonPane, mBottomRight.h + 15, mBottomRight.v - 30);
	return noErr;

}

