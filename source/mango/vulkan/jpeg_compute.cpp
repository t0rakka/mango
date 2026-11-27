/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "jpeg_compute.hpp"

#include <algorithm>
#include <cstring>
#include <future>
#include <iterator>
#include <limits>
#include <memory>
#include <vector>

#include <mango/core/bits.hpp>
#include <mango/core/thread.hpp>
#include <mango/core/timer.hpp>
#include <mango/vulkan/compiler.hpp>

namespace mango::vulkan::detail
{

namespace
{

constexpr int kLb = 10;
constexpr int kLut = 1 << kLb;
constexpr int kNcp = 1;
constexpr int kPieces = kNcp + 1;
constexpr u32 kChunkBits = 1024;
constexpr u32 kPieceBits = kChunkBits / kPieces;
constexpr int kMaxPasses = 8;
constexpr int kSyncWg = 128;
constexpr int kWriteWg = 128;
constexpr int kDcTile = 1024; // MCUs per DC scan workgroup: 256 threads x 4
constexpr int kQueries = 2;
// flags[0..kMaxPasses) + VkDispatchIndirectCommand (3 x uint)
constexpr VkDeviceSize kFlagsBytes = VkDeviceSize(kMaxPasses + 3) * 4;
constexpr VkDeviceSize kIndirectOffset = VkDeviceSize(kMaxPasses) * 4;

enum Binding : u32
{
    BindWords = 0,
    BindChunks = 1,
    BindSegs = 2,
    BindTables = 3,
    BindStIn = 4,
    BindStOut = 5,
    BindCnt = 6,
    BindCps = 7,
    BindCpc = 8,
    BindFirst = 9,
    BindCoef = 10,
    BindDcd = 11,
    BindQt = 12,
    BindFlags = 13,
    BindImage = 14,
    BindSegStart = 15,
    BindAgg = 16,
};

struct ChunkGpu
{
    u32 bit0;
    u32 bit1;
    u32 seg0;
    u32 seg1;
    s32 block0;
    s32 block1;
};

struct SegGpu
{
    s32 chunk0;
    s32 nchunks;
    s32 block0;
    s32 mcu0;
    s32 mcus;
};

// 16-bit LUT entries two per word, Huffman values four per word: the whole
// thing fits in shared memory next to the sync state.
struct TablesGpu
{
    s32 maxc[8 * 18];
    s32 voff[8 * 18];
    u32 hv[8 * 256 / 4];
    u32 lut[8 * kLut / 2];
};

struct TablesCacheEntry
{
    TablesGpu tables {};
    s32 qt[4 * 64] {};
    u32 dc_pack = 0;
    u32 ac_pack = 0;
    u32 comp_pack = 0;
    int ntables = 0;
};

static_assert(sizeof(JpegComputePush) <= 128, "push constants");

u64 mixHash(u64 h, u64 v)
{
    h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
    return h;
}

u64 hashBytes(u64 h, const void* data, size_t bytes)
{
    const u8* p = static_cast<const u8*>(data);
    for (size_t i = 0; i < bytes; ++i)
        h = mixHash(h, p[i]);
    return h;
}

u64 pipelineSetKey(int ntables, const JpegComputePush& push)
{
    u64 h = mixHash(0xC0FFEEu, u64(ntables));
    h = mixHash(h, u64(push.bpm));
    h = mixHash(h, u64(push.xblock) | (u64(push.yblock) << 16) | (u64(push.components) << 32));
    h = mixHash(h, u64(push.mcus_per_group));
    h = mixHash(h, u64(push.y_hsf) | (u64(push.y_vsf) << 8) | (u64(push.y_off) << 16));
    h = mixHash(h, u64(push.cb_hsf) | (u64(push.cb_vsf) << 8) | (u64(push.cb_off) << 16));
    h = mixHash(h, u64(push.cr_hsf) | (u64(push.cr_vsf) << 8) | (u64(push.cr_off) << 16));
    return h;
}

constexpr char kCommon[] = R"glsl(
#version 450
layout(std430, binding = 0) readonly buffer Words { uint words[]; };
struct Chunk { uint bit0; uint bit1; uint seg0; uint seg1; int block0; int block1; };
layout(std430, binding = 1) readonly buffer Chunks { Chunk chunks[]; };
struct Seg { int chunk0; int nchunks; int block0; int mcu0; int mcus; };
layout(std430, binding = 2) readonly buffer Segs { Seg segs[]; };
layout(std430, binding = 3) readonly buffer Tables { int tmaxc[144]; int tvoff[144]; uint thv[512]; uint tlut[]; };
layout(std430, binding = 4) buffer StIn { uvec2 stIn[]; };
layout(std430, binding = 5) buffer StOut { uvec2 stOut[]; };
layout(std430, binding = 6) buffer Cnt { int cnt[]; };
layout(std430, binding = 7) buffer Cps { uvec2 cps[]; };
layout(std430, binding = 8) buffer Cpc { int cpc[]; };
layout(std430, binding = 9) buffer First { int firstb[]; };
layout(std430, binding = 10) buffer Coef { uint coef[]; };
layout(std430, binding = 11) buffer Dcd { int dcd[]; };
layout(std430, binding = 12) readonly buffer Qt { int qt[256]; };
layout(std430, binding = 13) buffer Flags { uint flags[]; };
layout(std430, binding = 15) readonly buffer SegStart { uint segStart[]; };
layout(std430, binding = 16) buffer Agg { ivec4 agg[]; };

layout(push_constant) uniform Push {
    int nch;
    int pass;
    int rounds;
    int bpm;
    uint dcPack;
    uint acPack;
    uint compPack;
    uint nwords;
    int width, height, xmcu, xblock, yblock, components;
    int y_hsf, y_vsf, y_qt, y_off;
    int cb_hsf, cb_vsf, cb_qt, cb_off;
    int cr_hsf, cr_vsf, cr_qt, cr_off;
    int mcusPerGroup;
    int nmcu;
} pc;
)glsl";

constexpr char kHuffman[] = R"glsl(
shared uint slut[NT << (LB - 1)];
shared int smaxc[NT * 18];
shared int svoff[NT * 18];
shared uint shv[NT * 64];

void loadTabs()
{
    uint t = gl_LocalInvocationIndex;
    uint n = uint(HUFF_WG);
    for (uint i = t; i < uint(NT << (LB - 1)); i += n)
        slut[i] = tlut[i];
    for (uint i = t; i < uint(NT * 18); i += n)
    {
        smaxc[i] = tmaxc[i];
        svoff[i] = tvoff[i];
    }
    for (uint i = t; i < uint(NT * 64); i += n)
        shv[i] = thv[i];
    barrier();
}

struct Rd { uint wi; uint a; uint b; };

// Scan bytes are uploaded in file order; the bit reader wants big-endian words.
uint wordAt(uint i)
{
    uint w = i < pc.nwords ? words[i] : 0u;
    w = ((w & 0x00ff00ffu) << 8) | ((w >> 8) & 0x00ff00ffu);
    return (w << 16) | (w >> 16);
}

void seek(inout Rd r, uint pos)
{
    r.wi = pos >> 5;
    r.a = wordAt(r.wi);
    r.b = wordAt(r.wi + 1u);
}

uint peek(inout Rd r, uint pos)
{
    uint at = pos >> 5;
    while (r.wi != at)
    {
        r.a = r.b;
        r.wi++;
        r.b = wordAt(r.wi + 1u);
    }
    uint n = pos & 31u;
    return n == 0u ? r.a : ((r.a << n) | (r.b >> (32u - n)));
}

int symEntry(int sym, int len, bool dc)
{
    int s = sym & 15;
    int r = sym >> 4;
    int za = dc ? 1 : (s != 0 ? r + 1 : (r == 15 ? 16 : 64));
    return (len + s) | (s << 5) | (za << 9);
}

int hent(int ti, uint bits, bool dc)
{
    int idx = (ti << LB) | int(bits >> (32 - LB));
    uint w = slut[idx >> 1];
    int e = int((idx & 1) != 0 ? (w >> 16) : (w & 0xffffu));
    if (e != 0)
        return e;
    int l = LB + 1;
    int code = int(bits >> (31 - LB));
    while (l <= 16 && code > smaxc[ti * 18 + l])
    {
        ++l;
        code = int(bits >> uint(32 - l));
    }
    if (l > 16)
        return 16 | ((dc ? 1 : 64) << 9);
    int k = ti * 256 + code + svoff[ti * 18 + l];
    int sym = int((shv[k >> 2] >> (uint(k & 3) * 8u)) & 0xffu);
    return symEntry(sym, l, dc);
}

int tableOf(int c, bool dc)
{
    return int(((dc ? pc.dcPack : pc.acPack) >> (3u * uint(c))) & 7u);
}

void stepSkip(inout Rd r, inout uint pos, int c, inout int z)
{
    uint bits = peek(r, pos);
    bool dc = z == 0;
    int h = hent(tableOf(c, dc), bits, dc);
    pos += uint(h & 31);
    z += (h >> 9) & 127;
}

int nextBlock(int c)
{
    return c + 1 == pc.bpm ? 0 : c + 1;
}

uvec2 pack(uint pos, int c, int z)
{
    return uvec2(pos, uint((c << 8) | z));
}
)glsl";

