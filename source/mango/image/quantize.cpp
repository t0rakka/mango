/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    Original NeuQuant implementation (C) 1994 Anthony Becker
    Based on Self Organizing Map (SOM) neural network algorithm by Kohonen
*/
#include <mango/math/math.hpp>
#include <mango/image/quantize.hpp>
#include <algorithm>

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

    #define prime1          499
    #define prime2          491
    #define prime3          487
    #define prime4          503

    #define netbiasshift	4
    #define ncycles         100

    #define intbiasshift    16
    #define intbias         ((int(1)) << intbiasshift)
    #define gammashift      10
    #define betashift       10
    #define beta            (intbias >> betashift)
    #define betagamma       (intbias << (gammashift - betashift))

    #define radiusbiasshift	6
    #define radiusbias      ((int(1)) << radiusbiasshift)
    #define initradius      (INITRAD * radiusbias)
    #define radiusdec       30

    #define alphabiasshift  10
    #define radbiasshift    8

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
                p[0] -= (a * (p[0] - r)) >> (alphabiasshift + radbiasshift);
                p[1] -= (a * (p[1] - g)) >> (alphabiasshift + radbiasshift);
                p[2] -= (a * (p[2] - b)) >> (alphabiasshift + radbiasshift);
            }
            if (k > lo)
            {
                p = network[k--];
                p[0] -= (a * (p[0] - r)) >> (alphabiasshift + radbiasshift);
                p[1] -= (a * (p[1] - g)) >> (alphabiasshift + radbiasshift);
                p[2] -= (a * (p[2] - b)) >> (alphabiasshift + radbiasshift);
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
        int alpha = int(1) << alphabiasshift;
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
                constexpr int c_bias = 1 << (netbiasshift - 1);
                network[i][j] = math::clamp((network[i][j] + c_bias) >> netbiasshift, 0, 255);
            }

            network[i][3] = i;
        }
    }

} // namespace

namespace mango::image
{

    // ----------------------------------------------------------------------------
    // ColorQuantizer
    // ----------------------------------------------------------------------------

    ColorQuantizer::ColorQuantizer(const Surface& source, float quality)
    {
        quality = math::clamp(quality, 0.0f, 1.0f);
        int sample_factor = std::max(1, 30 - int(quality * 29.0f + 1.0f));

        // convert to correct format when required
        TemporaryBitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        NeuQuant nq(temp.image, temp.width * temp.height * 4, sample_factor);

        m_palette.size = NETSIZE;

        for (int i = 0; i < NETSIZE; ++i)
        {
            int r = nq.network[i][0];
            int g = nq.network[i][1];
            int b = nq.network[i][2];
            m_palette.color[i] = Color(r, g, b, 0xff);

            m_network[i][0] = nq.network[i][0];
            m_network[i][1] = nq.network[i][1];
            m_network[i][2] = nq.network[i][2];
            m_network[i][3] = nq.network[i][3];
        }

        buildIndex();
    }

    ColorQuantizer::ColorQuantizer(const Palette& palette)
    {
        if (palette.size < 1 || palette.size > 256)
        {
            MANGO_EXCEPTION("[ColorQuantizer] The palette size must be between 1 and 256.");
        }

        m_palette.size = palette.size;

        for (u32 i = 0; i < palette.size; ++i)
        {
            const Color& c = palette[i];
            m_palette[i] = c;

            m_network[i][0] = c.r;
            m_network[i][1] = c.g;
            m_network[i][2] = c.b;
            m_network[i][3] = int(i);
        }

        if (palette.size >= NETSIZE)
        {
            buildIndex();
        }
    }

    ColorQuantizer::~ColorQuantizer()
    {
    }

    const Palette& ColorQuantizer::getPalette() const
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

        // convert to correct format when required
        Bitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        const int width = temp.width;
        const int height = temp.height;

