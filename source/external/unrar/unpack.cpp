#include "rar.hpp"

/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2000                                                               *
 *  Contents: memory allocation routines                                    *
 ****************************************************************************/

SubAllocator::SubAllocator()
{
  Clean();
}


void SubAllocator::Clean()
{
  SubAllocatorSize=0;
}


inline void SubAllocator::InsertNode(void* p,int indx) 
{
  ((RAR_NODE*) p)->next=FreeList[indx].next;
  FreeList[indx].next=(RAR_NODE*) p;
}


inline void* SubAllocator::RemoveNode(int indx) 
{
  RAR_NODE* RetVal=FreeList[indx].next;
  FreeList[indx].next=RetVal->next;
  return RetVal;
}


inline uint SubAllocator::U2B(int NU) 
{ 
  // We calculate the size of units in bytes based on real UNIT_SIZE.
  // In original implementation it was 8*NU+4*NU.
  return UNIT_SIZE*NU;
}



// Calculate RAR_MEM_BLK+Items address. Real RAR_MEM_BLK size must be
// equal to UNIT_SIZE, so we cannot just add Items to RAR_MEM_BLK address.
inline RAR_MEM_BLK* SubAllocator::MBPtr(RAR_MEM_BLK *BasePtr,int Items)
{
  return((RAR_MEM_BLK*)( ((byte *)(BasePtr))+U2B(Items) ));
}


inline void SubAllocator::SplitBlock(void* pv,int OldIndx,int NewIndx)
{
  int i, UDiff=Indx2Units[OldIndx]-Indx2Units[NewIndx];
  byte* p=((byte*) pv)+U2B(Indx2Units[NewIndx]);
  if (Indx2Units[i=Units2Indx[UDiff-1]] != UDiff) 
  {
    InsertNode(p,--i);
    p += U2B(i=Indx2Units[i]);
    UDiff -= i;
  }
  InsertNode(p,Units2Indx[UDiff-1]);
}




void SubAllocator::StopSubAllocator()
{
  if ( SubAllocatorSize ) 
  {
    SubAllocatorSize=0;
    free(HeapStart);
  }
}


bool SubAllocator::StartSubAllocator(int SASize)
{
  long t=SASize << 20;
  if (SubAllocatorSize == t)
    return true;
  StopSubAllocator();

  // Original algorithm expects FIXED_UNIT_SIZE, but actual structure size
  // can be larger. So let's recalculate the allocated size and add two more
  // units: one as reserve for HeapEnd overflow checks and another
  // to provide the space to correctly align UnitsStart.
  long AllocSize=t/FIXED_UNIT_SIZE*UNIT_SIZE+2*UNIT_SIZE;
  if ((HeapStart=(byte *)malloc(AllocSize)) == NULL)
  {
      // TODO: memory error
    return false;
  }

  // HeapEnd did not present in original algorithm. We added it to control
  // invalid memory access attempts when processing corrupt archived data.
  HeapEnd=HeapStart+AllocSize-UNIT_SIZE;

  SubAllocatorSize=t;
  return true;
}


void SubAllocator::InitSubAllocator()
{
  int i, k;
  memset(FreeList,0,sizeof(FreeList));
  pText=HeapStart;

  // Original algorithm operates with 12 byte FIXED_UNIT_SIZE, but actual
  // size of RAR_MEM_BLK and PPM_CONTEXT structures can exceed this value
  // because of alignment and larger pointer fields size.
  // So we define UNIT_SIZE for this larger size and adjust memory
  // pointers accordingly.

  // Size2 is (HiUnit-LoUnit) memory area size to allocate as originally
  // supposed by compression algorithm. It is 7/8 of total allocated size.
  uint Size2=uint(FIXED_UNIT_SIZE*(SubAllocatorSize/8/FIXED_UNIT_SIZE*7));

  // RealSize2 is the real adjusted size of (HiUnit-LoUnit) memory taking
  // into account that our UNIT_SIZE can be larger than FIXED_UNIT_SIZE.
  uint RealSize2=Size2/FIXED_UNIT_SIZE*UNIT_SIZE;

  // Size1 is the size of memory area from HeapStart to FakeUnitsStart
  // as originally supposed by compression algorithm. This area can contain
  // different data types, both single symbols and structures.
  long Size1=SubAllocatorSize-Size2;

  // Real size of this area. We correct it according to UNIT_SIZE vs
  // FIXED_UNIT_SIZE difference. Also we add one more UNIT_SIZE
  // to compensate a possible reminder from Size1/FIXED_UNIT_SIZE,
  // which would be lost otherwise. We add UNIT_SIZE instead of 
  // this Size1%FIXED_UNIT_SIZE reminder, because it allows to align
  // UnitsStart easily and adding more than reminder is ok for algorithm.
  uint RealSize1=uint(Size1/FIXED_UNIT_SIZE*UNIT_SIZE+UNIT_SIZE);

  // RealSize1 must be divided by UNIT_SIZE without a reminder, so UnitsStart
  // is aligned to UNIT_SIZE. It is important for those architectures,
  // where a proper memory alignment is mandatory. Since we produce RealSize1
  // multiplying by UNIT_SIZE, this condition is always true. So LoUnit,
  // UnitsStart, HeapStart are properly aligned,
  LoUnit=UnitsStart=HeapStart+RealSize1;

  // When we reach FakeUnitsStart, we restart the model. It is where
  // the original algorithm expected to see UnitsStart. Real UnitsStart
  // can have a larger value.
  FakeUnitsStart=HeapStart+Size1;

  HiUnit=LoUnit+RealSize2;
  for (i=0,k=1;i < N1     ;i++,k += 1)
    Indx2Units[i]=k;
  for (k++;i < N1+N2      ;i++,k += 2)
    Indx2Units[i]=k;
  for (k++;i < N1+N2+N3   ;i++,k += 3)
    Indx2Units[i]=k;
  for (k++;i < N1+N2+N3+N4;i++,k += 4)
    Indx2Units[i]=k;
  for (GlueCount=k=i=0;k < 128;k++)
  {
    i += (Indx2Units[i] < k+1);
    Units2Indx[k]=i;
  }
}


inline void SubAllocator::GlueFreeBlocks()
{
  RAR_MEM_BLK s0, * p, * p1;
  int i, k, sz;
  if (LoUnit != HiUnit)
    *LoUnit=0;
  for (i=0, s0.next=s0.prev=&s0;i < N_INDEXES;i++)
    while ( FreeList[i].next )
    {
      p=(RAR_MEM_BLK*)RemoveNode(i);
      p->insertAt(&s0);
      p->Stamp=0xFFFF;
      p->NU=Indx2Units[i];
    }
  for (p=s0.next;p != &s0;p=p->next)
    while ((p1=MBPtr(p,p->NU))->Stamp == 0xFFFF && int(p->NU)+p1->NU < 0x10000)
    {
      p1->remove();
      p->NU += p1->NU;
    }
  while ((p=s0.next) != &s0)
  {
    for (p->remove(), sz=p->NU;sz > 128;sz -= 128, p=MBPtr(p,128))
      InsertNode(p,N_INDEXES-1);
    if (Indx2Units[i=Units2Indx[sz-1]] != sz)
    {
      k=sz-Indx2Units[--i];
      InsertNode(MBPtr(p,sz-k),k-1);
    }
    InsertNode(p,i);
  }
}

void* SubAllocator::AllocUnitsRare(int indx)
{
  if ( !GlueCount )
  {
    GlueCount = 255;
    GlueFreeBlocks();
    if ( FreeList[indx].next )
      return RemoveNode(indx);
  }
  int i=indx;
  do
  {
    if (++i == N_INDEXES)
    {
      GlueCount--;
      i=U2B(Indx2Units[indx]);
      int j=FIXED_UNIT_SIZE*Indx2Units[indx];
      if (FakeUnitsStart-pText > j)
      {
        FakeUnitsStart-=j;
        UnitsStart -= i;
        return(UnitsStart);
      }
      return(NULL);
    }
  } while ( !FreeList[i].next );
  void* RetVal=RemoveNode(i);
  SplitBlock(RetVal,i,indx);
  return RetVal;
}


inline void* SubAllocator::AllocUnits(int NU)
{
  int indx=Units2Indx[NU-1];
  if ( FreeList[indx].next )
    return RemoveNode(indx);
  void* RetVal=LoUnit;
  LoUnit += U2B(Indx2Units[indx]);
  if (LoUnit <= HiUnit)
    return RetVal;
  LoUnit -= U2B(Indx2Units[indx]);
  return AllocUnitsRare(indx);
}


void* SubAllocator::AllocContext()
{
  if (HiUnit != LoUnit)
    return (HiUnit -= UNIT_SIZE);
  if ( FreeList->next )
    return RemoveNode(0);
  return AllocUnitsRare(0);
}


void* SubAllocator::ExpandUnits(void* OldPtr,int OldNU)
{
  int i0=Units2Indx[OldNU-1], i1=Units2Indx[OldNU-1+1];
  if (i0 == i1)
    return OldPtr;
  void* ptr=AllocUnits(OldNU+1);
  if ( ptr ) 
  {
    memcpy(ptr,OldPtr,U2B(OldNU));
    InsertNode(OldPtr,i0);
  }
  return ptr;
}


void* SubAllocator::ShrinkUnits(void* OldPtr,int OldNU,int NewNU)
{
  int i0=Units2Indx[OldNU-1], i1=Units2Indx[NewNU-1];
  if (i0 == i1)
    return OldPtr;
  if ( FreeList[i1].next )
  {
    void* ptr=RemoveNode(i1);
    memcpy(ptr,OldPtr,U2B(NewNU));
    InsertNode(OldPtr,i0);
    return ptr;
  } 
  else 
  {
    SplitBlock(OldPtr,i0,i1);
    return OldPtr;
  }
}


void SubAllocator::FreeUnits(void* ptr,int OldNU)
{
  InsertNode(ptr,Units2Indx[OldNU-1]);
}


// ------------------------------------------------------------------------------
// unpack15
// ------------------------------------------------------------------------------

#define STARTL1  2
static unsigned int DecL1[]={0x8000,0xa000,0xc000,0xd000,0xe000,0xea00,
                             0xee00,0xf000,0xf200,0xf200,0xffff};
static unsigned int PosL1[]={0,0,0,2,3,5,7,11,16,20,24,32,32};

#define STARTL2  3
static unsigned int DecL2[]={0xa000,0xc000,0xd000,0xe000,0xea00,0xee00,
                             0xf000,0xf200,0xf240,0xffff};
static unsigned int PosL2[]={0,0,0,0,5,7,9,13,18,22,26,34,36};

#define STARTHF0  4
static unsigned int DecHf0[]={0x8000,0xc000,0xe000,0xf200,0xf200,0xf200,
                              0xf200,0xf200,0xffff};
static unsigned int PosHf0[]={0,0,0,0,0,8,16,24,33,33,33,33,33};


#define STARTHF1  5
static unsigned int DecHf1[]={0x2000,0xc000,0xe000,0xf000,0xf200,0xf200,
                              0xf7e0,0xffff};
static unsigned int PosHf1[]={0,0,0,0,0,0,4,44,60,76,80,80,127};


#define STARTHF2  5
static unsigned int DecHf2[]={0x1000,0x2400,0x8000,0xc000,0xfa00,0xffff,
                              0xffff,0xffff};
static unsigned int PosHf2[]={0,0,0,0,0,0,2,7,53,117,233,0,0};


#define STARTHF3  6
static unsigned int DecHf3[]={0x800,0x2400,0xee00,0xfe80,0xffff,0xffff,
                              0xffff};
static unsigned int PosHf3[]={0,0,0,0,0,0,0,2,16,218,251,0,0};


#define STARTHF4  8
static unsigned int DecHf4[]={0xff00,0xffff,0xffff,0xffff,0xffff,0xffff};
static unsigned int PosHf4[]={0,0,0,0,0,0,0,0,0,255,0,0,0};


