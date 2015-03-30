/* 
*******************************************************************

FILENAME:	cdtdefs.h

PURPOSE:	Compressed data cluster error codes and delcspec

COMMENTS:	
						
AUTHOR: 	Keith Richardson
			Copyright (C) 2006 Micromass Ltd - All Rights Reserved

*********************************************************************
*/

#ifndef CDT_DEFS
#define CDT_DEFS

#ifdef CDT_EXPORTS
#define CDT_API __declspec(dllexport)
#else
#define CDT_API __declspec(dllimport)
#endif

typedef int				CDC_RESULT;

const int CDC_BASE	= 0;

//	CDC result and error codes.
const CDC_RESULT    CDC_OK					=	0;
const CDC_RESULT	CDC_ERROR				=	CDC_BASE +	1;
const CDC_RESULT	CDC_UNINITIALISED		=	CDC_BASE +	2;
const CDC_RESULT	CDC_DATA_NOT_LOADED		=	CDC_BASE +	3;
const CDC_RESULT	CDC_FILEOPEN_ERROR		=	CDC_BASE +	4;
const CDC_RESULT	CDC_INFO_ERROR			=	CDC_BASE +	5;
const CDC_RESULT	CDC_UNSUPPORTED_FORMAT	=	CDC_BASE +	6;
const CDC_RESULT	CDC_OUT_OF_RANGE		=	CDC_BASE +	7;

#endif