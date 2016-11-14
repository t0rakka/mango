#ifndef _RAR_DATAIO_
#define _RAR_DATAIO_

class CmdAdd;
class Unpack;


class ComprDataIO
{
  public:
    bool UnpackFromMemory;
    size_t UnpackFromMemorySize;
    byte *UnpackFromMemoryAddr;

    bool UnpackToMemory;
    size_t UnpackToMemorySize;
    byte *UnpackToMemoryAddr;

    int64 UnpPackedSize;

    CryptData Crypt;
    CryptData Decrypt;

  public:
    ComprDataIO();
    void Init();
    int UnpRead(byte *Addr,size_t Count);
    void UnpWrite(byte *Addr,size_t Count);
    void SetAV15Encryption();
    void SetCmt13Encryption();

    int64 TotalPackRead;
    int64 UnpArcSize;
    int64 CurPackRead,CurPackWrite,CurUnpRead,CurUnpWrite;

    uint PackFileCRC,UnpFileCRC,PackedCRC;

    int Encryption;
    int Decryption;
};

#endif
