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
#include <memory>  // for auto_ptr
using namespace std;
#include "wtransform"


__CWTLIB_BEGIN_NAMESPACE


WTransform::WTransform(const RangeFunctor& Scales, const RangeFunctor& Translations,
                       const Wavelet& MotherWavelet, const string& Name)
{
    _re = _im = NULL;
    _wavelet = NULL;
    _scales = NULL;
    _translations = NULL;

    _name = Name;
    _rows = Scales.steps();
    _cols = Translations.steps();

    // transform result cannot be empty
    if (_rows == 0 || _cols == 0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    try {
        // copy necessary objects
        _scales = Scales.clone();
        _translations = Translations.clone();
        _wavelet = MotherWavelet.clone();

        // allocate storage
        _re = new cwt_float_t[ _rows * _cols ];
        _im = new cwt_float_t[ _rows * _cols ];

        // init storage with zeros
        memset((void *)_re, 0, _rows * _cols * sizeof(cwt_float_t));
        memset((void *)_im, 0, _rows * _cols * sizeof(cwt_float_t));
    }
    catch (...) {
        delete _scales;
        delete _translations;
        delete _wavelet;
        delete[] _re;
        delete[] _im;
        throw;
    }
}

WTransform::WTransform(const WTransform& Src)
{
    _scales = NULL;
    _translations = NULL;
    _wavelet = NULL;
    _re = _im = NULL;

    assign(Src);
}

WTransform::~WTransform()
{
    delete _scales;
    delete _translations;
    delete _wavelet;
    delete[] _re;
    delete[] _im;
}

cwt_float_t WTransform::re(cwt_uint_t row, cwt_uint_t col) const throw(out_of_range)
{
    if (row >= _rows || col >= _cols)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return _re[row*_cols + col];
}

void WTransform::re(cwt_uint_t row, cwt_uint_t col, cwt_float_t v) throw(out_of_range)
{
    if (row >= _rows || col >= _cols)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    _re[row*_cols + col] = v;
}

cwt_float_t WTransform::im(cwt_uint_t row, cwt_uint_t col) const throw(out_of_range)
{
    if (row >= _rows || col >= _cols)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    return _im[row*_cols + col];
}

void WTransform::im(cwt_uint_t row, cwt_uint_t col, cwt_float_t v) throw(out_of_range)
{
    if (row >= _rows || col >= _cols)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    _im[row*_cols + col] = v;
}

cwt_float_t WTransform::mag(cwt_uint_t row, cwt_uint_t col) const throw(out_of_range)
{
    if (row >= _rows || col >= _cols)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    cwt_uint_t idx = row*_cols + col;
    return sqrt(_re[idx]*_re[idx] + _im[idx]*_im[idx]);
}

cwt_float_t WTransform::ang(cwt_uint_t row, cwt_uint_t col) const throw(out_of_range)
{
    if (row >= _rows || col >= _cols)
        throw CWTLIB_EXCEPTION_OUT_OF_RANGE();

    cwt_uint_t idx = row*_cols + col;
    return atan2(_im[idx], _re[idx]);
}

cwt_uint_t WTransform::rows() const
{
    return _rows;
}

cwt_uint_t WTransform::cols() const
{
    return _cols;
}

const Wavelet& WTransform::motherWavelet() const
{
    return *_wavelet;
}

const RangeFunctor& WTransform::scales() const
{
    return *_scales;
}

const RangeFunctor& WTransform::translations() const
{
    return *_translations;
}

const string& WTransform::getName() const
{
    return _name;
}

void WTransform::setName(const string& Name)
{
    _name = Name;
}

void WTransform::assign(const WTransform& Src)
{
    cwt_float_t *tmp_re = NULL, *tmp_im = NULL;
    cwt_uint_t tmp_cols, tmp_rows;
    RangeFunctor *tmp_scales, *tmp_translations;
    auto_ptr<RangeFunctor> tmp_scales_owner, tmp_translations_owner;
    Wavelet *tmp_wavelet;
    auto_ptr<Wavelet> tmp_wavelet_owner;

    if (&Src == this) return;

    tmp_rows = Src.scales().steps();
    tmp_cols = Src.translations().steps();

    // check new dimensions
    if (tmp_rows == 0 || tmp_cols == 0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    try {
        // copy necessary objects and set owners of its pointers
        // to ensure memory will be freed on exceptions
        tmp_scales = Src.scales().clone();
        tmp_scales_owner.reset(tmp_scales);
        tmp_translations = Src.translations().clone();
        tmp_translations_owner.reset(tmp_translations);
        tmp_wavelet = Src.motherWavelet().clone();
        tmp_wavelet_owner.reset(tmp_wavelet);

        // allocate temporary storage
        tmp_re = new cwt_float_t[ tmp_rows * tmp_cols ];
        tmp_im = new cwt_float_t[ tmp_rows * tmp_cols ];

        // copy data
        memcpy((void *)tmp_re, (void *)Src._re, tmp_rows * tmp_cols * sizeof(cwt_float_t));
        memcpy((void *)tmp_im, (void *)Src._im, tmp_rows * tmp_cols * sizeof(cwt_float_t));

        //// delete previously stored copies of the objects and assign new ones

        // scales
        delete _scales;
        _scales = tmp_scales;
        tmp_scales_owner.release();

        // translations
        delete _translations;
        _translations = tmp_translations;
        tmp_translations_owner.release();

        // wavelet
        delete _wavelet;
        _wavelet = tmp_wavelet;
        tmp_wavelet_owner.release();
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

    _rows = tmp_rows;
    _cols = tmp_cols;

    _name = Src._name;
}

WTransform* WTransform::clone() const
{
    return new WTransform(*this);
}

ReDataProxy WTransform::reData() const
{
    return ReDataProxy(_re, _rows * _cols);
}

ImDataProxy WTransform::imData() const
{
    return ImDataProxy(_im, _rows * _cols);
}

WTransform& WTransform::operator=(const WTransform& Src)
{
    if (this != &Src)
        assign(Src);

    return *this;
}


__CWTLIB_END_NAMESPACE
