/*
MIT License

Copyright (c) 2016 David Goosen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __PROCESS_WRANGLER_H__
#define __PROCESS_WRANGLER_H__

#include <stdint.h>

#include "pw_error.h"

// TODO: Make max processes an internal constant
#define PW_MAX_PROCESSES 0x8000
#define PW_PROCESS_NAME_LENGTH 260

typedef struct 
{
  uint32_t numCores;
  uint64_t totalPhysicalMemory;
  uint64_t usedPhysicalMemory;
  float cpuUsage;

} PW_SystemInfo;

typedef struct
{
  uint32_t id;
  uint32_t numThreads;
  uint64_t workingSetSize;
  char name[ PW_PROCESS_NAME_LENGTH ];

} PW_Process;

SYMBOL_EXPORT extern int PW_Initialize(void);
SYMBOL_EXPORT extern int PW_UpdateProcessList( void );
SYMBOL_EXPORT extern int PW_GetProcessList( PW_Process *processes, uint32_t count );
SYMBOL_EXPORT extern void PW_ClearProcessList();
SYMBOL_EXPORT extern int PW_KillProcesses( uint32_t *processIds, uint32_t count );
SYMBOL_EXPORT extern int PW_GetSystemInfo( PW_SystemInfo *systemInfo );

#endif /* __PROCESS_WRANGLER_H__ */