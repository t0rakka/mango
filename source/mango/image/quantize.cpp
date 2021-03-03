/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    Original NeuQuant implementation (C) 1994 Anthony Becker
    Based on Self Organizing Map (SOM) neural network algorithm by Kohonen
*/
#include <mango/core/bits.hpp>
#include <mango/math/math.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/quantize.hpp>

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // constants
    // ------------------------------------------------------------

    enum
    {
        NETSIZE = 256,
        INITRAD = NETSIZE >> 3
    };

    #define prime1		    499
    #define prime2		    491
    #define prime3		    487
    #define prime4		    503

    #define netbiasshift	4
    #define ncycles			100

    #define intbiasshift    16
    #define intbias			(((int) 1) << intbiasshift)
    #define gammashift  	10
    #define gamma   		(((int) 1) << gammashift)
    #define betashift  		10
    #define beta			(intbias >> betashift)
    #define betagamma		(intbias << (gammashift - betashift))

    #define radiusbiasshift	6
    #define radiusbias		(((int) 1) << radiusbiasshift)
    #define initradius		(INITRAD * radiusbias)
    #define radiusdec		30

    #define alphabiasshift	10
    #define initalpha		(((int) 1) << alphabiasshift)

    #define radbiasshift	8
    #define alpharadbshift  (alphabiasshift + radbiasshift)

    // ------------------------------------------------------------
    // NeuQuant
    // ------------------------------------------------------------

    struct NeuQuant
    {
        u8* m_image;
        int m_length_count;
        int m_sample_factor; // 1..30

        int network[NETSIZE][4];
        int bias[NETSIZE];
        int freq[NETSIZE];
        int radpower[INITRAD];

        NeuQuant(u8* image, int length, int sample_factor);
        ~NeuQuant();

        int contest(int r, int g, int b);
        void alterSingle(int alpha, int i, int r, int g, int b);
        void alterNeigh(int rad, int i, int r, int g, int b);
        void learn();
        void unbias();
    };

    NeuQuant::NeuQuant(u8* image, int length, int sample_factor)
    {
        m_image = image;
        m_length_count = length;
        m_sample_factor = sample_factor;

        for (int i = 0; i < NETSIZE; ++i)
        {
            int* p = network[i];
            p[0] = p[1] = p[2] = (i << (netbiasshift + 8)) / NETSIZE;
            freq[i] = intbias / NETSIZE;
            bias[i] = 0;
        }

        learn();
        unbias();
    }

    NeuQuant::~NeuQuant()
    {
    }

    int NeuQuant::contest(int r, int g, int b)
    {
        int	bestd = ~(((int)1) << 31);
        int	bestbiasd = bestd;
        int	bestpos = -1;
        int	bestbiaspos = bestpos;
        int* p = bias;
        int* f = freq;

        for (int i = 0; i < NETSIZE; ++i)
        {
            const int* n = network[i];
            int dist = std::abs(n[0] - r) +
                       std::abs(n[1] - g) +
                       std::abs(n[2] - b);

            if (dist < bestd)
            {
                bestd = dist;
                bestpos = i;
            }

            int biasdist = dist - ((*p) >> (intbiasshift - netbiasshift));
            if (biasdist < bestbiasd)
            {
                bestbiasd = biasdist;
                bestbiaspos = i;
            }

            int betafreq = (*f >> betashift);
            *f++ -= betafreq;
            *p++ +=(betafreq << gammashift);
        }

        freq[bestpos] += beta;
        bias[bestpos] -= betagamma;

        return bestbiaspos;
    }

    void NeuQuant::alterSingle(int alpha, int i, int r, int g, int b)
    {
        int* n = network[i];
        n[0] -= (alpha * (n[0] - r)) >> alphabiasshift;
        n[1] -= (alpha * (n[1] - g)) >> alphabiasshift;
        n[2] -= (alpha * (n[2] - b)) >> alphabiasshift;
    }

    void NeuQuant::alterNeigh(int rad, int i, int r, int g, int b)
    {
        const int lo = std::max(i - rad, -1);
        const int hi = std::min(i + rad, int(NETSIZE));
        int	j = i + 1;
        int	k = i - 1;

        const int* q = radpower;
        while ((j < hi) || (k > lo))
        {
            int* p;
            int	a = *(++q);

            if (j < hi)
            {
                p = network[j++];
                p[0] -= (a * (p[0] - r)) >> alpharadbshift;
                p[1] -= (a * (p[1] - g)) >> alpharadbshift;
                p[2] -= (a * (p[2] - b)) >> alpharadbshift;
            }
            if (k > lo)
            {
                p = network[k--];
                p[0] -= (a * (p[0] - r)) >> alpharadbshift;
                p[1] -= (a * (p[1] - g)) >> alpharadbshift;
                p[2] -= (a * (p[2] - b)) >> alpharadbshift;
            }
        }
    }

    void NeuQuant::learn()
    {
        const int alphadec = 30 + ((m_sample_factor - 1) / 3);

        u8*	p = m_image;
        u8*	lim = m_image + m_length_count;
        int samplepixels = m_length_count / (4 * m_sample_factor);
        int delta = samplepixels / ncycles;
        int alpha = initalpha;
        int radius = initradius;

        int rad = radius >> radiusbiasshift;
        if (rad > 1)
        {
            int	radrad = rad * rad;
            int	adder = 1;
            int	bigalpha = (alpha << radbiasshift) / radrad;

            for (int i = 0; i < rad; ++i)
            {
                radpower[i] = bigalpha * radrad;
                radrad -= adder;
                adder += 2;
            }
        }
        else
        {
            rad = 0;
        }

        int	step;
        if (m_length_count % prime1)
        {
            step = 4 * prime1;
        }
        else
        {
            if (m_length_count % prime2)
            {
                step = 4 * prime2;
            }
            else
            {
                if (m_length_count % prime3)
                {
                    step = 4 * prime3;
                }
                else
                {
                    step = 4 * prime4;
                }
            }
        }

        int j, r, g, b;
        int i = 0;
        int	phase = 0;
        while (i++ < samplepixels)
        {
            r = p[0] << netbiasshift;
            g = p[1] << netbiasshift;
            b = p[2] << netbiasshift;
            j = contest(r, g, b);

            alterSingle(alpha, j, r, g, b);
            if (rad)
            {
                alterNeigh(rad, j, r, g, b);
            }

            p += step;
            while (p >= lim)
            {
                p -= m_length_count;
            }

            if (++phase == delta)
            {
                phase = 0;

                alpha -= alpha / alphadec;
                radius -= radius / radiusdec;
                rad = radius >> radiusbiasshift;

                if (rad > 1)
                {
                    int	radrad = rad * rad;
                    int	adder = 1;
                    int	bigalpha = (alpha << radbiasshift) / radrad;

                    for (int k = 0; k < rad; ++k)
                    {
                        radpower[k] = bigalpha * radrad;
                        radrad -= adder;
                        adder += 2;
                    }
                }
                else
                {
                    rad = 0;
                }
            }
        }
    }

    void NeuQuant::unbias()
    {
        for (int i = 0; i < NETSIZE; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                constexpr int bias = 1 << (netbiasshift - 1);
                network[i][j] = math::clamp((network[i][j] + bias) >> netbiasshift, 0, 255);
            }

            network[i][3] = i;
        }
    }

} // namespace