constexpr char kSyncMain[] = R"glsl(
layout(local_size_x = SYNC_WG) in;

shared uvec2 so[SYNC_WG];
shared uint anyCh;

// Walk from (pos, c, z) to end, recording the state at the first symbol
// boundary past each PIECE_BITS mark. With cmp, stop at the first mark whose
// state matches this chunk's previous walk and shift the later block counts.
bool runChunk(inout Rd r, inout uint pos, inout int c, inout int z, uint base, uint end,
              uint cpBase, inout int n, bool cmp)
{
    int done = 0;
    for (int j = 0; j <= NCP; ++j)
    {
        uint mark = j < NCP ? min(base + uint((j + 1) * PIECE_BITS), end) : end;
        while (pos < mark)
        {
            stepSkip(r, pos, c, z);
            if (z >= 64)
            {
                z = 0;
                c = nextBlock(c);
                ++done;
            }
        }
        if (j == NCP)
            break;
        uvec2 st = pack(pos, c, z);
        uint at = cpBase + uint(j);
        if (cmp && st == cps[at])
        {
            int shift = done - cpc[at];
            for (int k = j; k < NCP; ++k)
                cpc[cpBase + uint(k)] += shift;
            n += shift;
            return true;
        }
        cps[at] = st;
        cpc[at] = done;
    }
    n = done;
    return false;
}

void main()
{
    if (pc.pass > 0 && flags[pc.pass - 1] == 0u)
        return;
    loadTabs();

    int tid = int(gl_LocalInvocationID.x);
    int i = int(gl_WorkGroupID.x) * SYNC_WG + tid;
    bool valid = i < pc.nch;
    bool known = !valid || chunks[i].bit0 == chunks[i].seg0;
    uint cpBase = uint(max(i, 0)) * uint(NCP);

    Rd r;
    uvec2 myIn = uvec2(0u);
    uvec2 myOut = uvec2(0u);
    int n = 0;
    if (valid)
    {
        if (pc.pass == 0)
        {
            uint pos = chunks[i].bit0;
            int c = 0;
            int z = 0;
            myIn = pack(pos, 0, 0);
            seek(r, pos);
            runChunk(r, pos, c, z, chunks[i].bit0, chunks[i].bit1, cpBase, n, false);
            myOut = pack(pos, c, z);
        }
        else
        {
            myIn = stIn[i];
            myOut = stOut[i];
            n = cnt[i];
        }
    }
    uvec2 out0 = myOut;
    uvec2 globPrev = uvec2(0u);
    if (pc.pass > 0 && valid && tid == 0 && !known)
        globPrev = stOut[i - 1];
    so[tid] = myOut;
    if (tid == 0)
        anyCh = 0u;
    barrier();

    bool more = false;
    for (int it = 0; it < pc.rounds; ++it)
    {
        bool ch = false;
        if (valid && !known && (tid > 0 || pc.pass > 0))
        {
            uvec2 want = tid == 0 ? globPrev : so[tid - 1];
            if (want != myIn)
            {
                myIn = want;
                uint pos = want.x;
                int c = int((want.y >> 8) & 255u);
                int z = int(want.y & 255u);
                seek(r, pos);
                bool same = runChunk(r, pos, c, z, chunks[i].bit0, chunks[i].bit1, cpBase, n, true);
                uvec2 e = same ? myOut : pack(pos, c, z);
                if (e != myOut)
                {
                    myOut = e;
                    ch = true;
                }
            }
        }
        barrier();
        so[tid] = myOut;
        if (ch)
            anyCh = 1u;
        barrier();
        more = anyCh != 0u;
        barrier();
        if (tid == 0)
            anyCh = 0u;
        if (!more)
            break;
    }
    if (valid)
    {
        stIn[i] = myIn;
        stOut[i] = myOut;
        cnt[i] = n;
    }
    int last = min(SYNC_WG, pc.nch - int(gl_WorkGroupID.x) * SYNC_WG) - 1;
    if (tid == last && (myOut != out0 || more || (pc.pass == 0 && gl_NumWorkGroups.x > 1u)))
        flags[pc.pass] = 1u;
}
)glsl";

// One thread: turn flags[pass] into VkDispatchIndirectCommand at flags[8..10].
// Empty groupCountX skips the next sync when the previous pass converged.
constexpr char kSyncGateMain[] = R"glsl(
layout(local_size_x = 1) in;
void main()
{
    const uint base = 8u;
    uint groups = 0u;
    if (flags[uint(pc.pass)] != 0u)
    {
        uint wg = uint(max(pc.rounds, 1));
        groups = (uint(pc.nch) + wg - 1u) / wg;
    }
    flags[base + 0u] = groups;
    flags[base + 1u] = 1u;
    flags[base + 2u] = 1u;
}
)glsl";

constexpr char kPrefixMain[] = R"glsl(
layout(local_size_x = 256) in;

// Each lane owns PK consecutive chunks, so a whole-file segment (one serial
// scan) takes a handful of tiles instead of one per 256 chunks.
const int PK = 16;

shared int sh[256];
shared int run;

void main()
{
    Seg s = segs[gl_WorkGroupID.x];
    uint t = gl_LocalInvocationID.x;
    if (t == 0u)
        run = 0;
    barrier();
    for (int tile = 0; tile < s.nchunks; tile += 256 * PK)
    {
        int c0 = tile + int(t) * PK;
        int v[PK];
        int sum = 0;
        for (int k = 0; k < PK; ++k)
        {
            v[k] = c0 + k < s.nchunks ? cnt[s.chunk0 + c0 + k] : 0;
            sum += v[k];
        }
        sh[t] = sum;
        barrier();
        for (uint off = 1u; off < 256u; off <<= 1u)
        {
            int x = sh[t];
            if (t >= off)
                x += sh[t - off];
            barrier();
            sh[t] = x;
            barrier();
        }
        int b = s.block0 + run + sh[t] - sum;
        for (int k = 0; k < PK; ++k)
        {
            if (c0 + k < s.nchunks)
                firstb[s.chunk0 + c0 + k] = b;
            b += v[k];
        }
        barrier();
        if (t == 0u)
            run += sh[255];
        barrier();
    }
}
)glsl";

constexpr char kWriteMain[] = R"glsl(
layout(local_size_x = WRITE_WG) in;

// A block belongs to the piece its DC symbol starts in: a piece first skips
// the rest of a block begun before it, and finishes its own last block past
// its end. AC values go out two per word in zigzag order, every word up to the
// last nonzero one; word 0's low half (the DC slot) holds how many there are.
void main()
{
    loadTabs();
    int gi = int(gl_GlobalInvocationID.x);
    int i = gi / (NCP + 1);
    int piece = gi - i * (NCP + 1);
    if (i >= pc.nch)
        return;
    Chunk ch = chunks[i];
    uvec2 s = piece == 0 ? stIn[i] : cps[uint(i * NCP + piece - 1)];
    uint pos = s.x;
    int bc = int((s.y >> 8) & 255u);
    int z = int(s.y & 255u);
    int B = firstb[i] + (piece == 0 ? 0 : cpc[i * NCP + piece - 1]);
    uint mark = piece < NCP ? min(ch.bit0 + uint((piece + 1) * PIECE_BITS), ch.bit1) : ch.bit1;

    // One loop per symbol, block ends handled inline: a per-block inner loop
    // holds every lane of the subgroup until its longest block is done.
    bool skip = z != 0;
    if (!skip && !(pos < mark && B < ch.block1))
        return;
    Rd r;
    seek(r, pos);
    uint base = uint(B) * 32u;
    int dcv = 0;
    uint pend = 0u;
    uint w0 = 0u;
    int pq = 0;
    for (;;)
    {
        uint bits = peek(r, pos);
        bool dc = z == 0;
        int h = hent(tableOf(bc, dc), bits, dc);
        int tot = h & 31;
        int sz = (h >> 5) & 15;
        int za = (h >> 9) & 127;
        if (sz != 0 && !skip)
        {
            int v = int((bits << uint(tot - sz)) >> uint(32 - sz));
            if (v < (1 << (sz - 1)))
                v = v - (1 << sz) + 1;
            int zi = z + za - 1;
            if (dc)
                dcv = v;
            else if (zi < 64)
            {
                int q = zi >> 1;
                if (q != pq)
                {
                    if (pq == 0)
                        w0 = pend;
                    else
                        coef[base + uint(pq)] = pend;
                    for (int k = pq + 1; k < q; ++k)
                        coef[base + uint(k)] = 0u;
                    pend = 0u;
                    pq = q;
                }
                pend |= (uint(v) & 0xffffu) << (uint(zi & 1) * 16u);
            }
        }
        pos += uint(tot);
        z += za;
        if (z >= 64)
        {
            if (!skip)
            {
                if (pq == 0)
                    w0 = pend;
                else
                    coef[base + uint(pq)] = pend;
                coef[base] = (w0 & 0xffff0000u) | uint(pq + 1);
                dcd[B] = dcv;
            }
            skip = false;
            z = 0;
            bc = nextBlock(bc);
            ++B;
            if (!(pos < mark && B < ch.block1))
                break;
            base = uint(B) * 32u;
            dcv = 0;
            pend = 0u;
            w0 = 0u;
            pq = 0;
        }
    }
}
)glsl";

