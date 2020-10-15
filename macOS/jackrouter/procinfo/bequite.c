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

// bequite.c

#include <stdio.h>
#include "procinfo.h"
#include <signal.h>
#include "bequite.h"

typedef struct processo
{
    char nome[30];
    int pid;
}
InfoProc;

int manyProcesses()
{
    kinfo_proc* pInfo;
    int quanti;
    pInfo = test(&quanti);
    return quanti;
}

InfoProc ottieniInfo(int n)
{
    kinfo_proc* pInfo;
    int quanti;
    pInfo = test(&quanti);
    InfoProc result;
    strcpy(result.nome, pInfo[n].kp_proc.p_comm);
    result.pid = pInfo[n].kp_proc.p_pid;
    return result;
}

char* bequite_get_name(int n)
{
    kinfo_proc* pInfo;
    int quanti;
    pInfo = test(&quanti);
    char *result;
    result = pInfo[n].kp_proc.p_comm;
    if (bequite_get_flag(n) == 1)
        strcat(result, " #stopped#");
    return result;
}

int bequite_get_pid(int n)
{
    kinfo_proc* pInfo;
    int quanti;
    pInfo = test(&quanti);
    return pInfo[n].kp_proc.p_pid;
}

void stop(int n)
{
    kill(bequite_get_pid(n), SIGSTOP);
}

void reStart(int n)
{
    kill(bequite_get_pid(n), SIGCONT);
}

int bequite_get_flag(int n)
{
    kinfo_proc* pInfo;
    int quanti;
    pInfo = test(&quanti);
    int stat;
    stat = pInfo[n].kp_proc.p_stat;
    int result;
    if (stat != 4)
        result = 0;
    if (stat == 4)
        result = 1;
    return result;
}

char* bequite_getNameFromPid(int pid)
{
    int quanti = manyProcesses();
    int i;
    for (i = 0; i < quanti; i++) {
        if (pid == bequite_get_pid(i)) {
            if (strcmp("LaunchCFMApp", bequite_get_name(i)) == 0) { // Look for the name using CARBON (for CFM applications)
                OSErr err;
                ProcessSerialNumber process;
                CFStringRef nomeStr;
                err = GetCurrentProcess(&process);
                if (err == noErr)
                    err = CopyProcessName(&process, &nomeStr);
                if (err != noErr)
                    return bequite_get_name(i);
                else {
                    char* name = (char*) CFStringGetCStringPtr(nomeStr, NULL);
                    if (name == NULL) {
                        char buffer[128]; // A locally stack allocated buffer.....
                        return (CFStringGetCString(nomeStr, buffer, 128, NULL)) ? buffer : NULL;
                    } else {
                        return name;
                    }
                }
            }
            return bequite_get_name(i);
        }
    }
    return NULL;
}


