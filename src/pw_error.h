#ifndef __PW_ERROR_H__
#define __PW_ERROR_H__

#include <stdint.h>

#ifdef _WIN32
#define SYMBOL_EXPORT __declspec(dllexport)
#else
#define SYMBOL_EXPORT
#endif

#define PW_ERROR_MESSAGE_LENGTH 320

typedef enum
{
  PW_ERROR_NONE = 0,
  PW_ERROR_INVALID_ARGUMENT = 1,
  PW_ERROR_INTERNAL,
} PW_ErrorCode;

typedef struct
{
  PW_ErrorCode code;
  uint32_t line;
  const char *file;
  const char *function;
  char message[ PW_ERROR_MESSAGE_LENGTH ];
} PW_Error;

SYMBOL_EXPORT extern void PW_ClearErrors();
SYMBOL_EXPORT extern int PW_GetError( PW_Error *error );
SYMBOL_EXPORT extern uint32_t PW_GetErrorCount();

#endif /* __PW_ERROR_H__ */