namespace mango::image
{

    ColorQuantizer::ColorQuantizer(const Surface& source, float quality)
    {
        quality = math::clamp(quality, 0.0f, 1.0f);
        int sample_factor = std::max(1, 30 - int(quality * 29.0f + 1.0f));

        Bitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
        NeuQuant nq(temp.image, temp.width * temp.height * 4, sample_factor);

        m_palette.size = NETSIZE;

        for (int i = 0; i < NETSIZE; ++i)
        {
            m_palette.color[i].r = nq.network[i][0];
            m_palette.color[i].g = nq.network[i][1];
            m_palette.color[i].b = nq.network[i][2];
            m_palette.color[i].a = 0xff;

            m_network[i][0] = nq.network[i][0];
            m_network[i][1] = nq.network[i][1];
            m_network[i][2] = nq.network[i][2];
            m_network[i][3] = nq.network[i][3];
        }

        buildIndex();
    }

    ColorQuantizer::ColorQuantizer(const Palette& palette)
    {
        m_palette = palette;

        for (int i = 0; i < NETSIZE; ++i)
        {
            m_network[i][0] = palette[i].r;
            m_network[i][1] = palette[i].g;
            m_network[i][2] = palette[i].b;
            m_network[i][3] = i;
        }

        buildIndex();
    }

    ColorQuantizer::~ColorQuantizer()
    {
    }

    Palette ColorQuantizer::getPalette() const
    {
        return m_palette;
    }

