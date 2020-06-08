#ifndef _TCycBuffer_H_
#define _TCycBuffer_H_

#include "KYCache.h"

namespace KYLib
{
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/* TCycBuffer - ʹ��ѭ����ʽ�Ļ������� */

// ע: Ϊ�˶��̴߳�ȡ��ȫ, Push �� Pop ���������߳�ʱ����ͬʱ����������Ҫ����,
//     �����߳� Push ʱ������������, ���߳� Pop/Peek/Lose ʱ������������!

    class TCycBuffer
    {
    private:
        // ���ݿ������
        typedef struct
        {
            Longword    PushSize;            // ѹ�����ݳߴ�
            Longword    PopSize;             // �������ݳߴ�
            void*       next;                // ��һ��
        } TIndex, *PIndex;

    public:
        // ���ݿ�ĳߴ�����
        enum TType    
        {
            stSize2_8   = 0,     // 2^8  �ֽ���: 256
            stSize2_9   = 1,     // 2^9  �ֽ���: 512
            stSize2_10  = 2,     // 2^10 �ֽ���: 1024
            stSize2_11  = 3,     // 2^11 �ֽ���: 2048
            stSize2_12  = 4,     // 2^12 �ֽ���: 4096
            stSize2_13  = 5,     // 2^13 �ֽ���: 8192
            stSize2_14  = 6,     // 2^14 �ֽ���: 16384
            stSize2_15  = 7,     // 2^15 �ֽ���: 32768
            stSize2_16  = 8      // 2^16 �ֽ���: 65536
        };   

        // ��д���ݵķ���, �������ݳߴ�
        typedef long (TObject::*TDoRead)(void* AData, long ASize);
        typedef long (TObject::*TDoWrite)(const void* AData, long ASize);

        // ��д���ݵ� Push �� Pop ��������
        // ע: ��Ϊ Push �� Pop �������ط���, �������޷�ָ���ĸ�����, ������ȷ���巽
        // ������, �����ñ�����ʶ�𵽾����ĸ�������
        typedef Longword (TCycBuffer::*TDoPop)(void* AData, Longword ASize);
        typedef Longword (TCycBuffer::*TDoPush)(const void* AData, Longword ASize);

    public:
        // ���캯��
        // 1. AMaxCache   ���ݿ����󻺳����
        // 2. ASizeType   ���ݿ�ĳߴ�����, ����ÿ�����ݿ�ĳߴ綨��
        TCycBuffer(Word AMaxCache = 64, Byte ASizeType = stSize2_10);
        virtual ~TCycBuffer();

        // ����
        Longword       Size() const         { return push_size_ - pop_size_; }
        Longword       MaxSize() const      { return max_size_; }             // default: 0xFFFFFFFF
        Longword       PopSize() const      { return pop_size_; }             // default: 0
        Longword       PushSize() const     { return push_size_; }            // default: 0
        Longword       BlockSize() const    { return block_size_; }           // default: 2 ^ (ASizeType + 8)
        TType          SizeType() const     { return (TType)size_type_; }     // default: ASizeType   

        // �����������ߴ�
        void           SetMaxSize(Longword AMaxSize)
                        { max_size_ = AMaxSize; }

        // ���ݼ��뻺����
        Longword       Push(TDoRead ADoRead, void* AObject, Longword ASize);
        Longword       Push(const void* AData, Longword ASize);

        Longword       Push(TCycBuffer& ABuffer, Longword ASize)
                        { return Push((TDoRead)(TDoPop)&TCycBuffer::Pop, &ABuffer, ASize); }
        Longword       Push(TCycBuffer& ABuffer)
                        { return Push(ABuffer, 0xFFFFFFFF); }
        // Longword       Push(const KYString& AText)
                        // { return Push((const char*)AText, AText.Length()); }

        // �ӻ������ж�ȡ����, ���ض�ȡ�����ݳߴ�
        Longword       Pop(TDoWrite ADoWrite, void* AObject, Longword ASize);
        Longword       Pop(void* AData, Longword ASize);

        Longword       Pop(TCycBuffer& ABuffer, Longword ASize)
                        { return Pop((TDoWrite)(TDoPush)&TCycBuffer::Push, &ABuffer, ASize); }
        Longword       Pop(TCycBuffer& ABuffer)
                        { return Pop(ABuffer, 0xFFFFFFFF); }

        // �ӻ�������Ԥ��ȡ����, ����Ԥ��ȡ�����ݳߴ�
        Longword       Peek(TDoWrite ADoWrite, void* AObject, Longword ASize) const;
        Longword       Peek(void* AData, Longword ASize) const;

        Longword       Peek(TCycBuffer& ABuffer, Longword ASize) const
                        { return Peek((TDoWrite)(TDoPush)&TCycBuffer::Push, &ABuffer, ASize); }
        Longword       Peek(TCycBuffer& ABuffer) const
                        { return Peek(ABuffer, 0xFFFFFFFF); }
        
        // �ӻ������ж���ָ���ߴ������, ���ض��������ݳߴ�
        Longword       Lose(Longword ASize);
        Longword       Lose() { return Lose(0xFFFFFFFF); }

    private:
        // �����ݿ���ѹ������
        Longword       DoPush(TIndex* AIndex, TDoRead ADoRead, void* AObject, Longword ASize) const;
        Longword       DoPush(TIndex* AIndex, const void* AData, Longword ASize) const;

        // �����ݿ���Ԥ��ȡ����
        Longword       DoPeek(TIndex* AIndex, TDoWrite ADoWrite, void* AObject, Longword ASize) const;
        Longword       DoPeek(TIndex* AIndex, void* AData, Longword ASize) const;

        // �����ݿ��е�������
        Longword       DoPop(TIndex* AIndex, TDoWrite ADoWrite, void* AObject, Longword ASize) const;
        Longword       DoPop(TIndex* AIndex, void* AData, Longword ASize) const;

    private:
        TIndex*        head_;          // ���ݿ��ͷ����
        TIndex*        tail_;          // ���ݿ��β����
        Longword       block_size_;          // ���ݿ�ĳߴ�
        Longword       block_mask_;          // ���ݿ������
        Longword       push_size_;           // ѹ�����ݳߴ�
        Longword       pop_size_;            // �������ݳߴ�
        Longword       max_size_;            // �������ߴ�
        Byte           size_type_;           // ���ݿ�ĳߴ缶��
        TKYCache*      cache_;
    };

}

#endif