void Unpack::Unpack15(bool Solid)
{
  {
    UnpInitData(Solid);
    OldUnpInitData(Solid);
    UnpReadBuf();
    if (!Solid)
    {
      InitHuff();
      UnpPtr=0;
    }
    else
      UnpPtr=WrPtr;
    --DestUnpSize;
  }

  if (DestUnpSize>=0)
  {
    GetFlagsBuf();
    FlagsCnt=8;
  }

  while (DestUnpSize>=0)
  {
    UnpPtr&=MAXWINMASK;

    if (InAddr>ReadTop-30 && !UnpReadBuf())
      break;
    if (((WrPtr-UnpPtr) & MAXWINMASK)<270 && WrPtr!=UnpPtr)
    {
      OldUnpWriteBuf();
    }
    if (StMode)
    {
      HuffDecode();
      continue;
    }

    if (--FlagsCnt < 0)
    {
      GetFlagsBuf();
      FlagsCnt=7;
    }

    if (FlagBuf & 0x80)
    {
      FlagBuf<<=1;
      if (Nlzb > Nhfb)
        LongLZ();
      else
        HuffDecode();
    }
    else
    {
      FlagBuf<<=1;
      if (--FlagsCnt < 0)
      {
        GetFlagsBuf();
        FlagsCnt=7;
      }
      if (FlagBuf & 0x80)
      {
        FlagBuf<<=1;
        if (Nlzb > Nhfb)
          HuffDecode();
        else
          LongLZ();
      }
      else
      {
        FlagBuf<<=1;
        ShortLZ();
      }
    }
  }
  OldUnpWriteBuf();
}


void Unpack::OldUnpWriteBuf()
{
  if (UnpPtr!=WrPtr)
    UnpSomeRead=true;
  if (UnpPtr<WrPtr)
  {
    UnpIO->UnpWrite(&Window[WrPtr],-(int)WrPtr & MAXWINMASK);
    UnpIO->UnpWrite(Window,UnpPtr);
    UnpAllBuf=true;
  }
  else
    UnpIO->UnpWrite(&Window[WrPtr],UnpPtr-WrPtr);
  WrPtr=UnpPtr;
}


#define GetShortLen1(pos) ((pos)==1 ? Buf60+3:ShortLen1[pos])
#define GetShortLen2(pos) ((pos)==3 ? Buf60+3:ShortLen2[pos])

void Unpack::ShortLZ()
{
  static unsigned int ShortLen1[]={1,3,4,4,5,6,7,8,8,4,4,5,6,6,4,0};
  static unsigned int ShortXor1[]={0,0xa0,0xd0,0xe0,0xf0,0xf8,0xfc,0xfe,
                                   0xff,0xc0,0x80,0x90,0x98,0x9c,0xb0};
  static unsigned int ShortLen2[]={2,3,3,3,4,4,5,6,6,4,4,5,6,6,4,0};
  static unsigned int ShortXor2[]={0,0x40,0x60,0xa0,0xd0,0xe0,0xf0,0xf8,
                                   0xfc,0xc0,0x80,0x90,0x98,0x9c,0xb0};


  unsigned int Length,SaveLength;
  unsigned int LastDistance;
  unsigned int Distance;
  int DistancePlace;
  NumHuf=0;

  unsigned int BitField=fgetbits();
  if (LCount==2)
  {
    faddbits(1);
    if (BitField >= 0x8000)
    {
      OldCopyString((unsigned int)LastDist,LastLength);
      return;
    }
    BitField <<= 1;
    LCount=0;
  }

  BitField>>=8;

//  not thread safe, replaced by GetShortLen1 and GetShortLen2 macro
//  ShortLen1[1]=ShortLen2[3]=Buf60+3;

  if (AvrLn1<37)
  {
    for (Length=0;;Length++)
      if (((BitField^ShortXor1[Length]) & (~(0xff>>GetShortLen1(Length))))==0)
        break;
    faddbits(GetShortLen1(Length));
  }
  else
  {
    for (Length=0;;Length++)
      if (((BitField^ShortXor2[Length]) & (~(0xff>>GetShortLen2(Length))))==0)
        break;
    faddbits(GetShortLen2(Length));
  }

  if (Length >= 9)
  {
    if (Length == 9)
    {
      LCount++;
      OldCopyString((unsigned int)LastDist,LastLength);
      return;
    }
    if (Length == 14)
    {
      LCount=0;
      Length=DecodeNum(fgetbits(),STARTL2,DecL2,PosL2)+5;
      Distance=(fgetbits()>>1) | 0x8000;
      faddbits(15);
      LastLength=Length;
      LastDist=Distance;
      OldCopyString(Distance,Length);
      return;
    }

    LCount=0;
    SaveLength=Length;
    Distance=OldDist[(OldDistPtr-(Length-9)) & 3];
    Length=DecodeNum(fgetbits(),STARTL1,DecL1,PosL1)+2;
    if (Length==0x101 && SaveLength==10)
    {
      Buf60 ^= 1;
      return;
    }
    if (Distance > 256)
      Length++;
    if (Distance >= MaxDist3)
      Length++;

    OldDist[OldDistPtr++]=Distance;
    OldDistPtr = OldDistPtr & 3;
    LastLength=Length;
    LastDist=Distance;
    OldCopyString(Distance,Length);
    return;
  }

  LCount=0;
  AvrLn1 += Length;
  AvrLn1 -= AvrLn1 >> 4;

  DistancePlace=DecodeNum(fgetbits(),STARTHF2,DecHf2,PosHf2) & 0xff;
  Distance=ChSetA[DistancePlace];
  if (--DistancePlace != -1)
  {
    LastDistance=ChSetA[DistancePlace];
    ChSetA[DistancePlace+1]=LastDistance;
    ChSetA[DistancePlace]=Distance;
  }
  Length+=2;
  OldDist[OldDistPtr++] = ++Distance;
  OldDistPtr = OldDistPtr & 3;
  LastLength=Length;
  LastDist=Distance;
  OldCopyString(Distance,Length);
}


void Unpack::LongLZ()
{
  unsigned int Length;
  unsigned int Distance;
  unsigned int DistancePlace,NewDistancePlace;
  unsigned int OldAvr2,OldAvr3;

  NumHuf=0;
  Nlzb+=16;
  if (Nlzb > 0xff)
  {
    Nlzb=0x90;
    Nhfb >>= 1;
  }
  OldAvr2=AvrLn2;

  unsigned int BitField=fgetbits();
  if (AvrLn2 >= 122)
    Length=DecodeNum(BitField,STARTL2,DecL2,PosL2);
  else
    if (AvrLn2 >= 64)
      Length=DecodeNum(BitField,STARTL1,DecL1,PosL1);
    else
      if (BitField < 0x100)
      {
        Length=BitField;
        faddbits(16);
      }
      else
      {
        for (Length=0;((BitField<<Length)&0x8000)==0;Length++)
          ;
        faddbits(Length+1);
      }

  AvrLn2 += Length;
  AvrLn2 -= AvrLn2 >> 5;

  BitField=fgetbits();
  if (AvrPlcB > 0x28ff)
    DistancePlace=DecodeNum(BitField,STARTHF2,DecHf2,PosHf2);
  else
    if (AvrPlcB > 0x6ff)
      DistancePlace=DecodeNum(BitField,STARTHF1,DecHf1,PosHf1);
    else
      DistancePlace=DecodeNum(BitField,STARTHF0,DecHf0,PosHf0);

  AvrPlcB += DistancePlace;
  AvrPlcB -= AvrPlcB >> 8;
  while (1)
  {
    Distance = ChSetB[DistancePlace & 0xff];
    NewDistancePlace = NToPlB[Distance++ & 0xff]++;
    if (!(Distance & 0xff))
      CorrHuff(ChSetB,NToPlB);
    else
      break;
  }

  ChSetB[DistancePlace]=ChSetB[NewDistancePlace];
  ChSetB[NewDistancePlace]=Distance;

  Distance=((Distance & 0xff00) | (fgetbits() >> 8)) >> 1;
  faddbits(7);

  OldAvr3=AvrLn3;
  if (Length!=1 && Length!=4)
  {
    if (Length==0 && Distance <= MaxDist3)
    {
      AvrLn3++;
      AvrLn3 -= AvrLn3 >> 8;
    }
    else
    {
      if (AvrLn3 > 0)
        AvrLn3--;
    }
  }
  Length+=3;
  if (Distance >= MaxDist3)
    Length++;
  if (Distance <= 256)
    Length+=8;
  if (OldAvr3 > 0xb0 || (AvrPlc >= 0x2a00 && OldAvr2 < 0x40))
    MaxDist3=0x7f00;
  else
    MaxDist3=0x2001;
  OldDist[OldDistPtr++]=Distance;
  OldDistPtr = OldDistPtr & 3;
  LastLength=Length;
  LastDist=Distance;
  OldCopyString(Distance,Length);
}


void Unpack::HuffDecode()
{
  unsigned int CurByte,NewBytePlace;
  unsigned int Length;
  unsigned int Distance;
  int BytePlace;

  unsigned int BitField=fgetbits();

  if (AvrPlc > 0x75ff)
    BytePlace=DecodeNum(BitField,STARTHF4,DecHf4,PosHf4);
  else
    if (AvrPlc > 0x5dff)
      BytePlace=DecodeNum(BitField,STARTHF3,DecHf3,PosHf3);
    else
      if (AvrPlc > 0x35ff)
        BytePlace=DecodeNum(BitField,STARTHF2,DecHf2,PosHf2);
      else
        if (AvrPlc > 0x0dff)
          BytePlace=DecodeNum(BitField,STARTHF1,DecHf1,PosHf1);
        else
          BytePlace=DecodeNum(BitField,STARTHF0,DecHf0,PosHf0);
  BytePlace&=0xff;
  if (StMode)
  {
    if (BytePlace==0 && BitField > 0xfff)
      BytePlace=0x100;
    if (--BytePlace==-1)
    {
      BitField=fgetbits();
      faddbits(1);
      if (BitField & 0x8000)
      {
        NumHuf=StMode=0;
        return;
      }
      else
      {
        Length = (BitField & 0x4000) ? 4 : 3;
        faddbits(1);
        Distance=DecodeNum(fgetbits(),STARTHF2,DecHf2,PosHf2);
        Distance = (Distance << 5) | (fgetbits() >> 11);
        faddbits(5);
        OldCopyString(Distance,Length);
        return;
      }
    }
  }
  else
    if (NumHuf++ >= 16 && FlagsCnt==0)
      StMode=1;
  AvrPlc += BytePlace;
  AvrPlc -= AvrPlc >> 8;
  Nhfb+=16;
  if (Nhfb > 0xff)
  {
    Nhfb=0x90;
    Nlzb >>= 1;
  }

  Window[UnpPtr++]=(byte)(ChSet[BytePlace]>>8);
  --DestUnpSize;

  while (1)
  {
    CurByte=ChSet[BytePlace];
    NewBytePlace=NToPl[CurByte++ & 0xff]++;
    if ((CurByte & 0xff) > 0xa1)
      CorrHuff(ChSet,NToPl);
    else
      break;
  }

  ChSet[BytePlace]=ChSet[NewBytePlace];
  ChSet[NewBytePlace]=CurByte;
}


void Unpack::GetFlagsBuf()
{
  unsigned int Flags,NewFlagsPlace;
  unsigned int FlagsPlace=DecodeNum(fgetbits(),STARTHF2,DecHf2,PosHf2);

  while (1)
  {
    Flags=ChSetC[FlagsPlace];
    FlagBuf=Flags>>8;
    NewFlagsPlace=NToPlC[Flags++ & 0xff]++;
    if ((Flags & 0xff) != 0)
      break;
    CorrHuff(ChSetC,NToPlC);
  }

  ChSetC[FlagsPlace]=ChSetC[NewFlagsPlace];
  ChSetC[NewFlagsPlace]=Flags;
}


