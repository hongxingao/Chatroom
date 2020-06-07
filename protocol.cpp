#include "protocol.h"

namespace _ghx
{
    void  DWordTo4Byte(DWord AValue, Byte* ABytes)
    {
        ABytes[0] = AValue;
        ABytes[1] = (AValue >> 8);
        ABytes[2] = (AValue >> 16);
        ABytes[3] = (AValue >> 24);
    }

    void WordTo2Byte(Word AValue, Byte* ABytes)
    {
        ABytes[0] = AValue;
        ABytes[1] = (AValue >> 8);
    }

    DWord _4ByteToDWord(const Byte* ABytes)
    {
        return ABytes[0] | (ABytes[1] << 8) | (ABytes[2] << 16) | (ABytes[3] << 24);
    }

    Word _2ByteToWord(const Byte* ABytes)
    {
        return ABytes[0] | (ABytes[1] << 8);
    }

    // 字节流-》消息头
    bool BufferToHead(const Byte* ABuffer, TDataHead& AHead)
    {
        bool result = false;
        DWord tmp_crc = _4ByteToDWord(ABuffer);
        if (BufferToCRC32(ABuffer + sizeof(DWord), sizeof(TDataHead)-sizeof(DWord)) == tmp_crc)
        {
            AHead.crc = tmp_crc;
            AHead.serial_num = _4ByteToDWord(ABuffer + __Offset__(TDataHead, serial_num));
            AHead.len = _2ByteToWord(ABuffer + __Offset__(TDataHead, len));
            AHead.msg_type = ABuffer[__Offset__(TDataHead, msg_type)];
            result = true;
        }

        return result;
    }

    // 消息头转字节流
    void HeadToBuffer(const TDataHead& AHead, Byte* ABuffer)
    {
        DWordTo4Byte(AHead.serial_num, ABuffer + __Offset__(TDataHead, serial_num));
        WordTo2Byte(AHead.len, ABuffer + __Offset__(TDataHead, len));
        ABuffer[__Offset__(TDataHead, msg_type)] = AHead.msg_type;
        DWord tmp_crc = BufferToCRC32(ABuffer + sizeof(DWord), sizeof(TDataHead)-sizeof(DWord));
        DWordTo4Byte(tmp_crc, ABuffer);
    }

    // 字节流-》登录消息体
    bool BufferToLogin(const TDataHead& AHead, const Byte* ABuffer, TLoginMsg& AMsg)
    {
        bool result = false;
        if (AHead.len == sizeof(DWord))
        {
            AMsg.head = AHead;
            AMsg.id = _4ByteToDWord(ABuffer);
            result = true;
        }

        return result;
    }

    // 登录消息体-》字节流
    void LoginToBuffer(const TLoginMsg& AMsg, Byte* ABuffer)
    {
        HeadToBuffer(AMsg.head, ABuffer);
        DWordTo4Byte(AMsg.id, ABuffer + kHeadLen);
    }

    // 字节流-》发送消息体
    bool BufferToSend(const TDataHead& AHead, const Byte* ABuffer, TSendMsg& AMsg)
    {
        bool result = false;
        if (AHead.len > sizeof(DWord))
        {
            AMsg.head = AHead;
            AMsg.id = _4ByteToDWord(ABuffer);
            AMsg.data.SetString((char*)ABuffer + sizeof(DWord), AHead.len - sizeof(DWord));
            result = true;
        }

        return result;
    }

    // 发送消息体-》字节流
    void SendToBuffer(const TSendMsg& AMsg, Byte* ABuffer)
    {
        HeadToBuffer(AMsg.head, ABuffer);
        DWordTo4Byte(AMsg.id, ABuffer + kHeadLen);
        memcpy(ABuffer + kHeadLen + sizeof(DWord), (char*)AMsg.data, AMsg.data.Length());
    }

    // 字节流-》广播消息体
    bool BufferToBroadcast(const TDataHead& AHead, const Byte* ABuffer, TBroadcastdMsg& AMsg)
    {
        bool result = false;
        if (AHead.len > 0)
        {
            AMsg.head = AHead;
            AMsg.data.SetString((char*)ABuffer, AHead.len);
            result = true;
        }

        return result;
    }

