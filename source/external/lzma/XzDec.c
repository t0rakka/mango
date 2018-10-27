/* XzDec.c -- Xz Decode
2018-04-24 : Igor Pavlov : Public domain */

#include "Precomp.h"

// #include <stdio.h>

// #define XZ_DUMP

/* #define XZ_DUMP */

#ifdef XZ_DUMP
#include <stdio.h>
#endif

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

#define PRF_STR(s) PRF(printf("\n" s "\n"))
#define PRF_STR_INT(s, d) PRF(printf("\n" s " %d\n", (unsigned)d))

#include <stdlib.h>
#include <string.h>

#include "7zCrc.h"
#include "Alloc.h"
#include "Bra.h"
#include "CpuArch.h"
#include "Delta.h"
#include "Lzma2Dec.h"

// #define USE_SUBBLOCK

#ifdef USE_SUBBLOCK
#include "Bcj3Dec.c"
#include "SbDec.h"
#endif

#include "Xz.h"

#define XZ_CHECK_SIZE_MAX 64

#define CODER_BUF_SIZE ((size_t)1 << 17)

unsigned Xz_ReadVarInt(const Byte *p, size_t maxSize, UInt64 *value)
{
  unsigned i, limit;
  *value = 0;
  limit = (maxSize > 9) ? 9 : (unsigned)maxSize;

  for (i = 0; i < limit;)
  {
    Byte b = p[i];
    *value |= (UInt64)(b & 0x7F) << (7 * i++);
    if ((b & 0x80) == 0)
      return (b == 0 && i != 1) ? 0 : i;
  }
  return 0;
}

/* ---------- BraState ---------- */

#define BRA_BUF_SIZE (1 << 14)

typedef struct
{
  size_t bufPos;
  size_t bufConv;
  size_t bufTotal;

  int encodeMode;

  UInt32 methodId;
  UInt32 delta;
  UInt32 ip;
  UInt32 x86State;
  Byte deltaState[DELTA_STATE_SIZE];

  Byte buf[BRA_BUF_SIZE];
} CBraState;

static void BraState_Free(void *pp, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, pp);
}

static SRes BraState_SetProps(void *pp, const Byte *props, size_t propSize, ISzAllocPtr alloc)
{
  CBraState *p = ((CBraState *)pp);
  UNUSED_VAR(alloc);
  p->ip = 0;
  if (p->methodId == XZ_ID_Delta)
  {
    if (propSize != 1)
      return SZ_ERROR_UNSUPPORTED;
    p->delta = (unsigned)props[0] + 1;
  }
  else
  {
    if (propSize == 4)
    {
      UInt32 v = GetUi32(props);
      switch (p->methodId)
      {
        case XZ_ID_PPC:
        case XZ_ID_ARM:
        case XZ_ID_SPARC:
          if ((v & 3) != 0)
            return SZ_ERROR_UNSUPPORTED;
          break;
        case XZ_ID_ARMT:
          if ((v & 1) != 0)
            return SZ_ERROR_UNSUPPORTED;
          break;
        case XZ_ID_IA64:
          if ((v & 0xF) != 0)
            return SZ_ERROR_UNSUPPORTED;
          break;
      }
      p->ip = v;
    }
    else if (propSize != 0)
      return SZ_ERROR_UNSUPPORTED;
  }
  return SZ_OK;
}

static void BraState_Init(void *pp)
{
  CBraState *p = ((CBraState *)pp);
  p->bufPos = p->bufConv = p->bufTotal = 0;
  x86_Convert_Init(p->x86State);
  if (p->methodId == XZ_ID_Delta)
    Delta_Init(p->deltaState);
}


#define CASE_BRA_CONV(isa) case XZ_ID_ ## isa: size = isa ## _Convert(data, size, p->ip, p->encodeMode); break;

static SizeT BraState_Filter(void *pp, Byte *data, SizeT size)
{
  CBraState *p = ((CBraState *)pp);
  switch (p->methodId)
  {
    case XZ_ID_Delta:
      if (p->encodeMode)
        Delta_Encode(p->deltaState, p->delta, data, size);
      else
        Delta_Decode(p->deltaState, p->delta, data, size);
      break;
    case XZ_ID_X86:
      size = x86_Convert(data, size, p->ip, &p->x86State, p->encodeMode);
      break;
    CASE_BRA_CONV(PPC)
    CASE_BRA_CONV(IA64)
    CASE_BRA_CONV(ARM)
    CASE_BRA_CONV(ARMT)
    CASE_BRA_CONV(SPARC)
  }
  p->ip += (UInt32)size;
  return size;
}


static SRes BraState_Code2(void *pp,
    Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, int srcWasFinished,
    ECoderFinishMode finishMode,
    // int *wasFinished
    ECoderStatus *status)
{
  CBraState *p = ((CBraState *)pp);
  SizeT destRem = *destLen;
  SizeT srcRem = *srcLen;
  UNUSED_VAR(finishMode);

  *destLen = 0;
  *srcLen = 0;
  // *wasFinished = False;
  *status = CODER_STATUS_NOT_FINISHED;
  
  while (destRem > 0)
  {
    if (p->bufPos != p->bufConv)
    {
      size_t size = p->bufConv - p->bufPos;
      if (size > destRem)
        size = destRem;
      memcpy(dest, p->buf + p->bufPos, size);
      p->bufPos += size;
      *destLen += size;
      dest += size;
      destRem -= size;
      continue;
    }
    
    p->bufTotal -= p->bufPos;
    memmove(p->buf, p->buf + p->bufPos, p->bufTotal);
    p->bufPos = 0;
    p->bufConv = 0;
    {
      size_t size = BRA_BUF_SIZE - p->bufTotal;
      if (size > srcRem)
        size = srcRem;
      memcpy(p->buf + p->bufTotal, src, size);
      *srcLen += size;
      src += size;
      srcRem -= size;
      p->bufTotal += size;
    }
    if (p->bufTotal == 0)
      break;
    
    p->bufConv = BraState_Filter(pp, p->buf, p->bufTotal);

    if (p->bufConv == 0)
    {
      if (!srcWasFinished)
        break;
      p->bufConv = p->bufTotal;
    }
  }

  if (p->bufTotal == p->bufPos && srcRem == 0 && srcWasFinished)
  {
    *status = CODER_STATUS_FINISHED_WITH_MARK;
    // *wasFinished = 1;
  }

  return SZ_OK;
}


