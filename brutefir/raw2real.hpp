/*
 * (c) 2011 Victor Su
 * (c) 2001-2004 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _RAW2REAL_HPP_
#define _RAW2REAL_HPP_

#include "global.h"

namespace raw2real
{
    void
    raw2realf(void *_realbuf,
              void *_rawbuf,
              int bytes,
              int shift,
              bool isfloat,
              int spacing,
              bool swap,
              int n_samples);

    void
    raw2reald(void *_realbuf,
              void *_rawbuf,
              int bytes,
              int shift,
              bool isfloat,
              int spacing,
              bool swap,
              int n_samples);
}

#endif
