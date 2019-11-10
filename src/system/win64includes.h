#pragma once

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 

#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Ensure that we fix a version for Windows IE extensions
#undef _WIN32_IE
#define _WIN32_IE 0x0600

//
// Un-sabotage plaform-secific header guards
//

// windows.h
#undef _WINDOWS_

// winsock2.h
#undef _WINSOCK2API_

// objbase.h
#undef _OBJBASE_H_
#undef __RPC_H__
#undef __RPCNDR_H__

// xinput.h
#undef _XINPUT_H_

// io.h
#undef _INC_IO

// sys/types.h
#undef _INC_TYPES

// sys/stat.h
#undef _INC_STAT

#include <windows.h>
