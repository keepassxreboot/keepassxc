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

#include "phantomcolor.h"
#include <cfloat>
#include <cmath>

namespace Phantom
{
    namespace
    {

        // Th`ese declarations originate from hsluv.h, from the hsluv-c library. The
        // hpluv functions have been removed, as they are unnecessary for Phantom.
        /**
         * Convert HSLuv to RGB.
         *
         * @param h Hue. Between 0.0 and 360.0.
         * @param s Saturation. Between 0.0 and 100.0.
         * @param l Lightness. Between 0.0 and 100.0.
         * @param[out] pr Red component. Between 0.0 and 1.0.
         * @param[out] pr Green component. Between 0.0 and 1.0.
         * @param[out] pr Blue component. Between 0.0 and 1.0.
         */
        void hsluv2rgb(double h, double s, double l, double* pr, double* pg, double* pb);

        /**
         * Convert RGB to HSLuv.
         *
         * @param r Red component. Between 0.0 and 1.0.
         * @param g Green component. Between 0.0 and 1.0.
         * @param b Blue component. Between 0.0 and 1.0.
         * @param[out] ph Hue. Between 0.0 and 360.0.
         * @param[out] ps Saturation. Between 0.0 and 100.0.
         * @param[out] pl Lightness. Between 0.0 and 100.0.
         */
        void rgb2hsluv(double r, double g, double b, double* ph, double* ps, double* pl);

        // Contents below originate from hsluv.c from the hsluv-c library. They have
        // been wrapped in a C++ namespace to avoid collisions and to reduce the
        // translation unit count, and hsluv's own sRGB conversion code has been
        // stripped out (sRGB conversion is now performed in the Phantom color code
        // when going to/from the Rgb type.)
        //
        // If you need to update the hsluv-c code, be mindful of the removed sRGB
        // conversions -- you will need to make similar modifications to the upstream
        // hsluv-c code. Also note that that the hpluv (pastel) functions have been
        // removed, as they are not used in Phantom.
        typedef struct Triplet_tag Triplet;
        struct Triplet_tag
        {
            double a;
            double b;
            double c;
        };

        /* for RGB */
        const Triplet m[3] = {{3.24096994190452134377, -1.53738317757009345794, -0.49861076029300328366},
                              {-0.96924363628087982613, 1.87596750150772066772, 0.04155505740717561247},
                              {0.05563007969699360846, -0.20397695888897656435, 1.05697151424287856072}};

        /* for XYZ */
        const Triplet m_inv[3] = {{0.41239079926595948129, 0.35758433938387796373, 0.18048078840183428751},
                                  {0.21263900587151035754, 0.71516867876775592746, 0.07219231536073371500},
                                  {0.01933081871559185069, 0.11919477979462598791, 0.95053215224966058086}};

        const double ref_u = 0.19783000664283680764;
        const double ref_v = 0.46831999493879100370;

        const double kappa = 903.29629629629629629630;
        const double epsilon = 0.00885645167903563082;

        typedef struct Bounds_tag Bounds;
        struct Bounds_tag
        {
            double a;
            double b;
        };

        void get_bounds(double l, Bounds bounds[6])
        {
            double tl = l + 16.0;
            double sub1 = (tl * tl * tl) / 1560896.0;
            double sub2 = (sub1 > epsilon ? sub1 : (l / kappa));
            int channel;
            int t;

            for (channel = 0; channel < 3; channel++) {
                double m1 = m[channel].a;
                double m2 = m[channel].b;
                double m3 = m[channel].c;

                for (t = 0; t < 2; t++) {
                    double top1 = (284517.0 * m1 - 94839.0 * m3) * sub2;
                    double top2 = (838422.0 * m3 + 769860.0 * m2 + 731718.0 * m1) * l * sub2 - 769860.0 * t * l;
                    double bottom = (632260.0 * m3 - 126452.0 * m2) * sub2 + 126452.0 * t;

                    bounds[channel * 2 + t].a = top1 / bottom;
                    bounds[channel * 2 + t].b = top2 / bottom;
                }
            }
        }

        double ray_length_until_intersect(double theta, const Bounds* line)
        {
            return line->b / (sin(theta) - line->a * cos(theta));
        }

