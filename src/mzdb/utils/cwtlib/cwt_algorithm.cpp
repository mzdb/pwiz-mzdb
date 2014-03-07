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
#include <memory> // for auto_ptr
#include <algorithm>
#include <stdexcept>
using namespace std;
#include "cwt_algorithm"


__CWTLIB_BEGIN_NAMESPACE


// complex multiplication
#define CMPLX_MUL_RE(r1, i1, r2, i2)  (r1*r2-i1*i2)
#define CMPLX_MUL_IM(r1, i1, r2, i2)  (r1*i2+i1*r2)


// ==================== Locally used functions ================================


// compute fast Fourier transform
static void fft(cwt_float_t *re, cwt_float_t *im, cwt_uint_t n, int isign)
{
    cwt_uint_t  i, j, k, l, le, le1, ip, n2;
    cwt_float_t wpr, wpi, wr, wi, wtr, wti;

    n2 = n>>1;
    j = 1;
    for (i=0; i<n-1; i++) {
        if (i<j) {
            wtr     = re[j-1];
            wti     = im[j-1];
            re[j-1] = re[i];
            im[j-1] = im[i];
            re[i]   = wtr;
            im[i]   = wti;
        }
        k = n2;
        while (k<j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
    l=1;
    k=n;
    while (k>>=1) {
        le1 = (le=1<<l++) >> 1;
        wtr = CWT_PI / (cwt_float_t)le1;
        wpr = cos(wtr); wpi = -isign*sin(wtr);
        wr = 1.0;       wi = 0.0;
        for (j=0; j<le1; j++) {
            for (i=j; i<n; i+=le) {
                ip = i + le1;
                wtr    = wr*re[ip] - wi*im[ip];
                wti    = wi*re[ip] + wr*im[ip];
                re[ip] = re[i] - wtr;
                im[ip] = im[i] - wti;
                re[i]  = re[i] + wtr;
                im[i]  = im[i] + wti;
            }
            wr = (wtr=wr)*wpr - wi*wpi;
            wi = wi*wpr + wtr*wpi;
        }
    }
}


// ==================== Continuous wavelet transform algorithms ===============

// CWT computation AS IS
WTransform* CWTalgorithm::cwt(const Signal& s, const RangeFunctor& Scales,
        const RangeFunctor& Translations, const Wavelet& MotherWavelet,
        cwt_uint_t ivalp, const string& Name)
{
    // Result
    WTransform *wt;
    auto_ptr<WTransform> wt_owner;
    // pointers to internal Signal/WTransform data
    cwt_float_t *s_re, *s_im;
    cwt_float_t *wt_re, *wt_im;
    // signal params
    cwt_uint_t n = s.length();
    cwt_float_t fs = s.getFs();
    // WT params
    cwt_float_t a, b, T;
    cwt_float_t i, istep;
    // indexes and dimensions
    cwt_uint_t dx, dy;
    cwt_uint_t rows, cols;
    cwt_uint_t row, row_dx;


    // check arguments
    if (Scales.steps() == 0 || Translations.steps() == 0 || n == 0 ||
        fs <= 0.0 || ivalp == 0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    // create result object
    wt = new WTransform(Scales, Translations, MotherWavelet, Name);
    wt_owner.reset(wt); // set ownership

    // obtain result dimensions and pointers to data
    rows = wt->rows();
    cols = wt->cols();
    wt_re = &wt->reData()[0];
    wt_im = &wt->imData()[0];
    s_re = &s.reData()[0];
    s_im = &s.imData()[0];

    // index step (used in convolution stage)
    istep = 1.0 / (cwt_float_t)ivalp;

    // Scales
    for (dy = 0; dy < rows; dy++) {
        // obtain current scale
        a = Scales(dy) * fs;
        if (a == 0.0) a = TINY;

        // set starting index of current row
        row = dy * cols;

        // Translations
        for (dx = 0; dx < cols; dx++) {
            // obtain current translation
            b = Translations(dx) * fs;

            // index of convolution result
            row_dx = row + dx;

            // Perform convolution
            wt_re[row_dx] = 0.0;
            wt_im[row_dx] = 0.0;
            for (i = 0.0; i < n; i += istep) {
                T = (i - b) / a;
                wt_re[row_dx] += CMPLX_MUL_RE(s_re[(cwt_uint_t)i], s_im[(cwt_uint_t)i],
                                    MotherWavelet.reT(T), -MotherWavelet.imT(T));
                wt_im[row_dx] += CMPLX_MUL_IM(s_re[(cwt_uint_t)i], s_im[(cwt_uint_t)i],
                                    MotherWavelet.reT(T), -MotherWavelet.imT(T));
                // NOTE: "-" before Wavelet imaginary part indicates complex
                // conjunction.
            }
            wt_re[row_dx] *= 1.0 / (sqrt(a) * (cwt_float_t)ivalp);
            wt_im[row_dx] *= 1.0 / (sqrt(a) * (cwt_float_t)ivalp);
        }
    }

    wt_owner.release(); // release ownership

    return wt;
}

// CWT computation AS IS considering short-term nature of a wavelet
WTransform* CWTalgorithm::cwto1(const Signal& s, const RangeFunctor& Scales,
        const RangeFunctor& Translations, const Wavelet& MotherWavelet,
        cwt_uint_t ivalp, const string& Name)
{
    // Result
    WTransform *wt;
    auto_ptr<WTransform> wt_owner;
    // pointers to internal Signal/WTransform data
    cwt_float_t *s_re, *s_im;
    cwt_float_t *wt_re, *wt_im;
    // signal params
    cwt_uint_t n = s.length();
    cwt_float_t fs = s.getFs();
    // WT params
    cwt_float_t a, b, T;
    cwt_float_t i, istep;
    // wavelet params
    cwt_float_t t1, t2;
    // indexes and dimensions
    cwt_uint_t dx, dy;
    cwt_uint_t rows, cols;
    cwt_uint_t row, row_dx;
    // precomputed values
    cwt_float_t a_esl, a_esr;


    // check arguments
    if (Scales.steps() == 0 || Translations.steps() == 0 || n == 0 ||
        fs <= 0.0 || ivalp == 0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    // create result object
    wt = new WTransform(Scales, Translations, MotherWavelet, Name);
    wt_owner.reset(wt); // set ownership

    // obtain result dimensions and pointers to data
    rows = wt->rows();
    cols = wt->cols();
    wt_re = &wt->reData()[0];
    wt_im = &wt->imData()[0];
    s_re = &s.reData()[0];
    s_im = &s.imData()[0];

    // index step (used in convolution stage)
    istep = 1.0 / (cwt_float_t)ivalp;

    // Scales
    for (dy = 0; dy < rows; dy++) {
        // obtain current scale
        a = Scales(dy) * fs;
        if (a == 0.0) a = TINY;

        // set starting index of current row
        row = dy * cols;

        // obtain wavelet support width on that scale
        a_esl = a*MotherWavelet.effL();  a_esr = a*MotherWavelet.effR();

        // Translations
        for (dx = 0; dx < cols; dx++) {
            // obtain current translation
            b = Translations(dx) * fs;

            // index of convolution result
            row_dx = row + dx;

            // compute time range where wavelet presents
            t1 = a_esl + b;          t2 = a_esr + b;
            if (t1 < 0.0) t1 = 0.0;  if (t2 >= n) t2 = (cwt_float_t)(n - 1);

            // Perform convolution
            wt_re[row_dx] = 0.0;
            wt_im[row_dx] = 0.0;
            for (i = t1; i <= t2; i += istep) {
                T = (i - b) / a;
                wt_re[row_dx] += CMPLX_MUL_RE(s_re[(cwt_uint_t)i], s_im[(cwt_uint_t)i],
                                    MotherWavelet.reT(T), -MotherWavelet.imT(T));
                wt_im[row_dx] += CMPLX_MUL_IM(s_re[(cwt_uint_t)i], s_im[(cwt_uint_t)i],
                                    MotherWavelet.reT(T), -MotherWavelet.imT(T));
                // NOTE: "-" before Wavelet imaginary part indicates complex
                // conjunction.
            }
            wt_re[row_dx] *= 1.0 / (sqrt(a) * (cwt_float_t)ivalp);
            wt_im[row_dx] *= 1.0 / (sqrt(a) * (cwt_float_t)ivalp);
        }
    }

    wt_owner.release(); // release ownership

    return wt;
}

// CWT computation AS IS considering short-term nature of a wavelet
// with its precompution
WTransform* CWTalgorithm::cwto2(const Signal& s, const RangeFunctor& Scales,
        const RangeFunctor& Translations, const Wavelet& MotherWavelet,
        cwt_uint_t ivalp, cwt_uint_t npoints, const string& Name)
{
    // Result
    WTransform *wt;
    auto_ptr<WTransform> wt_owner;
    // pointers to internal Signal/WTransform data
    cwt_float_t *s_re, *s_im;
    cwt_float_t *wt_re, *wt_im;
    // signal params
    cwt_uint_t n = s.length();
    cwt_float_t fs = s.getFs();
    // WT params
    cwt_float_t a, b;
    cwt_float_t i, istep;
    // indexes and dimensions
    cwt_uint_t dx, dy;
    cwt_uint_t rows, cols;
    cwt_uint_t row, row_dx;
    // variables for operating with precomputed wavelet
    cwt_float_t t1, t2;
    nr_vectorf W_re;
    nr_vectorf W_im;
    cwt_float_t wstep;
    cwt_int_t L, R, j;
    cwt_float_t d, ind;
    // precomputed values
    cwt_float_t a_esl, a_esr;


    // check arguments
    if (Scales.steps() == 0 || Translations.steps() == 0 || n == 0 ||
        fs <= 0.0 || ivalp == 0 || npoints == 0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    // create result object
    wt = new WTransform(Scales, Translations, MotherWavelet, Name);
    wt_owner.reset(wt); // set ownership

    // obtain result dimensions and pointers to data
    rows = wt->rows();
    cols = wt->cols();
    wt_re = &wt->reData()[0];
    wt_im = &wt->imData()[0];
    s_re = &s.reData()[0];
    s_im = &s.imData()[0];

    //// Precompute wavelet values
    // time step for wavelet compution
    wstep = (cwt_float_t)( (MotherWavelet.effR() - MotherWavelet.effL()) / npoints );
    // left and right indexes
    L = (cwt_int_t)floor(MotherWavelet.effL() / wstep);
    R = (cwt_int_t)ceil(MotherWavelet.effR() / wstep);
    // init nr_vectors
    W_re.reinit(L, R);
    W_im.reinit(L, R);
    // fill vectors with wavelet values
    for (j = L, i = MotherWavelet.effL(); j <= R; j++, i += wstep) {
        W_re[j] = MotherWavelet.reT(i);
        W_im[j] = MotherWavelet.imT(i);
    }
    // scale factor for indexing
    d = (cwt_float_t)( (R - L) / (MotherWavelet.effR() - MotherWavelet.effL()) );

    // index step (used in convolution stage)
    istep = 1.0 / (cwt_float_t)ivalp;

    // Scales
    for (dy = 0; dy < rows; dy++) {
        // obtain current scale
        a = Scales(dy) * fs;
        if (a == 0.0) a = TINY;

        // set starting index of current row
        row = dy * cols;

        // obtain wavelet support width on that scale
        a_esl = a*MotherWavelet.effL(); a_esr = a*MotherWavelet.effR();

        // Translations
        for (dx = 0; dx < cols; dx++) {
            // obtain current translation
            b = Translations(dx) * fs;

            // index of convolution result
            row_dx = row + dx;

            // compute time range where wavelet presents
            t1 = a_esl + b;          t2 = a_esr + b;
            if (t1 < 0.0) t1 = 0.0;  if (t2 >= n) t2 = (cwt_float_t)(n - 1);

            // Perform convolution
            wt_re[row_dx] = 0.0;
            wt_im[row_dx] = 0.0;
            for (i = t1; i <= t2; i += istep) {
                ind = d * (i - b) / a;
                wt_re[row_dx] += CMPLX_MUL_RE(s_re[(cwt_uint_t)i], s_im[(cwt_uint_t)i],
                                    W_re[(cwt_int_t)ind], -W_im[(cwt_int_t)ind]);
                wt_im[row_dx] += CMPLX_MUL_IM(s_re[(cwt_uint_t)i], s_im[(cwt_uint_t)i],
                                    W_re[(cwt_int_t)ind], -W_im[(cwt_int_t)ind]);
                // NOTE: "-" before Wavelet imaginary part indicates complex
                // conjunction.
            }
            wt_re[row_dx] *= 1.0 / (sqrt(a) * (cwt_float_t)ivalp);
            wt_im[row_dx] *= 1.0 / (sqrt(a) * (cwt_float_t)ivalp);
        }
    }

    wt_owner.release(); // release ownership

    return wt;
}

// CWT computation AS IS considering short-term nature of a wavelet
// with its precompution and dynamic adjusting ivalp parameter
WTransform* CWTalgorithm::cwto3(const Signal& s, const RangeFunctor& Scales,
        const RangeFunctor& Translations, const Wavelet& MotherWavelet,
        cwt_uint_t ivalp, cwt_uint_t npoints, const string& Name)
{
    // Result
    WTransform *wt;
    auto_ptr<WTransform> wt_owner;
    // pointers to internal Signal/WTransform data
    cwt_float_t *s_re, *s_im;
    cwt_float_t *wt_re, *wt_im;
    // signal params
    cwt_uint_t n = s.length();
    cwt_float_t fs = s.getFs();
    // WT params
    cwt_float_t a, b;
    cwt_float_t i, istep;
    // indexes and dimensions
    cwt_uint_t dx, dy;
    cwt_uint_t rows, cols;
    cwt_uint_t row, row_dx;
    // variables for operating with precomputed wavelet
    cwt_float_t t1, t2;
    nr_vectorf W_re;
    nr_vectorf W_im;
    cwt_float_t wstep;
    cwt_int_t L, R, j;
    cwt_float_t d, ind;
    // precomputed values
    cwt_float_t ivalp_amin, a_esl, a_esr;


    // check arguments
    if (Scales.steps() == 0 || Translations.steps() == 0 || n == 0 ||
        fs <= 0.0 || ivalp == 0 || npoints == 0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    // create result object
    wt = new WTransform(Scales, Translations, MotherWavelet, Name);
    wt_owner.reset(wt); // set ownership

    // obtain result dimensions and pointers to data
    rows = wt->rows();
    cols = wt->cols();
    wt_re = &wt->reData()[0];
    wt_im = &wt->imData()[0];
    s_re = &s.reData()[0];
    s_im = &s.imData()[0];

    //// Precompute wavelet values
    // time step for wavelet compution
    wstep = (cwt_float_t)( (MotherWavelet.effR() - MotherWavelet.effL()) / npoints );
    // left and right indexes
    L = (cwt_int_t)floor(MotherWavelet.effL() / wstep);
    R = (cwt_int_t)ceil(MotherWavelet.effR() / wstep);
    // init nr_vectors
    W_re.reinit(L, R);
    W_im.reinit(L, R);
    // fill vectors with wavelet values
    for (j = L, i = MotherWavelet.effL(); j <= R; j++, i += wstep) {
        W_re[j] = MotherWavelet.reT(i);
        W_im[j] = MotherWavelet.imT(i);
    }
    // scale factor for indexing
    d = (cwt_float_t)( (R - L) / (MotherWavelet.effR() - MotherWavelet.effL()) );

    // index step (used in convolution stage)
    istep = 1.0 / (cwt_float_t)ivalp;
    // this used to get default ivalp parameter for minimal possible scale
    ivalp_amin = (cwt_float_t)ivalp * min(Scales.start(), Scales.end());

    // Scales
    for (dy = 0; dy < rows; dy++) {
        // obtain current scale
        a = Scales(dy) * fs;
        if (a == 0.0) a = TINY;

        // set starting index of current row
        row = dy * cols;

        // compute ivalp and istep for current a
        ivalp = (cwt_uint_t)ceil( ivalp_amin / a );
        if (ivalp == 0) ivalp = 1; // ivalp cannot be 0
        istep = 1.0 / (cwt_float_t)ivalp;

        // obtain wavelet support width on that scale
        a_esl = a*MotherWavelet.effL(); a_esr = a*MotherWavelet.effR();

        // Translations
        for (dx = 0; dx < cols; dx++) {
            // obtain current translation
            b = Translations(dx) * fs;

            // index of convolution result
            row_dx = row + dx;

            // compute time range where wavelet presents
            t1 = a_esl + b;          t2 = a_esr + b;
            if (t1 < 0.0) t1 = 0.0;  if (t2 >= n) t2 = (cwt_float_t)(n - 1);

            // Perform convolution
            wt_re[row_dx] = 0.0;
            wt_im[row_dx] = 0.0;
            for (i = t1; i <= t2; i += istep) {
                ind = d * (i - b) / a;
                wt_re[row_dx] += CMPLX_MUL_RE(s_re[(cwt_uint_t)i], s_im[(cwt_uint_t)i],
                                    W_re[(cwt_int_t)ind], -W_im[(cwt_int_t)ind]);
                wt_im[row_dx] += CMPLX_MUL_IM(s_re[(cwt_uint_t)i], s_im[(cwt_uint_t)i],
                                    W_re[(cwt_int_t)ind], -W_im[(cwt_int_t)ind]);
                // NOTE: "-" before Wavelet imaginary part indicates complex
                // conjunction.
            }
            wt_re[row_dx] *= 1.0 / (sqrt(a) * (cwt_float_t)ivalp);
            wt_im[row_dx] *= 1.0 / (sqrt(a) * (cwt_float_t)ivalp);
        }
    }

    wt_owner.release(); // release ownership

    return wt;
}

// CWT computation using FFT
WTransform* CWTalgorithm::cwtft(const Signal& s, const RangeFunctor& Scales,
        const Wavelet& MotherWavelet, const string& Name)
{
    // Result
    WTransform *wt;
    auto_ptr<WTransform> wt_owner;
    // indexes and dimensions
    cwt_uint_t dx, dy;
    cwt_uint_t rows, cols;
    // local copy of a source signal
    Signal f(s);
    // signal params
    cwt_uint_t n = f.length();
    cwt_float_t fs = f.getFs();
    // pointers to internal data
    cwt_float_t *wt_re, *wt_im;
    cwt_float_t *f_re, *f_im;
    cwt_float_t *r_re, *r_im;
    // variables for wavelet computation
    cwt_float_t a, w, W_re, W_im;
    // precomputed values
    cwt_float_t sqrt_a_n;
    cwt_float_t twoPIn = CWT_2PI / (cwt_float_t)n;


    // check arguments
    if (Scales.steps() == 0 || fs <= 0.0)
        throw CWTLIB_EXCEPTION_INVALID_ARG();

    // check that signal length is an integer power of two
    dx = n;
    if (((dx - 1) & dx) || (dx == 0 || dx == 1 || dx == 2)) {
        printf("Not a chance !\n");
        throw CWTLIB_EXCEPTION_INVALID_ARG();
        //return false;
    }

    /*while (dx != 1) {
        if ( dx&1 && dx>1 ) {
            //printf("Signal length is not a power of 2... failed\n");
            throw CWTLIB_EXCEPTION_INVALID_ARG();
        }
        dx >>= 1;
    }*/

    // create result object
    wt = new WTransform(Scales, LinearRangeFunctor(0.0, f.getDt(), f.time()),
                        MotherWavelet, Name);
    wt_owner.reset(wt); // set ownership

    // obtain result dimensions and data pointers
    rows = wt->rows();
    cols = wt->cols();
    wt_re = &wt->reData()[0];
    wt_im = &wt->imData()[0];
    f_re = &f.reData()[0];
    f_im = &f.imData()[0];

    // forward Fourier transform of a signal copy
    /*printf("\r%d", n);*/
    fft(f_re, f_im, n, 1);

    // Scales
    for (dy = 0; dy < rows; dy++)
    {
        // get current scale
        a = Scales(dy) * fs;
        if (a == 0.0) a = TINY;

        sqrt_a_n = sqrt(a) / (cwt_float_t)n; // precompution

        // set pointers to transform result 
        r_re = &wt_re[dy * cols];
        r_im = &wt_im[dy * cols];

        // Convolute 
        for (dx = 0; dx < cols; dx++)
        {
            // calculate wave number w
            if (dx<=n>>1)
              w = twoPIn * a * (cwt_float_t)(dx);
            else
              w =-twoPIn * a * (cwt_float_t)(n-dx);

            // calculate wavelet
            W_re = sqrt_a_n * MotherWavelet.reF(w);
            W_im = sqrt_a_n * MotherWavelet.imF(w);

            // compute result
            r_re[dx] = f_re[dx] * W_re + f_im[dx] * W_im;
            r_im[dx] = f_im[dx] * W_re - f_re[dx] * W_im;
        }
        // inverse Fourier transform
        fft(r_re, r_im, n, -1);
    }

    wt_owner.release(); // release ownership

    return wt;
}


__CWTLIB_END_NAMESPACE