constexpr char kDcScan[] = R"glsl(
layout(local_size_x = 256) in;

// Segmented scan element: x = a segment starts inside, yzw = DC sums per
// component after the last start. a comes before b.
ivec4 combine(ivec4 a, ivec4 b)
{
    return b.x != 0 ? b : ivec4(a.x, a.yzw + b.yzw);
}

bool isStart(int m)
{
    return ((segStart[m >> 5] >> uint(m & 31)) & 1u) != 0u;
}

int compOf(int k)
{
    return int((pc.compPack >> (2u * uint(k))) & 3u);
}

shared ivec4 sa[256];

ivec4 scan256(ivec4 v, uint t)
{
    sa[t] = v;
    barrier();
    for (uint off = 1u; off < 256u; off <<= 1u)
    {
        ivec4 x = sa[t];
        if (t >= off)
            x = combine(sa[t - off], x);
        barrier();
        sa[t] = x;
        barrier();
    }
    return sa[t];
}
)glsl";

// pass 0: per-tile totals into agg[group]. pass 1: absolute DC values.
constexpr char kDcLocalMain[] = R"glsl(
void main()
{
    uint t = gl_LocalInvocationID.x;
    int g = int(gl_WorkGroupID.x);
    int m0 = g * 1024 + int(t) * 4;
    int m1 = min(m0 + 4, pc.nmcu);
    ivec4 acc = ivec4(0);
    for (int m = m0; m < m1; ++m)
    {
        if (isStart(m))
            acc = ivec4(1, 0, 0, 0);
        int p = m * pc.bpm;
        for (int k = 0; k < pc.bpm; ++k)
        {
            int ci = compOf(k);
            int v = dcd[p + k];
            if (ci == 0) acc.y += v;
            else if (ci == 1) acc.z += v;
            else acc.w += v;
        }
    }
    ivec4 incl = scan256(acc, t);
    if (pc.pass == 0)
    {
        if (t == 255u)
            agg[g] = incl;
        return;
    }
    ivec4 excl = t == 0u ? ivec4(0) : sa[t - 1u];
    ivec4 run = combine(agg[int(gl_NumWorkGroups.x) + g], excl);
    for (int m = m0; m < m1; ++m)
    {
        if (isStart(m))
            run = ivec4(1, 0, 0, 0);
        int p = m * pc.bpm;
        for (int k = 0; k < pc.bpm; ++k)
        {
            int ci = compOf(k);
            int v = dcd[p + k];
            if (ci == 0) { run.y += v; dcd[p + k] = run.y; }
            else if (ci == 1) { run.z += v; dcd[p + k] = run.z; }
            else { run.w += v; dcd[p + k] = run.w; }
        }
    }
}
)glsl";

// One workgroup: exclusive prefix of the tile totals into agg[ngroups + g].
constexpr char kDcGroupsMain[] = R"glsl(
shared ivec4 carry;

void main()
{
    uint t = gl_LocalInvocationID.x;
    int ngroups = pc.nch;
    if (t == 0u)
        carry = ivec4(0);
    barrier();
    for (int tile = 0; tile < ngroups; tile += 256)
    {
        int g = tile + int(t);
        ivec4 v = g < ngroups ? agg[g] : ivec4(0);
        ivec4 incl = scan256(v, t);
        ivec4 excl = t == 0u ? ivec4(0) : sa[t - 1u];
        if (g < ngroups)
            agg[ngroups + g] = combine(carry, excl);
        barrier();
        if (t == 0u)
            carry = combine(carry, sa[255]);
        barrier();
    }
}
)glsl";

constexpr char kIdctMain[] = R"glsl(
layout(local_size_x = 256) in;
layout(binding = 14, rgba8) uniform writeonly image2D uOut;

const int zigzag[64] = int[](
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
);

layout(std430, binding = 10) readonly buffer Coef4 { uvec4 coef4[]; };

// 32 block slots of 8 lanes. nat holds dezigzag'd coeffs, then the finished
// samples. The frame layout (BPM, XB, YB, *_H, *_V, *_OFF, NCOMP, MPG)
// is compiled in so the per-pixel sampling math is shifts and constants.
// Rows are 9 words and blocks 72, so the 8 lanes of a block reading a column
// or writing a row, and the 4 blocks of a 32-wide subgroup, hit distinct banks.
// Both IDCT passes work in place (read to registers, barrier, write back):
// a second array costs more in occupancy than the extra barriers.
const int RS = 9;
const int BS = 8 * RS;
shared int nat[32 * BS];
shared ivec2 origin[MPG];

void idct_pass(inout int x0, inout int x1, inout int x2, inout int x3,
               inout int y0, inout int y1, inout int y2, inout int y3,
               int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7)
{
    int n0 = (s2 + s6) * 2217;
    int t2 = n0 + s6 * (-7567);
    int t3 = n0 + s2 * 3135;
    int t0 = (s0 + s4) << 12;
    int t1 = (s0 - s4) << 12;
    x0 = t0 + t3;
    x3 = t0 - t3;
    x1 = t1 + t2;
    x2 = t1 - t2;

    int p1 = s7 + s1;
    int p2 = s5 + s3;
    int p3 = s7 + s3;
    int p4 = s5 + s1;
    int p5 = (p3 + p4) * 4816;
    p1 = p1 * (-3685) + p5;
    p2 = p2 * (-10497) + p5;
    p3 = p3 * (-8034);
    p4 = p4 * (-1597);
    y0 = p1 + p3 + s7 * 1223;
    y1 = p2 + p4 + s5 * 8410;
    y2 = p2 + p3 + s3 * 12586;
    y3 = p1 + p4 + s1 * 6149;
}

int clamp_u8(int v)
{
    return clamp(v, 0, 255);
}

int qt_index_of(int b)
{
    if (b >= Y_OFF && b < Y_OFF + Y_H * Y_V)
        return pc.y_qt;
#if NCOMP > 1
    if (b >= CB_OFF && b < CB_OFF + CB_H * CB_V)
        return pc.cb_qt;
#endif
#if NCOMP > 2
    if (b >= CR_OFF && b < CR_OFF + CR_H * CR_V)
        return pc.cr_qt;
#endif
    return 0;
}

int sample_at(int mcu_local, int first_block, int hsf, int vsf, int lx, int ly)
{
    int block_w = XB / hsf;
    int block_h = YB / vsf;
    int bx = lx / block_w;
    int by = ly / block_h;
    int spx = ((lx % block_w) * 8) / block_w;
    int spy = ((ly % block_h) * 8) / block_h;
    int block = mcu_local * BPM + first_block + by * hsf + bx;
    return nat[block * BS + spy * RS + spx];
}

