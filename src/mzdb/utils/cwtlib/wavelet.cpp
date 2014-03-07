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

#include <cmath>
#include <stdexcept>
using namespace std;
#include "wavelet"


__CWTLIB_BEGIN_NAMESPACE


// ==================== Abstract wavelet class ================================

Wavelet::Wavelet(const string& Name)
{
    _name = Name;
}

Wavelet::Wavelet(const Wavelet& Src)
{
    _name = Src._name;
}

Wavelet::~Wavelet()
{}

const string& Wavelet::name() const
{
    return _name;
}

// ==================== Mexican Hat wavelet ================================

// default params
static const char         MexicanHat_Name[] = "MexicanHat";
static const cwt_float_t  MexicanHat_Fc     = (1.0 / CWT_PI);
// L2 norm. c = 2 / ( sqrt(3) * pi^(1/4) )
static const cwt_float_t  MexicanHat_c      = 0.8673250705840776;
static const cwt_float_t  MexicanHat_r      = 5.0;

MexicanHat::MexicanHat()
  : Wavelet(MexicanHat_Name)
{ }

MexicanHat::MexicanHat(const MexicanHat& Src)
  : Wavelet(Src)
{ }

cwt_float_t MexicanHat::reT(cwt_float_t t) const
{
    t = t * t;
    return MexicanHat_c * (1.0 - t) * exp(-t / 2.0);
}

cwt_float_t MexicanHat::imT(cwt_float_t t) const
{
    return 0.0;
}

cwt_float_t MexicanHat::reF(cwt_float_t w) const
{
    w = w * w;
    return MexicanHat_c * CWT_SQRT2PI * w * exp(-w / 2.0);
}

cwt_float_t MexicanHat::imF(cwt_float_t w) const
{
    return 0.0;
}

cwt_float_t MexicanHat::cFreq() const
{
    return MexicanHat_Fc;
}

cwt_float_t MexicanHat::effL() const
{
    return -MexicanHat_r;
}

cwt_float_t MexicanHat::effR() const
{
    return +MexicanHat_c;
}

Wavelet* MexicanHat::clone() const
{
    return new MexicanHat(*this);
}

// ==================== Complex Morlet wavelet ================================

// default params
static const char         ComplexMorlet_Name[] = "ComplexMorlet";
static const cwt_float_t  ComplexMorlet_Fc     = 0.8;
static const cwt_float_t  ComplexMorlet_Fb     = 2.0;

ComplexMorlet::ComplexMorlet()
  : Wavelet(ComplexMorlet_Name)
{
    _fc = ComplexMorlet_Fc;
    _fb = ComplexMorlet_Fb;
    // compute L2 norm ...
    _c = 1.0 / sqrt(CWT_PI * _fb);
    // ... and effective support boundary values
    _effl = -2.0*_fb;
    _effr = +2.0*_fb;
}

ComplexMorlet::ComplexMorlet(cwt_float_t Fc, cwt_float_t Fb)
  : Wavelet(ComplexMorlet_Name)
{
    if (Fc <= 0.0 || Fb <= 0.0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    _fc = Fc;
    _fb = Fb;
    _c = 1.0 / sqrt(CWT_PI * _fb);
    _effl = -2.0*_fb;
    _effr = +2.0*_fb;
}

ComplexMorlet::ComplexMorlet(const ComplexMorlet& Src)
  : Wavelet(Src)
{
    _fc = Src._fc;
    _fb = Src._fb;
    _c = Src._c;
    _effl = Src._effl;
    _effr = Src._effr;
}

cwt_float_t ComplexMorlet::reT(cwt_float_t t) const
{
    return _c * exp(-(t*t) / _fb) * cos(CWT_2PI * _fc * t);
}

cwt_float_t ComplexMorlet::imT(cwt_float_t t) const
{
    return _c * exp(-(t*t) / _fb) * sin(CWT_2PI * _fc * t);
}

cwt_float_t ComplexMorlet::reF(cwt_float_t w) const
{
    cwt_float_t br;

    br = (w - CWT_2PI * _fc);
    return exp(-_fb * br * br / 4.0);
}

cwt_float_t ComplexMorlet::imF(cwt_float_t w) const
{
    return 0.0;
}

cwt_float_t ComplexMorlet::fBand() const
{
    return _fb;
}

cwt_float_t ComplexMorlet::cFreq() const
{
    return _fc;
}

cwt_float_t ComplexMorlet::effL() const
{
    return _effl;
}

cwt_float_t ComplexMorlet::effR() const
{
    return _effr;
}

Wavelet* ComplexMorlet::clone() const
{
    return new ComplexMorlet(*this);
}


__CWTLIB_END_NAMESPACE
