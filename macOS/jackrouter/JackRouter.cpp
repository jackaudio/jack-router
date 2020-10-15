/*
Copyright © Stefan Werner stefan@keindesign.de, Grame 2003-2006

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
grame@rd.grame.fr
*/

#include <CoreFoundation/CoreFoundation.h>
#include <CoreAudio/AudioHardwarePlugIn.h>
#include <IOKit/audio/IOAudioTypes.h>
#include <pthread.h>

#include "TJackClient.h"

#ifdef __cplusplus
extern "C"
{
#endif

    //#define PRINTDEBUG 1

    // The layout for an instance of MyType.
    typedef struct _MyType {
        AudioHardwarePlugInInterface *_testInterface;
        CFUUIDRef _factoryID;
        UInt32 _refCount;
    }
    MyType;

    // Forward declaration for the IUnknown implementation.
    static void _deallocMyType(MyType * obj);

    // Implementation of the IUnknown QueryInterface function.
    static HRESULT myQueryInterface(void * obj, REFIID iid, LPVOID *ppv) {
        // Create a CoreFoundation UUIDRef for the requested interface.
        CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(NULL, iid);

#if PRINTDEBUG

        printf("JAS: mQueryInterface %ld\n", iid);
#endif

        // Test the requested ID against the valid interfaces.
#ifdef kAudioHardwarePlugInInterface2ID

        if (CFEqual(interfaceID, kAudioHardwarePlugInInterfaceID) || CFEqual(interfaceID, kAudioHardwarePlugInInterface2ID)) {
#if PRINTDEBUG
            printf("JAS: mQueryInterface : kAudioHardwarePlugInInterface2ID\n");
#endif
#else

        if (CFEqual(interfaceID, kAudioHardwarePlugInInterfaceID)) {
#if PRINTDEBUG
            printf("JAS: mQueryInterface : kAudioHardwarePlugInInterfaceID\n");
#endif
#endif
            // If the TestInterface was requested, bump the ref count,
            // set the ppv parameter equal to the instance, and
            // return good status.
            ((MyType *)obj)->_testInterface->AddRef(obj);
            *ppv = obj;
            CFRelease(interfaceID);
            return S_OK;
        } else if (CFEqual(interfaceID, IUnknownUUID)) {
            // If the IUnknown interface was requested, same as above.
            ((MyType *)obj)->_testInterface->AddRef(obj);
            *ppv = obj;
            CFRelease(interfaceID);
            return S_OK;
        } else {
            // Requested interface unknown, bail with error.
            *ppv = NULL;
            CFRelease(interfaceID);
            return E_NOINTERFACE;
        }
    }

    // Implementation of reference counting for this type.
    // Whenever an interface is requested, bump the refCount for
    // the instance. NOTE: returning the refcount is a convention
    // but is not required so donâ€™t rely on it.
    static ULONG myAddRef(void * obj) {
#if PRINTDEBUG
        printf("JAS: myAddRef\n");
#endif

        ((MyType *)obj)->_refCount += 1;
        return ((MyType *)obj)->_refCount;
    }

    // When an interface is released, decrement the refCount.
    // If the refCount goes to zero, deallocate the instance.
    static ULONG myRelease(void * obj) {
#if PRINTDEBUG
        printf("JAS: myRelease\n");
#endif

        ((MyType *)obj)->_refCount -= 1;
        if (((MyType *)obj)->_refCount == 0) {
            _deallocMyType((MyType *)obj);
            return 0;
        } else
            return ((MyType *)obj)->_refCount;
    }

    // The TestInterface function table.

#ifdef kAudioHardwarePlugInInterface2ID

    static AudioHardwarePlugInInterface testInterfaceFtbl = {
                NULL,                             // Required padding for COM
                myQueryInterface,                 // These three are the required COM functions
                myAddRef,
                myRelease,
                TJackClient::Initialize,
                TJackClient::Teardown,
                TJackClient::DeviceAddIOProc,
                TJackClient::DeviceRemoveIOProc,
                TJackClient::DeviceStart,
                TJackClient::DeviceStop,
                TJackClient::DeviceRead,
                TJackClient::DeviceGetCurrentTime,
                TJackClient::DeviceTranslateTime,
                TJackClient::DeviceGetPropertyInfo,
                TJackClient::DeviceGetProperty,
                TJackClient::DeviceSetProperty,
                TJackClient::StreamGetPropertyInfo,
                TJackClient::StreamGetProperty,
                TJackClient::StreamSetProperty,
                TJackClient::DeviceStartAtTime,
                TJackClient::DeviceGetNearestStartTime
            };

#else

    static AudioHardwarePlugInInterface testInterfaceFtbl = {
                NULL,                             // Required padding for COM
                myQueryInterface,                 // These three are the required COM functions
                myAddRef,
                myRelease,
                TJackClient::Initialize,
                TJackClient::Teardown,
                TJackClient::DeviceAddIOProc,
                TJackClient::DeviceRemoveIOProc,
                TJackClient::DeviceStart,
                TJackClient::DeviceStop,
                TJackClient::DeviceRead,
                TJackClient::DeviceGetCurrentTime,
                TJackClient::DeviceTranslateTime,
                TJackClient::DeviceGetPropertyInfo,
                TJackClient::DeviceGetProperty,
                TJackClient::DeviceSetProperty,
                TJackClient::StreamGetPropertyInfo,
                TJackClient::StreamGetProperty,
                TJackClient::StreamSetProperty
            };

#endif

    // Utility function that allocates a new instance.
    static MyType *_allocMyType(CFUUIDRef factoryID) {
#if PRINTDEBUG
        printf("JAS: _allocMyType\n");
#endif

        // Allocate memory for the new instance.
        MyType *newOne = (MyType *)malloc( sizeof(MyType) );

        // Point to the function table
        newOne->_testInterface = &testInterfaceFtbl;

        // Retain and keep an open instance refcount
        // for each factory.
        newOne->_factoryID = (CFUUIDRef)CFRetain( factoryID );
        CFPlugInAddInstanceForFactory( factoryID );

        // This function returns the IUnknown interface
        // so set the refCount to one.
        newOne->_refCount = 1;
        return newOne;
    }

    // Utility function that deallocates the instance when
    // the refCount goes to zero.
    static void _deallocMyType(MyType * obj) {
#if PRINTDEBUG
        printf("JAS: _deallocMyType\n");
#endif

        CFUUIDRef factoryID = obj->_factoryID;
        free(obj);
        if (factoryID) {
            CFPlugInRemoveInstanceForFactory(factoryID);
            CFRelease(factoryID);
        }
    }

    // Implementation of the factory function for this type.
    void *MyFactory(CFAllocatorRef allocator, CFUUIDRef typeID) {
        // If correct type is being requested, allocate an
        // instance of TestType and return the IUnknown interface.

        if (CFEqual(typeID, kAudioHardwarePlugInTypeID)) {
#if PRINTDEBUG
            printf("JAS: MyFactory kAudioHardwarePlugInTypeID\n");
#endif

#ifdef kAudioHardwarePlugInInterface2ID

            MyType *result = _allocMyType( kAudioHardwarePlugInInterface2ID );
#else

            MyType *result = _allocMyType( kAudioHardwarePlugInInterfaceID );
#endif

            return result;
        } else {
            // If the requested type is incorrect, return NULL.
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif

