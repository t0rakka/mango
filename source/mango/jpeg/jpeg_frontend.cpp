/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/image/jpeg.hpp>
#include "jpeg.hpp"

namespace mango::image::jpeg
{

    struct JpegDecoder::Impl
    {
        struct Interface : ImageDecodeInterface
        {
            // Owned by JpegDecoder so StreamDecoder has a cancellation / callback host
            // without requiring the client to open ImageDecoder.
        };

        Interface interface;
        Parser parser;
        JpegInfo cached;

        Impl(ConstMemory memory)
            : parser(&interface, memory)
        {
            rebuildInfo();
        }

        void rebuildInfo()
        {
            cached = {};
            cached.coeff_order = parser.base().coeffOrder();

            if (!parser.header)
            {
                cached.error = parser.header.info;
                return;
            }

            const StreamDecoder& d = parser.base();

            cached.valid = true;
            cached.width = parser.header.width;
            cached.height = parser.header.height;
            cached.precision = d.precision();
            cached.components = d.components();

            cached.baseline = d.isBaseline();
            cached.progressive = d.isProgressive();
            cached.lossless = d.isLossless();
            cached.arithmetic = d.isArithmetic();
            cached.differential = d.isDifferential();
            cached.multiscan = d.isMultiscan();
            cached.ultrahdr = parser.hasGainMap();

            cached.restart_intervals = d.hasRestartInterval();
            cached.mango1 = d.hasMango1();

            cached.encoding = d.encoding();
            cached.compression = d.isArithmetic() ? "Arithmetic" : "Huffman";
            cached.chroma = d.chromaSubsampling();

            cached.xblock = d.mcuWidth();
            cached.yblock = d.mcuHeight();
            cached.xmcu = d.mcuColumns();
            cached.ymcu = d.mcuRows();
            cached.blocks_in_mcu = d.blocksInMcu();

            const Frame* frames = d.componentFrames();
            for (int i = 0; i < cached.components && i < 4; ++i)
            {
                cached.component[i].id = frames[i].compid;
                cached.component[i].hsf = frames[i].hsf;
                cached.component[i].vsf = frames[i].vsf;
                cached.component[i].qt = frames[i].tq;
            }

            const QuantTable* qt = d.quantizationTables();
            const bool spectral = cached.coeff_order == CoeffOrder::Spectral;
            for (int i = 0; i < 4; ++i)
            {
                cached.quant_bits[i] = qt[i].bits;
                if (spectral)
                {
                    cached.quant[i] = d.spectralQuantTable(i);
                }
                else
                {
                    cached.quant[i] = qt[i].table;
                }
            }
        }
    };

    JpegDecoder::JpegDecoder(ConstMemory memory)
        : m(std::make_unique<Impl>(memory))
    {
    }

    JpegDecoder::~JpegDecoder() = default;

    const JpegInfo& JpegDecoder::info() const
    {
        return m->cached;
    }

    void JpegDecoder::setCoeffOrder(CoeffOrder order)
    {
        m->parser.base().setCoeffOrder(order);
        m->rebuildInfo();
    }

    CoeffOrder JpegDecoder::coeffOrder() const
    {
        return m->parser.base().coeffOrder();
    }

    void JpegDecoder::setCoeffSink(CoeffSink sink, void* user)
    {
        m->parser.base().setCoeffSink(sink, user);
    }

    void JpegDecoder::setCoeffStorage(s16* data, size_t samples)
    {
        m->parser.base().setCoeffStorage(data, samples);
    }

    ImageDecodeStatus JpegDecoder::decodePixels(const Surface& dest, const ImageDecodeOptions& options)
    {
        return m->parser.decode(dest, options);
    }

    ImageDecodeStatus JpegDecoder::decodeCoefficients(const ImageDecodeOptions& options)
    {
        return m->parser.base().decodeCoefficients(options);
    }

    EntropyStream JpegDecoder::extractEntropy()
    {
        return m->parser.base().extractEntropy();
    }

    void JpegDecoder::cancel()
    {
        m->interface.cancelled = true;
    }

    bool JpegDecoder::cancelled() const
    {
        return m->interface.cancelled.load(std::memory_order_relaxed);
    }

} // namespace mango::image::jpeg
