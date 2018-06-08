﻿// Guids.h
//

// do not use #pragma once - used by ctc compiler
#ifndef __GUIDS_H_
#define __GUIDS_H_

#ifndef _CTC_GUIDS_


#define guidAllowParamsPkg   CLSID_AllowParamsPackage

// Command set guid for our commands (used with IOleCommandTarget)
// {92590d08-2494-43ee-92cb-d2babfcecb93}
DEFINE_GUID(guidAllowParamsCmdSet, 
0x92590D08, 0x2494, 0x43EE, 0x92, 0xCB, 0xD2, 0xBA, 0xBF, 0xCE, 0xCB, 0x93);

// {83285929-227C-11d3-B870-00C04F79F802}
DEFINE_GUID(Group_Undefined, 
0x83285929, 0x227c, 0x11d3, 0xb8, 0x70, 0x0, 0xc0, 0x4f, 0x79, 0xf8, 0x2);

#else  // _CTC_GUIDS

#define Group_Undefined         { 0x83285929, 0x227C, 0x11d3, { 0xB8, 0x70, 0x00, 0xC0, 0x4F, 0x79, 0xF8, 0x02 } }

#define guidAllowParamsPkg      { 0x10C83BB4, 0xE36C, 0x44A7, { 0xAF, 0x3D, 0xD1, 0xD5, 0x9F, 0x30, 0xE3, 0xAA } }
#define guidAllowParamsCmdSet	  { 0x92590D08, 0x2494, 0x43EE, { 0x92, 0xCB, 0xD2, 0xBA, 0xBF, 0xCE, 0xCB, 0x93 } }


#endif // _CTC_GUIDS_

#endif // __GUIDS_H_
