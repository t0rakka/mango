#include "rar.hpp"

ComprDataIO::ComprDataIO()
{
  Init();
}


void ComprDataIO::Init()
{
  UnpackFromMemory = false;
  UnpackToMemory = false;
  UnpPackedSize=0;
  Encryption=0;
  Decryption=0;
  TotalPackRead=0;
  CurPackRead=CurPackWrite=CurUnpRead=CurUnpWrite=0;
  PackFileCRC=UnpFileCRC=PackedCRC=0xffffffff;
}


int ComprDataIO::UnpRead(byte *Addr,size_t Count)
{
    int RetCode=0;
    //int TotalRead=0;
    byte *ReadAddr = Addr;

    if (Count > 0)
    {
        size_t ReadSize = ((int64)Count > UnpPackedSize) ? (size_t)UnpPackedSize : Count;

        memcpy(ReadAddr, UnpackFromMemoryAddr, ReadSize);
        RetCode = (int)ReadSize;
        UnpackFromMemorySize -= ReadSize;
        UnpackFromMemoryAddr += ReadSize;

        CurUnpRead+=RetCode;
        //TotalRead+=RetCode;

        ReadAddr += RetCode;
        Count -= RetCode;

        UnpPackedSize -= RetCode;
    }

  /*
  if (RetCode!=-1)
  {
    RetCode=TotalRead;
    if (Decryption)
      if (Decryption<20)
        Decrypt.Crypt(Addr,RetCode,(Decryption==15) ? NEW_CRYPT : OLD_DECODE);
      else
        if (Decryption==20)
          for (int I=0;I<RetCode;I+=16)
            Decrypt.DecryptBlock20(&Addr[I]);
        else
        {
          int CryptSize=(RetCode & 0xf)==0 ? RetCode:((RetCode & ~0xf)+16);
          Decrypt.DecryptBlock(Addr,CryptSize);
        }
  }
  */

  return(RetCode);
}



void ComprDataIO::UnpWrite(byte *Addr,size_t Count)
{
    if (Count <= UnpackToMemorySize)
    {
        memcpy(UnpackToMemoryAddr,Addr,Count);
        UnpackToMemoryAddr+=Count;
        UnpackToMemorySize-=Count;
    }

    CurUnpWrite += Count;

    //if (!SkipUnpCRC)
    {
        /*
        if (((Archive *)SrcFile)->OldFormat)
            UnpFileCRC = OldCRC((ushort)UnpFileCRC,Addr,Count);
        else
            UnpFileCRC = CRC(UnpFileCRC,Addr,Count);
        */
    }
}


/*
void ComprDataIO::SetEncryption(int Method,SecPassword *Password,const byte *Salt,bool Encrypt,bool HandsOffHash)
{
  if (Encrypt)
  {
    Encryption=Password->IsSet() ? Method:0;
    Crypt.SetCryptKeys(Password,Salt,Encrypt,false,HandsOffHash);
  }
  else
  {
    Decryption=Password->IsSet() ? Method:0;
    Decrypt.SetCryptKeys(Password,Salt,Encrypt,Method<29,HandsOffHash);
  }
}
*/

void ComprDataIO::SetAV15Encryption()
{
  Decryption=15;
  Decrypt.SetAV15Encryption();
}


void ComprDataIO::SetCmt13Encryption()
{
  Decryption=13;
  Decrypt.SetCmt13Encryption();
}