        double max_chroma_for_lh(double l, double h)
        {
            double min_len = DBL_MAX;
            double hrad = h * 0.01745329251994329577; /* (2 * pi / 360) */
            Bounds bounds[6];
            int i;

            get_bounds(l, bounds);
            for (i = 0; i < 6; i++) {
                double len = ray_length_until_intersect(hrad, &bounds[i]);

                if (len >= 0 && len < min_len)
                    min_len = len;
            }
            return min_len;
        }

        double dot_product(const Triplet* t1, const Triplet* t2)
        {
            return (t1->a * t2->a + t1->b * t2->b + t1->c * t2->c);
        }

        void xyz2rgb(Triplet* in_out)
        {
            double r = dot_product(&m[0], in_out);
            double g = dot_product(&m[1], in_out);
            double b = dot_product(&m[2], in_out);
            in_out->a = r;
            in_out->b = g;
            in_out->c = b;
        }

        void rgb2xyz(Triplet* in_out)
        {
            Triplet rgbl = {in_out->a, in_out->b, in_out->c};
            double x = dot_product(&m_inv[0], &rgbl);
            double y = dot_product(&m_inv[1], &rgbl);
            double z = dot_product(&m_inv[2], &rgbl);
            in_out->a = x;
            in_out->b = y;
            in_out->c = z;
        }

        /* http://en.wikipedia.org/wiki/CIELUV
         * In these formulas, Yn refers to the reference white point. We are using
         * illuminant D65, so Yn (see refY in Maxima file) equals 1. The formula is
         * simplified accordingly.
         */
        double y2l(double y)
        {
            if (y <= epsilon) {
                return y * kappa;
            } else {
                return 116.0 * cbrt(y) - 16.0;
            }
        }

        double l2y(double l)
        {
            if (l <= 8.0) {
                return l / kappa;
            } else {
                double x = (l + 16.0) / 116.0;
                return (x * x * x);
            }
        }

        void xyz2luv(Triplet* in_out)
        {
            double divisor = in_out->a + (15.0 * in_out->b) + (3.0 * in_out->c);
            if (divisor <= 0.00000001) {
                in_out->a = 0.0;
                in_out->b = 0.0;
                in_out->c = 0.0;
                return;
            }

            double var_u = (4.0 * in_out->a) / divisor;
            double var_v = (9.0 * in_out->b) / divisor;
            double l = y2l(in_out->b);
            double u = 13.0 * l * (var_u - ref_u);
            double v = 13.0 * l * (var_v - ref_v);

            in_out->a = l;
            if (l < 0.00000001) {
                in_out->b = 0.0;
                in_out->c = 0.0;
            } else {
                in_out->b = u;
                in_out->c = v;
            }
        }

        void luv2xyz(Triplet* in_out)
        {
            if (in_out->a <= 0.00000001) {
                /* Black will create a divide-by-zero error. */
                in_out->a = 0.0;
                in_out->b = 0.0;
                in_out->c = 0.0;
                return;
            }

            double var_u = in_out->b / (13.0 * in_out->a) + ref_u;
            double var_v = in_out->c / (13.0 * in_out->a) + ref_v;
            double y = l2y(in_out->a);
            double x = -(9.0 * y * var_u) / ((var_u - 4.0) * var_v - var_u * var_v);
            double z = (9.0 * y - (15.0 * var_v * y) - (var_v * x)) / (3.0 * var_v);
            in_out->a = x;
            in_out->b = y;
            in_out->c = z;
        }

        void luv2lch(Triplet* in_out)
        {
            double l = in_out->a;
            double u = in_out->b;
            double v = in_out->c;
            double h;
            double c = sqrt(u * u + v * v);

            /* Grays: disambiguate hue */
            if (c < 0.00000001) {
                h = 0;
            } else {
                h = atan2(v, u) * 57.29577951308232087680; /* (180 / pi) */
                if (h < 0.0)
                    h += 360.0;
            }

            in_out->a = l;
            in_out->b = c;
            in_out->c = h;
        }

        void lch2luv(Triplet* in_out)
        {
            double hrad = in_out->c * 0.01745329251994329577; /* (pi / 180.0) */
            double u = cos(hrad) * in_out->b;
            double v = sin(hrad) * in_out->b;

            in_out->b = u;
            in_out->c = v;
        }