void Unpack::OldUnpInitData(int Solid)
{
  if (!Solid)
  {
    AvrPlcB=AvrLn1=AvrLn2=AvrLn3=NumHuf=Buf60=0;
    AvrPlc=0x3500;
    MaxDist3=0x2001;
    Nhfb=Nlzb=0x80;
  }
  FlagsCnt=0;
  FlagBuf=0;
  StMode=0;
  LCount=0;
  ReadTop=0;
}


void Unpack::InitHuff()
{
  for (unsigned int I=0;I<256;I++)
  {
    ChSet[I]=ChSetB[I]=I<<8;
    ChSetA[I]=I;
    ChSetC[I]=((~I+1) & 0xff)<<8;
  }
  memset(NToPl,0,sizeof(NToPl));
  memset(NToPlB,0,sizeof(NToPlB));
  memset(NToPlC,0,sizeof(NToPlC));
  CorrHuff(ChSetB,NToPlB);
}


void Unpack::CorrHuff(ushort *CharSet,byte *NumToPlace)
{
  int I,J;
  for (I=7;I>=0;I--)
    for (J=0;J<32;J++,CharSet++)
      *CharSet=(*CharSet & ~0xff) | I;
  memset(NumToPlace,0,sizeof(NToPl));
  for (I=6;I>=0;I--)
    NumToPlace[I]=(7-I)*32;
}


void Unpack::OldCopyString(unsigned int Distance,unsigned int Length)
{
  DestUnpSize-=Length;
  while (Length--)
  {
    Window[UnpPtr]=Window[(UnpPtr-Distance) & MAXWINMASK];
    UnpPtr=(UnpPtr+1) & MAXWINMASK;
  }
}


uint Unpack::DecodeNum(uint Num,uint StartPos,uint *DecTab,uint *PosTab)
{
  int I;
  for (Num&=0xfff0,I=0;DecTab[I]<=Num;I++)
    StartPos++;
  faddbits(StartPos);
  return(((Num-(I ? DecTab[I-1]:0))>>(16-StartPos))+PosTab[StartPos]);
}


// ------------------------------------------------------------------------------
// unpack20
// ------------------------------------------------------------------------------

void Unpack::CopyString20(unsigned int Length,unsigned int Distance)
{
  LastDist=OldDist[OldDistPtr++ & 3]=Distance;
  LastLength=Length;
  DestUnpSize-=Length;

  unsigned int DestPtr=UnpPtr-Distance;
  if (DestPtr<MAXWINSIZE-300 && UnpPtr<MAXWINSIZE-300)
  {
    Window[UnpPtr++]=Window[DestPtr++];
    Window[UnpPtr++]=Window[DestPtr++];
    while (Length>2)
    {
      Length--;
      Window[UnpPtr++]=Window[DestPtr++];
    }
  }
  else
    while (Length--)
    {
      Window[UnpPtr]=Window[DestPtr++ & MAXWINMASK];
      UnpPtr=(UnpPtr+1) & MAXWINMASK;
    }
}


void Unpack::Unpack20(bool Solid)
{
  static unsigned char LDecode[]={0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224};
  static unsigned char LBits[]=  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};
  static int DDecode[]={0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768U,49152U,65536,98304,131072,196608,262144,327680,393216,458752,524288,589824,655360,720896,786432,851968,917504,983040};
  static unsigned char DBits[]=  {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13,    14,    14,   15,   15,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16};
  static unsigned char SDDecode[]={0,4,8,16,32,64,128,192};
  static unsigned char SDBits[]=  {2,2,3, 4, 5, 6,  6,  6};
  unsigned int Bits;

  {
    UnpInitData(Solid);
    if (!UnpReadBuf())
      return;
    if (!Solid)
      if (!ReadTables20())
        return;
    --DestUnpSize;
  }

  while (DestUnpSize>=0)
  {
    UnpPtr&=MAXWINMASK;

    if (InAddr>ReadTop-30)
      if (!UnpReadBuf())
        break;
    if (((WrPtr-UnpPtr) & MAXWINMASK)<270 && WrPtr!=UnpPtr)
    {
      OldUnpWriteBuf();
    }
    if (UnpAudioBlock)
    {
      int AudioNumber=DecodeNumber(&MD[UnpCurChannel]);

      if (AudioNumber==256)
      {
        if (!ReadTables20())
          break;
        continue;
      }
      Window[UnpPtr++]=DecodeAudio(AudioNumber);
      if (++UnpCurChannel==UnpChannels)
        UnpCurChannel=0;
      --DestUnpSize;
      continue;
    }

    int Number=DecodeNumber(&LD);
    if (Number<256)
    {
      Window[UnpPtr++]=(byte)Number;
      --DestUnpSize;
      continue;
    }
    if (Number>269)
    {
      int Length=LDecode[Number-=270]+3;
      if ((Bits=LBits[Number])>0)
      {
        Length+=getbits()>>(16-Bits);
        addbits(Bits);
      }

      int DistNumber=DecodeNumber(&DD);
      unsigned int Distance=DDecode[DistNumber]+1;
      if ((Bits=DBits[DistNumber])>0)
      {
        Distance+=getbits()>>(16-Bits);
        addbits(Bits);
      }

      if (Distance>=0x2000)
      {
        Length++;
        if (Distance>=0x40000L)
          Length++;
      }

      CopyString20(Length,Distance);
      continue;
    }
    if (Number==269)
    {
      if (!ReadTables20())
        break;
      continue;
    }
    if (Number==256)
    {
      CopyString20(LastLength,LastDist);
      continue;
    }
    if (Number<261)
    {
      unsigned int Distance=OldDist[(OldDistPtr-(Number-256)) & 3];
      int LengthNumber=DecodeNumber(&RD);
      int Length=LDecode[LengthNumber]+2;
      if ((Bits=LBits[LengthNumber])>0)
      {
        Length+=getbits()>>(16-Bits);
        addbits(Bits);
      }
      if (Distance>=0x101)
      {
        Length++;
        if (Distance>=0x2000)
        {
          Length++;
          if (Distance>=0x40000)
            Length++;
        }
      }
      CopyString20(Length,Distance);
      continue;
    }
    if (Number<270)
    {
      unsigned int Distance=SDDecode[Number-=261]+1;
      if ((Bits=SDBits[Number])>0)
      {
        Distance+=getbits()>>(16-Bits);
        addbits(Bits);
      }
      CopyString20(2,Distance);
      continue;
   }
  }
  ReadLastTables();
  OldUnpWriteBuf();
}


bool Unpack::ReadTables20()
{
  byte BitLength[BC20];
  unsigned char Table[MC20*4];
  int TableSize,N,I;
  if (InAddr>ReadTop-25)
    if (!UnpReadBuf())
      return(false);
  unsigned int BitField=getbits();
  UnpAudioBlock=(BitField & 0x8000);

  if (!(BitField & 0x4000))
    memset(UnpOldTable20,0,sizeof(UnpOldTable20));
  addbits(2);

  if (UnpAudioBlock)
  {
    UnpChannels=((BitField>>12) & 3)+1;
    if (UnpCurChannel>=UnpChannels)
      UnpCurChannel=0;
    addbits(2);
    TableSize=MC20*UnpChannels;
  }
  else
    TableSize=NC20+DC20+RC20;

  for (I=0;I<BC20;I++)
  {
    BitLength[I]=(byte)(getbits() >> 12);
    addbits(4);
  }
  MakeDecodeTables(BitLength,&BD,BC20);
  I=0;
  while (I<TableSize)
  {
    if (InAddr>ReadTop-5)
      if (!UnpReadBuf())
        return(false);
    int Number=DecodeNumber(&BD);
    if (Number<16)
    {
      Table[I]=(Number+UnpOldTable20[I]) & 0xf;
      I++;
    }
    else
      if (Number==16)
      {
        N=(getbits() >> 14)+3;
        addbits(2);
        while (N-- > 0 && I<TableSize)
        {
          Table[I]=Table[I-1];
          I++;
        }
      }
      else
      {
        if (Number==17)
        {
          N=(getbits() >> 13)+3;
          addbits(3);
        }
        else
        {
          N=(getbits() >> 9)+11;
          addbits(7);
        }
        while (N-- > 0 && I<TableSize)
          Table[I++]=0;
      }
  }
  if (InAddr>ReadTop)
    return(true);
  if (UnpAudioBlock)
    for (I=0;I<UnpChannels;I++)
      MakeDecodeTables(&Table[I*MC20],&MD[I],MC20);
  else
  {
    MakeDecodeTables(&Table[0],&LD,NC20);
    MakeDecodeTables(&Table[NC20],&DD,DC20);
    MakeDecodeTables(&Table[NC20+DC20],&RD,RC20);
  }
  memcpy(UnpOldTable20,Table,sizeof(UnpOldTable20));
  return(true);
}


void Unpack::ReadLastTables()
{
  if (ReadTop>=InAddr+5)
  {
    if (UnpAudioBlock)
    {
      if (DecodeNumber(&MD[UnpCurChannel])==256)
        ReadTables20();
    }
    else
    {
      if (DecodeNumber(&LD)==269)
        ReadTables20();
    }
  }
}


void Unpack::UnpInitData20(int Solid)
{
  if (!Solid)
  {
    UnpAudioBlock=UnpChannelDelta=UnpCurChannel=0;
    UnpChannels=1;

    memset(AudV,0,sizeof(AudV));
    memset(UnpOldTable20,0,sizeof(UnpOldTable20));
    memset(MD,0,sizeof(MD));
  }
}


byte Unpack::DecodeAudio(int Delta)
{
  struct AudioVariables *V=&AudV[UnpCurChannel];
  V->ByteCount++;
  V->D4=V->D3;
  V->D3=V->D2;
  V->D2=V->LastDelta-V->D1;
  V->D1=V->LastDelta;
  int PCh=8*V->LastChar+V->K1*V->D1+V->K2*V->D2+V->K3*V->D3+V->K4*V->D4+V->K5*UnpChannelDelta;
  PCh=(PCh>>3) & 0xFF;

  unsigned int Ch=PCh-Delta;

  int D=((signed char)Delta)<<3;

  V->Dif[0]+=abs(D);
  V->Dif[1]+=abs(D-V->D1);
  V->Dif[2]+=abs(D+V->D1);
  V->Dif[3]+=abs(D-V->D2);
  V->Dif[4]+=abs(D+V->D2);
  V->Dif[5]+=abs(D-V->D3);
  V->Dif[6]+=abs(D+V->D3);
  V->Dif[7]+=abs(D-V->D4);
  V->Dif[8]+=abs(D+V->D4);
  V->Dif[9]+=abs(D-UnpChannelDelta);
  V->Dif[10]+=abs(D+UnpChannelDelta);

  UnpChannelDelta=V->LastDelta=(signed char)(Ch-V->LastChar);
  V->LastChar=Ch;

  if ((V->ByteCount & 0x1F)==0)
  {
    unsigned int MinDif=V->Dif[0],NumMinDif=0;
    V->Dif[0]=0;
    for (int I=1;I<int(sizeof(V->Dif)/sizeof(V->Dif[0]));I++)
    {
      if (V->Dif[I]<MinDif)
      {
        MinDif=V->Dif[I];
        NumMinDif=I;
      }
      V->Dif[I]=0;
    }
    switch(NumMinDif)
    {
      case 1:
        if (V->K1>=-16)
          V->K1--;
        break;
      case 2:
        if (V->K1<16)
          V->K1++;
        break;
      case 3:
        if (V->K2>=-16)
          V->K2--;
        break;
      case 4:
        if (V->K2<16)
          V->K2++;
        break;
      case 5:
        if (V->K3>=-16)
          V->K3--;
        break;
      case 6:
        if (V->K3<16)
          V->K3++;
        break;
      case 7:
        if (V->K4>=-16)
          V->K4--;
        break;
      case 8:
        if (V->K4<16)
          V->K4++;
        break;
      case 9:
        if (V->K5>=-16)
          V->K5--;
        break;
      case 10:
        if (V->K5<16)
          V->K5++;
        break;
    }
  }
  return((byte)Ch);
}


