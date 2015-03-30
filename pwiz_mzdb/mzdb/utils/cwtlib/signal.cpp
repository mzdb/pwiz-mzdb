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
#include <cstring> // for memcpy() and memset()
using namespace std;
#include "signal"


__CWTLIB_BEGIN_NAMESPACE


Signal::Signal()
{
    _re = _im = NULL;
    _length = 0;
    _fs = 1.0;
    _name = "";
}

Signal::Signal(cwt_uint_t Length, const cwt_float_t *Real, const cwt_float_t *Imag,
               cwt_float_t Fs, const string& Name)
{
    if (Fs <= 0.0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    _fs = Fs;
    _length = Length;
    _name = Name;
    _re = _im = NULL;

    if(_length == 0)
        return;

    // memory allocation in safe manner
    try {
        _re = new cwt_float_t[_length];
        _im = new cwt_float_t[_length];
    }
    catch (...) {
        delete[] _re;
        delete[] _im;
        throw;
    }

    // init real part
    if (Real != NULL)
        memcpy((void *)_re, (void *)Real, _length * sizeof(cwt_float_t));
    else
        memset((void *)_re, 0, _length * sizeof(cwt_float_t));

    // init imaginary part
    if (Imag != NULL)
        memcpy((void *)_im, (void *)Imag, _length * sizeof(cwt_float_t));
    else
        memset((void *)_im, 0, _length * sizeof(cwt_float_t));
}

Signal::Signal(const Signal& Src)
{
    _re = _im = NULL;
    assign(Src);
}

Signal::~Signal()
{
    delete[] _re;
    delete[] _im;
}

cwt_float_t Signal::re(cwt_uint_t i) const throw(out_of_range)
{
    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return _re[i];
}

cwt_float_t Signal::re(cwt_float_t t) const throw(out_of_range)
{
    cwt_uint_t i = (cwt_uint_t)(t * _fs);

    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return _re[i];
}

void Signal::re(cwt_uint_t i, cwt_float_t v) throw(out_of_range)
{
    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    _re[i] = v;
}

void Signal::re(cwt_float_t t, cwt_float_t v) throw(out_of_range)
{
    cwt_uint_t i = (cwt_uint_t)(t * _fs);

    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    _re[i] = v;
}

cwt_float_t Signal::im(cwt_uint_t i) const throw(out_of_range)
{
    if(i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return _im[i];
}

cwt_float_t Signal::im(cwt_float_t t) const throw(out_of_range)
{
    cwt_uint_t i = (cwt_uint_t)(t * _fs);

    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return _im[i];
}

void Signal::im(cwt_uint_t i, cwt_float_t v) throw(out_of_range)
{
    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    _im[i] = v;
}

void Signal::im(cwt_float_t t, cwt_float_t v) throw(out_of_range)
{
    cwt_uint_t i = (cwt_uint_t)(t * _fs);

    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    _im[i] = v;
}

cwt_float_t Signal::mag(cwt_uint_t i) const throw(out_of_range)
{
    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return sqrt(_re[i]*_re[i] + _im[i]*_im[i]);
}

cwt_float_t Signal::mag(cwt_float_t t) const throw(out_of_range)
{
    cwt_uint_t i = (cwt_uint_t)(t * _fs);

    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return sqrt(_re[i]*_re[i] + _im[i]*_im[i]);
}

cwt_float_t Signal::ang(cwt_uint_t i) const throw(out_of_range)
{
    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return atan2(_im[i], _re[i]);
}

cwt_float_t Signal::ang(cwt_float_t t) const throw(out_of_range)
{
    cwt_uint_t i = (cwt_uint_t)(t * _fs);

    if (i >= _length)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return atan2(_im[i], _re[i]);
}

cwt_uint_t Signal::length() const
{
    return _length;
}

cwt_float_t Signal::time() const
{
    return (cwt_float_t)(_length - 1) / _fs;
}

const string& Signal::getName() const
{
    return _name;
}

void Signal::setName(const string& Name)
{
    _name = Name;
}

cwt_float_t Signal::getFs() const
{
    return _fs;
}

void Signal::setFs(cwt_float_t Fs)
{
    if (Fs <= 0.0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    _fs = Fs;
}

cwt_float_t Signal::getDt() const
{
    return 1.0 / _fs;
}

void Signal::setDt(cwt_float_t Dt)
{
    if (Dt <= 0.0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    _fs = 1.0 / Dt;
}

void Signal::assign(const Signal& Src)
{
    cwt_float_t *tmp_re = NULL, *tmp_im = NULL;

    if(&Src == this) return;

    try {
        if (Src._length != 0) {
            // safe memory allocation using temporary pointers
            tmp_re = new cwt_float_t[Src._length];
            tmp_im = new cwt_float_t[Src._length];

            // copy data from specified signal object
            memcpy((void *)tmp_re, (void *)Src._re, Src._length * sizeof(cwt_float_t));
            memcpy((void *)tmp_im, (void *)Src._im, Src._length * sizeof(cwt_float_t));
        }
    }
    catch (...) {
        delete[] tmp_re;
        delete[] tmp_im;
        throw;
    }

    delete[] _re;
    delete[] _im;
 
    _re = tmp_re;
    _im = tmp_im;

    _name = Src._name;
    _length = Src._length;
    _fs = Src._fs;
}

void Signal::resize(cwt_uint_t NewSize)
{
    cwt_float_t *tmp_re = NULL, *tmp_im = NULL;
    cwt_int_t cpy_size;

    // if resizing is not needed
    if (_length == NewSize)
        return;

    // delete internal data if NewSize is zero
    if (NewSize == 0) {
        _length = 0;
        delete[] _re;
        delete[] _im;
        _re = NULL;
        _im = NULL;
        return;
    }

    // data size need to be copied from old storage
    cpy_size = (NewSize < _length) ? NewSize : _length;

    // allocate new storage
    try {
        tmp_re = new cwt_float_t[NewSize];
        tmp_im = new cwt_float_t[NewSize];
    }
    catch (...) {
        delete[] tmp_re;
        delete[] tmp_im;
        throw;
    }

    // copy
    memcpy((void *)tmp_re, (void *)_re, cpy_size * sizeof(cwt_float_t));
    memcpy((void *)tmp_im, (void *)_im, cpy_size * sizeof(cwt_float_t));

    // pad with zeros if needed
    if (NewSize > _length) {
        for (cwt_uint_t i = _length; i < NewSize; i++) {
            tmp_re[i] = 0.0;
            tmp_im[i] = 0.0;
        }
    }

    delete[] _re;
    delete[] _im;

    _length = NewSize;

    _re = tmp_re;
    _im = tmp_im;
}

Signal* Signal::clone() const
{
    return new Signal(*this);
}

ReDataProxy Signal::reData() const
{
    return ReDataProxy(_re, _length);
}

ImDataProxy Signal::imData() const
{
    return ImDataProxy(_im, _length);
}

Signal& Signal::operator=(const Signal& Src)
{
    if (this != &Src)
        assign(Src);

    return *this;
}


__CWTLIB_END_NAMESPACE