void main()
{
    int lane = int(gl_LocalInvocationIndex);
    int slot = lane >> 3;
    int col = lane & 7;
    int group0 = int(gl_WorkGroupID.x) * MPG;
    const int used = MPG * BPM;

    int mcu_local = slot / BPM;
    int b = slot - mcu_local * BPM;
    bool live = slot < used && (group0 + mcu_local) < pc.nmcu;
    int dst = slot * BS;
    int blk = (group0 + mcu_local) * BPM + b;
    int qbase = qt_index_of(b) * 64;

    if (lane < MPG)
    {
        int m = group0 + lane;
        origin[lane] = ivec2((m % pc.xmcu) * XB, (m / pc.xmcu) * YB);
    }

    // Lane col owns zigzag positions col*8 .. col*8+7, i.e. words col*4 .. col*4+3.
    bool dcOnly = false;
    if (live)
    {
        uint w0 = coef[uint(blk) * 32u];
        uint nw = w0 & 0xffffu;
        int dc = dcd[blk];
        dcOnly = nw == 1u && (w0 >> 16) == 0u;
        if (dcOnly)
        {
            int v = clamp_u8((((dc * qt[qbase]) << 2) * 4096 + 0x10000 + (128 << 17)) >> 17);
            for (int k = 0; k < 8; ++k)
                nat[dst + col * RS + k] = v;
        }
        else
        {
            uint w4 = uint(col * 4);
            uvec4 w = w4 < nw ? coef4[uint(blk) * 8u + uint(col)] : uvec4(0u);
            w = mix(uvec4(0u), w, lessThan(uvec4(w4) + uvec4(0u, 1u, 2u, 3u), uvec4(nw)));
            for (int k = 0; k < 4; ++k)
            {
                int i = col * 8 + k * 2;
                int z0 = zigzag[i];
                int z1 = zigzag[i + 1];
                nat[dst + z0 + (z0 >> 3)] = i == 0 ? dc : (int(w[k] << 16) >> 16);
                nat[dst + z1 + (z1 >> 3)] = int(w[k]) >> 16;
            }
        }
    }
    barrier();

    bool idct = live && !dcOnly;
    int n0 = 0, n1 = 0, n2 = 0, n3 = 0, n4 = 0, n5 = 0, n6 = 0, n7 = 0;
    if (idct)
    {
        n0 = nat[dst + col];
        n1 = nat[dst + col + RS];
        n2 = nat[dst + col + RS * 2];
        n3 = nat[dst + col + RS * 3];
        n4 = nat[dst + col + RS * 4];
        n5 = nat[dst + col + RS * 5];
        n6 = nat[dst + col + RS * 6];
        n7 = nat[dst + col + RS * 7];
    }
    barrier();

    if (idct)
    {
        int t = dst + col * RS;
        if ((n1 | n2 | n3 | n4 | n5 | n6 | n7) != 0)
        {
            int x0, x1, x2, x3, y0, y1, y2, y3;
            idct_pass(x0, x1, x2, x3, y0, y1, y2, y3,
                n0 * qt[qbase + col], n1 * qt[qbase + col + 8],
                n2 * qt[qbase + col + 16], n3 * qt[qbase + col + 24],
                n4 * qt[qbase + col + 32], n5 * qt[qbase + col + 40],
                n6 * qt[qbase + col + 48], n7 * qt[qbase + col + 56]);
            const int bias = 0x200;
            x0 += bias; x1 += bias; x2 += bias; x3 += bias;
            nat[t + 0] = (x0 + y3) >> 10;
            nat[t + 1] = (x1 + y2) >> 10;
            nat[t + 2] = (x2 + y1) >> 10;
            nat[t + 3] = (x3 + y0) >> 10;
            nat[t + 4] = (x3 - y0) >> 10;
            nat[t + 5] = (x2 - y1) >> 10;
            nat[t + 6] = (x1 - y2) >> 10;
            nat[t + 7] = (x0 - y3) >> 10;
        }
        else
        {
            int dc = (n0 * qt[qbase + col]) << 2;
            for (int k = 0; k < 8; ++k)
                nat[t + k] = dc;
        }
    }
    barrier();

    if (idct)
    {
        int row = col;
        n0 = nat[dst + row];
        n1 = nat[dst + row + RS];
        n2 = nat[dst + row + RS * 2];
        n3 = nat[dst + row + RS * 3];
        n4 = nat[dst + row + RS * 4];
        n5 = nat[dst + row + RS * 5];
        n6 = nat[dst + row + RS * 6];
        n7 = nat[dst + row + RS * 7];
    }
    barrier();

    if (idct)
    {
        int row = col;
        int x0, x1, x2, x3, y0, y1, y2, y3;
        idct_pass(x0, x1, x2, x3, y0, y1, y2, y3, n0, n1, n2, n3, n4, n5, n6, n7);
        const int shift = 17;
        const int bias = 0x10000 + (128 << shift);
        x0 += bias; x1 += bias; x2 += bias; x3 += bias;
        int o = dst + row * RS;
        nat[o + 0] = clamp_u8((x0 + y3) >> shift);
        nat[o + 1] = clamp_u8((x1 + y2) >> shift);
        nat[o + 2] = clamp_u8((x2 + y1) >> shift);
        nat[o + 3] = clamp_u8((x3 + y0) >> shift);
        nat[o + 4] = clamp_u8((x3 - y0) >> shift);
        nat[o + 5] = clamp_u8((x2 - y1) >> shift);
        nat[o + 6] = clamp_u8((x1 - y2) >> shift);
        nat[o + 7] = clamp_u8((x0 - y3) >> shift);
    }
    barrier();
    // Each lane colours a horizontal pixel pair; a chroma block at least 16
    // pixels wide gives both pixels of an (even-aligned) pair the same sample.
    const int area2 = XB * YB / 2;
    for (int p = lane; p < MPG * area2; p += 256)
    {
        int mcu_i = p / area2;
        int pix = p - mcu_i * area2;
        if (group0 + mcu_i >= pc.nmcu)
            continue;
        int lx = (pix % (XB / 2)) * 2;
        int ly = pix / (XB / 2);
        int px = origin[mcu_i].x + lx;
        int py = origin[mcu_i].y + ly;
        if (px >= pc.width || py >= pc.height)
            continue;

        int Y0 = sample_at(mcu_i, Y_OFF, Y_H, Y_V, lx, ly);
        int Y1 = sample_at(mcu_i, Y_OFF, Y_H, Y_V, lx + 1, ly);
#if NCOMP == 1
        ivec3 c0 = ivec3(Y0);
        ivec3 c1 = ivec3(Y1);
#else
        int Cb0 = sample_at(mcu_i, CB_OFF, CB_H, CB_V, lx, ly) - 128;
        int Cr0 = sample_at(mcu_i, CR_OFF, CR_H, CR_V, lx, ly) - 128;
#if XB / CB_H >= 16
        int Cb1 = Cb0;
#else
        int Cb1 = sample_at(mcu_i, CB_OFF, CB_H, CB_V, lx + 1, ly) - 128;
#endif
#if XB / CR_H >= 16
        int Cr1 = Cr0;
#else
        int Cr1 = sample_at(mcu_i, CR_OFF, CR_H, CR_V, lx + 1, ly) - 128;
#endif
        ivec3 c0 = clamp(Y0 + ivec3((Cr0 * 91881) >> 16, -((Cb0 * 22554 + Cr0 * 46802) >> 16), (Cb0 * 116130) >> 16), 0, 255);
        ivec3 c1 = clamp(Y1 + ivec3((Cr1 * 91881) >> 16, -((Cb1 * 22554 + Cr1 * 46802) >> 16), (Cb1 * 116130) >> 16), 0, 255);
#endif
        imageStore(uOut, ivec2(px, py), vec4(vec3(c0), 255.0) * (1.0 / 255.0));
        if (px + 1 < pc.width)
            imageStore(uOut, ivec2(px + 1, py), vec4(vec3(c1), 255.0) * (1.0 / 255.0));
    }
}
)glsl";

int symEntryHost(int sym, int len, bool dc)
{
    int s = sym & 15;
    int r = sym >> 4;
    int za = dc ? 1 : (s ? r + 1 : (r == 15 ? 16 : 64));
    return (len + s) | (s << 5) | (za << 9);
}

void buildSlot(const image::jpeg::HuffmanCodeTable& src, bool dc,
    std::vector<u16>& lut, std::vector<u8>& hv, TablesGpu& dst, int slot)
{
    for (int i = 0; i < 18; ++i)
        dst.maxc[slot * 18 + i] = -1;
    dst.maxc[slot * 18 + 17] = 0x7fffffff;
    int code = 0;
    int p = 0;
    for (int len = 1; len <= 16; ++len)
    {
        int nl = src.bits[len - 1];
        if (nl)
        {
            dst.voff[slot * 18 + len] = p - code;
            for (int k = 0; k < nl && p + k < 256; ++k)
            {
                int sym = src.values[p + k];
                hv[slot * 256 + p + k] = u8(sym);
                if (len <= kLb)
                {
                    int lo = (code + k) << (kLb - len);
                    int n = 1 << (kLb - len);
                    u16 ent = u16(symEntryHost(sym, len, dc));
                    for (int i = 0; i < n; ++i)
                        lut[slot * kLut + lo + i] = ent;
                }
            }
            p += nl;
            code += nl;
            dst.maxc[slot * 18 + len] = code - 1;
        }
        code <<= 1;
    }
}