        for (int y = 0; y < height; ++y)
        {
            Color* s = temp.address<Color>(0, y + 0);
            Color* n = temp.address<Color>(0, y + 1); // clipped in the loop
            u8* d = dest.address<u8>(0, y);

            for (int x = 0; x < width; ++x)
            {
                int r = s[x].r;
                int g = s[x].g;
                int b = s[x].b;
                int index = getIndex(r, g, b);
                d[x] = u8(index);

                if (dithering)
                {
                    // quantization error
                    r -= m_palette[index].r;
                    g -= m_palette[index].g;
                    b -= m_palette[index].b;

                    const auto distribute = [] (Color& color, int r, int g, int b, int scale)
                    {
                        r = math::clamp(color.r + (r * scale / 16), 0, 255);
                        g = math::clamp(color.g + (g * scale / 16), 0, 255);
                        b = math::clamp(color.b + (b * scale / 16), 0, 255);
                        color = Color(r, g, b, color.a);
                    };

                    // distribute the error to neighbouring pixels with Floyd-Steinberg weights
                    if (x < width - 1)
                    {
                        distribute(s[x + 1], r, g, b, 7);

                        // clipping
                        if (y < height - 1)
                        {
                            // clipping
                            if (x > 0)
                            {
                                distribute(n[x - 1], r, g, b, 3);
                            }

                            distribute(n[x + 0], r, g, b, 5);
                            distribute(n[x + 1], r, g, b, 1);
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
        const int lookup = int(m_palette.size);

        if (lookup > 0 && lookup < NETSIZE)
        {
            int bestd = 1000;
            int best = 0;

            for (int i = 0; i < lookup; ++i)
            {
                const int* p = m_network[i];
                const int dist = std::abs(p[0] - r) +
                                 std::abs(p[1] - g) +
                                 std::abs(p[2] - b);

                if (dist < bestd)
                {
                    bestd = dist;
                    best = i;
                }
            }

            return best;
        }

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

    // ----------------------------------------------------------------------------
    // QuantizedBitmap
    // ----------------------------------------------------------------------------

    QuantizedBitmap::QuantizedBitmap(const Surface& source, float quality, bool dithering)
        : Bitmap(source.width, source.height, IndexedFormat(8))
    {
        // convert to correct format when required
        TemporaryBitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        ColorQuantizer qt(temp, quality);
        qt.quantize(*this, temp, dithering);

        *this->palette = qt.getPalette();
    }

    QuantizedBitmap::QuantizedBitmap(const Surface& source, const Palette& palette, bool dithering)
        : Bitmap(source.width, source.height, IndexedFormat(8))
    {
        ColorQuantizer qt(palette);
        qt.quantize(*this, source, dithering);

        *this->palette = palette;
    }

    QuantizedBitmap::~QuantizedBitmap()
    {
    }

    static
    int pngIndexBitDepthFromTableSize(int table_size)
    {
        switch (table_size)
        {
            case 2: return 1;
            case 4: return 2;
            case 16: return 4;
            case 256: return 8;
            default: return 8;
        }
    }

    static
    int colorDistance(const Color& a, const Color& b)
    {
        return std::abs(int(a.r) - int(b.r)) +
               std::abs(int(a.g) - int(b.g)) +
               std::abs(int(a.b) - int(b.b));
    }

    static
    Color meanImageColor(const Surface& rgba)
    {
        u64 sum_r = 0;
        u64 sum_g = 0;
        u64 sum_b = 0;
        u64 count = 0;

        for (int y = 0; y < rgba.height; ++y)
        {
            const Color* scan = rgba.address<Color>(0, y);

            for (int x = 0; x < rgba.width; ++x)
            {
                sum_r += scan[x].r;
                sum_g += scan[x].g;
                sum_b += scan[x].b;
                ++count;
            }
        }

        if (!count)
        {
            return Color(0, 0, 0, 0xff);
        }

        return Color(u8(sum_r / count), u8(sum_g / count), u8(sum_b / count), 0xff);
    }

    static
    Palette selectRepresentativeColors(const Surface& rgba, const Palette& learned, int color_count)
    {
        Palette selected;
        selected.size = u32(color_count);

        if (color_count <= 0)
        {
            return selected;
        }

        if (color_count == 1)
        {
            const Color mean = meanImageColor(rgba);
            int best = 0;
            int bestd = colorDistance(mean, learned[0]);

            for (int i = 1; i < NETSIZE; ++i)
            {
                const int dist = colorDistance(mean, learned[i]);
                if (dist < bestd)
                {
                    bestd = dist;
                    best = i;
                }
            }

            selected[0] = learned[best];
            return selected;
        }

        bool used[NETSIZE] {};

        int first = 0;
        int first_score = -1;

        for (int i = 0; i < NETSIZE; ++i)
        {
            int score = 0;

            for (int j = 0; j < NETSIZE; ++j)
            {
                score = std::max(score, colorDistance(learned[i], learned[j]));
            }

            if (score > first_score)
            {
                first_score = score;
                first = i;
            }
        }

        selected[0] = learned[first];
        used[first] = true;

        for (int k = 1; k < color_count; ++k)
        {
            int pick = -1;
            int pick_dist = -1;

            for (int i = 0; i < NETSIZE; ++i)
            {
                if (used[i])
                {
                    continue;
                }

                int min_dist = colorDistance(learned[i], selected[0]);

                for (int j = 1; j < k; ++j)
                {
                    min_dist = std::min(min_dist, colorDistance(learned[i], selected[j]));
                }

                if (min_dist > pick_dist)
                {
                    pick_dist = min_dist;
                    pick = i;
                }
            }

            if (pick < 0)
            {
                pick = k;
            }

            selected[k] = learned[pick];
            used[pick] = true;
        }

        return selected;
    }

    static
    int pngTableSizeFromColorCount(int color_count)
    {
        if (color_count <= 2)
            return 2;
        if (color_count <= 4)
            return 4;
        if (color_count <= 16)
            return 16;
        return 256;
    }

    static
    int gifTableSizeFromColorCount(int color_count)
    {
        int table_size = 2;
        while (table_size < color_count && table_size < 256)
        {
            table_size <<= 1;
        }
        return table_size;
    }

    static
    int indexBitDepthFromTableSize(int table_size)
    {
        int bits = 0;
        for (int size = table_size; size > 1; size >>= 1)
        {
            ++bits;
        }
        return bits;
    }

    IndexedEncodeTarget resolveIndexedEncodeTarget(int requested_colors, const std::string& extension)
    {
        IndexedEncodeTarget target;

        target.color_count = std::max(1, std::min(requested_colors, 256));

        if (extension == ".gif")
        {
            target.table_size = gifTableSizeFromColorCount(target.color_count);
            target.index_bit_depth = indexBitDepthFromTableSize(target.table_size);
        }
        else
        {
            // PNG and other 1/2/4/8-bit indexed encoders use PNG-style rounding.
            MANGO_UNREFERENCED(extension);
            target.table_size = pngTableSizeFromColorCount(target.color_count);
            target.index_bit_depth = pngIndexBitDepthFromTableSize(target.table_size);
        }

        return target;
    }

    std::unique_ptr<Bitmap> quantizeToIndexedBitmap(const Surface& source, int requested_colors,
        bool dither, float quality, const std::string& extension,
        IndexedEncodeTarget* out_target)
    {
        IndexedEncodeTarget target = resolveIndexedEncodeTarget(requested_colors, extension);

        if (out_target)
        {
            *out_target = target;
        }

        TemporaryBitmap rgba(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        ColorQuantizer trainer(rgba, quality);
        const Palette& learned = trainer.getPalette();

        const Palette selected = selectRepresentativeColors(rgba, learned, target.color_count);

        Palette quant_palette;
        quant_palette.size = u32(target.color_count);

        for (int i = 0; i < target.color_count; ++i)
        {
            quant_palette[i] = selected[i];
        }

        Palette fixed;
        fixed.size = u32(target.table_size);

        for (int i = 0; i < target.table_size; ++i)
        {
            if (i < target.color_count)
            {
                fixed[i] = selected[i];
            }
            else
            {
                fixed[i] = Color(0, 0, 0, 0);
            }
        }

        auto bitmap = std::make_unique<Bitmap>(source.width, source.height, IndexedFormat(8));

        ColorQuantizer quantizer(quant_palette);
        quantizer.quantize(*bitmap, rgba, dither);

        *bitmap->palette = fixed;

        return bitmap;
    }

} // namespace mango::image