SRes BraState_SetFromMethod(IStateCoder *p, UInt64 id, int encodeMode, ISzAllocPtr alloc)
{
  CBraState *decoder;
  if (id < XZ_ID_Delta || id > XZ_ID_SPARC)
    return SZ_ERROR_UNSUPPORTED;
  decoder = p->p;
  if (!decoder)
  {
    decoder = (CBraState *)ISzAlloc_Alloc(alloc, sizeof(CBraState));
    if (!decoder)
      return SZ_ERROR_MEM;
    p->p = decoder;
    p->Free = BraState_Free;
    p->SetProps = BraState_SetProps;
    p->Init = BraState_Init;
    p->Code2 = BraState_Code2;
    p->Filter = BraState_Filter;
  }
  decoder->methodId = (UInt32)id;
  decoder->encodeMode = encodeMode;
  return SZ_OK;
}



/* ---------- SbState ---------- */

#ifdef USE_SUBBLOCK

static void SbState_Free(void *pp, ISzAllocPtr alloc)
{
  CSbDec *p = (CSbDec *)pp;
  SbDec_Free(p);
  ISzAlloc_Free(alloc, pp);
}

static SRes SbState_SetProps(void *pp, const Byte *props, size_t propSize, ISzAllocPtr alloc)
{
  UNUSED_VAR(pp);
  UNUSED_VAR(props);
  UNUSED_VAR(alloc);
  return (propSize == 0) ? SZ_OK : SZ_ERROR_UNSUPPORTED;
}

static void SbState_Init(void *pp)
{
  SbDec_Init((CSbDec *)pp);
}

static SRes SbState_Code2(void *pp, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    int srcWasFinished, ECoderFinishMode finishMode,
    // int *wasFinished
    ECoderStatus *status)
{
  CSbDec *p = (CSbDec *)pp;
  SRes res;
  UNUSED_VAR(srcWasFinished);
  p->dest = dest;
  p->destLen = *destLen;
  p->src = src;
  p->srcLen = *srcLen;
  p->finish = finishMode; /* change it */
  res = SbDec_Decode((CSbDec *)pp);
  *destLen -= p->destLen;
  *srcLen -= p->srcLen;
  // *wasFinished = (*destLen == 0 && *srcLen == 0); /* change it */
  *status = (*destLen == 0 && *srcLen == 0) ?
      CODER_STATUS_FINISHED_WITH_MARK :
      CODER_STATUS_NOT_FINISHED;
  return res;
}

static SRes SbState_SetFromMethod(IStateCoder *p, ISzAllocPtr alloc)
{
  CSbDec *decoder = (CSbDec *)p->p;
  if (!decoder)
  {
    decoder = (CSbDec *)ISzAlloc_Alloc(alloc, sizeof(CSbDec));
    if (!decoder)
      return SZ_ERROR_MEM;
    p->p = decoder;
    p->Free = SbState_Free;
    p->SetProps = SbState_SetProps;
    p->Init = SbState_Init;
    p->Code2 = SbState_Code2;
    p->Filter = NULL;
  }
  SbDec_Construct(decoder);
  SbDec_SetAlloc(decoder, alloc);
  return SZ_OK;
}

#endif



/* ---------- Lzma2 ---------- */

typedef struct
{
  CLzma2Dec decoder;
  Bool outBufMode;
} CLzma2Dec_Spec;


static void Lzma2State_Free(void *pp, ISzAllocPtr alloc)
{
  CLzma2Dec_Spec *p = (CLzma2Dec_Spec *)pp;
  if (p->outBufMode)
    Lzma2Dec_FreeProbs(&p->decoder, alloc);
  else
    Lzma2Dec_Free(&p->decoder, alloc);
  ISzAlloc_Free(alloc, pp);
}

static SRes Lzma2State_SetProps(void *pp, const Byte *props, size_t propSize, ISzAllocPtr alloc)
{
  if (propSize != 1)
    return SZ_ERROR_UNSUPPORTED;
  {
    CLzma2Dec_Spec *p = (CLzma2Dec_Spec *)pp;
    if (p->outBufMode)
      return Lzma2Dec_AllocateProbs(&p->decoder, props[0], alloc);
    else
      return Lzma2Dec_Allocate(&p->decoder, props[0], alloc);
  }
}

static void Lzma2State_Init(void *pp)
{
  Lzma2Dec_Init(&((CLzma2Dec_Spec *)pp)->decoder);
}


/*
  if (outBufMode), then (dest) is not used. Use NULL.
         Data is unpacked to (spec->decoder.decoder.dic) output buffer.
*/

static SRes Lzma2State_Code2(void *pp, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    int srcWasFinished, ECoderFinishMode finishMode,
    // int *wasFinished,
    ECoderStatus *status)
{
  CLzma2Dec_Spec *spec = (CLzma2Dec_Spec *)pp;
  ELzmaStatus status2;
  /* ELzmaFinishMode fm = (finishMode == LZMA_FINISH_ANY) ? LZMA_FINISH_ANY : LZMA_FINISH_END; */
  SRes res;
  UNUSED_VAR(srcWasFinished);
  if (spec->outBufMode)
  {
    SizeT dicPos = spec->decoder.decoder.dicPos;
    SizeT dicLimit = dicPos + *destLen;
    res = Lzma2Dec_DecodeToDic(&spec->decoder, dicLimit, src, srcLen, (ELzmaFinishMode)finishMode, &status2);
    *destLen = spec->decoder.decoder.dicPos - dicPos;
  }
  else
    res = Lzma2Dec_DecodeToBuf(&spec->decoder, dest, destLen, src, srcLen, (ELzmaFinishMode)finishMode, &status2);
  // *wasFinished = (status2 == LZMA_STATUS_FINISHED_WITH_MARK);
  // ECoderStatus values are identical to ELzmaStatus values of LZMA2 decoder
  *status = (ECoderStatus) status2;
  return res;
}


static SRes Lzma2State_SetFromMethod(IStateCoder *p, Byte *outBuf, size_t outBufSize, ISzAllocPtr alloc)
{
  CLzma2Dec_Spec *spec = (CLzma2Dec_Spec *)p->p;
  if (!spec)
  {
    spec = (CLzma2Dec_Spec *)ISzAlloc_Alloc(alloc, sizeof(CLzma2Dec_Spec));
    if (!spec)
      return SZ_ERROR_MEM;
    p->p = spec;
    p->Free = Lzma2State_Free;
    p->SetProps = Lzma2State_SetProps;
    p->Init = Lzma2State_Init;
    p->Code2 = Lzma2State_Code2;
    p->Filter = NULL;
    Lzma2Dec_Construct(&spec->decoder);
  }
  spec->outBufMode = False;
  if (outBuf)
  {
    spec->outBufMode = True;
    spec->decoder.decoder.dic = outBuf;
    spec->decoder.decoder.dicBufSize = outBufSize;
  }
  return SZ_OK;
}


static SRes Lzma2State_ResetOutBuf(IStateCoder *p, Byte *outBuf, size_t outBufSize)
{
  CLzma2Dec_Spec *spec = (CLzma2Dec_Spec *)p->p;
  if ((spec->outBufMode && !outBuf) || (!spec->outBufMode && outBuf))
    return SZ_ERROR_FAIL;
  if (outBuf)
  {
    spec->decoder.decoder.dic = outBuf;
    spec->decoder.decoder.dicBufSize = outBufSize;
  }
  return SZ_OK;
}