void memBarrier(VkCommandBuffer cmd, VkAccessFlags src, VkAccessFlags dst,
    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    VkMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = src,
        .dstAccessMask = dst,
    };
    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void computeBarrier(VkCommandBuffer cmd)
{
    memBarrier(cmd, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void fillLayout(const image::jpeg::JpegInfo& info, int bpm, JpegComputePush& push)
{
    push.bpm = bpm;
    push.width = info.width;
    push.height = info.height;
    push.xmcu = info.xmcu;
    push.xblock = info.xblock;
    push.yblock = info.yblock;
    push.components = info.components;
    int off = 0;
    auto setComp = [&] (int idx, s32& hsf, s32& vsf, s32& qt, s32& o)
    {
        if (idx >= info.components)
        {
            hsf = vsf = 1;
            qt = 0;
            o = 0;
            return;
        }
        hsf = info.component[idx].hsf;
        vsf = info.component[idx].vsf;
        qt = info.component[idx].qt;
        o = off;
        off += hsf * vsf;
    };
    setComp(0, push.y_hsf, push.y_vsf, push.y_qt, push.y_off);
    setComp(1, push.cb_hsf, push.cb_vsf, push.cb_qt, push.cb_off);
    setComp(2, push.cr_hsf, push.cr_vsf, push.cr_qt, push.cr_off);
    push.mcus_per_group = std::max(32 / std::max(push.bpm, 1), 1);
    push.nmcu = info.xmcu * info.ymcu;
}

std::string idctDefines(const JpegComputePush& p)
{
    auto d = [] (const char* name, int v)
    {
        return std::string("#define ") + name + " " + std::to_string(v) + "\n";
    };
    return d("BPM", p.bpm) + d("XB", p.xblock) + d("YB", p.yblock) + d("NCOMP", p.components)
        + d("MPG", p.mcus_per_group)
        + d("Y_H", p.y_hsf) + d("Y_V", p.y_vsf) + d("Y_OFF", p.y_off)
        + d("CB_H", p.cb_hsf) + d("CB_V", p.cb_vsf) + d("CB_OFF", p.cb_off)
        + d("CR_H", p.cr_hsf) + d("CR_V", p.cr_vsf) + d("CR_OFF", p.cr_off);
}

// Frame layout of a common sampling for pipeline warmup (quant tables come
// from push constants, so they do not matter here).
JpegComputePush warmLayout(int components, int yh, int yv)
{
    image::jpeg::JpegInfo info;
    info.components = components;
    info.xblock = 8 * yh;
    info.yblock = 8 * yv;
    info.component[0].hsf = yh;
    info.component[0].vsf = yv;
    JpegComputePush push = {};
    fillLayout(info, components == 1 ? 1 : yh * yv + 2, push);
    return push;
}

std::string huffmanDefines(int ntables, int wg, const char* wg_name)
{
    const std::string w = std::to_string(wg);
    return "#define NT " + std::to_string(std::max(ntables, 1))
        + "\n#define LB " + std::to_string(kLb)
        + "\n#define NCP " + std::to_string(kNcp)
        + "\n#define PIECE_BITS " + std::to_string(kPieceBits)
        + "\n#define " + wg_name + " " + w
        + "\n#define HUFF_WG " + w + "\n";
}

VkDeviceSize align16(VkDeviceSize v)
{
    return (v + 15) & ~VkDeviceSize(15);
}

} // namespace

// -----------------------------------------------------------------------
// JpegCompute
// -----------------------------------------------------------------------

JpegCompute::JpegCompute(VkPhysicalDevice physicalDevice, VkDevice device, Allocator& allocator,
    StagingArena& staging, int slots)
    : m_physical_device(physicalDevice)
    , m_device(device)
    , m_allocator(allocator)
    , m_staging(staging)
{
    VkFormatProperties fp = {};
    vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &fp);
    if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
    {
        return;
    }

    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    m_tick_ns = props.limits.timestampPeriod;

    VkDescriptorSetLayoutBinding bindings[kJpegComputeBindings] = {};
    for (u32 i = 0; i < kJpegComputeBindings; ++i)
    {
        const VkDescriptorType type = i == BindImage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i] = { i, type, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
    }

    VkDescriptorSetLayoutCreateInfo set_info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = kJpegComputeBindings,
        .pBindings = bindings,
    };
    if (vkCreateDescriptorSetLayout(m_device, &set_info, nullptr, &m_set_layout) != VK_SUCCESS)
    {
        return;
    }

    VkPushConstantRange push = { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(JpegComputePush) };
    VkPipelineLayoutCreateInfo layout_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push,
    };
    if (vkCreatePipelineLayout(m_device, &layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
    {
        return;
    }

    slots = std::max(slots, 1);

    VkDescriptorPoolSize sizes[2] =
    {
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, u32(kJpegComputeBindings - 1) * u32(slots) },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, u32(slots) },
    };
    VkDescriptorPoolCreateInfo pool_info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = u32(slots),
        .poolSizeCount = 2,
        .pPoolSizes = sizes,
    };
    if (vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
    {
        return;
    }

    m_slots.resize(slots);
    for (Slot& slot : m_slots)
    {
        VkDescriptorSetAllocateInfo ai =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_set_layout,
        };
        if (vkAllocateDescriptorSets(m_device, &ai, &slot.set) != VK_SUCCESS)
        {
            return;
        }

        if (m_tick_ns > 0.0f)
        {
            VkQueryPoolCreateInfo qp =
            {
                .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                .queryType = VK_QUERY_TYPE_TIMESTAMP,
                .queryCount = kQueries,
            };
            if (vkCreateQueryPool(m_device, &qp, nullptr, &slot.queries) != VK_SUCCESS)
            {
                slot.queries = VK_NULL_HANDLE;
            }
        }
    }

    m_supported = true;
    m_warmup_thread = std::thread([this] { warmup(); });
}

JpegCompute::~JpegCompute()
{
    {
        std::lock_guard lock(m_pipeline_mutex);
        m_stop_warmup = true;
    }
    m_compile_cv.notify_all();
    if (m_warmup_thread.joinable())
        m_warmup_thread.join();

    for (Slot& slot : m_slots)
    {
        for (BufferAllocation& buffer : slot.buffer)
        {
            if (buffer)
            {
                m_allocator.destroyBuffer(buffer);
            }
        }

        if (slot.queries)
        {
            vkDestroyQueryPool(m_device, slot.queries, nullptr);
        }
    }

    // Every compile has finished: prepare() runs on threads the owner has joined.
    for (auto& it : m_pipelines)
    {
        VkPipeline pipeline = it.second.get();
        if (pipeline)
        {
            vkDestroyPipeline(m_device, pipeline, nullptr);
        }
    }

    if (m_descriptor_pool)
    {
        vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);
    }

    if (m_pipeline_layout)
    {
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
    }

    if (m_set_layout)
    {
        vkDestroyDescriptorSetLayout(m_device, m_set_layout, nullptr);
    }
}

const char* JpegCompute::classify(const image::jpeg::JpegInfo& info)
{
    if (!info.valid)
        return "invalid JPEG header";
    if (info.ultrahdr)
        return "UltraHDR gain map";
    if (info.arithmetic)
        return "arithmetic coding";
    if (info.lossless)
        return "lossless";
    if (info.differential)
        return "hierarchical (differential)";
    if (info.progressive)
        return "progressive";
    if (info.multiscan)
        return "multi-scan sequential";
    if (info.precision != 8)
        return "12-bit precision";
    if (info.components != 1 && info.components != 3)
        return info.components == 4 ? "CMYK / 4 components" : "unsupported component count";
    if (info.rgb)
        return "RGB colorspace (no YCbCr)";

    if (info.components == 3)
    {
        const image::jpeg::ComponentInfo& y = info.component[0];
        const image::jpeg::ComponentInfo& cb = info.component[1];
        const image::jpeg::ComponentInfo& cr = info.component[2];
        const bool luma = (y.hsf == 1 || y.hsf == 2) && (y.vsf == 1 || y.vsf == 2);
        const bool chroma = cb.hsf == 1 && cb.vsf == 1 && cr.hsf == 1 && cr.vsf == 1;
        if (!luma || !chroma)
            return "chroma sampling";
    }

    if (info.xblock != 8 && info.xblock != 16)
        return "MCU width";
    if (info.yblock != 8 && info.yblock != 16)
        return "MCU height";
    if (info.blocks_in_mcu < 1 || info.blocks_in_mcu > 10)
        return "MCU block count";

    return nullptr;
}

// Each variant compiles exactly once. Try never blocks: a missing variant is
// queued for the warmup thread. Build compiles a missing variant inline and
// skips one already in progress (the warmup thread must not wait for itself).
// Wait compiles inline or blocks on the compile in progress.
VkPipeline JpegCompute::pipeline(const std::string& key, const std::string& source, Resolve mode,
    double& compile_ms, std::string& error)
{
    std::promise<VkPipeline> promise;
    std::shared_future<VkPipeline> future;
    {
        std::lock_guard lock(m_pipeline_mutex);
        auto it = m_pipelines.find(key);
        if (it != m_pipelines.end())
        {
            future = it->second;
        }
        else if (mode == Resolve::Try)
        {
            CompileRequest request;
            request.key = key;
            request.source = source;
            m_pipelines.emplace(key, request.promise.get_future().share());
            m_compile_queue.push_back(std::move(request));
            m_compile_cv.notify_one();
            return VK_NULL_HANDLE;
        }
        else
        {
            m_pipelines.emplace(key, promise.get_future().share());
        }
    }

    if (future.valid())
    {
        const bool ready = future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        if (!ready && mode != Resolve::Wait)
            return VK_NULL_HANDLE;

        const u64 t0 = Time::us();
        VkPipeline result = future.get();
        compile_ms += double(Time::us() - t0) / 1000.0;
        if (!result)
            error = "jpeg compute pipeline unavailable";
        return result;
    }

    const u64 t0 = Time::us();
    VkPipeline result = compilePipeline(key, source, error);
    promise.set_value(result);
    compile_ms += double(Time::us() - t0) / 1000.0;
    return result;
}