// ------------------------------------------------------------------------------
// RangeCoder
// ------------------------------------------------------------------------------


inline unsigned int RangeCoder::GetChar()
{
  return UnpackRead->GetChar();
}

void RangeCoder::InitDecoder(Unpack *UnpackRead)
{
  RangeCoder::UnpackRead = UnpackRead;

  low=code=0;
  range=uint(-1);
  for (int i=0;i < 4;i++)
    code=(code << 8) | GetChar();
}

enum
{
    BOT = 1 << 15,
    BOT_MASK = BOT - 1,
    TOP = 1 << 24,
};

#define ARI_DEC_NORMALIZE(code,low,range,read)                           \
{                                                                        \
  while ((low^(low+range))<TOP || (range<BOT && ((range=(0-low)&(BOT_MASK)),1))) \
  {                                                                      \
    code=(code << 8) | read->GetChar();                                  \
    range <<= 8;                                                         \
    low <<= 8;                                                           \
  }                                                                      \
}

/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2000                                                               *
 *  Contents: model description and encoding/decoding routines              *
 ****************************************************************************/

inline PPM_CONTEXT* PPM_CONTEXT::createChild(ModelPPM *Model,STATE* pStats,
                                             STATE& FirstState)
{
  PPM_CONTEXT* pc = (PPM_CONTEXT*) Model->SubAlloc.AllocContext();
  if ( pc ) 
  {
    pc->NumStats=1;                     
    pc->OneState=FirstState;
    pc->Suffix=this;                    
    pStats->Successor=pc;
  }
  return pc;
}


ModelPPM::ModelPPM()
{
  MinContext=NULL;
  MaxContext=NULL;
  MedContext=NULL;
}


void ModelPPM::RestartModelRare()
{
  int i, k, m;
  memset(CharMask,0,sizeof(CharMask));
  SubAlloc.InitSubAllocator();
  InitRL=-(MaxOrder < 12 ? MaxOrder:12)-1;
  MinContext = MaxContext = (PPM_CONTEXT*) SubAlloc.AllocContext();
  MinContext->Suffix=NULL;
  OrderFall=MaxOrder;
  MinContext->U.SummFreq=(MinContext->NumStats=256)+1;
  FoundState=MinContext->U.Stats=(STATE*)SubAlloc.AllocUnits(256/2);
  for (RunLength=InitRL, PrevSuccess=i=0;i < 256;i++) 
  {
    MinContext->U.Stats[i].Symbol=i;      
    MinContext->U.Stats[i].Freq=1;
    MinContext->U.Stats[i].Successor=NULL;
  }
  
  static const ushort InitBinEsc[]={
    0x3CDD,0x1F3F,0x59BF,0x48F3,0x64A1,0x5ABC,0x6632,0x6051
  };

  for (i=0;i < 128;i++)
    for (k=0;k < 8;k++)
      for (m=0;m < 64;m += 8)
        BinSumm[i][k+m]=BIN_SCALE-InitBinEsc[k]/(i+2);
  for (i=0;i < 25;i++)
    for (k=0;k < 16;k++)            
      SEE2Cont[i][k].init(5*i+10);
}


void ModelPPM::StartModelRare(int MaxOrder)
{
  int i, k, m ,Step;
  EscCount=1;
/*
  if (MaxOrder < 2) 
  {
    memset(CharMask,0,sizeof(CharMask));
    OrderFall=ModelPPM::MaxOrder;
    MinContext=MaxContext;
    while (MinContext->Suffix != NULL)
    {
      MinContext=MinContext->Suffix;
      OrderFall--;
    }
    FoundState=MinContext->U.Stats;
    MinContext=MaxContext;
  } 
  else 
*/
  {
    ModelPPM::MaxOrder=MaxOrder;
    RestartModelRare();
    NS2BSIndx[0]=2*0;
    NS2BSIndx[1]=2*1;
    memset(NS2BSIndx+2,2*2,9);
    memset(NS2BSIndx+11,2*3,256-11);
    for (i=0;i < 3;i++)
      NS2Indx[i]=i;
    for (m=i, k=Step=1;i < 256;i++) 
    {
      NS2Indx[i]=m;
      if ( !--k ) 
      { 
        k = ++Step;
        m++; 
      }
    }
    memset(HB2Flag,0,0x40);
    memset(HB2Flag+0x40,0x08,0x100-0x40);
    DummySEE2Cont.Shift=PERIOD_BITS;
  }
}


void PPM_CONTEXT::rescale(ModelPPM *Model)
{
  int OldNS=NumStats, i=NumStats-1, Adder, EscFreq;
  STATE* p1, * p;
  for (p=Model->FoundState;p != U.Stats;p--)
    _PPMD_SWAP(p[0],p[-1]);
  U.Stats->Freq += 4;
  U.SummFreq += 4;
  EscFreq=U.SummFreq-p->Freq;
  Adder=(Model->OrderFall != 0);
  U.SummFreq = (p->Freq=(p->Freq+Adder) >> 1);
  do 
  {
    EscFreq -= (++p)->Freq;
    U.SummFreq += (p->Freq=(p->Freq+Adder) >> 1);
    if (p[0].Freq > p[-1].Freq) 
    {
      STATE tmp=*(p1=p);
      do 
      { 
        p1[0]=p1[-1]; 
      } while (--p1 != U.Stats && tmp.Freq > p1[-1].Freq);
      *p1=tmp;
    }
  } while ( --i );
  if (p->Freq == 0) 
  {
    do 
    { 
      i++; 
    } while ((--p)->Freq == 0);
    EscFreq += i;
    if ((NumStats -= i) == 1) 
    {
      STATE tmp=*U.Stats;
      do 
      { 
        tmp.Freq-=(tmp.Freq >> 1); 
        EscFreq>>=1; 
      } while (EscFreq > 1);
      Model->SubAlloc.FreeUnits(U.Stats,(OldNS+1) >> 1);
      *(Model->FoundState=&OneState)=tmp;  return;
    }
  }
  U.SummFreq += (EscFreq -= (EscFreq >> 1));
  int n0=(OldNS+1) >> 1, n1=(NumStats+1) >> 1;
  if (n0 != n1)
    U.Stats = (STATE*) Model->SubAlloc.ShrinkUnits(U.Stats,n0,n1);
  Model->FoundState=U.Stats;
}


inline PPM_CONTEXT* ModelPPM::CreateSuccessors(bool Skip,STATE* p1)
{
#ifdef __ICL
  static
#endif
  STATE UpState;
  PPM_CONTEXT* pc=MinContext, * UpBranch=FoundState->Successor;
  STATE * p, * ps[MAX_O], ** pps=ps;
  if ( !Skip ) 
  {
    *pps++ = FoundState;
    if ( !pc->Suffix )
      goto NO_LOOP;
  }
  if ( p1 ) 
  {
    p=p1;
    pc=pc->Suffix;
    goto LOOP_ENTRY;
  }
  do 
  {
    pc=pc->Suffix;
    if (pc->NumStats != 1) 
    {
      if ((p=pc->U.Stats)->Symbol != FoundState->Symbol)
        do 
        {
          p++; 
        } while (p->Symbol != FoundState->Symbol);
    } 
    else
      p=&(pc->OneState);
LOOP_ENTRY:
    if (p->Successor != UpBranch) 
    {
      pc=p->Successor;
      break;
    }
    *pps++ = p;
  } while ( pc->Suffix );
NO_LOOP:
  if (pps == ps)
    return pc;
  UpState.Symbol=*(byte*) UpBranch;
  UpState.Successor=(PPM_CONTEXT*) (((byte*) UpBranch)+1);
  if (pc->NumStats != 1) 
  {
    if ((byte*) pc <= SubAlloc.pText)
      return(NULL);
    if ((p=pc->U.Stats)->Symbol != UpState.Symbol)
    do 
    { 
      p++; 
    } while (p->Symbol != UpState.Symbol);
    uint cf=p->Freq-1;
    uint s0=pc->U.SummFreq-pc->NumStats-cf;
    UpState.Freq=1+((2*cf <= s0)?(5*cf > s0):((2*cf+3*s0-1)/(2*s0)));
  } 
  else
    UpState.Freq=pc->OneState.Freq;
  do 
  {
    pc = pc->createChild(this,*--pps,UpState);
    if ( !pc )
      return NULL;
  } while (pps != ps);
  return pc;
}


inline void ModelPPM::UpdateModel()
{
  STATE fs = *FoundState, *p = NULL;
  PPM_CONTEXT *pc, *Successor;
  uint ns1, ns, cf, sf, s0;
  if (fs.Freq < MAX_FREQ/4 && (pc=MinContext->Suffix) != NULL) 
  {
    if (pc->NumStats != 1) 
    {
      if ((p=pc->U.Stats)->Symbol != fs.Symbol) 
      {
        do 
        { 
          p++; 
        } while (p->Symbol != fs.Symbol);
        if (p[0].Freq >= p[-1].Freq) 
        {
          _PPMD_SWAP(p[0],p[-1]); 
          p--;
        }
      }
      if (p->Freq < MAX_FREQ-9) 
      {
        p->Freq += 2;               
        pc->U.SummFreq += 2;
      }
    } 
    else 
    {
      p=&(pc->OneState);
      p->Freq += (p->Freq < 32);
    }
  }
  if ( !OrderFall ) 
  {
    MinContext=MaxContext=FoundState->Successor=CreateSuccessors(true,p);
    if ( !MinContext )
      goto RESTART_MODEL;
    return;
  }
  *SubAlloc.pText++ = fs.Symbol;                   
  Successor = (PPM_CONTEXT*) SubAlloc.pText;
  if (SubAlloc.pText >= SubAlloc.FakeUnitsStart)                
    goto RESTART_MODEL;
  if ( fs.Successor ) 
  {
    if ((byte*) fs.Successor <= SubAlloc.pText &&
        (fs.Successor=CreateSuccessors(false,p)) == NULL)
      goto RESTART_MODEL;
    if ( !--OrderFall ) 
    {
      Successor=fs.Successor;
      SubAlloc.pText -= (MaxContext != MinContext);
    }
  } 
  else 
  {
    FoundState->Successor=Successor;
    fs.Successor=MinContext;
  }
  s0=MinContext->U.SummFreq-(ns=MinContext->NumStats)-(fs.Freq-1);
  for (pc=MaxContext;pc != MinContext;pc=pc->Suffix) 
  {
    if ((ns1=pc->NumStats) != 1) 
    {
      if ((ns1 & 1) == 0) 
      {
        pc->U.Stats=(STATE*) SubAlloc.ExpandUnits(pc->U.Stats,ns1 >> 1);
        if ( !pc->U.Stats )           
          goto RESTART_MODEL;
      }
      pc->U.SummFreq += (2*ns1 < ns)+2*((4*ns1 <= ns) & (pc->U.SummFreq <= 8*ns1));
    } 
    else 
    {
      p=(STATE*) SubAlloc.AllocUnits(1);
      if ( !p )
        goto RESTART_MODEL;
      *p=pc->OneState;
      pc->U.Stats=p;
      if (p->Freq < MAX_FREQ/4-1)
        p->Freq += p->Freq;
      else
        p->Freq  = MAX_FREQ-4;
      pc->U.SummFreq=p->Freq+InitEsc+(ns > 3);
    }
    cf=2*fs.Freq*(pc->U.SummFreq+6);
    sf=s0+pc->U.SummFreq;
    if (cf < 6*sf) 
    {
      cf=1+(cf > sf)+(cf >= 4*sf);
      pc->U.SummFreq += 3;
    }
    else 
    {
      cf=4+(cf >= 9*sf)+(cf >= 12*sf)+(cf >= 15*sf);
      pc->U.SummFreq += cf;
    }
    p=pc->U.Stats+ns1;
    p->Successor=Successor;
    p->Symbol = fs.Symbol;
    p->Freq = cf;
    pc->NumStats=++ns1;
  }
  MaxContext=MinContext=fs.Successor;
  return;
RESTART_MODEL:
  RestartModelRare();
  EscCount=0;
}


// Tabulated escapes for exponential symbol distribution
static const byte ExpEscape[16]={ 25,14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))



inline void PPM_CONTEXT::decodeBinSymbol(ModelPPM *Model)
{
  STATE& rs=OneState;
  Model->HiBitsFlag=Model->HB2Flag[Model->FoundState->Symbol];
  ushort& bs=Model->BinSumm[rs.Freq-1][Model->PrevSuccess+
           Model->NS2BSIndx[Suffix->NumStats-1]+
           Model->HiBitsFlag+2*Model->HB2Flag[rs.Symbol]+
           ((Model->RunLength >> 26) & 0x20)];
  if (Model->Coder.GetCurrentShiftCount(TOT_BITS) < bs) 
  {
    Model->FoundState=&rs;
    rs.Freq += (rs.Freq < 128);
    Model->Coder.SubRange.LowCount=0;
    Model->Coder.SubRange.HighCount=bs;
    bs = GET_SHORT16(bs+INTERVAL-GET_MEAN(bs,PERIOD_BITS,2));
    Model->PrevSuccess=1;
    Model->RunLength++;
  } 
  else 
  {
    Model->Coder.SubRange.LowCount=bs;
    bs = GET_SHORT16(bs-GET_MEAN(bs,PERIOD_BITS,2));
    Model->Coder.SubRange.HighCount=BIN_SCALE;
    Model->InitEsc=ExpEscape[bs >> 10];
    Model->NumMasked=1;
    Model->CharMask[rs.Symbol]=Model->EscCount;
    Model->PrevSuccess=0;
    Model->FoundState=NULL;
  }
}


inline void PPM_CONTEXT::update1(ModelPPM *Model,STATE* p)
{
  (Model->FoundState=p)->Freq += 4;              
  U.SummFreq += 4;
  if (p[0].Freq > p[-1].Freq) 
  {
    _PPMD_SWAP(p[0],p[-1]);                   
    Model->FoundState=--p;
    if (p->Freq > MAX_FREQ)             
      rescale(Model);
  }
}




inline bool PPM_CONTEXT::decodeSymbol1(ModelPPM *Model)
{
  Model->Coder.SubRange.scale=U.SummFreq;
  STATE* p=U.Stats;
  int i, HiCnt;
  int count=Model->Coder.GetCurrentCount();
  if (count>=(int)Model->Coder.SubRange.scale)
    return(false);
  if (count < (HiCnt=p->Freq)) 
  {
    Model->PrevSuccess=(2*(Model->Coder.SubRange.HighCount=HiCnt) > Model->Coder.SubRange.scale);
    Model->RunLength += Model->PrevSuccess;
    (Model->FoundState=p)->Freq=(HiCnt += 4);
    U.SummFreq += 4;
    if (HiCnt > MAX_FREQ)
      rescale(Model);
    Model->Coder.SubRange.LowCount=0;
    return(true);
  }
  else
    if (Model->FoundState==NULL)
      return(false);
  Model->PrevSuccess=0;
  i=NumStats-1;
  while ((HiCnt += (++p)->Freq) <= count)
    if (--i == 0) 
    {
      Model->HiBitsFlag=Model->HB2Flag[Model->FoundState->Symbol];
      Model->Coder.SubRange.LowCount=HiCnt;
      Model->CharMask[p->Symbol]=Model->EscCount;
      i=(Model->NumMasked=NumStats)-1;
      Model->FoundState=NULL;
      do 
      { 
        Model->CharMask[(--p)->Symbol]=Model->EscCount; 
      } while ( --i );
      Model->Coder.SubRange.HighCount=Model->Coder.SubRange.scale;
      return(true);
    }
  Model->Coder.SubRange.LowCount=(Model->Coder.SubRange.HighCount=HiCnt)-p->Freq;
  update1(Model,p);
  return(true);
}


inline void PPM_CONTEXT::update2(ModelPPM *Model,STATE* p)
{
  (Model->FoundState=p)->Freq += 4;              
  U.SummFreq += 4;
  if (p->Freq > MAX_FREQ)                 
    rescale(Model);
  Model->EscCount++;
  Model->RunLength=Model->InitRL;
}


inline SEE2_CONTEXT* PPM_CONTEXT::makeEscFreq2(ModelPPM *Model,int Diff)
{
  SEE2_CONTEXT* psee2c;
  if (NumStats != 256) 
  {
    psee2c=Model->SEE2Cont[Model->NS2Indx[Diff-1]]+
           (Diff < Suffix->NumStats-NumStats)+
           2*(U.SummFreq < 11*NumStats)+4*(Model->NumMasked > Diff)+
           Model->HiBitsFlag;
    Model->Coder.SubRange.scale=psee2c->getMean();
  }
  else 
  {
    psee2c=&Model->DummySEE2Cont;
    Model->Coder.SubRange.scale=1;
  }
  return psee2c;
}




inline bool PPM_CONTEXT::decodeSymbol2(ModelPPM *Model)
{
  int count, HiCnt, i=NumStats-Model->NumMasked;
  SEE2_CONTEXT* psee2c=makeEscFreq2(Model,i);
  STATE* ps[256], ** pps=ps, * p=U.Stats-1;
  HiCnt=0;
  do 
  {
    do 
    { 
      p++; 
    } while (Model->CharMask[p->Symbol] == Model->EscCount);
    HiCnt += p->Freq;
    *pps++ = p;
  } while ( --i );
  Model->Coder.SubRange.scale += HiCnt;
  count=Model->Coder.GetCurrentCount();
  if (count>=(int)Model->Coder.SubRange.scale)
    return(false);
  p=*(pps=ps);
  if (count < HiCnt) 
  {
    HiCnt=0;
    while ((HiCnt += p->Freq) <= count) 
      p=*++pps;
    Model->Coder.SubRange.LowCount = (Model->Coder.SubRange.HighCount=HiCnt)-p->Freq;
    psee2c->update();
    update2(Model,p);
  }
  else
  {
    Model->Coder.SubRange.LowCount=HiCnt;
    Model->Coder.SubRange.HighCount=Model->Coder.SubRange.scale;
    i=NumStats-Model->NumMasked;
    pps--;
    do 
    { 
      Model->CharMask[(*++pps)->Symbol]=Model->EscCount; 
    } while ( --i );
    psee2c->Summ += Model->Coder.SubRange.scale;
    Model->NumMasked = NumStats;
  }
  return(true);
}


inline void ModelPPM::ClearMask()
{
  EscCount=1;                             
  memset(CharMask,0,sizeof(CharMask));
}




// reset PPM variables after data error allowing safe resuming
// of further data processing
void ModelPPM::CleanUp()
{
  SubAlloc.StopSubAllocator();
  SubAlloc.StartSubAllocator(1);
  StartModelRare(2);
}


bool ModelPPM::DecodeInit(Unpack *UnpackRead,int &EscChar)
{
  int MaxOrder=UnpackRead->GetChar();
  bool Reset=(MaxOrder & 0x20)!=0;

  int MaxMB = 0;
  if (Reset)
    MaxMB=UnpackRead->GetChar();
  else
    if (SubAlloc.GetAllocatedMemory()==0)
      return(false);
  if (MaxOrder & 0x40)
    EscChar=UnpackRead->GetChar();
  Coder.InitDecoder(UnpackRead);
  if (Reset)
  {
    MaxOrder=(MaxOrder & 0x1f)+1;
    if (MaxOrder>16)
      MaxOrder=16+(MaxOrder-16)*3;
    if (MaxOrder==1)
    {
      SubAlloc.StopSubAllocator();
      return(false);
    }
    SubAlloc.StartSubAllocator(MaxMB+1);
    StartModelRare(MaxOrder);
  }
  return(MinContext!=NULL);
}


int ModelPPM::DecodeChar()
{
  if ((byte*)MinContext <= SubAlloc.pText || (byte*)MinContext>SubAlloc.HeapEnd)
    return(-1);
  if (MinContext->NumStats != 1)      
  {
    if ((byte*)MinContext->U.Stats <= SubAlloc.pText || (byte*)MinContext->U.Stats>SubAlloc.HeapEnd)
      return(-1);
    if (!MinContext->decodeSymbol1(this))
      return(-1);
  }
  else                                
    MinContext->decodeBinSymbol(this);
  Coder.Decode();
  while ( !FoundState ) 
  {
    ARI_DEC_NORMALIZE(Coder.code,Coder.low,Coder.range,Coder.UnpackRead);
    do
    {
      OrderFall++;                
      MinContext=MinContext->Suffix;
      if ((byte*)MinContext <= SubAlloc.pText || (byte*)MinContext>SubAlloc.HeapEnd)
        return(-1);
    } while (MinContext->NumStats == NumMasked);
    if (!MinContext->decodeSymbol2(this))
      return(-1);
    Coder.Decode();
  }
  int Symbol=FoundState->Symbol;
  if (!OrderFall && (byte*) FoundState->Successor > SubAlloc.pText)
    MinContext=MaxContext=FoundState->Successor;
  else
  {
    UpdateModel();
    if (EscCount == 0)
      ClearMask();
  }
  ARI_DEC_NORMALIZE(Coder.code,Coder.low,Coder.range,Coder.UnpackRead);
  return(Symbol);
}

// ------------------------------------------------------------------------------
// Unpack
// ------------------------------------------------------------------------------


Unpack::Unpack(ComprDataIO *DataIO)
{
  UnpIO=DataIO;
  Window=NULL;
  UnpAllBuf=false;
  UnpSomeRead=false;
}


Unpack::~Unpack()
{
  delete[] Window;
  InitFilters();
}


void Unpack::Init()
{
  Window=new byte[MAXWINSIZE];

  if (Window==NULL)
  {
      // TODO: memory error
  }

  // Clean the window to generate the same output when unpacking corrupt
  // RAR files, which may access to unused areas of sliding dictionary.
  memset(Window,0,MAXWINSIZE);

  UnpInitData(false);

  // RAR 1.5 decompression initialization
  OldUnpInitData(false);
  InitHuff();
}


void Unpack::DoUnpack(int Method,bool Solid)
{
  switch(Method)
  {
    case 15: // rar 1.5 compression
      Unpack15(Solid);
      break;
    case 20: // rar 2.x compression
    case 26: // files larger than 2GB
      Unpack20(Solid);
      break;
    case 29: // rar 3.x compression
    case 36: // alternative hash
      Unpack29(Solid);
      break;
  }
}


inline void Unpack::InsertOldDist(unsigned int Distance)
{
  OldDist[3]=OldDist[2];
  OldDist[2]=OldDist[1];
  OldDist[1]=OldDist[0];
  OldDist[0]=Distance;
}


void Unpack::CopyString(uint Length,uint Distance)
{
  uint SrcPtr=UnpPtr-Distance;
  if (SrcPtr<MAXWINSIZE-MAX_LZ_MATCH && UnpPtr<MAXWINSIZE-MAX_LZ_MATCH)
  {
    // If we are not close to end of window, we do not need to waste time
    // to "& MAXWINMASK" pointer protection.

    byte *Src=Window+SrcPtr;
    byte *Dest=Window+UnpPtr;
    UnpPtr+=Length;

    while (Length>=8)
    {
      // Unroll the loop for 8 byte and longer strings.
      Dest[0]=Src[0];
      Dest[1]=Src[1];
      Dest[2]=Src[2];
      Dest[3]=Src[3];
      Dest[4]=Src[4];
      Dest[5]=Src[5];
      Dest[6]=Src[6];
      Dest[7]=Src[7];
      Src+=8;
      Dest+=8;
      Length-=8;
    }

    // Unroll the loop for 0 - 7 bytes left. Note that we use nested "if"s.
    if (Length>0) { Dest[0]=Src[0];
    if (Length>1) { Dest[1]=Src[1];
    if (Length>2) { Dest[2]=Src[2];
    if (Length>3) { Dest[3]=Src[3];
    if (Length>4) { Dest[4]=Src[4];
    if (Length>5) { Dest[5]=Src[5];
    if (Length>6) { Dest[6]=Src[6]; } } } } } } } // Close all nested "if"s.
  }
  else
    while (Length--) // Slow copying with all possible precautions.
    {
      Window[UnpPtr]=Window[SrcPtr++ & MAXWINMASK];
      UnpPtr=(UnpPtr+1) & MAXWINMASK;
    }
}


