/*
 * HSLuv-C: Human-friendly HSL
 * <http://github.com/hsluv/hsluv-c>
 * <http://www.hsluv.org/>
 *
 * Copyright (c) 2015 Alexei Boronine (original idea, JavaScript implementation)
 * Copyright (c) 2015 Roger Tallada (Obj-C implementation)
 * Copyright (c) 2017 Martin Mitas (C implementation, based on Obj-C implementation)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef PHANTOMCOLOR_H
#define PHANTOMCOLOR_H

#include <QColor>

namespace Phantom
{
    struct Rgb;
    struct Hsl;

    // A color presumed to be in linear space, represented as RGB. Values are in
    // the range 0.0 - 1.0. Conversions to and from QColor will assume the QColor
    // is in sRGB space, and sRGB conversion will be performed.
    struct Rgb
    {
        qreal r, g, b;
        Rgb()
        {
        }
        Rgb(qreal r, qreal g, qreal b)
            : r(r)
            , g(g)
            , b(b)
        {
        }

        inline Hsl toHsl() const;
        inline QColor toQColor() const;
        static inline Rgb ofHsl(const Hsl&);
        static inline Rgb ofQColor(const QColor&);

        static Rgb lerp(const Rgb& x, const Rgb& y, qreal a);
    };

    // A color represented as pseudo-CIE hue, saturation, and lightness. Hue is in
    // the range 0.0 - 360.0 (degrees). Lightness and saturation are in the range
    // 0.0 - 1.0. Using this and making adjustments to the L value will produce
    // more consistent and predictable results than QColor's .darker()/.lighter().
    // Note that this is not strictly CIE -- some of the colorspace is distorted so
    // that it can represented as a continuous coordinate space. Therefore not all
    // adjustments to the parameters will produce perfectly linear results with
    // regards to saturation and lightness. But it's still useful, and better than
    // QColor's .darker()/.lighter(). Additionally, the L value is more useful for
    // performing comparisons between two colors to measure relative and absolute
    // brightness.
    //
    // See the documentation for the hsluv library for more information. (Note that
    // for consistency we treat the S and L values in the range 0.0 - 1.0 instead
    // of 0.0 - 100.0 like hsluv-c on its own does.)
    struct Hsl
    {
        qreal h, s, l;
        Hsl()
        {
        }
        Hsl(qreal h, qreal s, qreal l)
            : h(h)
            , s(s)
            , l(l)
        {
        }

        inline Rgb toRgb() const;
        inline QColor toQColor() const;
        static inline Hsl ofRgb(const Rgb&);
        static inline Hsl ofQColor(const QColor&);
    };
    Rgb rgb_of_qcolor(const QColor& color);
    QColor qcolor_of_rgb(qreal r, qreal g, qreal b);
    Hsl hsl_of_rgb(qreal r, qreal g, qreal b);
    Rgb rgb_of_hsl(qreal h, qreal s, qreal l);

    // Clip a floating point value to the range 0.0 - 1.0.
    inline qreal saturate(qreal x)
    {
        if (x < 0.0)
            return 0.0;
        if (x > 1.0)
            return 1.0;
        return x;
    }

    inline qreal lerp(qreal x, qreal y, qreal a)
    {
        return (1.0 - a) * x + a * y;
    }

    // Linearly interpolate two QColors after trasnforming them to linear color
    // space, treating the QColor values as if they were in sRGB space. The
    // returned QColor is converted back to sRGB space.
    QColor lerpQColor(const QColor& x, const QColor& y, qreal a);

    Hsl Rgb::toHsl() const
    {
        return hsl_of_rgb(r, g, b);
    }

    QColor Rgb::toQColor() const
    {
        return qcolor_of_rgb(r, g, b);
    }

    Rgb Rgb::ofHsl(const Hsl& hsl)
    {
        return rgb_of_hsl(hsl.h, hsl.s, hsl.l);
    }

    Rgb Rgb::ofQColor(const QColor& color)
    {
        return rgb_of_qcolor(color);
    }

    Rgb Hsl::toRgb() const
    {
        return rgb_of_hsl(h, s, l);
    }

    QColor Hsl::toQColor() const
    {
        Rgb rgb = rgb_of_hsl(h, s, l);
        return qcolor_of_rgb(rgb.r, rgb.g, rgb.b);
    }

    Hsl Hsl::ofRgb(const Rgb& rgb)
    {
        return hsl_of_rgb(rgb.r, rgb.g, rgb.b);
    }

    Hsl Hsl::ofQColor(const QColor& color)
    {
        Rgb rgb = rgb_of_qcolor(color);
        return hsl_of_rgb(rgb.r, rgb.g, rgb.b);
    }

} // namespace Phantom

#endif
