#ifndef _TCycBuffer_H_
#define _TCycBuffer_H_

#include "KYCache.h"

namespace KYLib
{
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/* TCycBuffer - 使用循环方式的缓冲区类 */

// 注: 为了多线程存取安全, Push 和 Pop 分属二个线程时可以同时操作而不需要加锁,
//     但多线程 Push 时必须用锁控制, 多线程 Pop/Peek/Lose 时必须用锁控制!

    class TCycBuffer
    {
    private:
        // 数据块的索引
        typedef struct
        {
            Longword    PushSize;            // 压入数据尺寸
            Longword    PopSize;             // 弹出数据尺寸
            void*       next;                // 下一项
        } TIndex, *PIndex;

    public:
        // 数据块的尺寸类型
        enum TType    
        {
            stSize2_8   = 0,     // 2^8  字节数: 256
            stSize2_9   = 1,     // 2^9  字节数: 512
            stSize2_10  = 2,     // 2^10 字节数: 1024
            stSize2_11  = 3,     // 2^11 字节数: 2048
            stSize2_12  = 4,     // 2^12 字节数: 4096
            stSize2_13  = 5,     // 2^13 字节数: 8192
            stSize2_14  = 6,     // 2^14 字节数: 16384
            stSize2_15  = 7,     // 2^15 字节数: 32768
            stSize2_16  = 8      // 2^16 字节数: 65536
        };   

        // 读写数据的方法, 返回数据尺寸
        typedef long (TObject::*TDoRead)(void* AData, long ASize);
        typedef long (TObject::*TDoWrite)(const void* AData, long ASize);

        // 读写数据的 Push 和 Pop 方法类型
        // 注: 因为 Push 和 Pop 存在重载方法, 编译器无法指定哪个方法, 所以明确定义方
        // 法类型, 可以让编译器识别到具体哪个方法。
        typedef Longword (TCycBuffer::*TDoPop)(void* AData, Longword ASize);
        typedef Longword (TCycBuffer::*TDoPush)(const void* AData, Longword ASize);

    public:
        // 构造函数
        // 1. AMaxCache   数据块的最大缓冲个数
        // 2. ASizeType   数据块的尺寸类型, 既是每个数据块的尺寸定义
        TCycBuffer(Word AMaxCache = 64, Byte ASizeType = stSize2_10);
        virtual ~TCycBuffer();

        // 属性
        Longword       Size() const         { return push_size_ - pop_size_; }
        Longword       MaxSize() const      { return max_size_; }             // default: 0xFFFFFFFF
        Longword       PopSize() const      { return pop_size_; }             // default: 0
        Longword       PushSize() const     { return push_size_; }            // default: 0
        Longword       BlockSize() const    { return block_size_; }           // default: 2 ^ (ASizeType + 8)
        TType          SizeType() const     { return (TType)size_type_; }     // default: ASizeType   

        // 设置数据最大尺寸
        void           SetMaxSize(Longword AMaxSize)
                        { max_size_ = AMaxSize; }

        // 数据加入缓冲区
        Longword       Push(TDoRead ADoRead, void* AObject, Longword ASize);
        Longword       Push(const void* AData, Longword ASize);

        Longword       Push(TCycBuffer& ABuffer, Longword ASize)
                        { return Push((TDoRead)(TDoPop)&TCycBuffer::Pop, &ABuffer, ASize); }
        Longword       Push(TCycBuffer& ABuffer)
                        { return Push(ABuffer, 0xFFFFFFFF); }
        // Longword       Push(const KYString& AText)
                        // { return Push((const char*)AText, AText.Length()); }

        // 从缓冲区中读取数据, 返回读取的数据尺寸
        Longword       Pop(TDoWrite ADoWrite, void* AObject, Longword ASize);
        Longword       Pop(void* AData, Longword ASize);

        Longword       Pop(TCycBuffer& ABuffer, Longword ASize)
                        { return Pop((TDoWrite)(TDoPush)&TCycBuffer::Push, &ABuffer, ASize); }
        Longword       Pop(TCycBuffer& ABuffer)
                        { return Pop(ABuffer, 0xFFFFFFFF); }

        // 从缓冲区中预读取数据, 返回预读取的数据尺寸
        Longword       Peek(TDoWrite ADoWrite, void* AObject, Longword ASize) const;
        Longword       Peek(void* AData, Longword ASize) const;

        Longword       Peek(TCycBuffer& ABuffer, Longword ASize) const
                        { return Peek((TDoWrite)(TDoPush)&TCycBuffer::Push, &ABuffer, ASize); }
        Longword       Peek(TCycBuffer& ABuffer) const
                        { return Peek(ABuffer, 0xFFFFFFFF); }
        
        // 从缓冲区中丢弃指定尺寸的数据, 返回丢弃的数据尺寸
        Longword       Lose(Longword ASize);
        Longword       Lose() { return Lose(0xFFFFFFFF); }

    private:
        // 从数据块中压入数据
        Longword       DoPush(TIndex* AIndex, TDoRead ADoRead, void* AObject, Longword ASize) const;
        Longword       DoPush(TIndex* AIndex, const void* AData, Longword ASize) const;

        // 从数据块中预读取数据
        Longword       DoPeek(TIndex* AIndex, TDoWrite ADoWrite, void* AObject, Longword ASize) const;
        Longword       DoPeek(TIndex* AIndex, void* AData, Longword ASize) const;

        // 从数据块中弹出数据
        Longword       DoPop(TIndex* AIndex, TDoWrite ADoWrite, void* AObject, Longword ASize) const;
        Longword       DoPop(TIndex* AIndex, void* AData, Longword ASize) const;

    private:
        TIndex*        head_;          // 数据块的头索引
        TIndex*        tail_;          // 数据块的尾索引
        Longword       block_size_;          // 数据块的尺寸
        Longword       block_mask_;          // 数据块的掩码
        Longword       push_size_;           // 压入数据尺寸
        Longword       pop_size_;            // 弹出数据尺寸
        Longword       max_size_;            // 数据最大尺寸
        Byte           size_type_;           // 数据块的尺寸级别
        TKYCache*      cache_;
    };

}

#endif