inline uint Unpack::DecodeNumber(DecodeTable *Dec)
{
  // Left aligned 15 bit length raw bit field.
  uint BitField=getbits() & 0xfffe;

  if (BitField<Dec->DecodeLen[Dec->QuickBits])
  {
    uint Code=BitField>>(16-Dec->QuickBits);
    addbits(Dec->QuickLen[Code]);
    return Dec->QuickNum[Code];
  }

  // Detect the real bit length for current code.
  uint Bits=15;
  for (uint I=Dec->QuickBits+1;I<15;I++)
    if (BitField<Dec->DecodeLen[I])
    {
      Bits=I;
      break;
    }

  addbits(Bits);
  
  // Calculate the distance from the start code for current bit length.
  uint Dist=BitField-Dec->DecodeLen[Bits-1];

  // Start codes are left aligned, but we need the normal right aligned
  // number. So we shift the distance to the right.
  Dist>>=(16-Bits);

  // Now we can calculate the position in the code list. It is the sum
  // of first position for current bit length and right aligned distance
  // between our bit field and start code for current bit length.
  uint Pos=Dec->DecodePos[Bits]+Dist;

  // Out of bounds safety check required for damaged archives.
  if (Pos>=Dec->MaxNum)
    Pos=0;

  // Convert the position in the code list to position in alphabet
  // and return it.
  return(Dec->DecodeNum[Pos]);
}


// We use it instead of direct PPM.DecodeChar call to be sure that
// we reset PPM structures in case of corrupt data. It is important,
// because these structures can be invalid after PPM.DecodeChar returned -1.
inline int Unpack::SafePPMDecodeChar()
{
  int Ch=PPM.DecodeChar();
  if (Ch==-1)              // Corrupt PPM data found.
  {
    PPM.CleanUp();         // Reset possibly corrupt PPM data structures.
    UnpBlockType=BLOCK_LZ; // Set faster and more fail proof LZ mode.
  }
  return(Ch);
}


void Unpack::Unpack29(bool Solid)
{
  static unsigned char LDecode[]={0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224};
  static unsigned char LBits[]=  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};
  static int DDecode[DC];
  static byte DBits[DC];
  static int DBitLengthCounts[]= {4,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,14,0,12};
  static unsigned char SDDecode[]={0,4,8,16,32,64,128,192};
  static unsigned char SDBits[]=  {2,2,3, 4, 5, 6,  6,  6};
  unsigned int Bits;

  if (DDecode[1]==0)
  {
    int Dist=0,BitLength=0,Slot=0;
    for (int I=0;I<int(ASIZE(DBitLengthCounts));I++,BitLength++)
      for (int J=0;J<DBitLengthCounts[I];J++,Slot++,Dist+=(1<<BitLength))
      {
        DDecode[Slot]=Dist;
        DBits[Slot]=BitLength;
      }
  }

  FileExtracted=true;

  {
    UnpInitData(Solid);

    if (!UnpReadBuf())
      return;

    if ((!Solid || !TablesRead) && !ReadTables())
      return;
  }

  while (true)
  {
    UnpPtr&=MAXWINMASK;

    if (InAddr>ReadBorder)
    {
      if (!UnpReadBuf())
        break;
    }
    if (((WrPtr-UnpPtr) & MAXWINMASK)<260 && WrPtr!=UnpPtr)
    {
      UnpWriteBuf();
      if (WrittenFileSize>DestUnpSize)
        return;
    }
    if (UnpBlockType==BLOCK_PPM)
    {
      // Here speed is critical, so we do not use SafePPMDecodeChar,
      // because sometimes even the inline function can introduce
      // some additional penalty.
      int Ch=PPM.DecodeChar();
      if (Ch==-1)              // Corrupt PPM data found.
      {
        PPM.CleanUp();         // Reset possibly corrupt PPM data structures.
        UnpBlockType=BLOCK_LZ; // Set faster and more fail proof LZ mode.
        break;
      }
      if (Ch==PPMEscChar)
      {
        int NextCh=SafePPMDecodeChar();
        if (NextCh==0)  // End of PPM encoding.
        {
          if (!ReadTables())
            break;
          continue;
        }
        if (NextCh==-1) // Corrupt PPM data found.
          break;
        if (NextCh==2)  // End of file in PPM mode.
          break;
        if (NextCh==3)  // Read VM code.
        {
          if (!ReadVMCodePPM())
            break;
          continue;
        }
        if (NextCh==4) // LZ inside of PPM.
        {
          unsigned int Distance=0,Length;
          bool Failed=false;
          for (int I=0;I<4 && !Failed;I++)
          {
            int Ch=SafePPMDecodeChar();
            if (Ch==-1)
              Failed=true;
            else
              if (I==3)
                Length=(byte)Ch;
              else
                Distance=(Distance<<8)+(byte)Ch;
          }
          if (Failed)
            break;

          CopyString(Length+32,Distance+2);
          continue;
        }
        if (NextCh==5) // One byte distance match (RLE) inside of PPM.
        {
          int Length=SafePPMDecodeChar();
          if (Length==-1)
            break;
          CopyString(Length+4,1);
          continue;
        }
        // If we are here, NextCh must be 1, what means that current byte
        // is equal to our 'escape' byte, so we just store it to Window.
      }
      Window[UnpPtr++]=Ch;
      continue;
    }

    int Number=DecodeNumber(&LD);
    if (Number<256)
    {
      Window[UnpPtr++]=(byte)Number;
      continue;
    }
    if (Number>=271)
    {
      int Length=LDecode[Number-=271]+3;
      if ((Bits=LBits[Number])>0)
      {
        Length+=getbits()>>(16-Bits);
        addbits(Bits);
      }

      int DistNumber=DecodeNumber(&DD);
      unsigned int Distance=DDecode[DistNumber]+1;
      if ((Bits=DBits[DistNumber])>0)
      {
        if (DistNumber>9)
        {
          if (Bits>4)
          {
            Distance+=((getbits()>>(20-Bits))<<4);
            addbits(Bits-4);
          }
          if (LowDistRepCount>0)
          {
            LowDistRepCount--;
            Distance+=PrevLowDist;
          }
          else
          {
            int LowDist=DecodeNumber(&LDD);
            if (LowDist==16)
            {
              LowDistRepCount=LOW_DIST_REP_COUNT-1;
              Distance+=PrevLowDist;
            }
            else
            {
              Distance+=LowDist;
              PrevLowDist=LowDist;
            }
          }
        }
        else
        {
          Distance+=getbits()>>(16-Bits);
          addbits(Bits);
        }
      }

      if (Distance>=0x2000)
      {
        Length++;
        if (Distance>=0x40000L)
          Length++;
      }

      InsertOldDist(Distance);
      LastLength=Length;
      CopyString(Length,Distance);
      continue;
    }
    if (Number==256)
    {
      if (!ReadEndOfBlock())
        break;
      continue;
    }
    if (Number==257)
    {
      if (!ReadVMCode())
        break;
      continue;
    }
    if (Number==258)
    {
      if (LastLength!=0)
        CopyString(LastLength,OldDist[0]);
      continue;
    }
    if (Number<263)
    {
      int DistNum=Number-259;
      unsigned int Distance=OldDist[DistNum];
      for (int I=DistNum;I>0;I--)
        OldDist[I]=OldDist[I-1];
      OldDist[0]=Distance;

      int LengthNumber=DecodeNumber(&RD);
      int Length=LDecode[LengthNumber]+2;
      if ((Bits=LBits[LengthNumber])>0)
      {
        Length+=getbits()>>(16-Bits);
        addbits(Bits);
      }
      LastLength=Length;
      CopyString(Length,Distance);
      continue;
    }
    if (Number<272)
    {
      unsigned int Distance=SDDecode[Number-=263]+1;
      if ((Bits=SDBits[Number])>0)
      {
        Distance+=getbits()>>(16-Bits);
        addbits(Bits);
      }
      InsertOldDist(Distance);
      LastLength=2;
      CopyString(2,Distance);
      continue;
    }
  }
  UnpWriteBuf();
}


// Return 'false' to quit unpacking the current file or 'true' to continue.
bool Unpack::ReadEndOfBlock()
{
  unsigned int BitField=getbits();
  bool NewTable,NewFile=false;

  // "1"  - no new file, new table just here.
  // "00" - new file,    no new table.
  // "01" - new file,    new table (in beginning of next file).
  
  if ((BitField & 0x8000)!=0)
  {
    NewTable=true;
    addbits(1);
  }
  else
  {
    NewFile=true;
    NewTable=(BitField & 0x4000)!=0;
    addbits(2);
  }
  TablesRead=!NewTable;

  // Quit immediately if "new file" flag is set. If "new table" flag
  // is present, we'll read the table in beginning of next file
  // based on 'TablesRead' 'false' value.
  if (NewFile)
    return false;
  return ReadTables(); // Quit only if we failed to read tables.
}


bool Unpack::ReadVMCode()
{
  // Entire VM code is guaranteed to fully present in block defined 
  // by current Huffman table. Compressor checks that VM code does not cross
  // Huffman block boundaries.
  unsigned int FirstByte=getbits()>>8;
  addbits(8);
  int Length=(FirstByte & 7)+1;
  if (Length==7)
  {
    Length=(getbits()>>8)+7;
    addbits(8);
  }
  else
    if (Length==8)
    {
      Length=getbits();
      addbits(16);
    }
  Array<byte> VMCode(Length);
  for (int I=0;I<Length;I++)
  {
    // Try to read the new buffer if only one byte is left.
    // But if we read all bytes except the last, one byte is enough.
    if (InAddr>=ReadTop-1 && !UnpReadBuf() && I<Length-1)
      return(false);
    VMCode[I]=getbits()>>8;
    addbits(8);
  }
  return(AddVMCode(FirstByte,&VMCode[0],Length));
}


bool Unpack::ReadVMCodePPM()
{
  unsigned int FirstByte=SafePPMDecodeChar();
  if ((int)FirstByte==-1)
    return(false);
  int Length=(FirstByte & 7)+1;
  if (Length==7)
  {
    int B1=SafePPMDecodeChar();
    if (B1==-1)
      return(false);
    Length=B1+7;
  }
  else
    if (Length==8)
    {
      int B1=SafePPMDecodeChar();
      if (B1==-1)
        return(false);
      int B2=SafePPMDecodeChar();
      if (B2==-1)
        return(false);
      Length=B1*256+B2;
    }
  Array<byte> VMCode(Length);
  for (int I=0;I<Length;I++)
  {
    int Ch=SafePPMDecodeChar();
    if (Ch==-1)
      return(false);
    VMCode[I]=Ch;
  }
  return(AddVMCode(FirstByte,&VMCode[0],Length));
}


