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

// procinfo.c

#include "procinfo.h"

kinfo_proc* GetBSDProcessList(size_t *procCount, kinfo_proc infop)
// Returns a list of all BSD processes on the system. This routine
// allocates the list and puts it in *procList and a count of the
// number of entries in *procCount. You are responsible for freeing
// this list (use "free" from System framework).
// On success, the function returns 0.
// On error, the function returns a BSD errno value.
{
    int err;
    kinfo_proc* result;
    bool done;
    static const int name[] = {
                                  CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0
                              };
	size_t length;
    assert(procCount != NULL);
    *procCount = 0;

    result = NULL;
    done = false;
    do
    {
        assert(result == NULL);
        // Call sysctl with a NULL buffer.
        length = 0;
        err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1, NULL, &length, NULL, 0);
        if (err == -1) {
            err = errno;
        }
        // Allocate an appropriately sized buffer based on the results
        // from the previous call.
        if (err == 0) {
            result = malloc(length);
            if (result == NULL) {
                err = ENOMEM;
            }
        }

        if (err == 0) {
            err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1, result, &length, NULL, 0);
            if (err == -1) {
                err = errno;
            }
            if (err == 0) {
                done = true;
            } else if (err == ENOMEM) {
                assert(result != NULL);
                free(result);
                result = NULL;
                err = 0;
            }
        }
    } while (err == 0 && ! done);
    // Clean up and establish post conditions.
    if (err != 0 && result != NULL)
    {
        free(result);
        result = NULL;
    }

    if (err == 0)
    {
        *procCount = length / sizeof(kinfo_proc);
    }

    return result;
}

kinfo_proc* test(int *quanti)
{
    size_t b;
    kinfo_proc test;
    kinfo_proc *uu;
    uu = GetBSDProcessList(&b, test);
    *quanti = b;
    return uu;
}




