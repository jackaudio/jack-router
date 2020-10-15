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

// JackAU.r

#include <AudioUnit/AudioUnit.r>

// Note that resource IDs must be spaced 2 apart for the 'STR ' name and description
#define kAudioUnitResID_ElCAJAS			1000
#define kAudioUnitResID_ElCAJASGUI		2000


#define RES_ID			kAudioUnitResID_ElCAJAS
#define COMP_TYPE		'aufx'
#define COMP_SUBTYPE		'JASb'
#define COMP_MANUF		'ElCA' 	
#define VERSION			0x00010000
#define NAME			"ElementiCaotici: JACK-insert"
#define DESCRIPTION		"ElementiCaotici JackAudioServer Bus"
#define ENTRY_POINT		"ElCAJASEntry"

#include "AUResources.r"


//GUI

#define RES_ID			kAudioUnitResID_ElCAJASGUI
#define COMP_TYPE		kAudioUnitCarbonViewComponentType
#define COMP_SUBTYPE		'JASb'
#define COMP_MANUF		'ElCA' 	
#define VERSION			0x00010000
#define NAME			"ElementiCaotici: JACK-insert"
#define DESCRIPTION		"ElementiCaotici JackAudioServer Bus"
#define ENTRY_POINT		"ElCAJASViewEntry"

#include "AUResources.r"