bool Unpack::AddVMCode(unsigned int FirstByte,byte *Code,int CodeSize)
{
  VMCodeInp.InitBitInput();
  memcpy(VMCodeInp.InBuf,Code,Min(BitInput::MAX_SIZE,CodeSize));
  VM.Init();

  uint FiltPos;
  if (FirstByte & 0x80)
  {
    FiltPos=RarVM::ReadData(VMCodeInp);
    if (FiltPos==0)
      InitFilters();
    else
      FiltPos--;
  }
  else
    FiltPos=LastFilter; // Use the same filter as last time.

  if (FiltPos>Filters.Size() || FiltPos>OldFilterLengths.Size())
    return(false);
  LastFilter=FiltPos;
  bool NewFilter=(FiltPos==Filters.Size());

  UnpackFilter *StackFilter=new UnpackFilter; // New filter for PrgStack.

  UnpackFilter *Filter;
  if (NewFilter) // New filter code, never used before since VM reset.
  {
    // Too many different filters, corrupt archive.
    if (FiltPos>MAX_FILTERS)
    {
      delete StackFilter;
      return false;
    }

    Filters.Add(1);
    Filters[Filters.Size()-1]=Filter=new UnpackFilter;
    StackFilter->ParentFilter=(uint)(Filters.Size()-1);

    // Reserve one item, where we store the data block length of our new
    // filter entry. We'll set it to real block length below, after reading
    // it. But we need to initialize it now, because when processing corrupt
    // data, we can access this item even before we set it to real value.
    OldFilterLengths.Push(0);
    Filter->ExecCount=0;
  }
  else  // Filter was used in the past.
  {
    Filter=Filters[FiltPos];
    StackFilter->ParentFilter=FiltPos;
    Filter->ExecCount++;
  }

  int EmptyCount=0;
  for (uint I=0;I<PrgStack.Size();I++)
  {
    PrgStack[I-EmptyCount]=PrgStack[I];
    if (PrgStack[I]==NULL)
      EmptyCount++;
    if (EmptyCount>0)
      PrgStack[I]=NULL;
  }
  if (EmptyCount==0)
  {
    PrgStack.Add(1);
    EmptyCount=1;
  }
  int StackPos=(int)(PrgStack.Size()-EmptyCount);
  PrgStack[StackPos]=StackFilter;
  StackFilter->ExecCount=Filter->ExecCount;
 
  uint BlockStart=RarVM::ReadData(VMCodeInp);
  if (FirstByte & 0x40)
    BlockStart+=258;
  StackFilter->BlockStart=(BlockStart+UnpPtr)&MAXWINMASK;
  if (FirstByte & 0x20)
  {
    StackFilter->BlockLength=RarVM::ReadData(VMCodeInp);

    // Store the last data block length for current filter.
    OldFilterLengths[FiltPos]=StackFilter->BlockLength;
  }
  else
  {
    // Set the data block size to same value as the previous block size
    // for same filter. It is possible on corrupt data to access here a new 
    // and not filled yet item of OldFilterLengths array. This is why above
    // we set new OldFilterLengths items to zero.
    StackFilter->BlockLength=FiltPos<OldFilterLengths.Size() ? OldFilterLengths[FiltPos]:0;
  }

  StackFilter->NextWindow=WrPtr!=UnpPtr && ((WrPtr-UnpPtr)&MAXWINMASK)<=BlockStart;

//  DebugLog("\nNextWindow: UnpPtr=%08x WrPtr=%08x BlockStart=%08x",UnpPtr,WrPtr,BlockStart);

  memset(StackFilter->Prg.InitR,0,sizeof(StackFilter->Prg.InitR));
  StackFilter->Prg.InitR[3]=VM_GLOBALMEMADDR;
  StackFilter->Prg.InitR[4]=StackFilter->BlockLength;
  StackFilter->Prg.InitR[5]=StackFilter->ExecCount;

  if (FirstByte & 0x10)   // set registers to optional parameters if any
  {
    unsigned int InitMask=VMCodeInp.fgetbits()>>9;
    VMCodeInp.faddbits(7);
    for (int I=0;I<7;I++)
      if (InitMask & (1<<I))
        StackFilter->Prg.InitR[I]=RarVM::ReadData(VMCodeInp);
  }

  if (NewFilter)
  {
    uint VMCodeSize=RarVM::ReadData(VMCodeInp);
    if (VMCodeSize>=0x10000 || VMCodeSize==0)
      return(false);
    Array<byte> VMCode(VMCodeSize);
    for (uint I=0;I<VMCodeSize;I++)
    {
      if (VMCodeInp.Overflow(3))
        return(false);
      VMCode[I]=VMCodeInp.fgetbits()>>8;
      VMCodeInp.faddbits(8);
    }
    VM.Prepare(&VMCode[0],VMCodeSize,&Filter->Prg);
  }
  StackFilter->Prg.AltCmd=&Filter->Prg.Cmd[0];
  StackFilter->Prg.CmdCount=Filter->Prg.CmdCount;

  size_t StaticDataSize=Filter->Prg.StaticData.Size();
  if (StaticDataSize>0 && StaticDataSize<VM_GLOBALMEMSIZE)
  {
    // read statically defined data contained in DB commands
    StackFilter->Prg.StaticData.Add(StaticDataSize);
    memcpy(&StackFilter->Prg.StaticData[0],&Filter->Prg.StaticData[0],StaticDataSize);
  }

  if (StackFilter->Prg.GlobalData.Size()<VM_FIXEDGLOBALSIZE)
  {
    StackFilter->Prg.GlobalData.Reset();
    StackFilter->Prg.GlobalData.Add(VM_FIXEDGLOBALSIZE);
  }
  byte *GlobalData=&StackFilter->Prg.GlobalData[0];
  for (int I=0;I<7;I++)
    VM.SetLowEndianValue((uint *)&GlobalData[I*4],StackFilter->Prg.InitR[I]);
  VM.SetLowEndianValue((uint *)&GlobalData[0x1c],StackFilter->BlockLength);
  VM.SetLowEndianValue((uint *)&GlobalData[0x20],0);
  VM.SetLowEndianValue((uint *)&GlobalData[0x2c],StackFilter->ExecCount);
  memset(&GlobalData[0x30],0,16);

  if (FirstByte & 8) // Put the data block passed as parameter if any.
  {
    if (VMCodeInp.Overflow(3))
      return(false);
    uint DataSize=RarVM::ReadData(VMCodeInp);
    if (DataSize>VM_GLOBALMEMSIZE-VM_FIXEDGLOBALSIZE)
      return(false);
    size_t CurSize=StackFilter->Prg.GlobalData.Size();
    if (CurSize<DataSize+VM_FIXEDGLOBALSIZE)
      StackFilter->Prg.GlobalData.Add(DataSize+VM_FIXEDGLOBALSIZE-CurSize);
    byte *GlobalData=&StackFilter->Prg.GlobalData[VM_FIXEDGLOBALSIZE];
    for (uint I=0;I<DataSize;I++)
    {
      if (VMCodeInp.Overflow(3))
        return(false);
      GlobalData[I]=VMCodeInp.fgetbits()>>8;
      VMCodeInp.faddbits(8);
    }
  }
  return(true);
}


bool Unpack::UnpReadBuf()
{
    int DataSize = ReadTop - InAddr; // Data left to process.
    if (DataSize < 0)
        return(false);

    if (InAddr > BitInput::MAX_SIZE/2)
    {
        // If we already processed more than half of buffer, let's move
        // remaining data into beginning to free more space for new data.
        if (DataSize > 0)
            memmove(InBuf,InBuf+InAddr,DataSize);
        InAddr = 0;
        ReadTop = DataSize;
    }
    else
    {
        DataSize = ReadTop;
    }

    int ReadCode = UnpIO->UnpRead(InBuf + DataSize, (BitInput::MAX_SIZE - DataSize) & ~0xf);
    if (ReadCode > 0)
        ReadTop+=ReadCode;

    ReadBorder=ReadTop-30;

    return ReadCode != -1;
}


void Unpack::UnpWriteBuf()
{
  unsigned int WrittenBorder=WrPtr;
  unsigned int WriteSize=(UnpPtr-WrittenBorder)&MAXWINMASK;
  for (size_t I=0;I<PrgStack.Size();I++)
  {
    // Here we apply filters to data which we need to write.
    // We always copy data to virtual machine memory before processing.
    // We cannot process them just in place in Window buffer, because
    // these data can be used for future string matches, so we must
    // preserve them in original form.

    UnpackFilter *flt=PrgStack[I];
    if (flt==NULL)
      continue;
    if (flt->NextWindow)
    {
      flt->NextWindow=false;
      continue;
    }
    unsigned int BlockStart=flt->BlockStart;
    unsigned int BlockLength=flt->BlockLength;
    if (((BlockStart-WrittenBorder)&MAXWINMASK)<WriteSize)
    {
      if (WrittenBorder!=BlockStart)
      {
        UnpWriteArea(WrittenBorder,BlockStart);
        WrittenBorder=BlockStart;
        WriteSize=(UnpPtr-WrittenBorder)&MAXWINMASK;
      }
      if (BlockLength<=WriteSize)
      {
        unsigned int BlockEnd=(BlockStart+BlockLength)&MAXWINMASK;
        if (BlockStart<BlockEnd || BlockEnd==0)
          VM.SetMemory(0,Window+BlockStart,BlockLength);
        else
        {
          unsigned int FirstPartLength=MAXWINSIZE-BlockStart;
          VM.SetMemory(0,Window+BlockStart,FirstPartLength);
          VM.SetMemory(FirstPartLength,Window,BlockEnd);
        }

        VM_PreparedProgram *ParentPrg=&Filters[flt->ParentFilter]->Prg;
        VM_PreparedProgram *Prg=&flt->Prg;

        if (ParentPrg->GlobalData.Size()>VM_FIXEDGLOBALSIZE)
        {
          // Copy global data from previous script execution if any.
          Prg->GlobalData.Alloc(ParentPrg->GlobalData.Size());
          memcpy(&Prg->GlobalData[VM_FIXEDGLOBALSIZE],&ParentPrg->GlobalData[VM_FIXEDGLOBALSIZE],ParentPrg->GlobalData.Size()-VM_FIXEDGLOBALSIZE);
        }

        ExecuteCode(Prg);

        if (Prg->GlobalData.Size()>VM_FIXEDGLOBALSIZE)
        {
          // Save global data for next script execution.
          if (ParentPrg->GlobalData.Size()<Prg->GlobalData.Size())
            ParentPrg->GlobalData.Alloc(Prg->GlobalData.Size());
          memcpy(&ParentPrg->GlobalData[VM_FIXEDGLOBALSIZE],&Prg->GlobalData[VM_FIXEDGLOBALSIZE],Prg->GlobalData.Size()-VM_FIXEDGLOBALSIZE);
        }
        else
          ParentPrg->GlobalData.Reset();

        byte *FilteredData=Prg->FilteredData;
        unsigned int FilteredDataSize=Prg->FilteredDataSize;

        delete PrgStack[I];
        PrgStack[I]=NULL;
        while (I+1<PrgStack.Size())
        {
          UnpackFilter *NextFilter=PrgStack[I+1];
          if (NextFilter==NULL || NextFilter->BlockStart!=BlockStart ||
              NextFilter->BlockLength!=FilteredDataSize || NextFilter->NextWindow)
            break;

          // Apply several filters to same data block.

          VM.SetMemory(0,FilteredData,FilteredDataSize);

          VM_PreparedProgram *ParentPrg=&Filters[NextFilter->ParentFilter]->Prg;
          VM_PreparedProgram *NextPrg=&NextFilter->Prg;

          if (ParentPrg->GlobalData.Size()>VM_FIXEDGLOBALSIZE)
          {
            // Copy global data from previous script execution if any.
            NextPrg->GlobalData.Alloc(ParentPrg->GlobalData.Size());
            memcpy(&NextPrg->GlobalData[VM_FIXEDGLOBALSIZE],&ParentPrg->GlobalData[VM_FIXEDGLOBALSIZE],ParentPrg->GlobalData.Size()-VM_FIXEDGLOBALSIZE);
          }

          ExecuteCode(NextPrg);

          if (NextPrg->GlobalData.Size()>VM_FIXEDGLOBALSIZE)
          {
            // Save global data for next script execution.
            if (ParentPrg->GlobalData.Size()<NextPrg->GlobalData.Size())
              ParentPrg->GlobalData.Alloc(NextPrg->GlobalData.Size());
            memcpy(&ParentPrg->GlobalData[VM_FIXEDGLOBALSIZE],&NextPrg->GlobalData[VM_FIXEDGLOBALSIZE],NextPrg->GlobalData.Size()-VM_FIXEDGLOBALSIZE);
          }
          else
            ParentPrg->GlobalData.Reset();

          FilteredData=NextPrg->FilteredData;
          FilteredDataSize=NextPrg->FilteredDataSize;
          I++;
          delete PrgStack[I];
          PrgStack[I]=NULL;
        }
        UnpIO->UnpWrite(FilteredData,FilteredDataSize);
        UnpSomeRead=true;
        WrittenFileSize+=FilteredDataSize;
        WrittenBorder=BlockEnd;
        WriteSize=(UnpPtr-WrittenBorder)&MAXWINMASK;
      }
      else
      {
        // Current filter intersects the window write border, so we adjust
        // the window border to process this filter next time, not now.
        for (size_t J=I;J<PrgStack.Size();J++)
        {
          UnpackFilter *flt=PrgStack[J];
          if (flt!=NULL && flt->NextWindow)
            flt->NextWindow=false;
        }
        WrPtr=WrittenBorder;
        return;
      }
    }
  }
      
  UnpWriteArea(WrittenBorder,UnpPtr);
  WrPtr=UnpPtr;
}