VkPipeline JpegCompute::compilePipeline(const std::string& key, const std::string& source, std::string& error)
{
    Compiler compiler;
    Shader shader = compiler.compile(source, ShaderStage::Compute);
    if (!shader)
    {
        error = "jpeg compute shader compile failed (" + key + "):\n" + shader.log;
        return VK_NULL_HANDLE;
    }

    VkShaderModule module = Compiler::createShaderModule(m_device, shader);
    if (!module)
    {
        error = "jpeg compute shader module failed";
        return VK_NULL_HANDLE;
    }

    VkComputePipelineCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = module,
            .pName = "main",
        },
        .layout = m_pipeline_layout,
    };

    VkPipeline result = VK_NULL_HANDLE;
    VkResult vr = vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr, &result);
    vkDestroyShaderModule(m_device, module, nullptr);

    if (vr != VK_SUCCESS)
    {
        error = "jpeg compute pipeline failed";
        return VK_NULL_HANDLE;
    }

    return result;
}

bool JpegCompute::prepare(image::jpeg::JpegDecoder& decoder, JpegComputeJob& job, bool wait_for_pipelines, std::string& error)
{
    const image::jpeg::JpegInfo& info = decoder.info();

    u64 lap_us = Time::us();
    auto lap = [&lap_us] () -> double
    {
        const u64 now = Time::us();
        const double ms = double(now - lap_us) / 1000.0;
        lap_us = now;
        return ms;
    };

    image::jpeg::EntropyStream stream = decoder.extractEntropy(image::jpeg::EntropyExtract::Deferred);
    job.extract_ms = lap();
    if (!stream.ok)
    {
        error = stream.error.empty() ? "entropy extract failed" : "entropy: " + stream.error;
        return false;
    }

    if (stream.scan.size == 0 || stream.segments.empty())
    {
        error = "empty entropy stream";
        return false;
    }

    const int bpm = stream.blocks_in_mcu;
    if (bpm <= 0 || bpm > 10)
    {
        error = "MCU block count";
        return false;
    }

    JpegComputePush& push = job.push;
    push = {};
    fillLayout(info, bpm, push);
    push.rounds = kSyncWg;

    // Natural-order quantisation tables for the IDCT.
    s32 natural_qt[4 * 64] = {};
    for (int t = 0; t < 4; ++t)
    {
        if (info.quant[t])
        {
            for (int i = 0; i < 64; ++i)
                natural_qt[t * 64 + i] = info.quant[t][i];
        }
    }

    const size_t nseg = stream.segments.size();
    const int nmcu = push.nmcu;
    const int dc_groups = (nmcu + kDcTile - 1) / kDcTile;

    // Packing only shrinks the scan, so its file size bounds the words and the
    // chunk count (each segment adds at most one partial chunk); staging is
    // laid out before the scan is touched.
    const size_t stuffed = stream.scan.size;
    const size_t max_chunks = (stuffed * 8 + kChunkBits - 1) / kChunkBits + nseg;
    if (stuffed > size_t(std::numeric_limits<u32>::max() / 8) - 64)
    {
        error = "entropy stream too large";
        return false;
    }

    const size_t seg_start_words = size_t(nmcu + 31) / 32 + 1;
    const VkDeviceSize max_upload[] =
    {
        ((stuffed + 3u) & ~size_t(3)) + 32u,    // BindWords
        max_chunks * sizeof(ChunkGpu),          // BindChunks
        nseg * sizeof(SegGpu),                  // BindSegs
        sizeof(TablesGpu),                      // BindTables
        sizeof(natural_qt),                     // BindQt
        seg_start_words * 4,                    // BindSegStart
    };

    // One staging range holds everything the GPU reads from the host.
    const Binding uploads[] = { BindWords, BindChunks, BindSegs, BindTables, BindQt, BindSegStart };
    VkDeviceSize total = 0;
    for (size_t i = 0; i < std::size(uploads); ++i)
    {
        job.src_offset[uploads[i]] = total;
        total += align16(max_upload[i]);
    }

    // After extract, tables/pipelines and staging pack are independent. Run pack
    // on the ThreadPool so prepare wall ≈ max(tables+pipelines, alloc+unstuff)
    // and idle cores show up on the TP graph while scrolling.
    struct PackLane
    {
        bool ok = true;
        std::string error;
        size_t nch = 0;
        int block = 0;
        u32 nwords = 0;
        double alloc_ms = 0.0;
        double unstuff_ms = 0.0;
    } pack;

    ConcurrentQueue pack_queue;
    pack_queue.enqueue([&]
    {
        const u64 t_alloc0 = Time::us();
        job.staging = m_staging.acquire(total);
        pack.alloc_ms = double(Time::us() - t_alloc0) / 1000.0;
        if (!job.staging)
        {
            pack.ok = false;
            pack.error = "staging allocation failed";
            return;
        }
        job.staging_bytes = size_t(total);

        const u64 t_pack0 = Time::us();
        u8* dst = job.staging.mapped;
        u8* const words = dst + job.src_offset[BindWords];
        ChunkGpu* const chunks = reinterpret_cast<ChunkGpu*>(dst + job.src_offset[BindChunks]);
        SegGpu* const segs = reinterpret_cast<SegGpu*>(dst + job.src_offset[BindSegs]);

        // Build seg_start in cached memory: write-combined staging turns RMW |= into
        // a read of undefined neighbouring bits.
        std::vector<u32> seg_start_local(seg_start_words, 0);

        // Pack straight into staging in file byte order (the shader assembles
        // big-endian words), then cut chunks that never cross a segment.
        if (!decoder.packEntropy(stream, words))
        {
            pack.ok = false;
            pack.error = "entropy: " + stream.error;
            return;
        }

        size_t nch = 0;
        int block = 0;
        int mcu = 0;

        for (size_t s = 0; s < nseg; ++s)
        {
            const auto& seg = stream.segments[s];
            const u32 seg_bit = u32(seg.packed_offset) * 8;
            const u32 seg_end = u32(seg.packed_offset + seg.packed_size) * 8;

            SegGpu sg = {};
            sg.chunk0 = int(nch);
            sg.block0 = block;
            sg.mcu0 = mcu;
            sg.mcus = seg.mcus;
            for (u32 at = seg_bit; at < seg_end; at += kChunkBits)
            {
                ChunkGpu c = {};
                c.bit0 = at;
                c.bit1 = std::min(at + kChunkBits, seg_end);
                c.seg0 = seg_bit;
                c.seg1 = seg_end;
                c.block0 = block;
                c.block1 = block + seg.mcus * bpm;
                chunks[nch++] = c;
            }
            sg.nchunks = int(nch) - sg.chunk0;
            segs[s] = sg;
            if (mcu < nmcu)
                seg_start_local[size_t(mcu) >> 5] |= 1u << (mcu & 31);
            block += seg.mcus * bpm;
            mcu += seg.mcus;
        }

        std::memcpy(dst + job.src_offset[BindSegStart], seg_start_local.data(), seg_start_words * 4);

        const size_t nunstuffed = stream.packed_bytes;
        const size_t nbytes = ((nunstuffed + 3u) & ~size_t(3)) + 32u;
        std::memset(words + nunstuffed, 0, nbytes - nunstuffed);

        pack.nch = nch;
        pack.block = block;
        pack.nwords = u32(nbytes / 4);
        pack.unstuff_ms = double(Time::us() - t_pack0) / 1000.0;

        if (!nch)
        {
            pack.ok = false;
            pack.error = "no entropy chunks";
        }
    });

    // Tables + pipelines on this prepare worker (overlaps the pack lane).
    const u64 t_tables0 = Time::us();

    u64 tables_key = mixHash(0x7AB1Eu, u64(bpm));
    for (int b = 0; b < bpm; ++b)
    {
        const auto& mb = stream.block[b];
        tables_key = mixHash(tables_key, u64(mb.dc) | (u64(mb.ac) << 8) | (u64(mb.component) << 16));
    }
    for (const auto& table : stream.huffman)
    {
        tables_key = mixHash(tables_key, u64(table.cls) | (u64(table.id) << 8));
        tables_key = hashBytes(tables_key, table.bits, sizeof(table.bits));
        int count = 0;
        for (int i = 0; i < 16; ++i)
            count += table.bits[i];
        tables_key = hashBytes(tables_key, table.values, size_t(count));
    }
    tables_key = hashBytes(tables_key, natural_qt, sizeof(natural_qt));

    std::shared_ptr<const TablesCacheEntry> cached;
    {
        std::lock_guard lock(m_tables_mutex);
        auto it = m_tables_cache.find(tables_key);
        if (it != m_tables_cache.end())
            cached = std::static_pointer_cast<const TablesCacheEntry>(it->second);
    }

    std::shared_ptr<TablesCacheEntry> built;
    const TablesCacheEntry* tab_entry = nullptr;
    std::string tables_error;
    if (cached)
    {
        tab_entry = cached.get();
    }
    else
    {
        built = std::make_shared<TablesCacheEntry>();
        TablesGpu& tab = built->tables;
        std::vector<u16> lut(8 * kLut, 0);
        std::vector<u8> hv(8 * 256, 0);
        int slot_of[8];
        for (int i = 0; i < 8; ++i)
            slot_of[i] = -1;

        int ntables = 0;
        for (int b = 0; b < bpm; ++b)
        {
            const int used[2] = { stream.block[b].dc & 3, 4 + (stream.block[b].ac & 3) };
            for (int u : used)
            {
                if (slot_of[u] < 0)
                    slot_of[u] = ntables++;
            }
        }

        for (const auto& table : stream.huffman)
        {
            const int id = table.cls * 4 + table.id;
            if (id < 0 || id >= 8)
            {
                tables_error = "Huffman table id outside 0..3";
                break;
            }
            if (slot_of[id] >= 0)
                buildSlot(table, table.cls == 0, lut, hv, tab, slot_of[id]);
        }

        if (tables_error.empty())
        {
            for (int i = 0; i < 8 * kLut / 2; ++i)
                tab.lut[i] = u32(lut[2 * i]) | (u32(lut[2 * i + 1]) << 16);

            for (int i = 0; i < 8 * 256 / 4; ++i)
            {
                tab.hv[i] = u32(hv[4 * i]) | (u32(hv[4 * i + 1]) << 8)
                    | (u32(hv[4 * i + 2]) << 16) | (u32(hv[4 * i + 3]) << 24);
            }

            built->dc_pack = built->ac_pack = built->comp_pack = 0;
            for (int b = 0; b < bpm; ++b)
            {
                const auto& mb = stream.block[b];
                built->dc_pack |= u32(slot_of[mb.dc & 3]) << (3 * b);
                built->ac_pack |= u32(slot_of[4 + (mb.ac & 3)]) << (3 * b);
                built->comp_pack |= u32(mb.component & 3) << (2 * b);
            }
            built->ntables = ntables;
            std::memcpy(built->qt, natural_qt, sizeof(natural_qt));

            {
                std::lock_guard lock(m_tables_mutex);
                auto [it, inserted] = m_tables_cache.emplace(tables_key, built);
                if (!inserted)
                    built = std::const_pointer_cast<TablesCacheEntry>(
                        std::static_pointer_cast<const TablesCacheEntry>(it->second));
            }
            tab_entry = built.get();
        }
    }

    job.tables_ms = double(Time::us() - t_tables0) / 1000.0;

    bool resolved = false;
    if (tab_entry)
    {
        push.dc_pack = tab_entry->dc_pack;
        push.ac_pack = tab_entry->ac_pack;
        push.comp_pack = tab_entry->comp_pack;
        const int ntables = tab_entry->ntables;

        // Without waiting, missing variants are queued and this image takes the CPU path.
        const Resolve mode = wait_for_pipelines ? Resolve::Wait : Resolve::Try;
        const u64 t_pipe0 = Time::us();
        resolved = resolvePipelines(ntables, push, job.pipelines, mode, job.compile_ms, error);
        job.pipelines_ms = double(Time::us() - t_pipe0) / 1000.0;
        if (!resolved && error.empty())
            error = "compute pipelines compiling";
    }
    else if (!tables_error.empty())
    {
        error = tables_error;
    }

    pack_queue.wait();

    job.alloc_ms = pack.alloc_ms;
    job.unstuff_ms = pack.unstuff_ms;

    if (!pack.ok)
    {
        if (job.staging)
            m_staging.release(job.staging);
        error = pack.error;
        return false;
    }
    if (!resolved || !tab_entry)
    {
        if (job.staging)
            m_staging.release(job.staging);
        if (error.empty())
            error = tables_error.empty() ? "tables failed" : tables_error;
        return false;
    }

    push.nch = int(pack.nch);
    push.nwords = pack.nwords;

    const int nblk_alloc = std::max(pack.block, nmcu * bpm);

    job.chunks = int(pack.nch);
    job.segments = int(nseg);
    job.dc_groups = dc_groups;

    VkDeviceSize* need = job.need;
    need[BindWords] = VkDeviceSize(pack.nwords) * 4;
    need[BindChunks] = pack.nch * sizeof(ChunkGpu);
    need[BindSegs] = nseg * sizeof(SegGpu);
    need[BindTables] = sizeof(TablesGpu);
    need[BindStIn] = pack.nch * 8;
    need[BindStOut] = pack.nch * 8;
    need[BindCnt] = pack.nch * 4;
    need[BindCps] = pack.nch * kNcp * 8;
    need[BindCpc] = pack.nch * kNcp * 4;
    need[BindFirst] = pack.nch * 4;
    need[BindCoef] = VkDeviceSize(nblk_alloc) * 32 * 4;
    need[BindDcd] = VkDeviceSize(nblk_alloc) * 4;
    need[BindQt] = sizeof(natural_qt);
    need[BindFlags] = kFlagsBytes;
    need[BindImage] = 0;
    need[BindSegStart] = seg_start_words * 4;
    need[BindAgg] = VkDeviceSize(std::max(dc_groups, 1)) * 2 * 16;

    for (Binding b : uploads)
        job.src_size[b] = need[b];

    const u64 t_upload0 = Time::us();
    u8* dst = job.staging.mapped;
    std::memcpy(dst + job.src_offset[BindTables], &tab_entry->tables, sizeof(TablesGpu));
    std::memcpy(dst + job.src_offset[BindQt], tab_entry->qt, sizeof(tab_entry->qt));
    m_staging.flush(job.staging, 0, total);
    job.upload_ms = double(Time::us() - t_upload0) / 1000.0;

    return true;
}

