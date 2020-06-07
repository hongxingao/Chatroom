#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include "KYLib.h"

namespace _ghx
{
    // ��Ϣ����
    typedef enum TMsgTypeTag
    {
        kLogin                 = 0,    // ��¼����
        kLoginReply            = 1,    // ��¼�ظ�

        kLoginOut              = 2,    // �ǳ�����
        KLoginOutReply         = 3,    // �ǳ��ظ�

        kPrivateChat           = 4,    // ˽�ĸ�ĳid
        kPrivateChatReply      = 5,    // ˽�Ļظ�

        kTransPrivateChat      = 6,    // ת��˽��
        kTransPrivateChatReply = 7,    // ת��˽�Ļظ�

        kBroadCast             = 8,    // �㲥����
        kBroadCaseReply        = 9,    // �㲥�ظ�

        kTransBroadCast        = 10,    // ת���㲥����

        kQuery                 = 12,    // �б��ѯ����
        kQueryReply            = 13     // �б�ظ�
    } TMsgType;

    // ��Ϣͷ��  
    #pragma pack(push, 1)
    typedef struct
    {
        DWord           crc;            // У����
        DWord           serial_num;     // ��Ϣ���
        Word            len;            // ��Ϣ����
        Byte            msg_type;       // ��Ϣ��
    } TDataHead;
    #pragma pack(pop)

    // -----------������Ϣ------------

    // ��¼������Ϣ��
    typedef struct
    {
        TDataHead       head;            //ͷ��
        DWord           id;                //�û�id
    } TLoginMsg;

    // ����˽��,ת��˽��,ת���㲥��Ϣ��
    // ˽��ʱΪĿ��id�� ת��˽�ģ�ת���㲥ʱΪԴid
    typedef struct
    {
        TDataHead       head;
        DWord           id;
        KYString        data;
    } TSendMsg;
    // word size = data.length();
    // char* p = (char*)data;

    // ����㲥��Ϣ��
    typedef struct
    {
        TDataHead       head;
        KYString        data;;
    } TBroadcastdMsg;

    // �����ѯ��Ϣ��
    typedef struct
    {
        TDataHead        head;
        DWord            begin;
        DWord            end;
    } TQueryMsg;

    //----------�ظ���Ϣ

    //�ظ���¼  �ظ�˽��
    typedef struct
    {
        TDataHead        head;
        char             reply_type;
    } TReplyMsg;

    // �ظ��㲥
    typedef struct
    {
        TDataHead        head;
        char             reply_type;
        DWord            count;
    } TReplyBroadcast;

    // �ظ���ѯ
    typedef struct
    {
        TDataHead        head;
        char             reply_type;
        DWord            count;
        DWord*           data;
    } TReplyQuery;

    // ��Ϣ���� �����Ϣ�Ļ�Ӧ
    typedef struct
    {
        DWord            serial_num;        // ��Ϣ���
        DWORD            time;
        bool             is_reply;
        char             reply_type;
        TKYList          data;
    } TDataCtl;

    enum TMsgLen
    {
        // ��Ϣͷ������
        kHeadLen        = sizeof(TDataHead),

        // ������Ϣ�峤��
        kLoginMsgLen    = sizeof(TLoginMsg),
        kQueryMsgLen    = sizeof(TQueryMsg),

        // �ظ���Ϣ�峤��
        kReplyMsgLen    = sizeof(TReplyMsg),

        //��Ϣ���Ƴ���
        kMsgCtlLen      = sizeof(TDataCtl),

        // �����Ϣ����
        kMaxMsgLen      = 512
    };

    // �ֽ���ת������
    DWord _4ByteToDWord(const Byte* ABytes);
    void  DWordTo4Byte(DWord AValue, Byte* ABytes);
    Word _2ByteToWord(const Byte* ABytes);
    void WordTo2Byte(Word AValue, Byte* ABytes);

    // Э��ṹ��==���ֽ��� ת������  
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