static void MixCoder_Construct(CMixCoder *p, ISzAllocPtr alloc)
{
  unsigned i;
  p->alloc = alloc;
  p->buf = NULL;
  p->numCoders = 0;
  
  p->outBufSize = 0;
  p->outBuf = NULL;
  // p->SingleBufMode = False;

  for (i = 0; i < MIXCODER_NUM_FILTERS_MAX; i++)
    p->coders[i].p = NULL;
}


static void MixCoder_Free(CMixCoder *p)
{
  unsigned i;
  p->numCoders = 0;
  for (i = 0; i < MIXCODER_NUM_FILTERS_MAX; i++)
  {
    IStateCoder *sc = &p->coders[i];
    if (sc->p)
    {
      sc->Free(sc->p, p->alloc);
      sc->p = NULL;
    }
  }
  if (p->buf)
  {
    ISzAlloc_Free(p->alloc, p->buf);
    p->buf = NULL; /* 9.31: the BUG was fixed */
  }
}

static void MixCoder_Init(CMixCoder *p)
{
  unsigned i;
  for (i = 0; i < MIXCODER_NUM_FILTERS_MAX - 1; i++)
  {
    p->size[i] = 0;
    p->pos[i] = 0;
    p->finished[i] = 0;
  }
  for (i = 0; i < p->numCoders; i++)
  {
    IStateCoder *coder = &p->coders[i];
    coder->Init(coder->p);
    p->results[i] = SZ_OK;
  }
  p->outWritten = 0;
  p->wasFinished = False;
  p->res = SZ_OK;
  p->status = CODER_STATUS_NOT_SPECIFIED;
}


static SRes MixCoder_SetFromMethod(CMixCoder *p, unsigned coderIndex, UInt64 methodId, Byte *outBuf, size_t outBufSize)
{
  IStateCoder *sc = &p->coders[coderIndex];
  p->ids[coderIndex] = methodId;
  switch (methodId)
  {
    case XZ_ID_LZMA2: return Lzma2State_SetFromMethod(sc, outBuf, outBufSize, p->alloc);
    #ifdef USE_SUBBLOCK
    case XZ_ID_Subblock: return SbState_SetFromMethod(sc, p->alloc);
    #endif
  }
  if (coderIndex == 0)
    return SZ_ERROR_UNSUPPORTED;
  return BraState_SetFromMethod(sc, methodId, 0, p->alloc);
}


static SRes MixCoder_ResetFromMethod(CMixCoder *p, unsigned coderIndex, UInt64 methodId, Byte *outBuf, size_t outBufSize)
{
  IStateCoder *sc = &p->coders[coderIndex];
  switch (methodId)
  {
    case XZ_ID_LZMA2: return Lzma2State_ResetOutBuf(sc, outBuf, outBufSize);
  }
  return SZ_ERROR_UNSUPPORTED;
}



/*
 if (destFinish) - then unpack data block is finished at (*destLen) position,
                   and we can return data that were not processed by filter

output (status) can be :
  CODER_STATUS_NOT_FINISHED
  CODER_STATUS_FINISHED_WITH_MARK
  CODER_STATUS_NEEDS_MORE_INPUT - not implemented still
*/

static SRes MixCoder_Code(CMixCoder *p,
    Byte *dest, SizeT *destLen, int destFinish,
    const Byte *src, SizeT *srcLen, int srcWasFinished,
    ECoderFinishMode finishMode)
{
  SizeT destLenOrig = *destLen;
  SizeT srcLenOrig = *srcLen;

  *destLen = 0;
  *srcLen = 0;

  if (p->wasFinished)
    return p->res;
  
  p->status = CODER_STATUS_NOT_FINISHED;

  // if (p->SingleBufMode)
  if (p->outBuf)
  {
    SRes res;
    SizeT destLen2, srcLen2;
    int wasFinished;
    
    PRF_STR("------- MixCoder Single ----------");
      
    srcLen2 = srcLenOrig;
    destLen2 = destLenOrig;
    
    {
      IStateCoder *coder = &p->coders[0];
      res = coder->Code2(coder->p, NULL, &destLen2, src, &srcLen2, srcWasFinished, finishMode,
          // &wasFinished,
          &p->status);
      wasFinished = (p->status == CODER_STATUS_FINISHED_WITH_MARK);
    }
    
    p->res = res;
    
    /*
    if (wasFinished)
      p->status = CODER_STATUS_FINISHED_WITH_MARK;
    else
    {
      if (res == SZ_OK)
        if (destLen2 != destLenOrig)
          p->status = CODER_STATUS_NEEDS_MORE_INPUT;
    }
    */

    
    *srcLen = srcLen2;
    src += srcLen2;
    p->outWritten += destLen2;
    
    if (res != SZ_OK || srcWasFinished || wasFinished)
      p->wasFinished = True;
    
    if (p->numCoders == 1)
      *destLen = destLen2;
    else if (p->wasFinished)
    {
      unsigned i;
      size_t processed = p->outWritten;
      
      for (i = 1; i < p->numCoders; i++)
      {
        IStateCoder *coder = &p->coders[i];
        processed = coder->Filter(coder->p, p->outBuf, processed);
        if (wasFinished || (destFinish && p->outWritten == destLenOrig))
          processed = p->outWritten;
        PRF_STR_INT("filter", i);
      }
      *destLen = processed;
    }
    return res;
  }

  PRF_STR("standard mix");

  if (p->numCoders != 1)
  {
    if (!p->buf)
    {
      p->buf = (Byte *)ISzAlloc_Alloc(p->alloc, CODER_BUF_SIZE * (MIXCODER_NUM_FILTERS_MAX - 1));
      if (!p->buf)
        return SZ_ERROR_MEM;
    }
    
    finishMode = CODER_FINISH_ANY;
  }

  for (;;)
  {
    Bool processed = False;
    Bool allFinished = True;
    SRes resMain = SZ_OK;
    unsigned i;

    p->status = CODER_STATUS_NOT_FINISHED;
    /*
    if (p->numCoders == 1 && *destLen == destLenOrig && finishMode == LZMA_FINISH_ANY)
      break;
    */

    for (i = 0; i < p->numCoders; i++)
    {
      SRes res;
      IStateCoder *coder = &p->coders[i];
      Byte *dest2;
      SizeT destLen2, srcLen2; // destLen2_Orig;
      const Byte *src2;
      int srcFinished2;
      int encodingWasFinished;
      ECoderStatus status2;
      
      if (i == 0)
      {
        src2 = src;
        srcLen2 = srcLenOrig - *srcLen;
        srcFinished2 = srcWasFinished;
      }
      else
      {
        size_t k = i - 1;
        src2 = p->buf + (CODER_BUF_SIZE * k) + p->pos[k];
        srcLen2 = p->size[k] - p->pos[k];
        srcFinished2 = p->finished[k];
      }
      
      if (i == p->numCoders - 1)
      {
        dest2 = dest;
        destLen2 = destLenOrig - *destLen;
      }
      else
      {
        if (p->pos[i] != p->size[i])
          continue;
        dest2 = p->buf + (CODER_BUF_SIZE * i);
        destLen2 = CODER_BUF_SIZE;
      }
      
      // destLen2_Orig = destLen2;
      
      if (p->results[i] != SZ_OK)
      {
        if (resMain == SZ_OK)
          resMain = p->results[i];
        continue;
      }

      res = coder->Code2(coder->p,
          dest2, &destLen2,
          src2, &srcLen2, srcFinished2,
          finishMode,
          // &encodingWasFinished,
          &status2);

      if (res != SZ_OK)
      {
        p->results[i] = res;
        if (resMain == SZ_OK)
          resMain = res;
      }

      encodingWasFinished = (status2 == CODER_STATUS_FINISHED_WITH_MARK);
      
      if (!encodingWasFinished)
      {
        allFinished = False;
        if (p->numCoders == 1 && res == SZ_OK)
          p->status = status2;
      }

      if (i == 0)
      {
        *srcLen += srcLen2;
        src += srcLen2;
      }
      else
        p->pos[(size_t)i - 1] += srcLen2;

      if (i == p->numCoders - 1)
      {
        *destLen += destLen2;
        dest += destLen2;
      }
      else
      {
        p->size[i] = destLen2;
        p->pos[i] = 0;
        p->finished[i] = encodingWasFinished;
      }
      
      if (destLen2 != 0 || srcLen2 != 0)
        processed = True;
    }
    
    if (!processed)
    {
      if (allFinished)
        p->status = CODER_STATUS_FINISHED_WITH_MARK;
      return resMain;
    }
  }
}