    void ColorQuantizer::quantize(const Surface& dest, const Surface& source, bool dithering)
    {
        if (!dest.format.isIndexed())
        {
            MANGO_EXCEPTION("[ColorQuantizer] The destination surface must have indexed format.");
        }

        if (dest.width != source.width || dest.height != source.height)
        {
            MANGO_EXCEPTION("[ColorQuantizer] The destination and source dimensions must be identical.");
        }

        Bitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        int width = temp.width;
        int height = temp.height;

        for (int y = 0; y < height; ++y)
        {
            Color* s = temp.address<Color>(0, y + 0);
            Color* n = temp.address<Color>(0, y + 1);
            u8* d = dest.address<u8>(0, y);

            for (int x = 0; x < width; ++x)
            {
                int r = s[x].r;
                int g = s[x].g;
                int b = s[x].b;
                u8 index = getIndex(r, g, b);
                d[x] = index;

                if (dithering)
                {
                    // quantization error
                    r -= m_palette[index].r;
                    g -= m_palette[index].g;
                    b -= m_palette[index].b;

                    // distribute the error to neighbouring pixels with Floyd-Steinberg weights
                    if (x < width - 1)
                    {
                        s[x + 1].r = math::clamp(s[x + 1].r + (r * 7 / 16), 0, 255);
                        s[x + 1].g = math::clamp(s[x + 1].g + (g * 7 / 16), 0, 255);
                        s[x + 1].b = math::clamp(s[x + 1].b + (b * 7 / 16), 0, 255);

                        if (y < height - 1)
                        {
                            if (x > 0)
                            {
                                n[x - 1].r = math::clamp(n[x - 1].r + (r * 3 / 16), 0, 255);
                                n[x - 1].g = math::clamp(n[x - 1].g + (g * 3 / 16), 0, 255);
                                n[x - 1].b = math::clamp(n[x - 1].b + (b * 3 / 16), 0, 255);
                            }

                            n[x + 0].r = math::clamp(n[x + 0].r + (r * 5 / 16), 0, 255);
                            n[x + 0].g = math::clamp(n[x + 0].g + (g * 5 / 16), 0, 255);
                            n[x + 0].b = math::clamp(n[x + 0].b + (b * 5 / 16), 0, 255);

                            n[x + 1].r = math::clamp(n[x + 1].r + (r * 1 / 16), 0, 255);
                            n[x + 1].g = math::clamp(n[x + 1].g + (g * 1 / 16), 0, 255);
                            n[x + 1].b = math::clamp(n[x + 1].b + (b * 1 / 16), 0, 255);
                        }
                    }
                }
            }
        }
    }

    void ColorQuantizer::buildIndex()
    {
        int previouscol = 0;
        int startpos = 0;

        for (int i = 0; i < NETSIZE; ++i)
        {
            int* p = m_network[i];
            int smallpos = i;
            int smallval = p[1];

            int* q;
            for (int j = i + 1; j < NETSIZE; ++j)
            {
                q = m_network[j];
                if (q[1] < smallval)
                {
                    smallpos = j;
                    smallval = q[1];
                }
            }
            q = m_network[smallpos];

            if (i != smallpos)
            {
                std::swap(q[0], p[0]);
                std::swap(q[1], p[1]);
                std::swap(q[2], p[2]);
                std::swap(q[3], p[3]);
            }

            if (smallval != previouscol)
            {
                m_netindex[previouscol] = (startpos + i) >> 1;
                for (int j = previouscol + 1; j < smallval; ++j)
                {
                    m_netindex[j] = i;
                }
                previouscol = smallval;
                startpos = i;
            }
        }

        m_netindex[previouscol] = (startpos + (NETSIZE - 1)) >> 1;
        for (int j = previouscol + 1; j < 256; ++j)
        {
            m_netindex[j] = NETSIZE - 1;
        }
    }

    int ColorQuantizer::getIndex(int r, int g, int b) const
    {
        int	bestd = 1000;
        int	best = -1;
        int	i = m_netindex[g];
        int	j = i - 1;

        while ((i < NETSIZE) || (j >= 0))
        {
            if (i < NETSIZE)
            {
                const int* p = m_network[i];
                int dist = p[1] - g;
                if (dist >= bestd)
                {
                    i = NETSIZE;
                }
                else
                {
                    ++i;
                    if (dist < 0) dist = -dist;
                    dist += std::abs(p[0] - r);
                    if (dist < bestd)
                    {
                        dist += std::abs(p[2] - b);
                        if (dist < bestd)
                        {
                            bestd = dist;
                            best = p[3];
                        }
                    }
                }
            }

            if (j >= 0)
            {
                const int* p = m_network[j];
                int dist = g - p[1];
                if (dist >= bestd)
                {
                    j = -1;
                }
                else
                {
                    --j;
                    if (dist < 0) dist = -dist;
                    dist += std::abs(p[0] - r);
                    if (dist < bestd)
                    {
                        dist += std::abs(p[2] - b);
                        if (dist < bestd)
                        {
                            bestd = dist;
                            best = p[3];
                        }
                    }
                }
            }
        }

        return best;
    }

} // namespace mango::image
