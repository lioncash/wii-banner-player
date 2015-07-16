// Copyright (C) 2003 Dolphin Project.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.
//
// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/
//
// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#pragma once

#include "CommonTypes.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#if defined __GNUC__ && !defined __SSSE3__
#include <emmintrin.h>
static __inline __m128i __attribute__((__always_inline__))
_mm_shuffle_epi8(__m128i a, __m128i mask)
{
	__m128i result;
	__asm__("pshufb %1, %0"
		: "=x" (result)
		: "xm" (mask), "0" (a));
	return result;
}
#endif

#ifndef _WIN32

#include <cerrno>
#ifdef __linux__
#include <byteswap.h>
#endif

#else // WIN32
// Function Cross-Compatibility
	#define strcasecmp _stricmp
	#define strncasecmp _strnicmp
	#define unlink _unlink
	#define snprintf _snprintf
	#define vscprintf _vscprintf
char* strndup (char const *s, size_t n);	

// 64 bit offsets for windows
	#define fseeko _fseeki64
	#define ftello _ftelli64
	#define atoll _atoi64
	#define stat64 _stat64
	#define fstat64 _fstat64
	#define fileno _fileno
#endif // WIN32 ndef

namespace Common
{
inline u8 swap8(u8 _data) {return _data;}

#ifdef _WIN32
inline u16 swap16(u16 _data) {return _byteswap_ushort(_data);}
inline u32 swap32(u32 _data) {return _byteswap_ulong (_data);}
inline u64 swap64(u64 _data) {return _byteswap_uint64(_data);}
#elif __linux__
inline u16 swap16(u16 _data) {return bswap_16(_data);}
inline u32 swap32(u32 _data) {return bswap_32(_data);}
inline u64 swap64(u64 _data) {return bswap_64(_data);}
#elif __APPLE__
inline __attribute__((always_inline)) u16 swap16(u16 _data)
	{return (_data >> 8) | (_data << 8);}
inline __attribute__((always_inline)) u32 swap32(u32 _data)
	{return __builtin_bswap32(_data);}
inline __attribute__((always_inline)) u64 swap64(u64 _data)
	{return __builtin_bswap64(_data);}
#else
// Slow generic implementation.
inline u16 swap16(u16 data) {return (data >> 8) | (data << 8);}
inline u32 swap32(u32 data) {return (swap16(data) << 16) | swap16(data >> 16);}
inline u64 swap64(u64 data) {return ((u64)swap32(data) << 32) | swap32(data >> 32);}
#endif

inline u16 swap16(const u8* _pData) {return swap16(*(const u16*)_pData);}
inline u32 swap32(const u8* _pData) {return swap32(*(const u32*)_pData);}
inline u64 swap64(const u8* _pData) {return swap64(*(const u64*)_pData);}

}  // Namespace Common