        void hsluv2lch(Triplet* in_out)
        {
            double h = in_out->a;
            double s = in_out->b;
            double l = in_out->c;
            double c;

            /* White and black: disambiguate chroma */
            if (l > 99.9999999 || l < 0.00000001) {
                c = 0.0;
            } else {
                c = max_chroma_for_lh(l, h) / 100.0 * s;
            }

            /* Grays: disambiguate hue */
            if (s < 0.00000001)
                h = 0.0;

            in_out->a = l;
            in_out->b = c;
            in_out->c = h;
        }

        void lch2hsluv(Triplet* in_out)
        {
            double l = in_out->a;
            double c = in_out->b;
            double h = in_out->c;
            double s;

            /* White and black: disambiguate saturation */
            if (l > 99.9999999 || l < 0.00000001) {
                s = 0.0;
            } else {
                s = c / max_chroma_for_lh(l, h) * 100.0;
            }

            /* Grays: disambiguate hue */
            if (c < 0.00000001)
                h = 0.0;

            in_out->a = h;
            in_out->b = s;
            in_out->c = l;
        }

        void hsluv2rgb(double h, double s, double l, double* pr, double* pg, double* pb)
        {
            Triplet tmp = {h, s, l};

            hsluv2lch(&tmp);
            lch2luv(&tmp);
            luv2xyz(&tmp);
            xyz2rgb(&tmp);

            *pr = tmp.a;
            *pg = tmp.b;
            *pb = tmp.c;
        }

        void rgb2hsluv(double r, double g, double b, double* ph, double* ps, double* pl)
        {
            Triplet tmp = {r, g, b};

            rgb2xyz(&tmp);
            xyz2luv(&tmp);
            luv2lch(&tmp);
            lch2hsluv(&tmp);

            *ph = tmp.a;
            *ps = tmp.b;
            *pl = tmp.c;
        }

    } // namespace
} // namespace Phantom

// The code below is for Phantom, and is used for the Rgb/Hsl-based interface
// for color operations.
namespace Phantom
{
    namespace
    {
        // Note: these constants might be out of range when qreal is defined as float
        // instead of double.
        inline qreal linear_of_srgb(qreal x)
        {
            return x < 0.0404482362771082 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4f);
        }
        inline qreal srgb_of_linear(qreal x)
        {
            return x < 0.00313066844250063 ? x * 12.92 : std::pow(x, 1.0 / 2.4) * 1.055 - 0.055;
        }
    } // namespace

    Rgb rgb_of_qcolor(const QColor& color)
    {
        Rgb a;
        a.r = linear_of_srgb(color.red() / 255.0);
        a.g = linear_of_srgb(color.green() / 255.0);
        a.b = linear_of_srgb(color.blue() / 255.0);
        return a;
    }

    Hsl hsl_of_rgb(qreal r, qreal g, qreal b)
    {
        double h, s, l;
        rgb2hsluv(r, g, b, &h, &s, &l);
        s /= 100.0;
        l /= 100.0;
        return {h, s, l};
    }

    Rgb rgb_of_hsl(qreal h, qreal s, qreal l)
    {
        double r, g, b;
        hsluv2rgb(h, s * 100.0, l * 100.0, &r, &g, &b);
        return {r, g, b};
    }

    QColor qcolor_of_rgb(qreal r, qreal g, qreal b)
    {
        int r_ = static_cast<int>(std::lround(srgb_of_linear(r) * 255.0));
        int g_ = static_cast<int>(std::lround(srgb_of_linear(g) * 255.0));
        int b_ = static_cast<int>(std::lround(srgb_of_linear(b) * 255.0));
        return {r_, g_, b_};
    }

    QColor lerpQColor(const QColor& x, const QColor& y, qreal a)
    {
        Rgb x_ = rgb_of_qcolor(x);
        Rgb y_ = rgb_of_qcolor(y);
        Rgb z = Rgb::lerp(x_, y_, a);
        return qcolor_of_rgb(z.r, z.g, z.b);
    }

    Rgb Rgb::lerp(const Rgb& x, const Rgb& y, qreal a)
    {
        Rgb z;
        z.r = (1.0 - a) * x.r + a * y.r;
        z.g = (1.0 - a) * x.g + a * y.g;
        z.b = (1.0 - a) * x.b + a * y.b;
        return z;
    }
} // namespace Phantom
