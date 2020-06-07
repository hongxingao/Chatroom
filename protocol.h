#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include "KYLib.h"

namespace _ghx
{
    // 消息类型
    typedef enum TMsgTypeTag
    {
        kLogin                 = 0,    // 登录请求
        kLoginReply            = 1,    // 登录回复

        kLoginOut              = 2,    // 登出请求
        KLoginOutReply         = 3,    // 登出回复

        kPrivateChat           = 4,    // 私聊给某id
        kPrivateChatReply      = 5,    // 私聊回复

        kTransPrivateChat      = 6,    // 转发私聊
        kTransPrivateChatReply = 7,    // 转发私聊回复

        kBroadCast             = 8,    // 广播请求
        kBroadCaseReply        = 9,    // 广播回复

        kTransBroadCast        = 10,    // 转发广播请求

        kQuery                 = 12,    // 列表查询请求
        kQueryReply            = 13     // 列表回复
    } TMsgType;

    // 消息头部  
    #pragma pack(push, 1)
    typedef struct
    {
        DWord           crc;            // 校验码
        DWord           serial_num;     // 消息序号
        Word            len;            // 消息长度
        Byte            msg_type;       // 消息类
    } TDataHead;
    #pragma pack(pop)

    // -----------请求消息------------

    // 登录请求消息体
    typedef struct
    {
        TDataHead       head;            //头部
        DWord           id;                //用户id
    } TLoginMsg;

    // 请求私聊,转发私聊,转发广播消息体
    // 私聊时为目的id， 转发私聊，转发广播时为源id
    typedef struct
    {
        TDataHead       head;
        DWord           id;
        KYString        data;
    } TSendMsg;
    // word size = data.length();
    // char* p = (char*)data;

    // 请求广播消息体
    typedef struct
    {
        TDataHead       head;
        KYString        data;;
    } TBroadcastdMsg;

    // 请求查询消息体
    typedef struct
    {
        TDataHead        head;
        DWord            begin;
        DWord            end;
    } TQueryMsg;

    //----------回复消息

    //回复登录  回复私聊
    typedef struct
    {
        TDataHead        head;
        char             reply_type;
    } TReplyMsg;

    // 回复广播
    typedef struct
    {
        TDataHead        head;
        char             reply_type;
        DWord            count;
    } TReplyBroadcast;

    // 回复查询
    typedef struct
    {
        TDataHead        head;
        char             reply_type;
        DWord            count;
        DWord*           data;
    } TReplyQuery;

    // 消息控制 检查消息的回应
    typedef struct
    {
        DWord            serial_num;        // 消息序号
        DWORD            time;
        bool             is_reply;
        char             reply_type;
        TKYList          data;
    } TDataCtl;

    enum TMsgLen
    {
        // 消息头部长度
        kHeadLen        = sizeof(TDataHead),

        // 发送消息体长度
        kLoginMsgLen    = sizeof(TLoginMsg),
        kQueryMsgLen    = sizeof(TQueryMsg),

        // 回复消息体长度
        kReplyMsgLen    = sizeof(TReplyMsg),

        //消息控制长度
        kMsgCtlLen      = sizeof(TDataCtl),

        // 最大消息长度
        kMaxMsgLen      = 512
    };

    // 字节序转换函数
    DWord _4ByteToDWord(const Byte* ABytes);
    void  DWordTo4Byte(DWord AValue, Byte* ABytes);
    Word _2ByteToWord(const Byte* ABytes);
    void WordTo2Byte(Word AValue, Byte* ABytes);

    // 协议结构《==》字节流 转换函数  
    bool BufferToHead(const Byte* ABuffer, TDataHead& AHead);
    void HeadToBuffer(const TDataHead& AHead, Byte* ABuffer);

    bool BufferToLogin(const TDataHead& AHead, const Byte* ABuffer, TLoginMsg& AMsg);
    void LoginToBuffer(const TLoginMsg& AMsg, Byte* ABuffer);

    bool BufferToSend(const TDataHead& AHead, const Byte* ABuffer, TSendMsg& AMsg);
    void SendToBuffer(const TSendMsg& AMsg, Byte* ABuffer);

    bool BufferToBroadcast(const TDataHead& AHead, const Byte* ABuffer, TBroadcastdMsg& AMsg);
    void BroadcastToBuffer(const TBroadcastdMsg& AMsg, Byte* ABuffer);

    bool BufferToQuery(const TDataHead& AHead, const Byte* ABuffer, TQueryMsg& AMsg);
    void QueryToBuffer(const TQueryMsg& AMsg, Byte* ABuffer);

    bool BufferToReplyMsg(const TDataHead& AHead, const Byte* ABuffer, TReplyMsg& AMsg);
    void ReplyMsgToBuffer(const TReplyMsg& AMsg, Byte* ABuffer);

    bool BufferToReplyBroadcast(const TDataHead& AHead, const Byte* ABuffer, TReplyBroadcast& AMsg);
    void ReplyBroadcastToBuffer(const TReplyBroadcast& AMsg, Byte* ABuffer);

    bool BufferToReplyQuery(const TDataHead& AHead, const Byte* ABuffer, TReplyQuery& AMsg);
    void ReplyQueryToBuffer(const TReplyQuery& AMsg, Byte* ABuffer);
}

#endif