SRes Xz_ParseHeader(CXzStreamFlags *p, const Byte *buf)
{
  *p = (CXzStreamFlags)GetBe16(buf + XZ_SIG_SIZE);
  if (CrcCalc(buf + XZ_SIG_SIZE, XZ_STREAM_FLAGS_SIZE) !=
      GetUi32(buf + XZ_SIG_SIZE + XZ_STREAM_FLAGS_SIZE))
    return SZ_ERROR_NO_ARCHIVE;
  return XzFlags_IsSupported(*p) ? SZ_OK : SZ_ERROR_UNSUPPORTED;
}

static Bool Xz_CheckFooter(CXzStreamFlags flags, UInt64 indexSize, const Byte *buf)
{
  return indexSize == (((UInt64)GetUi32(buf + 4) + 1) << 2)
      && GetUi32(buf) == CrcCalc(buf + 4, 6)
      && flags == GetBe16(buf + 8)
      && buf[10] == XZ_FOOTER_SIG_0
      && buf[11] == XZ_FOOTER_SIG_1;
}

#define READ_VARINT_AND_CHECK(buf, pos, size, res) \
  { unsigned s = Xz_ReadVarInt(buf + pos, size - pos, res); \
  if (s == 0) return SZ_ERROR_ARCHIVE; pos += s; }


static Bool XzBlock_AreSupportedFilters(const CXzBlock *p)
{
  unsigned numFilters = XzBlock_GetNumFilters(p) - 1;
  unsigned i;
  {
    const CXzFilter *f = &p->filters[numFilters];
    if (f->id != XZ_ID_LZMA2 || f->propsSize != 1 || f->props[0] > 40)
      return False;
  }

  for (i = 0; i < numFilters; i++)
  {
    const CXzFilter *f = &p->filters[i];
    if (f->id == XZ_ID_Delta)
    {
      if (f->propsSize != 1)
        return False;
    }
    else if (f->id < XZ_ID_Delta
        || f->id > XZ_ID_SPARC
        || (f->propsSize != 0 && f->propsSize != 4))
      return False;
  }
  return True;
}


SRes XzBlock_Parse(CXzBlock *p, const Byte *header)
{
  unsigned pos;
  unsigned numFilters, i;
  unsigned headerSize = (unsigned)header[0] << 2;

  /* (headerSize != 0) : another code checks */

  if (CrcCalc(header, headerSize) != GetUi32(header + headerSize))
    return SZ_ERROR_ARCHIVE;

  pos = 1;
  p->flags = header[pos++];

  p->packSize = (UInt64)(Int64)-1;
  if (XzBlock_HasPackSize(p))
  {
    READ_VARINT_AND_CHECK(header, pos, headerSize, &p->packSize);
    if (p->packSize == 0 || p->packSize + headerSize >= (UInt64)1 << 63)
      return SZ_ERROR_ARCHIVE;
  }

  p->unpackSize = (UInt64)(Int64)-1;
  if (XzBlock_HasUnpackSize(p))
    READ_VARINT_AND_CHECK(header, pos, headerSize, &p->unpackSize);

  numFilters = XzBlock_GetNumFilters(p);
  for (i = 0; i < numFilters; i++)
  {
    CXzFilter *filter = p->filters + i;
    UInt64 size;
    READ_VARINT_AND_CHECK(header, pos, headerSize, &filter->id);
    READ_VARINT_AND_CHECK(header, pos, headerSize, &size);
    if (size > headerSize - pos || size > XZ_FILTER_PROPS_SIZE_MAX)
      return SZ_ERROR_ARCHIVE;
    filter->propsSize = (UInt32)size;
    memcpy(filter->props, header + pos, (size_t)size);
    pos += (unsigned)size;

    #ifdef XZ_DUMP
    printf("\nf[%u] = %2X: ", i, (unsigned)filter->id);
    {
      unsigned i;
      for (i = 0; i < size; i++)
        printf(" %2X", filter->props[i]);
    }
    #endif
  }

  if (XzBlock_HasUnsupportedFlags(p))
    return SZ_ERROR_UNSUPPORTED;

  while (pos < headerSize)
    if (header[pos++] != 0)
      return SZ_ERROR_ARCHIVE;
  return SZ_OK;
}




