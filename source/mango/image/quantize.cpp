/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
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
    // NeuQuant
    // ------------------------------------------------------------

    constexpr int NETSIZE = 256; // number of colors used
    constexpr int INITRAD = NETSIZE >> 3;

    #define prime1		499
    #define prime2		491
    #define prime3		487
    #define prime4		503

    #define netbiasshift	4
    #define ncycles			100

    #define intbiasshift    16
    #define intbias			(((int) 1) << intbiasshift)
    #define gammashift  	10
    #define gamma   		(((int) 1) << gammashift)
    #define betashift  		10
    #define beta			(intbias >> betashift)
    #define betagamma		(intbias << (gammashift-betashift))

    #define radiusbiasshift	6
    #define radiusbias		(((int) 1) << radiusbiasshift)
    #define initradius		(INITRAD * radiusbias)
    #define radiusdec		30

    #define alphabiasshift	10
    #define initalpha		(((int) 1) << alphabiasshift)

    #define radbiasshift	8
    #define radbias			(((int) 1) << radbiasshift)
    #define alpharadbshift  (alphabiasshift + radbiasshift)
    #define alpharadbias    (((int) 1) << alpharadbshift)

    class NeuQuant
    {
    protected:
        typedef int Sample[4];

        u8* m_image;
        int m_length_count;
        int m_sample_factor; // 1..30

        Sample m_network[NETSIZE];

        int netindex[256];
        int bias[NETSIZE];
        int freq[NETSIZE];
        int radpower[INITRAD];

    public:
        NeuQuant(u8* image, int length, int sample);
        ~NeuQuant();

        void unbias();
        void getPalette(Palette& palette) const;
        void buildIndex();
        int getIndex(int r, int g, int b) const;
        int contest(int b, int g, int r);
        void alter_single(int alpha, int i, int r, int g, int b);
        void alter_neigh(int rad, int i, int r, int g, int b);
        void learn();
    };

    NeuQuant::NeuQuant(u8* image, int length, int sample)
    {
        m_image = image;
        m_length_count = length;
        m_sample_factor = sample;

        for (int i = 0; i < NETSIZE; ++i)
        {
            int* p = m_network[i];
            p[0] = p[1] = p[2] = (i << (netbiasshift + 8)) / NETSIZE;
            freq[i] = intbias / NETSIZE;
            bias[i] = 0;
        }
    }

    NeuQuant::~NeuQuant()
    {
    }

    void NeuQuant::unbias()
    {
        for (int i = 0; i < NETSIZE; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                m_network[i][j] >>= netbiasshift;
            }
            m_network[i][3] = i;
        }
    }

    void NeuQuant::getPalette(Palette& palette) const
    {
        palette.size = NETSIZE;
        for (int i = 0; i < NETSIZE; ++i)
        {
            palette.color[i].b = m_network[i][0];
            palette.color[i].g = m_network[i][1];
            palette.color[i].r = m_network[i][2];
            palette.color[i].a = 0xff;
        }
    }

    void NeuQuant::buildIndex()
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
                netindex[previouscol] = (startpos + i) >> 1;
                for (int j = previouscol + 1; j < smallval; ++j)
                {
                    netindex[j] = i;
                }
                previouscol = smallval;
                startpos = i;
            }
        }
        netindex[previouscol] = (startpos + (NETSIZE - 1)) >> 1;
        for (int j = previouscol + 1; j < 256; ++j)
        {
            netindex[j] = NETSIZE - 1;
        }
    }

    int NeuQuant::getIndex(int r, int g, int b) const
    {
        int	bestd = 1000;
        int	best = -1;
        int	i = netindex[g];
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
                    int a = p[0] - b;
                    if (a < 0) a = -a;
                    dist += a;
                    if (dist < bestd)
                    {
                        a = p[2] - r;
                        if (a < 0) a = -a;
                        dist += a;
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
                    j--;
                    if (dist < 0) dist = -dist;
                    int a = p[0] - b;
                    if (a < 0) a = -a;
                    dist += a;
                    if (dist < bestd)
                    {
                        a = p[2] - r;
                        if (a < 0) a = -a;
                        dist += a;
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

    int NeuQuant::contest(int b, int g, int r)
    {
        int	bestd = ~(((int)1) << 31);
        int	bestbiasd = bestd;
        int	bestpos = -1;
        int	bestbiaspos = bestpos;
        int* p = bias;
        int* f = freq;

        Sample* pnet = m_network;
        for (int i = 0; i < NETSIZE; ++i)
        {
            int dist;
            int a;
            int biasdist;
            int betafreq;
            int* n = (int*)pnet++;

            dist = n[0] - b;
            if (dist < 0) dist = -dist;

            a = n[1] - g;
            if (a < 0) dist -= a;
            else       dist += a;

            a = n[2] - r;
            if (a < 0) dist -= a;
            else       dist += a;

            if (dist < bestd)
            {
                bestd=dist;
                bestpos = i;
            }
            biasdist = dist - ((*p) >> (intbiasshift - netbiasshift));
            if (biasdist < bestbiasd)
            {
                bestbiasd = biasdist;
                bestbiaspos = i;
            }

            betafreq = (*f >> betashift);
            *f++ -= betafreq;
            *p++ +=(betafreq << gammashift);
        }
        freq[bestpos] += beta;
        bias[bestpos] -= betagamma;

        return bestbiaspos;
    }

    void NeuQuant::alter_single(int alpha, int i, int r, int g, int b)
    {
        int* n = m_network[i];
        n[0] -= (alpha * (n[0] - b)) >> alphabiasshift;
        n[1] -= (alpha * (n[1] - g)) >> alphabiasshift;
        n[2] -= (alpha * (n[2] - r)) >> alphabiasshift;
    }

    void NeuQuant::alter_neigh(int rad, int i, int r, int g, int b)
    {
        const int lo = std::max(i - rad, -1);
        const int hi = std::min(i + rad, NETSIZE);
        int	j = i + 1;
        int	k = i - 1;

        int* q = radpower;
        while ((j < hi) || (k > lo))
        {
            int* p;
            int	a = *(++q);

            if (j < hi)
            {
                p = m_network[j++];
                p[0] -= (a * (p[0] - b)) >> alpharadbshift;
                p[1] -= (a * (p[1] - g)) >> alpharadbshift;
                p[2] -= (a * (p[2] - r)) >> alpharadbshift;
            }
            if (k > lo)
            {
                p = m_network[k--];
                p[0] -= (a * (p[0] - b)) >> alpharadbshift;
                p[1] -= (a * (p[1] - g)) >> alpharadbshift;
                p[2] -= (a * (p[2] - r)) >> alpharadbshift;
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

        int j, b, g, r;
        int i = 0;
        int	phase = 0;
        while (i++ < samplepixels)
        {
            b = p[0] << netbiasshift;
            g = p[1] << netbiasshift;
            r = p[2] << netbiasshift;
            j = contest(b, g, r);

            alter_single(alpha, j, r, g, b);
            if (rad)
            {
                alter_neigh(rad, j, r, g, b);
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

                    for (int jj=0; jj < rad; jj++)
                    {
                        radpower[jj] = bigalpha * radrad;
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

} // namespace

namespace mango {
namespace image {

    void ColorQuantizer::quantize(const Surface& dest, const Surface& source, const ColorQuantizeOptions& options)
    {
        float quality = clamp(options.quality, 0.0f, 1.0f);

        int width = source.width;
        int height = source.height;

        if (!dest.format.isIndexed())
        {
            MANGO_EXCEPTION("[ColorQuantizer] The destination surface must have indexed format.");
        }

        if (dest.width != source.width || dest.height != source.height)
        {
            MANGO_EXCEPTION("[ColorQuantizer] The destination and source dimensions must be identical.");
        }

        // NOTE: Don't try to "optimize" this even if the source is already in correct format;
        //       we need the storage to be continuous w/o padding for the NeuQuant.
        //       The dithering will also modify this temporary buffer.
        Bitmap temp(width, height, FORMAT_B8G8R8A8);
        temp.blit(0, 0, source);

        int sample = std::max(1, 30 - int(quality * 29.0f + 1.0f));
        NeuQuant nq(temp.image, width * height * 4, sample);

        nq.learn();
        nq.unbias();
        nq.getPalette(palette);
        nq.buildIndex();

        for (int y = 0; y < height; ++y)
        {
            ColorBGRA* s = temp.address<ColorBGRA>(0, y + 0);
            ColorBGRA* n = temp.address<ColorBGRA>(0, y + 1);
            u8* d = dest.address<u8>(0, y);

            for (int x = 0; x < width; ++x)
            {
                int r = s[x].r;
                int g = s[x].g;
                int b = s[x].b;
                u8 index = nq.getIndex(r, g, b);
                d[x] = index;

                if (options.dithering)
                {
                    // quantization error
                    r -= palette[index].r;
                    g -= palette[index].g;
                    b -= palette[index].b;

                    // distribute the error to neighbouring pixels with Floyd-Steinberg weights
                    if (x < width - 1)
                    {
                        s[x + 1].r = clamp(s[x + 1].r + (r * 7 / 16), 0, 255);
                        s[x + 1].g = clamp(s[x + 1].g + (g * 7 / 16), 0, 255);
                        s[x + 1].b = clamp(s[x + 1].b + (b * 7 / 16), 0, 255);

                        if (y < height - 1)
                        {
                            if (x > 0)
                            {
                                n[x - 1].r = clamp(n[x - 1].r + (r * 3 / 16), 0, 255);
                                n[x - 1].g = clamp(n[x - 1].g + (g * 3 / 16), 0, 255);
                                n[x - 1].b = clamp(n[x - 1].b + (b * 3 / 16), 0, 255);
                            }

                            n[x + 0].r = clamp(n[x + 0].r + (r * 5 / 16), 0, 255);
                            n[x + 0].g = clamp(n[x + 0].g + (g * 5 / 16), 0, 255);
                            n[x + 0].b = clamp(n[x + 0].b + (b * 5 / 16), 0, 255);

                            n[x + 1].r = clamp(n[x + 1].r + (r * 1 / 16), 0, 255);
                            n[x + 1].g = clamp(n[x + 1].g + (g * 1 / 16), 0, 255);
                            n[x + 1].b = clamp(n[x + 1].b + (b * 1 / 16), 0, 255);
                        }
                    }
                }
            }
        }
    }

} // namespace image
} // namespace mango