    // 广播消息体-》字节流
    void BroadcastToBuffer(const TBroadcastdMsg& AMsg, Byte* ABuffer)
    {
        HeadToBuffer(AMsg.head, ABuffer);
        memcpy(ABuffer + kHeadLen, (char*)AMsg.data, AMsg.data.Length());
    }

    // 字节流-》查询消息体
    bool BufferToQuery(const TDataHead& AHead, const Byte* ABuffer, TQueryMsg& AMsg)
    {
        bool result = false;
        if (AHead.len == sizeof(DWord) + sizeof(DWord))
        {
            AMsg.head = AHead;
            AMsg.begin = _4ByteToDWord(ABuffer);
            AMsg.end = _4ByteToDWord(ABuffer + sizeof(DWord));
            result = true;
        }

        return result;
    }

    // 查询消息体-》字节流
    void QueryToBuffer(const TQueryMsg& AMsg, Byte* ABuffer)
    {
        HeadToBuffer(AMsg.head, ABuffer);
        DWordTo4Byte(AMsg.begin, ABuffer + kHeadLen);
        DWordTo4Byte(AMsg.end, ABuffer + kHeadLen + sizeof(DWord));
    }

    // 字节流-》回复消息体
    bool BufferToReplyMsg(const TDataHead& AHead, const Byte* ABuffer, TReplyMsg& AMsg)
    {
        bool result = false;
        if (AHead.len == sizeof(char))
        {
            AMsg.head = AHead;
            AMsg.reply_type = *ABuffer;
            result = true;
        }

        return result;
    }

    // 回复消息体-》字节流
    void ReplyMsgToBuffer(const TReplyMsg& AMsg, Byte* ABuffer)
    {
        HeadToBuffer(AMsg.head, ABuffer);
        ABuffer[kHeadLen] = AMsg.reply_type;
    }

    // 字节流-》回复广播消息体
    bool BufferToReplyBroadcast(const TDataHead& AHead, const Byte* ABuffer, TReplyBroadcast& AMsg)
    {
        
        bool result = false;
        if (AHead.len == sizeof(char) + sizeof(DWord))
        {
            AMsg.head = AHead;
            AMsg.reply_type = *ABuffer;
            AMsg.count = _4ByteToDWord(ABuffer + sizeof(char));
            result = true;
        }

        return result;
    }

    // 回复广播消息体-》字节流
    void ReplyBroadcastToBuffer(const TReplyBroadcast& AMsg, Byte* ABuffer)
    {
        HeadToBuffer(AMsg.head, ABuffer);
        ABuffer[kHeadLen] = AMsg.reply_type;
        DWordTo4Byte(AMsg.count, ABuffer + kHeadLen + sizeof(char));
    }

    // 字节流-》回复查询消息体
    bool BufferToReplyQuery(const TDataHead& AHead, const Byte* ABuffer, TReplyQuery& AMsg)
    {
        bool result = false;
        if (AHead.len >= sizeof(char) + sizeof(DWord))
        {
            const Byte* tmp_buf = ABuffer;
            AMsg.head = AHead;
            AMsg.reply_type = *tmp_buf;
            AMsg.count = _4ByteToDWord(++tmp_buf);
            tmp_buf += 4;
            if (((AHead.len - 5) >> 2) == AMsg.count)
            {
                for (int i = 0; i < AMsg.count; i++, tmp_buf += 4)
                    AMsg.data[i] = _4ByteToDWord(tmp_buf);
                result = true;
            }
        }

        return result;
    }

    // 回复查询消息体-》字节流
    void ReplyQueryToBuffer(const TReplyQuery& AMsg, Byte* ABuffer)
    {
        Byte* tmp_buf = ABuffer;
        HeadToBuffer(AMsg.head, tmp_buf);
        tmp_buf += kHeadLen;
        *tmp_buf = AMsg.reply_type;
        DWordTo4Byte(AMsg.count, ++tmp_buf);
        tmp_buf += 4;
        for (int i = 0; i < AMsg.count; i++, tmp_buf += 4)
            DWordTo4Byte(AMsg.data[i], tmp_buf);
    }

}