// Huffman pipelines vary by table count, the IDCT by frame layout.
bool JpegCompute::resolvePipelines(int ntables, const JpegComputePush& push,
    JpegComputePipelines& p, Resolve mode, double& ms, std::string& error)
{
    const u64 set_key = pipelineSetKey(ntables, push);
    {
        std::lock_guard lock(m_pipeline_mutex);
        auto it = m_pipeline_sets.find(set_key);
        if (it != m_pipeline_sets.end()
            && it->second.sync && it->second.sync_gate && it->second.prefix
            && it->second.write && it->second.dc_local && it->second.dc_groups && it->second.idct)
        {
            p = it->second;
            return true;
        }
    }

    const std::string huff_sync = huffmanDefines(ntables, kSyncWg, "SYNC_WG");
    const std::string huff_write = huffmanDefines(ntables, kWriteWg, "WRITE_WG");
    const std::string idct = idctDefines(push);

    // Look every variant up (queueing the missing ones) before reporting a miss,
    // so one job requests its whole set at once.
    p.sync = pipeline("sync" + huff_sync, std::string(kCommon) + huff_sync + kHuffman + kSyncMain, mode, ms, error);
    p.sync_gate = pipeline("syncgate", std::string(kCommon) + kSyncGateMain, mode, ms, error);
    p.prefix = pipeline("prefix", std::string(kCommon) + kPrefixMain, mode, ms, error);
    p.write = pipeline("write" + huff_write, std::string(kCommon) + huff_write + kHuffman + kWriteMain, mode, ms, error);
    p.dc_local = pipeline("dclocal", std::string(kCommon) + kDcScan + kDcLocalMain, mode, ms, error);
    p.dc_groups = pipeline("dcgroups", std::string(kCommon) + kDcScan + kDcGroupsMain, mode, ms, error);
    p.idct = pipeline("idct" + idct, std::string(kCommon) + idct + kIdctMain, mode, ms, error);

    const bool ready = p.sync && p.sync_gate && p.prefix && p.write && p.dc_local && p.dc_groups && p.idct;
    if (ready)
    {
        std::lock_guard lock(m_pipeline_mutex);
        m_pipeline_sets[set_key] = p;
    }
    return ready;
}

