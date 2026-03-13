/*
 * Copyright (c) 1990-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
 /*
  * TIFF Library.
  *
  * CCITT Group 3 (T.4) and Group 4 (T.6) Compression Support.
  *
  * This file contains support for decoding and encoding TIFF
  * compression algorithms 2, 3, 4, and 32771.
  *
  * Decoder support is derived, with permission, from the code
  * in Frank Cringle's viewfax program;
  *      Copyright (C) 1990, 1995  Frank D. Cringle.
  */
/*
    The original libtiff and viewfax code has been modified to integrate into the mango framework.
*/

#include <vector>
#include <mango/core/core.hpp>

using namespace mango;

#define FAXMODE_CLASSIC 0x0000       /* default, include RTC */
#define FAXMODE_NORTC 0x0001         /* no RTC at end of data */
#define FAXMODE_NOEOL 0x0002         /* no EOL code at end of row */
#define FAXMODE_BYTEALIGN 0x0004     /* byte align row */
#define FAXMODE_WORDALIGN 0x0008     /* word align row */

#include "ccitt_fax_tables.hpp"


#if defined(MANGO_CPU_64BIT)
    #define ACCUMULATOR_BITS 64
    using AccumulatorType = u64;
#else
    #define ACCUMULATOR_BITS 32
    using AccumulatorType = u32;
#endif


static
void Fax3fillruns_bytepattern(u8 *buf, u32 *runs, u32 *erun, u32 lastx)
{
    if ((erun - runs) & 1)
        *erun++ = 0;
    u32 x = 0;
    for (; runs < erun && x < lastx; runs += 2)
    {
        u32 run = runs[0];
        if (x + run > lastx)
            run = lastx - x;
        if (run)
        {
            std::memset(buf, 0x00, run);
            buf += run;
            x += run;
        }
        run = runs[1];
        if (x + run > lastx)
            run = lastx - x;
        if (run)
        {
            std::memset(buf, 0xff, run);
            buf += run;
            x += run;
        }
    }
}

using FillRunsFunction = void (*)(u8 *buf, u32 *runs, u32 *erun, u32 lastx);

struct Fax3CodecState
{
    ConstMemory input;

    int mode;
    size_t rowbytes;
    u32 rowpixels;

    /* Decoder state info */
    AccumulatorType data;  /* bit accumulator */
    int bit;                  /* number of valid bits in data (0 .. ACCUMULATOR_BITS) */
    int EOLcnt;                  /* count of EOL codes recognized */
    int eofReachedCount;         /* number of times decode has been called with
                                    EOF already reached */
    int eolReachedCount;         /* number of times decode has been called with
                                    EOL already reached */
    int unexpectedReachedCount;  /* number of times decode has been called with
                                    "unexpedted" already reached */
    u32 *runs;              /* b&w runs for current/previous row */
    u32 nruns;              /* size of the refruns / curruns arrays */
    u32 *refruns;           /* runs for reference line */
    u32 *curruns;           /* runs for current line */

    int line;
};


#define unexpected(table, a0)                                                  \
    do                                                                         \
    {                                                                          \
        ++sp->unexpectedReachedCount;                                          \
    } while (0)


// TODO: rename
enum class Expand
{
    Success = 0,
    Error = -1,
    EndOfFile = -2,
    Retry = -3,
};