static SRes XzDecMix_Init(CMixCoder *p, const CXzBlock *block, Byte *outBuf, size_t outBufSize)
{
  unsigned i;
  Bool needReInit = True;
  unsigned numFilters = XzBlock_GetNumFilters(block);

  if (numFilters == p->numCoders && ((p->outBuf && outBuf) || (!p->outBuf && !outBuf)))
  {
    needReInit = False;
    for (i = 0; i < numFilters; i++)
      if (p->ids[i] != block->filters[numFilters - 1 - i].id)
      {
        needReInit = True;
        break;
      }
  }

  // p->SingleBufMode = (outBuf != NULL);
  p->outBuf = outBuf;
  p->outBufSize = outBufSize;

  // p->SingleBufMode = False;
  // outBuf = NULL;
  
  if (needReInit)
  {
    MixCoder_Free(p);
    for (i = 0; i < numFilters; i++)
    {
      RINOK(MixCoder_SetFromMethod(p, i, block->filters[numFilters - 1 - i].id, outBuf, outBufSize));
    }
    p->numCoders = numFilters;
  }
  else
  {
    RINOK(MixCoder_ResetFromMethod(p, 0, block->filters[numFilters - 1].id, outBuf, outBufSize));
  }

  for (i = 0; i < numFilters; i++)
  {
    const CXzFilter *f = &block->filters[numFilters - 1 - i];
    IStateCoder *sc = &p->coders[i];
    RINOK(sc->SetProps(sc->p, f->props, f->propsSize, p->alloc));
  }
  
  MixCoder_Init(p);
  return SZ_OK;
}



void XzUnpacker_Init(CXzUnpacker *p)
{
  p->state = XZ_STATE_STREAM_HEADER;
  p->pos = 0;
  p->numStartedStreams = 0;
  p->numFinishedStreams = 0;
  p->numTotalBlocks = 0;
  p->padSize = 0;
  p->decodeOnlyOneBlock = 0;

  p->parseMode = False;
  p->decodeToStreamSignature = False;

  // p->outBuf = NULL;
  // p->outBufSize = 0;
  p->outDataWritten = 0;
}


void XzUnpacker_SetOutBuf(CXzUnpacker *p, Byte *outBuf, size_t outBufSize)
{
  p->outBuf = outBuf;
  p->outBufSize = outBufSize;
}


void XzUnpacker_Construct(CXzUnpacker *p, ISzAllocPtr alloc)
{
  MixCoder_Construct(&p->decoder, alloc);
  p->outBuf = NULL;
  p->outBufSize = 0;
  XzUnpacker_Init(p);
}


void XzUnpacker_Free(CXzUnpacker *p)
{
  MixCoder_Free(&p->decoder);
}


void XzUnpacker_PrepareToRandomBlockDecoding(CXzUnpacker *p)
{
  p->indexSize = 0;
  p->numBlocks = 0;
  Sha256_Init(&p->sha);
  p->state = XZ_STATE_BLOCK_HEADER;
  p->pos = 0;
  p->decodeOnlyOneBlock = 1;
}


static void XzUnpacker_UpdateIndex(CXzUnpacker *p, UInt64 packSize, UInt64 unpackSize)
{
  Byte temp[32];
  unsigned num = Xz_WriteVarInt(temp, packSize);
  num += Xz_WriteVarInt(temp + num, unpackSize);
  Sha256_Update(&p->sha, temp, num);
  p->indexSize += num;
  p->numBlocks++;
}



