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
#include "range_functor"


__CWTLIB_BEGIN_NAMESPACE


// ==================== Abstract range functor class ==========================

RangeFunctor::RangeFunctor(const string& Name)
{
    _name = Name;
}

RangeFunctor::RangeFunctor(const RangeFunctor& Src)
{
    _name = Src._name;
}

RangeFunctor::~RangeFunctor()
{}

const string& RangeFunctor::name() const
{
    return _name;
}

// ==================== Linear range functor class ============================

// default params
static const char LinearRangeFunctor_Name[] = "LinearRange";

LinearRangeFunctor::LinearRangeFunctor(cwt_float_t Start, cwt_float_t Step, cwt_float_t End)
  : RangeFunctor(LinearRangeFunctor_Name)
{
    if (Start > End || Step <= 0.0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    _steps = (cwt_uint_t)floor( (End - Start) / Step ) + 1;
    _start = Start;
    _step = Step;
    // obtain actual ending value according to provided step size
    _end = _start + (_steps - 1) * _step;
}

LinearRangeFunctor::LinearRangeFunctor(const LinearRangeFunctor& Src)
  : RangeFunctor(Src)
{
    _start = Src._start;
    _step = Src._step;
    _end = Src._end;
    _steps = Src._steps;
}

cwt_float_t LinearRangeFunctor::start() const
{
    return _start;
}

cwt_float_t LinearRangeFunctor::step() const
{
    return _step;
}

cwt_float_t LinearRangeFunctor::end() const
{
    return _end;
}

cwt_uint_t LinearRangeFunctor::steps() const
{
    return _steps;
}

RangeFunctor* LinearRangeFunctor::clone() const
{
    return new LinearRangeFunctor(*this);
}

cwt_float_t LinearRangeFunctor::operator()(cwt_uint_t i) const
{
    if (i >= _steps)
       throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return _start + i * _step;
}


__CWTLIB_END_NAMESPACE