struct State
 {
    Fax3CodecState *sp;
    int a0;           // reference element
    int lastx;        // last element in row
    AccumulatorType BitAcc;  // bit accumulator
    int BitsAvail;               // number of valid bits in BitAcc (0 .. ACCUMULATOR_BITS)
    int RunLength;    // length of current run
    const u8 *cp;     // next byte of input data
    const u8 *ep;     // end of input data
    u32 *pa;          // place to stuff next run
    u32 *thisrun;     // current row's run array
    int EOLcnt;       // number of EOL codes recognized
    FaxTabEntry TabEnt;

    // 2D decoding state
    int b1; // next change on prev line
    u32 *pb; // next run in reference line

    FillRunsFunction fill = Fax3fillruns_bytepattern;

    State(Fax3CodecState* sp)
        : sp(sp)
    {
        lastx = sp->rowpixels;
    }

    void cache()
    {
        BitAcc = sp->data;
        BitsAvail = sp->bit;
        EOLcnt = sp->EOLcnt;
        cp = sp->input.address;
        ep = cp + sp->input.size;
    }

    void uncache()
    {
        sp->bit = BitsAvail;
        sp->data = BitAcc;
        sp->EOLcnt = EOLcnt;
        sp->input.size -= (cp - sp->input.address);
        sp->input.address = cp;
    }

    u32 getBits(int n) const
    {
        const AccumulatorType mask = (AccumulatorType(1) << n) - 1;
        return u32(BitAcc & mask);
    }

    void consumeBits(int n)
    {
        BitsAvail -= n;
        BitAcc >>= n;
    }

    bool ensureBits(int n)
    {
        while (BitsAvail < n)
        {
            if (cp >= ep)
            {
                // out of bytes to read
                if (BitsAvail == 0)
                    return false;
                BitsAvail = n; // pad with zeros
                return true;
            }

#if ACCUMULATOR_BITS == 64
            if (size_t(ep - cp) >= 8)
            {
                // read 8 bytes, merge only bytes that fit in the accumulator
                const int bytes_to_merge = (ACCUMULATOR_BITS - BitsAvail) / 8;
                u64 chunk = mango::littleEndian::uload64(cp);
                BitAcc |= AccumulatorType(chunk) << BitsAvail;
                BitsAvail += bytes_to_merge * 8;
                cp += bytes_to_merge;
                return true;
            }
#endif

            // read one byte
            BitAcc |= AccumulatorType(*cp++) << BitsAvail;
            BitsAvail += 8;
        }

        return true;
    }

    bool lookup8(int wid, const FaxTabEntry* tab)
    {
        if (!ensureBits(wid))
            return false;
        TabEnt = tab[getBits(wid)];
        consumeBits(TabEnt.nbits);
        return true;
    }

    bool lookup16(int wid, const FaxTabEntry* tab)
    {
        if (!ensureBits(wid))
            return false;
        TabEnt = tab[getBits(wid)];
        consumeBits(TabEnt.nbits);
        return true;
    }

    /*
    Append a run to the run length array for the
    current row and reset decoding state.
    */
    bool setValue(int x)
    {
        if (pa >= thisrun + sp->nruns)
        {
            return false;
        }
        *pa++ = RunLength + x;
        a0 += x;
        RunLength = 0;
        return true;
    }

    Expand check_b1()
    {
        if (pa != thisrun)
        {
            while (b1 <= a0 && b1 < lastx)
            {
                if (pb + 1 >= sp->refruns + sp->nruns)
                {
                    return Expand::Error;
                }
                b1 += pb[0] + pb[1];
                pb += 2;
            }
        }
        return Expand::Success;
    }

    /*
    Synchronize input decoding at the start of each
    row by scanning for an EOL (if appropriate) and
    skipping any trash data that might be present
    after a decoding error.  Note that the decoding
    done elsewhere that recognizes an EOL only consumes
    11 consecutive zero bits.  This means that if EOLcnt
    is non-zero then we still need to scan for the final flag
    bit that is part of the EOL code.
    */
    Expand sync_eol()
    {
        // skip EOL, if not present
        if (!(sp->mode & FAXMODE_NOEOL))
        {
            if (EOLcnt == 0)
            {
                for (;;)
                {
                    if (!ensureBits(11))
                    {
                        return Expand::EndOfFile;
                    }
                    if (getBits(11) == 0)
                        break; // EOL found
                    consumeBits(1);
                }
            }
        }

        // Now move after EOL or detect missing EOL.
        for (;;)
        {
            if (!ensureBits(8))
                goto noEOLFound;
            if (getBits(8))
                break;
            consumeBits(8);
        }

        while (getBits(1) == 0)
            consumeBits(1);
        consumeBits(1); // EOL bit
        EOLcnt = 0; // reset EOL counter/flag
        return Expand::Success;      // existing EOL skipped
    noEOLFound:
        sp->mode |= FAXMODE_NOEOL;
        return Expand::Retry;
    }

    /*
    Cleanup the array of runs after decoding a row.
    We adjust final runs to insure the user buffer is not
    overwritten and/or undecoded area is white filled.
    */
    Expand CLEANUP_RUNS()
    {
        if (RunLength)
        {
            if (!setValue(0))
                return Expand::Error;
        }

        if (a0 != lastx)
        {
            ++sp->eolReachedCount;
            while (a0 > lastx && pa > thisrun)
                a0 -= *--pa;
        }

        if (a0 < lastx)
        {
            if (a0 < 0)
                a0 = 0;
            if ((pa - thisrun) & 1)
            {
                if (!setValue(0))
                    return Expand::Error;
            }
            if (!setValue(lastx - a0))
                return Expand::Error;
        }
        else if (a0 > lastx)
        {
            if (!setValue(lastx))
                return Expand::Error;
            if (!setValue(0))
                return Expand::Error;
        }

        return Expand::Success;
    }

    Expand expand1D()
    {
        for (;;)
        {
            for (bool isWhiteDone = false; !isWhiteDone; )
            {
                if (!lookup16(12, g_FaxWhiteTable))
                {
                    goto eof1d;
                }
                switch (TabEnt.symbol)
                {
                    case S_EOL:
                        EOLcnt = 1;
                        goto done1d;
                    case S_TermW:
                        if (!setValue(TabEnt.run))
                            return Expand::Error;
                        isWhiteDone = true;
                        break;
                    case S_MakeUpW:
                    case S_MakeUp:
                        a0 += TabEnt.run;
                        RunLength += TabEnt.run;
                        break;
                    default:
                        unexpected("WhiteTable", a0);
                        goto done1d;
                }
            }

            if (a0 >= lastx)
                goto done1d;

            for (bool isBlackDone = false; !isBlackDone; )
            {
                if (!lookup16(13, g_FaxBlackTable))
                {
                    goto eof1d;
                }
                switch (TabEnt.symbol)
                {
                    case S_EOL:
                        EOLcnt = 1;
                        goto done1d;
                    case S_TermB:
                        if (!setValue(TabEnt.run))
                            return Expand::Error;
                        isBlackDone = true;
                        break;
                    case S_MakeUpB:
                    case S_MakeUp:
                        a0 += TabEnt.run;
                        RunLength += TabEnt.run;
                        break;
                    default:
                        unexpected("BlackTable", a0);
                        goto done1d;
                }
            }

            if (a0 >= lastx)
                goto done1d;

            if (*(pa - 1) == 0 && *(pa - 2) == 0)
                pa -= 2;
        }

    eof1d:
        ++sp->eofReachedCount;
        if (CLEANUP_RUNS() != Expand::Success)
            return Expand::Error;
        return Expand::EndOfFile;

    done1d:
        if (CLEANUP_RUNS() != Expand::Success)
            return Expand::Error;
        return Expand::Success;
    }

    Expand expand2D()
    {
        while (a0 < lastx)
        {
            if (pa >= thisrun + sp->nruns)
            {
                return Expand::Error;
            }

            if (!lookup8(7, g_FaxMainTable))
            {
                goto eof2d;
            }

            switch (TabEnt.symbol)
            {
                case S_Pass:
                    if (check_b1() == Expand::Error)
                    {
                        return Expand::Error;
                    }
                    if (pb + 1 >= sp->refruns + sp->nruns)
                    {
                        return Expand::Error;
                    }
                    b1 += *pb++;
                    RunLength += b1 - a0;
                    a0 = b1;
                    b1 += *pb++;
                    break;
                case S_Horiz:
                    if ((pa - thisrun) & 1)
                    {
                        for (bool isBlackDone = false; !isBlackDone; )
                        {
                            // black first
                            if (!lookup16(13, g_FaxBlackTable))
                            {
                                goto eof2d;
                            }
                            switch (TabEnt.symbol)
                            {
                                case S_TermB:
                                    if (!setValue(TabEnt.run))
                                        return Expand::Error;
                                    isBlackDone = true;
                                    break;
                                case S_MakeUpB:
                                case S_MakeUp:
                                    a0 += TabEnt.run;
                                    RunLength += TabEnt.run;
                                    break;
                                default:
                                    goto badBlack2d;
                            }
                        }

                        for (bool isWhiteDone = false; !isWhiteDone; )
                        {
                            // then white
                            if (!lookup16(12, g_FaxWhiteTable))
                            {
                                goto eof2d;
                            }
                            switch (TabEnt.symbol)
                            {
                                case S_TermW:
                                    if (!setValue(TabEnt.run))
                                        return Expand::Error;
                                    isWhiteDone = true;
                                    break;
                                case S_MakeUpW:
                                case S_MakeUp:
                                    a0 += TabEnt.run;
                                    RunLength += TabEnt.run;
                                    break;
                                default:
                                    goto badWhite2d;
                            }
                        }
                    }
                    else
                    {
                        for (bool isWhiteDone = false; !isWhiteDone; )
                        {
                            // white first
                            if (!lookup16(12, g_FaxWhiteTable))
                            {
                                goto eof2d;
                            }
                            switch (TabEnt.symbol)
                            {
                                case S_TermW:
                                    if (!setValue(TabEnt.run))
                                        return Expand::Error;
                                    isWhiteDone = true;
                                    break;
                                case S_MakeUpW:
                                case S_MakeUp:
                                    a0 += TabEnt.run;
                                    RunLength += TabEnt.run;
                                    break;
                                default:
                                    goto badWhite2d;
                            }
                        }

                        for (bool isBlackDone = false; !isBlackDone; )
                        {
                            // then black
                            if (!lookup16(13, g_FaxBlackTable))
                            {
                                goto eof2d;
                            }
                            switch (TabEnt.symbol)
                            {
                                case S_TermB:
                                    if (!setValue(TabEnt.run))
                                        return Expand::Error;
                                    isBlackDone = true;
                                    break;
                                case S_MakeUpB:
                                case S_MakeUp:
                                    a0 += TabEnt.run;
                                    RunLength += TabEnt.run;
                                    break;
                                default:
                                    goto badBlack2d;
                            }
                        }
                    }
                    if (check_b1() == Expand::Error)
                    {
                        return Expand::Error;
                    }
                    break;
                case S_V0:
                    if (check_b1() == Expand::Error)
                    {
                        return Expand::Error;
                    }
                    if (!setValue(b1 - a0))
                        return Expand::Error;
                    if (pb >= sp->refruns + sp->nruns)
                    {
                        return Expand::Error;
                    }
                    b1 += *pb++;
                    break;
                case S_VR:
                    if (check_b1() == Expand::Error)
                    {
                        return Expand::Error;
                    }
                    if (!setValue(b1 - a0 + TabEnt.run))
                        return Expand::Error;
                    if (pb >= sp->refruns + sp->nruns)
                    {
                        return Expand::Error;
                    }
                    b1 += *pb++;
                    break;
                case S_VL:
                    if (check_b1() == Expand::Error)
                    {
                        return Expand::Error;
                    }
                    if (b1 < (int)(a0 + TabEnt.run))
                    {
                        unexpected("VL", a0);
                        goto eol2d;
                    }
                    if (!setValue(b1 - a0 - TabEnt.run))
                        return Expand::Error;
                    b1 -= *--pb;
                    break;
                case S_Ext:
                    *pa++ = lastx - a0;
                    goto eol2d;
                case S_EOL:
                    *pa++ = lastx - a0;
                    if (!ensureBits(4))
                        goto eof2d;
                    if (getBits(4))
                        unexpected("EOL", a0);
                    consumeBits(4);
                    EOLcnt = 1;
                    goto eol2d;
                default:
                badMain2d:
                    unexpected("MainTable", a0);
                    goto eol2d;
                badBlack2d:
                    unexpected("BlackTable", a0);
                    goto eol2d;
                badWhite2d:
                    unexpected("WhiteTable", a0);
                    goto eol2d;
                eof2d:
                    ++sp->eofReachedCount;
                    if (CLEANUP_RUNS() != Expand::Success)
                        return Expand::Error;
                    return Expand::EndOfFile;
            }
        }

        if (RunLength)
        {
            if (RunLength + a0 < lastx)
            {
                // expect a final V0
                if (!ensureBits(1))
                    goto eof2d;
                if (!getBits(1))
                    goto badMain2d;
                consumeBits(1);
            }
            if (!setValue(0))
                return Expand::Error;
        }

    eol2d:
        if (CLEANUP_RUNS() != Expand::Success)
            return Expand::Error;
        return Expand::Success;
    }

    void Fax3PreDecode()
    {
        sp->bit = 0; // force initial read
        sp->data = 0;
        sp->EOLcnt = 0; // force initial scan for EOL
        sp->eofReachedCount = 0;
        sp->eolReachedCount = 0;
        sp->unexpectedReachedCount = 0;
        sp->curruns = sp->runs;
        if (sp->refruns)
        {
            // init reference line to white
            sp->refruns = sp->runs + sp->nruns;
            sp->refruns[0] = (u32)sp->rowpixels;
            sp->refruns[1] = 0;
        }
        sp->line = 0;
    }

    Expand Fax3DecodeRLE(Memory output)
    {
        int mode = sp->mode;

        if (output.size % sp->rowbytes)
        {
            return Expand::Error;
        }

        Expand result = Expand::Success;

        cache();
        thisrun = sp->curruns;

        while (output.size > 0)
        {
            a0 = 0;
            RunLength = 0;
            pa = thisrun;

            result = expand1D();
            switch (result)
            {
                case Expand::Success:
                case Expand::Retry:
                    break;
                case Expand::EndOfFile:
                    fill(output.address, thisrun, pa, lastx);
                    uncache();
                    [[fallthrough]];
                case Expand::Error:
                    return Expand::Error;
            }

            fill(output.address, thisrun, pa, lastx);

            // Cleanup at the end of the row.
            if (mode & FAXMODE_BYTEALIGN)
            {
                int n = BitsAvail & 7;
                consumeBits(n);
            }
            else if (mode & FAXMODE_WORDALIGN)
            {
                int n = BitsAvail & 15;
                consumeBits(n);
                if (BitsAvail == 0 && !is_aligned(cp, 2))
                    cp++;
            }

            output.address += sp->rowbytes;
            output.size -= sp->rowbytes;
            sp->line++;
        }

        uncache();
        return Expand::Success;
    }

    Expand Fax3Decode1D(Memory output)
    {
        if (output.size % sp->rowbytes)
        {
            return Expand::Error;
        }

        Expand result = Expand::Success;

      RETRY_WITHOUT_EOL_1D:
        cache();
        thisrun = sp->curruns;
        while (output.size > 0)
        {
            a0 = 0;
            RunLength = 0;
            pa = thisrun;
            Expand result = sync_eol();
            switch (result)
            {
                case Expand::Success:
                    break;
                case Expand::EndOfFile:
                    goto EOF1D;
                case Expand::Retry:
                    goto RETRY_WITHOUT_EOL_1D;
                case Expand::Error:
                    return Expand::Error;
            }

            result = expand1D();
            switch (result)
            {
                case Expand::Success:
                case Expand::Retry:
                    break;
                case Expand::EndOfFile:
                    fill(output.address, thisrun, pa, lastx);
                    uncache();
                    [[fallthrough]];
                case Expand::Error:
                    return Expand::Error;
            }

            fill(output.address, thisrun, pa, lastx);

            output.address += sp->rowbytes;
            output.size -= sp->rowbytes;
            sp->line++;
        }

        uncache();
        return Expand::Success;

    EOF1D:
        // premature EOF (only reached via goto from SYNC_EOL)
        if (CLEANUP_RUNS() != Expand::Success)
            return Expand::Error;
        fill(output.address, thisrun, pa, lastx);
        uncache();
        return Expand::Error;
    }

    Expand Fax3Decode2D(Memory output)
    {
        int is1D; // current line is 1d/2d-encoded

        if (output.size % sp->rowbytes)
        {
            return Expand::Error;
        }

        Expand result = Expand::Success;

    RETRY_WITHOUT_EOL_2D:
        cache();
        while (output.size > 0)
        {
            a0 = 0;
            RunLength = 0;
            pa = thisrun = sp->curruns;

            Expand result = sync_eol();
            switch (result)
            {
                case Expand::Success:
                    break;
                case Expand::EndOfFile:
                    goto EOF2D;
                case Expand::Retry:
                    goto RETRY_WITHOUT_EOL_2D;
                case Expand::Error:
                    return Expand::Error;
            }

            if (!ensureBits(1))
                goto EOF2D;
            is1D = getBits(1); // 1D/2D-encoding tag bit
            consumeBits(1);

            pb = sp->refruns;
            b1 = *pb++;

            if (is1D)
            {
                result = expand1D();
            }
            else
            {
                result = expand2D();
            }

            switch (result)
            {
                case Expand::Success:
                case Expand::Retry:
                    break;
                case Expand::EndOfFile:
                    fill(output.address, thisrun, pa, lastx);
                    uncache();
                    [[fallthrough]];
                case Expand::Error:
                    return Expand::Error;
            }

            fill(output.address, thisrun, pa, lastx);
            if (pa < thisrun + sp->nruns)
            {
                // imaginary change for reference
                if (!setValue(0))
                    return Expand::Error;
            }
            std::swap(sp->curruns, sp->refruns);

            output.address += sp->rowbytes;
            output.size -= sp->rowbytes;
            sp->line++;
        }

        uncache();
        return Expand::Success;

    EOF2D:
        // premature EOF
        if (CLEANUP_RUNS() != Expand::Success)
            return Expand::Error;
        fill(output.address, thisrun, pa, lastx);
        uncache();
        return Expand::Error;
    }

    Expand Fax4Decode(Memory output)
    {
        if (output.size % sp->rowbytes)
        {
            return Expand::Error;
        }

        Expand result = Expand::Success;

        cache();
        int start = sp->line;

        while (output.size > 0)
        {
            a0 = 0;
            RunLength = 0;
            pa = thisrun = sp->curruns;
            pb = sp->refruns;
            b1 = *pb++;

            result = expand2D();
            switch (result)
            {
                case Expand::Success:
                case Expand::Retry:
                    break;
                case Expand::Error:
                    return Expand::Error;
                case Expand::EndOfFile:
                    goto EOFG4;
            }

            if (EOLcnt)
                goto EOFG4;

            if (((lastx + 7) >> 3) > (int)output.size) // check for buffer overrun
            {
                return Expand::Error;
            }
            fill(output.address, thisrun, pa, lastx);

            // imaginary change for reference
            if (!setValue(0))
                return Expand::Error;
            std::swap(sp->curruns, sp->refruns);

            output.address += sp->rowbytes;
            output.size -= sp->rowbytes;
            sp->line++;

            continue;

        EOFG4:
            if (!ensureBits(13))
            {
                goto BADG4;
            }
        BADG4:
            consumeBits(13);
            if (((lastx + 7) >> 3) > (int)output.size) // check for buffer overrun
            {
                return Expand::Error;
            }
            fill(output.address, thisrun, pa, lastx);

            uncache();

            // don't error on badly-terminated strips
            return (sp->line != start
                        ? Expand::Success
                        : Expand::Error);
        }

        uncache();
        return Expand::Success;
    }
};