SRes XzUnpacker_Code(CXzUnpacker *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, int srcFinished,
    ECoderFinishMode finishMode, ECoderStatus *status)
{
  SizeT destLenOrig = *destLen;
  SizeT srcLenOrig = *srcLen;
  *destLen = 0;
  *srcLen = 0;
  *status = CODER_STATUS_NOT_SPECIFIED;

  for (;;)
  {
    SizeT srcRem;

    if (p->state == XZ_STATE_BLOCK)
    {
      SizeT destLen2 = destLenOrig - *destLen;
      SizeT srcLen2 = srcLenOrig - *srcLen;
      SRes res;

      ECoderFinishMode finishMode2 = finishMode;
      Bool srcFinished2 = srcFinished;
      Bool destFinish = False;

      if (p->block.packSize != (UInt64)(Int64)-1)
      {
        UInt64 rem = p->block.packSize - p->packSize;
        if (srcLen2 >= rem)
        {
          srcFinished2 = True;
          srcLen2 = (SizeT)rem;
        }
        if (rem == 0 && p->block.unpackSize == p->unpackSize)
          return SZ_ERROR_DATA;
      }

      if (p->block.unpackSize != (UInt64)(Int64)-1)
      {
        UInt64 rem = p->block.unpackSize - p->unpackSize;
        if (destLen2 >= rem)
        {
          destFinish = True;
          finishMode2 = CODER_FINISH_END;
          destLen2 = (SizeT)rem;
        }
      }

      /*
      if (srcLen2 == 0 && destLen2 == 0)
      {
        *status = CODER_STATUS_NOT_FINISHED;
        return SZ_OK;
      }
      */
      
      {
        res = MixCoder_Code(&p->decoder,
            (p->outBuf ? NULL : dest), &destLen2, destFinish,
            src, &srcLen2, srcFinished2,
            finishMode2);
        
        *status = p->decoder.status;
        XzCheck_Update(&p->check, (p->outBuf ? p->outBuf + p->outDataWritten : dest), destLen2);
        if (!p->outBuf)
          dest += destLen2;
        p->outDataWritten += destLen2;
      }
      
      (*srcLen) += srcLen2;
      src += srcLen2;
      p->packSize += srcLen2;
      (*destLen) += destLen2;
      p->unpackSize += destLen2;

      RINOK(res);

      if (*status != CODER_STATUS_FINISHED_WITH_MARK)
      {
        if (p->block.packSize == p->packSize
            && *status == CODER_STATUS_NEEDS_MORE_INPUT)
        {
          PRF_STR("CODER_STATUS_NEEDS_MORE_INPUT");
          *status = CODER_STATUS_NOT_SPECIFIED;
          return SZ_ERROR_DATA;
        }
        
        return SZ_OK;
      }
      {
        XzUnpacker_UpdateIndex(p, XzUnpacker_GetPackSizeForIndex(p), p->unpackSize);
        p->state = XZ_STATE_BLOCK_FOOTER;
        p->pos = 0;
        p->alignPos = 0;
        *status = CODER_STATUS_NOT_SPECIFIED;

        if ((p->block.packSize != (UInt64)(Int64)-1 && p->block.packSize != p->packSize)
           || (p->block.unpackSize != (UInt64)(Int64)-1 && p->block.unpackSize != p->unpackSize))
        {
          PRF_STR("ERROR: block.size mismatch");
          return SZ_ERROR_DATA;
        }
      }
      // continue;
    }

    srcRem = srcLenOrig - *srcLen;

    // XZ_STATE_BLOCK_FOOTER can transit to XZ_STATE_BLOCK_HEADER without input bytes
    if (srcRem == 0 && p->state != XZ_STATE_BLOCK_FOOTER)
    {
      *status = CODER_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }

    switch (p->state)
    {
      case XZ_STATE_STREAM_HEADER:
      {
        if (p->pos < XZ_STREAM_HEADER_SIZE)
        {
          if (p->pos < XZ_SIG_SIZE && *src != XZ_SIG[p->pos])
            return SZ_ERROR_NO_ARCHIVE;
          if (p->decodeToStreamSignature)
            return SZ_OK;
          p->buf[p->pos++] = *src++;
          (*srcLen)++;
        }
        else
        {
          RINOK(Xz_ParseHeader(&p->streamFlags, p->buf));
          p->numStartedStreams++;
          p->indexSize = 0;
          p->numBlocks = 0;
          Sha256_Init(&p->sha);
          p->state = XZ_STATE_BLOCK_HEADER;
          p->pos = 0;
        }
        break;
      }

      case XZ_STATE_BLOCK_HEADER:
      {
        if (p->pos == 0)
        {
          p->buf[p->pos++] = *src++;
          (*srcLen)++;
          if (p->buf[0] == 0)
          {
            if (p->decodeOnlyOneBlock)
              return SZ_ERROR_DATA;
            p->indexPreSize = 1 + Xz_WriteVarInt(p->buf + 1, p->numBlocks);
            p->indexPos = p->indexPreSize;
            p->indexSize += p->indexPreSize;
            Sha256_Final(&p->sha, p->shaDigest);
            Sha256_Init(&p->sha);
            p->crc = CrcUpdate(CRC_INIT_VAL, p->buf, p->indexPreSize);
            p->state = XZ_STATE_STREAM_INDEX;
            break;
          }
          p->blockHeaderSize = ((UInt32)p->buf[0] << 2) + 4;
          break;
        }
        
        if (p->pos != p->blockHeaderSize)
        {
          UInt32 cur = p->blockHeaderSize - p->pos;
          if (cur > srcRem)
            cur = (UInt32)srcRem;
          memcpy(p->buf + p->pos, src, cur);
          p->pos += cur;
          (*srcLen) += cur;
          src += cur;
        }
        else
        {
          RINOK(XzBlock_Parse(&p->block, p->buf));
          if (!XzBlock_AreSupportedFilters(&p->block))
            return SZ_ERROR_UNSUPPORTED;
          p->numTotalBlocks++;
          p->state = XZ_STATE_BLOCK;
          p->packSize = 0;
          p->unpackSize = 0;
          XzCheck_Init(&p->check, XzFlags_GetCheckType(p->streamFlags));
          if (p->parseMode)
          {
            p->headerParsedOk = True;
            return SZ_OK;
          }
          RINOK(XzDecMix_Init(&p->decoder, &p->block, p->outBuf, p->outBufSize));
        }
        break;
      }

      case XZ_STATE_BLOCK_FOOTER:
      {
        if ((((unsigned)p->packSize + p->alignPos) & 3) != 0)
        {
          if (srcRem == 0)
          {
            *status = CODER_STATUS_NEEDS_MORE_INPUT;
            return SZ_OK;
          }
          (*srcLen)++;
          p->alignPos++;
          if (*src++ != 0)
            return SZ_ERROR_CRC;
        }
        else
        {
          UInt32 checkSize = XzFlags_GetCheckSize(p->streamFlags);
          UInt32 cur = checkSize - p->pos;
          if (cur != 0)
          {
            if (srcRem == 0)
            {
              *status = CODER_STATUS_NEEDS_MORE_INPUT;
              return SZ_OK;
            }
            if (cur > srcRem)
              cur = (UInt32)srcRem;
            memcpy(p->buf + p->pos, src, cur);
            p->pos += cur;
            (*srcLen) += cur;
            src += cur;
            if (checkSize != p->pos)
              break;
          }
          {
            Byte digest[XZ_CHECK_SIZE_MAX];
            p->state = XZ_STATE_BLOCK_HEADER;
            p->pos = 0;
            if (XzCheck_Final(&p->check, digest) && memcmp(digest, p->buf, checkSize) != 0)
              return SZ_ERROR_CRC;
            if (p->decodeOnlyOneBlock)
            {
              *status = CODER_STATUS_FINISHED_WITH_MARK;
              return SZ_OK;
            }
          }
        }
        break;
      }

      case XZ_STATE_STREAM_INDEX:
      {
        if (p->pos < p->indexPreSize)
        {
          (*srcLen)++;
          if (*src++ != p->buf[p->pos++])
            return SZ_ERROR_CRC;
        }
        else
        {
          if (p->indexPos < p->indexSize)
          {
            UInt64 cur = p->indexSize - p->indexPos;
            if (srcRem > cur)
              srcRem = (SizeT)cur;
            p->crc = CrcUpdate(p->crc, src, srcRem);
            Sha256_Update(&p->sha, src, srcRem);
            (*srcLen) += srcRem;
            src += srcRem;
            p->indexPos += srcRem;
          }
          else if ((p->indexPos & 3) != 0)
          {
            Byte b = *src++;
            p->crc = CRC_UPDATE_BYTE(p->crc, b);
            (*srcLen)++;
            p->indexPos++;
            p->indexSize++;
            if (b != 0)
              return SZ_ERROR_CRC;
          }
          else
          {
            Byte digest[SHA256_DIGEST_SIZE];
            p->state = XZ_STATE_STREAM_INDEX_CRC;
            p->indexSize += 4;
            p->pos = 0;
            Sha256_Final(&p->sha, digest);
            if (memcmp(digest, p->shaDigest, SHA256_DIGEST_SIZE) != 0)
              return SZ_ERROR_CRC;
          }
        }
        break;
      }

      case XZ_STATE_STREAM_INDEX_CRC:
      {
        if (p->pos < 4)
        {
          (*srcLen)++;
          p->buf[p->pos++] = *src++;
        }
        else
        {
          p->state = XZ_STATE_STREAM_FOOTER;
          p->pos = 0;
          if (CRC_GET_DIGEST(p->crc) != GetUi32(p->buf))
            return SZ_ERROR_CRC;
        }
        break;
      }

      case XZ_STATE_STREAM_FOOTER:
      {
        UInt32 cur = XZ_STREAM_FOOTER_SIZE - p->pos;
        if (cur > srcRem)
          cur = (UInt32)srcRem;
        memcpy(p->buf + p->pos, src, cur);
        p->pos += cur;
        (*srcLen) += cur;
        src += cur;
        if (p->pos == XZ_STREAM_FOOTER_SIZE)
        {
          p->state = XZ_STATE_STREAM_PADDING;
          p->numFinishedStreams++;
          p->padSize = 0;
          if (!Xz_CheckFooter(p->streamFlags, p->indexSize, p->buf))
            return SZ_ERROR_CRC;
        }
        break;
      }

      case XZ_STATE_STREAM_PADDING:
      {
        if (*src != 0)
        {
          if (((UInt32)p->padSize & 3) != 0)
            return SZ_ERROR_NO_ARCHIVE;
          p->pos = 0;
          p->state = XZ_STATE_STREAM_HEADER;
        }
        else
        {
          (*srcLen)++;
          src++;
          p->padSize++;
        }
        break;
      }
      
      case XZ_STATE_BLOCK: break; /* to disable GCC warning */
    }
  }
  /*
  if (p->state == XZ_STATE_FINISHED)
    *status = CODER_STATUS_FINISHED_WITH_MARK;
  return SZ_OK;
  */
}