void Unpack::ExecuteCode(VM_PreparedProgram *Prg)
{
  if (Prg->GlobalData.Size()>0)
  {
    Prg->InitR[6]=(uint)WrittenFileSize;
    VM.SetLowEndianValue((uint *)&Prg->GlobalData[0x24],(uint)WrittenFileSize);
    VM.SetLowEndianValue((uint *)&Prg->GlobalData[0x28],(uint)(WrittenFileSize>>32));
    VM.Execute(Prg);
  }
}


void Unpack::UnpWriteArea(unsigned int StartPtr,unsigned int EndPtr)
{
  if (EndPtr!=StartPtr)
    UnpSomeRead=true;

  if (EndPtr<StartPtr)
  {
    UnpWriteData(&Window[StartPtr],-(int)StartPtr & MAXWINMASK);
    UnpWriteData(Window,EndPtr);
    UnpAllBuf=true;
  }
  else
  {
    UnpWriteData(&Window[StartPtr],EndPtr-StartPtr);
  }
}


void Unpack::UnpWriteData(byte *Data,size_t Size)
{
  if (WrittenFileSize>=DestUnpSize)
    return;

  size_t WriteSize=Size;
  int64 LeftToWrite=DestUnpSize-WrittenFileSize;
  if ((int64)WriteSize>LeftToWrite)
    WriteSize=(size_t)LeftToWrite;
  UnpIO->UnpWrite(Data,WriteSize);
  WrittenFileSize+=Size;
}

bool Unpack::ReadTables()
{
  byte BitLength[BC];
  byte Table[HUFF_TABLE_SIZE];
  if (InAddr>ReadTop-25)
    if (!UnpReadBuf())
      return(false);
  faddbits((8-InBit)&7);
  uint BitField=fgetbits();
  if (BitField & 0x8000)
  {
    UnpBlockType=BLOCK_PPM;
    return(PPM.DecodeInit(this,PPMEscChar));
  }
  UnpBlockType=BLOCK_LZ;
  
  PrevLowDist=0;
  LowDistRepCount=0;

  if (!(BitField & 0x4000))
    memset(UnpOldTable,0,sizeof(UnpOldTable));
  faddbits(2);

  for (int I=0;I<BC;I++)
  {
    int Length=(byte)(fgetbits() >> 12);
    faddbits(4);
    if (Length==15)
    {
      int ZeroCount=(byte)(fgetbits() >> 12);
      faddbits(4);
      if (ZeroCount==0)
        BitLength[I]=15;
      else
      {
#if __GNUC__ >= 7 && !defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

        ZeroCount+=2;
        while (ZeroCount-- > 0 && I<int(sizeof(BitLength)/sizeof(BitLength[0])))
          BitLength[I++]=0;
        I--;

#if __GNUC__ >= 7 && !defined(__clang__)
    #pragma GCC diagnostic pop
#endif
      }
    }
    else
      BitLength[I]=Length;
  }
  MakeDecodeTables(BitLength,&BD,BC);

  const int TableSize=HUFF_TABLE_SIZE;
  for (int I=0;I<TableSize;)
  {
    if (InAddr>ReadTop-5)
      if (!UnpReadBuf())
        return(false);
    int Number=DecodeNumber(&BD);
    if (Number<16)
    {
      Table[I]=(Number+UnpOldTable[I]) & 0xf;
      I++;
    }
    else
      if (Number<18)
      {
        int N;
        if (Number==16)
        {
          N=(fgetbits() >> 13)+3;
          faddbits(3);
        }
        else
        {
          N=(fgetbits() >> 9)+11;
          faddbits(7);
        }
        while (N-- > 0 && I<TableSize)
        {
          Table[I]=Table[I-1];
          I++;
        }
      }
      else
      {
        int N;
        if (Number==18)
        {
          N=(fgetbits() >> 13)+3;
          faddbits(3);
        }
        else
        {
          N=(fgetbits() >> 9)+11;
          faddbits(7);
        }
        while (N-- > 0 && I<TableSize)
          Table[I++]=0;
      }
  }
  TablesRead=true;
  if (InAddr>ReadTop)
    return(false);
  MakeDecodeTables(&Table[0],&LD,NC);
  MakeDecodeTables(&Table[NC],&DD,DC);
  MakeDecodeTables(&Table[NC+DC],&LDD,LDC);
  MakeDecodeTables(&Table[NC+DC+LDC],&RD,RC);
  memcpy(UnpOldTable,Table,sizeof(UnpOldTable));
  return(true);
}


void Unpack::UnpInitData(int Solid)
{
  if (!Solid)
  {
    TablesRead=false;
    memset(OldDist,0,sizeof(OldDist));
    OldDistPtr=0;
    LastDist=LastLength=0;
//    memset(Window,0,MAXWINSIZE);
    memset(UnpOldTable,0,sizeof(UnpOldTable));
    memset(&LD,0,sizeof(LD));
    memset(&DD,0,sizeof(DD));
    memset(&LDD,0,sizeof(LDD));
    memset(&RD,0,sizeof(RD));
    memset(&BD,0,sizeof(BD));
    UnpPtr=WrPtr=0;
    PPMEscChar=2;
    UnpBlockType=BLOCK_LZ;

    InitFilters();
  }
  InitBitInput();
  WrittenFileSize=0;
  ReadTop=0;
  ReadBorder=0;
  UnpInitData20(Solid);
}


void Unpack::InitFilters()
{
  OldFilterLengths.Reset();
  LastFilter=0;

  for (size_t I=0;I<Filters.Size();I++)
    delete Filters[I];
  Filters.Reset();
  for (size_t I=0;I<PrgStack.Size();I++)
    delete PrgStack[I];
  PrgStack.Reset();
}


// LengthTable contains the length in bits for every element of alphabet.
// Dec is the structure to decode Huffman code/
// Size is size of length table and DecodeNum field in Dec structure,
void Unpack::MakeDecodeTables(byte *LengthTable,DecodeTable *Dec,uint Size)
{
  // Size of alphabet and DecodePos array.
  Dec->MaxNum=Size;

  // Calculate how many entries for every bit length in LengthTable we have.
  uint LengthCount[16];
  memset(LengthCount,0,sizeof(LengthCount));
  for (size_t I=0;I<Size;I++)
    LengthCount[LengthTable[I] & 0xf]++;

  // We must not calculate the number of zero length codes.
  LengthCount[0]=0;

  // Set the entire DecodeNum to zero.
  memset(Dec->DecodeNum,0,Size*sizeof(*Dec->DecodeNum));

  // Initialize not really used entry for zero length code.
  Dec->DecodePos[0]=0;

  // Start code for bit length 1 is 0.
  Dec->DecodeLen[0]=0;

  // Right aligned upper limit code for current bit length.
  uint UpperLimit=0;

  for (size_t I=1;I<16;I++)
  {
    // Adjust the upper limit code.
    UpperLimit+=LengthCount[I];

    // Left aligned upper limit code.
    uint LeftAligned=UpperLimit<<(16-I);

    // Prepare the upper limit code for next bit length.
    UpperLimit*=2;

    // Store the left aligned upper limit code.
    Dec->DecodeLen[I]=(uint)LeftAligned;

    // Every item of this array contains the sum of all preceding items.
    // So it contains the start position in code list for every bit length. 
    Dec->DecodePos[I]=Dec->DecodePos[I-1]+LengthCount[I-1];
  }

  // Prepare the copy of DecodePos. We'll modify this copy below,
  // so we cannot use the original DecodePos.
  uint CopyDecodePos[16];
  memcpy(CopyDecodePos,Dec->DecodePos,sizeof(CopyDecodePos));

  // For every bit length in the bit length table and so for every item
  // of alphabet.
  for (uint I=0;I<Size;I++)
  {
    // Get the current bit length.
    byte CurBitLength=LengthTable[I] & 0xf;

    if (CurBitLength!=0)
    {
      // Last position in code list for current bit length.
      uint LastPos=CopyDecodePos[CurBitLength];

      // Prepare the decode table, so this position in code list will be
      // decoded to current alphabet item number.
      Dec->DecodeNum[LastPos]=(ushort)I;

      // We'll use next position number for this bit length next time.
      // So we pass through the entire range of positions available
      // for every bit length.
      CopyDecodePos[CurBitLength]++;
    }
  }

  // Define the number of bits to process in quick mode. We use more bits
  // for larger alphabets. More bits means that more codes will be processed
  // in quick mode, but also that more time will be spent to preparation
  // of tables for quick decode.
  switch (Size)
  {
    case NC:
    case NC20:
      Dec->QuickBits=MAX_QUICK_DECODE_BITS;
      break;
    default:
      Dec->QuickBits=MAX_QUICK_DECODE_BITS-3;
      break;
  }

  // Size of tables for quick mode.
  uint QuickDataSize=1<<Dec->QuickBits;

  // Bit length for current code, start from 1 bit codes. It is important
  // to use 1 bit instead of 0 for minimum code length, so we are moving
  // forward even when processing a corrupt archive.
  uint CurBitLength=1;

  // For every right aligned bit string which supports the quick decoding.
  for (uint Code=0;Code<QuickDataSize;Code++)
  {
    // Left align the current code, so it will be in usual bit field format.
    uint BitField=Code<<(16-Dec->QuickBits);

    // Prepare the table for quick decoding of bit lengths.

    // Find the upper limit for current bit field and adjust the bit length
    // accordingly if necessary.
    while (CurBitLength < ASIZE(Dec->DecodeLen) &&  BitField >= Dec->DecodeLen[CurBitLength])
      CurBitLength++;

    // Translation of right aligned bit string to bit length.
    Dec->QuickLen[Code]=CurBitLength;

    // Prepare the table for quick translation of position in code list
    // to position in alphabet.

    // Calculate the distance from the start code for current bit length.
    uint Dist=BitField-Dec->DecodeLen[CurBitLength-1];

    // Right align the distance.
    Dist>>=(16-CurBitLength);

    // Now we can calculate the position in the code list. It is the sum
    // of first position for current bit length and right aligned distance
    // between our bit field and start code for current bit length.
    uint Pos=Dec->DecodePos[CurBitLength]+Dist;

    if (Pos<Size) // Safety check for damaged archives.
    {
      // Define the code to alphabet number translation.
      Dec->QuickNum[Code]=Dec->DecodeNum[Pos];
    }
    else
      Dec->QuickNum[Code]=0;
  }
}