bool ccitt_rle_decompress(Memory output, ConstMemory input, u32 width, u32 height, bool word_aligned)
{
    Fax3CodecState sp;
    std::memset(&sp, 0, sizeof(sp));
    sp.input = input;

    u32 rowpixels = width;
    size_t rowbytes = width;//(width + 7) / 8;

    sp.rowpixels = rowpixels;
    sp.rowbytes = rowbytes;

    sp.nruns = round_ceil(rowpixels + 1, 32);

    std::vector<u32> temp_runs(sp.nruns * 2, 0);

    sp.runs = temp_runs.data();
    sp.curruns = sp.runs;
    sp.refruns = nullptr;

    // TODO: resolve mode from group3_options
    sp.mode = word_aligned ? FAXMODE_NORTC | FAXMODE_NOEOL | FAXMODE_WORDALIGN
                           : FAXMODE_NORTC | FAXMODE_NOEOL | FAXMODE_BYTEALIGN;

    State state(&sp);
    state.Fax3PreDecode();
    auto result = state.Fax3DecodeRLE(output);
    if (result != Expand::Success)
    {
        return false;
    }

    return true;
}

bool ccitt_group3_decompress(Memory output, ConstMemory input, u32 width, u32 height, bool is_2d)
{
    Fax3CodecState sp;
    std::memset(&sp, 0, sizeof(sp));
    sp.input = input;

    u32 rowpixels = width;
    size_t rowbytes = width;//(width + 7) / 8;

    sp.rowpixels = rowpixels;
    sp.rowbytes = rowbytes;

    int needsRefLine = 1;

    sp.nruns = round_ceil(rowpixels + 1, 32);
    if (needsRefLine)
        sp.nruns = sp.nruns * 2;

    std::vector<u32> temp_runs(sp.nruns * 2, 0);

    sp.runs = temp_runs.data();
    sp.curruns = sp.runs;
    sp.refruns = (needsRefLine) ? (sp.runs + sp.nruns) : nullptr;

    // TODO: resolve mode from group3_options
    sp.mode = FAXMODE_NORTC |FAXMODE_BYTEALIGN;

    State state(&sp);
    state.Fax3PreDecode();

    auto result = is_2d ? state.Fax3Decode2D(output) : state.Fax3Decode1D(output);
    if (result != Expand::Success)
    {
        return false;
    }

    return true;
}

bool ccitt_group4_decompress(Memory output, ConstMemory input, u32 width, u32 height)
{
    Fax3CodecState sp;
    std::memset(&sp, 0, sizeof(sp));
    sp.input = input;

    u32 rowpixels = width;
    size_t rowbytes = width;//(width + 7) / 8;

    sp.rowpixels = rowpixels;
    sp.rowbytes = rowbytes;

    int needsRefLine = 1;

    sp.nruns = round_ceil(rowpixels + 1, 32);
    if (needsRefLine)
        sp.nruns = sp.nruns * 2;

    std::vector<u32> temp_runs(sp.nruns * 2, 0);

    sp.runs = temp_runs.data();
    sp.curruns = sp.runs;
    sp.refruns = (needsRefLine) ? (sp.runs + sp.nruns) : nullptr;

    // TODO: resolve mode from group4_options
    sp.mode = FAXMODE_NORTC | FAXMODE_NOEOL | FAXMODE_BYTEALIGN;

    State state(&sp);
    state.Fax3PreDecode();
    auto result = state.Fax4Decode(output);
    if (result != Expand::Success)
    {
        return false;
    }

    return true;
}