void JpegCompute::warmup()
{
    // Most common first: 4 tables (separate luma/chroma), then shared tables.
    struct Variant { int ntables, components, yh, yv; };
    static const Variant variants[] =
    {
        { 4, 3, 2, 2 },     // 4:2:0
        { 4, 3, 1, 1 },     // 4:4:4
        { 4, 3, 2, 1 },     // 4:2:2
        { 2, 1, 1, 1 },     // grayscale
        { 2, 3, 2, 2 },
        { 2, 3, 1, 1 },
        { 4, 3, 1, 2 },     // 4:4:0
    };
    const size_t count = sizeof(variants) / sizeof(variants[0]);
    size_t next = 0;

    for (;;)
    {
        CompileRequest request;
        {
            std::unique_lock lock(m_pipeline_mutex);
            m_compile_cv.wait(lock, [&] { return m_stop_warmup || !m_compile_queue.empty() || next < count; });
            if (m_stop_warmup)
                break;
            if (!m_compile_queue.empty())
            {
                request = std::move(m_compile_queue.front());
                m_compile_queue.pop_front();
            }
        }

        // Variants a decode is waiting for come before the speculative ones.
        if (!request.key.empty())
        {
            std::string error;
            request.promise.set_value(compilePipeline(request.key, request.source, error));
            continue;
        }

        const Variant& v = variants[next++];
        JpegComputePipelines p;
        double ms = 0.0;
        std::string error;
        resolvePipelines(v.ntables, warmLayout(v.components, v.yh, v.yv), p, Resolve::Build, ms, error);
    }

    std::lock_guard lock(m_pipeline_mutex);
    for (CompileRequest& request : m_compile_queue)
        request.promise.set_value(VK_NULL_HANDLE);
    m_compile_queue.clear();
}

bool JpegCompute::acquire(JpegComputeJob& job)
{
    for (size_t i = 0; i < m_slots.size(); ++i)
    {
        if (!m_slots[i].busy)
        {
            m_slots[i].busy = true;
            job.slot = int(i);
            return true;
        }
    }
    return false;
}

bool JpegCompute::record(VkCommandBuffer cmd, JpegComputeJob& job, VkImage image, VkImageView storageView, std::string& error)
{
    Slot& slot = m_slots[job.slot];

    // ReBAR: host-written uploads already live in device-local memory — bind the
    // staging ranges directly and skip vkCmdCopyBuffer for those bindings.
    const bool direct_uploads = job.staging.device_local && job.staging.buffer;
    const Binding upload_bindings[] = { BindWords, BindChunks, BindSegs, BindTables, BindQt, BindSegStart };
    auto isUpload = [&] (u32 b) -> bool
    {
        for (Binding u : upload_bindings)
            if (u32(u) == b)
                return true;
        return false;
    };

    // Grow the slot's device buffers to fit this frame; the slot is idle, so
    // the old ones can go right away. Direct uploads need no device-local twin.
    for (u32 i = 0; i < kJpegComputeBindings; ++i)
    {
        if (i == BindImage || (direct_uploads && isUpload(i)) || slot.capacity[i] >= job.need[i])
            continue;

        if (slot.buffer[i])
            m_allocator.destroyBuffer(slot.buffer[i]);

        // 25% headroom keeps a stream of similar images from reallocating each time.
        const VkDeviceSize size = std::max<VkDeviceSize>(align16(job.need[i] + job.need[i] / 4), 16);
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (i == BindFlags)
            usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        slot.buffer[i] = m_allocator.createBuffer(size, usage, MemoryUsage::GpuOnly, false);
        if (!slot.buffer[i])
        {
            slot.capacity[i] = 0;
            error = "jpeg compute device buffer allocation failed";
            return false;
        }
        slot.capacity[i] = size;
    }

    VkDescriptorBufferInfo buffer_info[kJpegComputeBindings] = {};
    VkWriteDescriptorSet writes[kJpegComputeBindings] = {};
    for (u32 i = 0; i < kJpegComputeBindings; ++i)
    {
        if (i == BindImage)
            continue;
        if (direct_uploads && isUpload(i) && job.src_size[i])
            buffer_info[i] = { job.staging.buffer, job.staging.offset + job.src_offset[i], job.src_size[i] };
        else
            buffer_info[i] = { slot.buffer[i].buffer, 0, VK_WHOLE_SIZE };
        writes[i] =
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = slot.set,
            .dstBinding = i,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &buffer_info[i],
        };
    }
    VkDescriptorImageInfo image_info =
    {
        .sampler = VK_NULL_HANDLE,
        .imageView = storageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    writes[BindImage] =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = slot.set,
        .dstBinding = BindImage,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &image_info,
    };
    vkUpdateDescriptorSets(m_device, kJpegComputeBindings, writes, 0, nullptr);

    if (slot.queries)
    {
        vkCmdResetQueryPool(cmd, slot.queries, 0, kQueries);
        vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, slot.queries, 0);
    }

    if (!direct_uploads)
    {
        for (u32 i = 0; i < kJpegComputeBindings; ++i)
        {
            if (job.src_size[i])
            {
                VkBufferCopy region = { job.staging.offset + job.src_offset[i], 0, job.src_size[i] };
                vkCmdCopyBuffer(cmd, job.staging.buffer, slot.buffer[i].buffer, 1, &region);
            }
        }
    }
    vkCmdFillBuffer(cmd, slot.buffer[BindDcd].buffer, 0, job.need[BindDcd], 0);
    vkCmdFillBuffer(cmd, slot.buffer[BindFlags].buffer, 0, job.need[BindFlags], 0);

    // Host writes (ReBAR) and/or copies + prior slot users must finish first.
    const VkAccessFlags src_access = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT
        | (direct_uploads ? VK_ACCESS_HOST_WRITE_BIT : 0);
    const VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        | (direct_uploads ? VK_PIPELINE_STAGE_HOST_BIT : 0);
    memBarrier(cmd, src_access, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        src_stage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    JpegComputePush push = job.push;
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, 0, 1, &slot.set, 0, nullptr);

    auto dispatch = [&] (VkPipeline pipeline, u32 groups)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
        vkCmdDispatch(cmd, std::max(groups, 1u), 1, 1);
    };

    const u32 chunks = u32(job.chunks);
    const u32 sync_groups = (chunks + kSyncWg - 1u) / kSyncWg;

    // Single workgroup: local rounds already converge; extra passes are no-ops.
    // Multi-WG: pass 0 is always full; later passes use DispatchIndirect so a
    // converged flags[pass-1]==0 launches zero groups instead of a full sync.
    const int sync_passes = sync_groups <= 1u ? 1 : kMaxPasses;
    push.pass = 0;
    dispatch(job.pipelines.sync, sync_groups);
    for (int pass = 1; pass < sync_passes; ++pass)
    {
        computeBarrier(cmd);
        push.pass = pass - 1;
        dispatch(job.pipelines.sync_gate, 1);

        VkBufferMemoryBarrier to_indirect =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = slot.buffer[BindFlags].buffer,
            .offset = kIndirectOffset,
            .size = 12,
        };
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
            0, 0, nullptr, 1, &to_indirect, 0, nullptr);

        push.pass = pass;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, job.pipelines.sync);
        vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
        vkCmdDispatchIndirect(cmd, slot.buffer[BindFlags].buffer, kIndirectOffset);
    }
    computeBarrier(cmd);

    dispatch(job.pipelines.prefix, u32(job.segments));
    computeBarrier(cmd);

    dispatch(job.pipelines.write, (chunks * kPieces + kWriteWg - 1u) / kWriteWg);
    computeBarrier(cmd);

    const u32 dc_groups = u32(job.dc_groups);
    push.pass = 0;
    dispatch(job.pipelines.dc_local, dc_groups);
    computeBarrier(cmd);
    push.nch = s32(dc_groups);
    dispatch(job.pipelines.dc_groups, 1);
    computeBarrier(cmd);
    push.nch = job.push.nch;
    push.pass = 1;
    dispatch(job.pipelines.dc_local, dc_groups);

    VkImageMemoryBarrier to_general =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    VkMemoryBarrier coeffs_ready =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
    };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 1, &coeffs_ready, 0, nullptr, 1, &to_general);

    const u32 idct_groups = (u32(push.nmcu) + u32(push.mcus_per_group) - 1u) / u32(push.mcus_per_group);
    dispatch(job.pipelines.idct, idct_groups);

    VkImageMemoryBarrier to_read =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr, 1, &to_read);

    if (slot.queries)
    {
        vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, slot.queries, 1);
    }

    return true;
}

double JpegCompute::gpuMilliseconds(const JpegComputeJob& job) const
{
    if (job.slot < 0)
        return 0.0;

    const Slot& slot = m_slots[job.slot];
    if (!slot.queries)
        return 0.0;

    u64 ts[kQueries] = {};
    if (vkGetQueryPoolResults(m_device, slot.queries, 0, kQueries, sizeof(ts), ts, sizeof(u64),
            VK_QUERY_RESULT_64_BIT) != VK_SUCCESS)
        return 0.0;

    return double(ts[1] - ts[0]) * double(m_tick_ns) / 1e6;
}

void JpegCompute::release(JpegComputeJob& job)
{
    m_staging.release(job.staging);

    if (job.slot >= 0)
    {
        m_slots[job.slot].busy = false;
        job.slot = -1;
    }
}

size_t JpegCompute::deviceBytes() const
{
    size_t bytes = 0;
    for (const Slot& slot : m_slots)
    {
        for (VkDeviceSize c : slot.capacity)
            bytes += size_t(c);
    }
    return bytes;
}

} // namespace mango::vulkan::detail