SRes XzUnpacker_CodeFull(CXzUnpacker *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen,
    ECoderFinishMode finishMode, ECoderStatus *status)
{
  XzUnpacker_Init(p);
  XzUnpacker_SetOutBuf(p, dest, *destLen);

  return XzUnpacker_Code(p,
      NULL, destLen,
      src, srcLen, True,
      finishMode, status);
}


Bool XzUnpacker_IsBlockFinished(const CXzUnpacker *p)
{
  return (p->state == XZ_STATE_BLOCK_HEADER) && (p->pos == 0);
}

Bool XzUnpacker_IsStreamWasFinished(const CXzUnpacker *p)
{
  return (p->state == XZ_STATE_STREAM_PADDING) && (((UInt32)p->padSize & 3) == 0);
}

UInt64 XzUnpacker_GetExtraSize(const CXzUnpacker *p)
{
  UInt64 num = 0;
  if (p->state == XZ_STATE_STREAM_PADDING)
    num = p->padSize;
  else if (p->state == XZ_STATE_STREAM_HEADER)
    num = p->padSize + p->pos;
  return num;
}





void XzDecMtProps_Init(CXzDecMtProps *p)
{
  p->inBufSize_ST = 1 << 18;
  p->outStep_ST = 1 << 20;
  p->ignoreErrors = False;
}




/* ---------- CXzDecMt ---------- */

typedef struct
{
  CAlignOffsetAlloc alignOffsetAlloc;
  ISzAllocPtr allocMid;

  CXzDecMtProps props;
  size_t unpackBlockMaxSize;
  
  ISeqInStream *inStream;
  ISeqOutStream *outStream;
  ICompressProgress *progress;
  // CXzStatInfo *stat;

  Bool finishMode;
  Bool outSize_Defined;
  UInt64 outSize;

  UInt64 outProcessed;
  UInt64 inProcessed;
  UInt64 readProcessed;
  Bool readWasFinished;
  SRes readRes;
  SRes writeRes;

  Byte *outBuf;
  size_t outBufSize;
  Byte *inBuf;
  size_t inBufSize;
  Bool dec_created;
  CXzUnpacker dec;

  ECoderStatus status;
  SRes codeRes;

} CXzDecMt;



CXzDecMtHandle XzDecMt_Create(ISzAllocPtr alloc, ISzAllocPtr allocMid)
{
  CXzDecMt *p = (CXzDecMt *)ISzAlloc_Alloc(alloc, sizeof(CXzDecMt));
  if (!p)
    return NULL;
  
  AlignOffsetAlloc_CreateVTable(&p->alignOffsetAlloc);
  p->alignOffsetAlloc.baseAlloc = alloc;
  p->alignOffsetAlloc.numAlignBits = 7;
  p->alignOffsetAlloc.offset = 0;

  p->allocMid = allocMid;

  p->outBuf = NULL;
  p->outBufSize = 0;
  p->inBuf = NULL;
  p->inBufSize = 0;
  p->dec_created = False;

  p->unpackBlockMaxSize = 0;

  XzDecMtProps_Init(&p->props);

  return p;
}


static void XzDecMt_FreeSt(CXzDecMt *p)
{
  if (p->dec_created)
  {
    XzUnpacker_Free(&p->dec);
    p->dec_created = False;
  }
  
  if (p->outBuf)
  {
    ISzAlloc_Free(p->allocMid, p->outBuf);
    p->outBuf = NULL;
  }
  p->outBufSize = 0;
  
  if (p->inBuf)
  {
    ISzAlloc_Free(p->allocMid, p->inBuf);
    p->inBuf = NULL;
  }
  p->inBufSize = 0;
}


void XzDecMt_Destroy(CXzDecMtHandle pp)
{
  CXzDecMt *p = (CXzDecMt *)pp;

  XzDecMt_FreeSt(p);

  ISzAlloc_Free(p->alignOffsetAlloc.baseAlloc, pp);
}




void XzStatInfo_Clear(CXzStatInfo *p)
{
  p->InSize = 0;
  p->OutSize = 0;
  
  p->NumStreams = 0;
  p->NumBlocks = 0;
  
  p->UnpackSize_Defined = False;
  
  p->NumStreams_Defined = False;
  p->NumBlocks_Defined = False;
  
  // p->IsArc = False;
  // p->UnexpectedEnd = False;
  // p->Unsupported = False;
  // p->HeadersError = False;
  // p->DataError = False;
  // p->CrcError = False;

  p->DataAfterEnd = False;
  p->DecodingTruncated = False;
  
  p->DecodeRes = SZ_OK;
  p->ReadRes = SZ_OK;
  p->ProgressRes = SZ_OK;

  p->CombinedRes = SZ_OK;
  p->CombinedRes_Type = SZ_OK;
}



