/*
 *   Continuous Wavelet Transform Library
 *   Copyright (C) 2004-2009 Stepan V. Karpenko <carp@mail.ru>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the
 *   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 *   Boston, MA  02111-1307  USA
 */

#ifndef __CWTLIB_CONFIG__
#define __CWTLIB_CONFIG__


using std::invalid_argument;
using std::out_of_range;
using std::string;


#define CWTLIB_DEBUG  1  /* Debug: 0 - off, 1 - on */

#ifdef __GNUC__
#  define __CWTLIB_FUNCTION  __PRETTY_FUNCTION__
#else
#  define __CWTLIB_FUNCTION  __FUNCTION__
#endif

// exceptions used in cwtlib
#define CWTLIB_EXCEPTION_INVALID_ARG()  \
                invalid_argument( string(__CWTLIB_FUNCTION) + \
                string(": invalid argument") )
#define CWTLIB_EXCEPTION_OUT_OF_RANGE()  \
                out_of_range( string(__CWTLIB_FUNCTION) + \
                string(": out of range") )

// very small value
#define TINY 1E-200

// constants declarations
#define CWT_PI      3.141592653589793238462643383279502884197 /* PI         */
#define CWT_2PI     6.283185307179586476925286766559005768394 /* 2*PI       */
#define CWT_SQRTPI  1.772453850905516027298167483341145182797 /* sqrt(PI)   */
#define CWT_SQRT2PI 2.506628274631000502415765284811045253006 /* sqrt(2*PI) */

#ifndef PI
#  define PI CWT_PI
#endif

// cwtlib namespace definitions
#define __CWTLIB_BEGIN_NAMESPACE namespace cwtlib {
#define __CWTLIB_END_NAMESPACE }


#endif