static SRes XzDecMt_Decode_ST(CXzDecMt *p
    , CXzStatInfo *stat)
{
  size_t outPos;
  size_t inPos, inLim;
  const Byte *inData;
  UInt64 inPrev, outPrev;

  CXzUnpacker *dec;

  if (!p->outBuf || p->outBufSize != p->props.outStep_ST)
  {
    ISzAlloc_Free(p->allocMid, p->outBuf);
    p->outBufSize = 0;
    p->outBuf = (Byte *)ISzAlloc_Alloc(p->allocMid, p->props.outStep_ST);
    if (!p->outBuf)
      return SZ_ERROR_MEM;
    p->outBufSize = p->props.outStep_ST;
  }

  if (!p->inBuf || p->inBufSize != p->props.inBufSize_ST)
  {
    ISzAlloc_Free(p->allocMid, p->inBuf);
    p->inBufSize = 0;
    p->inBuf = (Byte *)ISzAlloc_Alloc(p->allocMid, p->props.inBufSize_ST);
    if (!p->inBuf)
      return SZ_ERROR_MEM;
    p->inBufSize = p->props.inBufSize_ST;
  }

  dec = &p->dec;
  dec->decodeToStreamSignature = False;
  // dec->decodeOnlyOneBlock = False;

  XzUnpacker_SetOutBuf(dec, NULL, 0);

  inPrev = p->inProcessed;
  outPrev = p->outProcessed;

  inPos = 0;
  inLim = 0;
  inData = NULL;
  outPos = 0;

  for (;;)
  {
    SizeT outSize;
    Bool finished;
    ECoderFinishMode finishMode;
    SizeT inProcessed;
    ECoderStatus status;
    SRes res;

    SizeT outProcessed;



    if (inPos == inLim)
    {
      if (!p->readWasFinished)
      {
        inPos = 0;
        inLim = p->inBufSize;
        inData = p->inBuf;
        p->readRes = ISeqInStream_Read(p->inStream, (void *)inData, &inLim);
        p->readProcessed += inLim;
        if (inLim == 0 || p->readRes != SZ_OK)
          p->readWasFinished = True;
      }
    }

    outSize = p->props.outStep_ST - outPos;

    finishMode = CODER_FINISH_ANY;
    if (p->outSize_Defined)
    {
      const UInt64 rem = p->outSize - p->outProcessed;
      if (outSize >= rem)
      {
        outSize = (SizeT)rem;
        if (p->finishMode)
          finishMode = CODER_FINISH_END;
      }
    }

    inProcessed = inLim - inPos;
    outProcessed = outSize;

    res = XzUnpacker_Code(dec, p->outBuf + outPos, &outProcessed,
        inData + inPos, &inProcessed,
        (inPos == inLim), // srcFinished
        finishMode, &status);

    p->codeRes = res;
    p->status = status;

    inPos += inProcessed;
    outPos += outProcessed;
    p->inProcessed += inProcessed;
    p->outProcessed += outProcessed;

    finished = ((inProcessed == 0 && outProcessed == 0) || res != SZ_OK);

    if (finished || outProcessed >= outSize)
      if (outPos != 0)
      {
        size_t written = ISeqOutStream_Write(p->outStream, p->outBuf, outPos);
        p->outProcessed += written;
        if (written != outPos)
        {
          stat->CombinedRes_Type = SZ_ERROR_WRITE;
          return SZ_ERROR_WRITE;
        }
        outPos = 0;
      }

    if (p->progress && res == SZ_OK)
    {
      UInt64 inDelta = p->inProcessed - inPrev;
      UInt64 outDelta = p->outProcessed - outPrev;
      if (inDelta >= (1 << 22) || outDelta >= (1 << 22))
      {
        res = ICompressProgress_Progress(p->progress, p->inProcessed, p->outProcessed);
        if (res != SZ_OK)
        {
          stat->CombinedRes_Type = SZ_ERROR_PROGRESS;
          stat->ProgressRes = res;
          return res;
        }
        inPrev = p->inProcessed;
        outPrev = p->outProcessed;
      }
    }

    if (finished)
      return res;
  }
}

static SRes XzStatInfo_SetStat(const CXzUnpacker *dec,
    int finishMode,
    UInt64 readProcessed, UInt64 inProcessed,
    SRes res, ECoderStatus status,
    Bool decodingTruncated,
    CXzStatInfo *stat)
{
  UInt64 extraSize;
  
  stat->DecodingTruncated = (Byte)(decodingTruncated ? 1 : 0);
  stat->InSize = inProcessed;
  stat->NumStreams = dec->numStartedStreams;
  stat->NumBlocks = dec->numTotalBlocks;
  
  stat->UnpackSize_Defined = True;
  stat->NumStreams_Defined = True;
  stat->NumBlocks_Defined = True;
  
  extraSize = XzUnpacker_GetExtraSize(dec);
  
  if (res == SZ_OK)
  {
    if (status == CODER_STATUS_NEEDS_MORE_INPUT)
    {
      // CODER_STATUS_NEEDS_MORE_INPUT is expected status for correct xz streams
      extraSize = 0;
      if (!XzUnpacker_IsStreamWasFinished(dec))
        res = SZ_ERROR_INPUT_EOF;
    }
    else if (!decodingTruncated || finishMode) // (status == CODER_STATUS_NOT_FINISHED)
      res = SZ_ERROR_DATA;
  }
  else if (res == SZ_ERROR_NO_ARCHIVE)
  {
    /*
    SZ_ERROR_NO_ARCHIVE is possible for 2 states:
      XZ_STATE_STREAM_HEADER  - if bad signature or bad CRC
      XZ_STATE_STREAM_PADDING - if non-zero padding data
    extraSize / inProcessed don't include "bad" byte
    */
    if (inProcessed != extraSize) // if good streams before error
      if (extraSize != 0 || readProcessed != inProcessed)
      {
        stat->DataAfterEnd = True;
        // there is some good xz stream before. So we set SZ_OK
        res = SZ_OK;
      }
  }
  
  stat->DecodeRes = res;

  stat->InSize -= extraSize;
  return res;
}


SRes XzDecMt_Decode(CXzDecMtHandle pp,
    const CXzDecMtProps *props,
    const UInt64 *outDataSize, int finishMode,
    ISeqOutStream *outStream,
    // Byte *outBuf, size_t *outBufSize,
    ISeqInStream *inStream,
    // const Byte *inData, size_t inDataSize,
    CXzStatInfo *stat,
    int *isMT,
    ICompressProgress *progress)
{
  CXzDecMt *p = (CXzDecMt *)pp;
  
  XzStatInfo_Clear(stat);

  p->props = *props;

  p->inStream = inStream;
  p->outStream = outStream;
  p->progress = progress;
  // p->stat = stat;

  p->outSize = 0;
  p->outSize_Defined = False;
  if (outDataSize)
  {
    p->outSize_Defined = True;
    p->outSize = *outDataSize;
  }

  p->finishMode = finishMode;

  // p->outSize = 457; p->outSize_Defined = True; p->finishMode = False; // for test

  p->writeRes = SZ_OK;
  p->outProcessed = 0;
  p->inProcessed = 0;
  p->readProcessed = 0;
  p->readWasFinished = False;

  p->codeRes = 0;
  p->status = CODER_STATUS_NOT_SPECIFIED;

  if (!p->dec_created)
  {
    XzUnpacker_Construct(&p->dec, &p->alignOffsetAlloc.vt);
    p->dec_created = True;
  }
  XzUnpacker_Init(&p->dec);
  

  *isMT = False;

  
  *isMT = False;

  {
    SRes res = XzDecMt_Decode_ST(p
        , stat
        );

    XzStatInfo_SetStat(&p->dec,
        p->finishMode,
        p->readProcessed, p->inProcessed,
        p->codeRes, p->status,
        False, // truncated
        stat);

    if (res == SZ_OK)
    {
      /*
      if (p->writeRes != SZ_OK)
      {
        res = p->writeRes;
        stat->CombinedRes_Type = SZ_ERROR_WRITE;
      }
      else
      */
      if (p->readRes != SZ_OK && p->inProcessed == p->readProcessed)
      {
        res = p->readRes;
        stat->ReadRes = res;
        stat->CombinedRes_Type = SZ_ERROR_READ;
      }
    }

    stat->CombinedRes = res;
    if (stat->CombinedRes_Type == SZ_OK)
      stat->CombinedRes_Type = res;
    return res;
  }
}
