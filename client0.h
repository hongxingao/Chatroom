#ifndef _client0_H_
#define _client0_H_

#include "KYLib.h"
#include "TCPObjs.h"
#include "protocol.h"

using namespace TCPUtils;

namespace _ghx
{
    typedef KYLib::TObject TObject;
    typedef TKYMapIntKey<TDataCtl> TDataCtlMap;

    // �յ�˽�Ż��߹㲥�¼�
    typedef void (TObject::*TDORecvMsg)(DWord ASourId, const KYString& AData);
    typedef struct
    {
        TDORecvMsg Method;
        void* Object;
    } TOnRecvMsg;

    // �Ͽ�����
    typedef void (TObject::*TDODisConn)();
    typedef struct
    {
        TDODisConn Method;
        void* Object;
    } TOnDisConn;

    class TClient
    {
    public:
        // ״̬
        typedef enum TState
        {
            kClosed        = 0,   // �ѹر�
            kClosing       = 1,   // ���ڹر�
            kOpening       = 2,   // ���ڴ�
            kOpened        = 3,   // �Ѵ�
        } TState;

        ~TClient();
        TClient(const KYString& AIp = "127.0.0.1", long APort = 8886);
        // ����
        DWORD           id() const            { return id_; }
        long            send_queue() const    { return conn_obj_->SendQueue(); }
        char*           curr_addr() const     { return conn_obj_->CurrAddr(); }
        long            curr_port() const     { return conn_obj_->CurrPort(); }
        char*           peer_addr() const     { return conn_obj_->PeerAddr(); }
        long            peer_port() const     { return conn_obj_->PeerPort(); }
        long            keep_timeout() const  { return conn_obj_->KeepTimeout(); }
        long            keep_interval() const { return conn_obj_->KeepInterval(); }
        long            keep_retryTimes()const{ return conn_obj_->KeepRetryTimes(); }

        // ��������
        long            set_send_queue(long AQueueSize)       { return conn_obj_->SetSendQueue(AQueueSize); }
        long            set_curr_addr(const char* Addr)       { return conn_obj_->SetCurrAddr(Addr); }
        long            set_curr_port(long APort)             { return conn_obj_->SetCurrPort(APort); }
        long            set_peer_addr(const char* Addr)       { return conn_obj_->SetPeerAddr(Addr); }
        long            set_peer_port(long APort)             { return conn_obj_->SetPeerPort(APort); }
        long            set_keep_timeout(long ATimeout)       { return conn_obj_->SetKeepTimeout(ATimeout); }
        long            set_keep_interval(long AInterval)     { return conn_obj_->SetKeepInterval(AInterval); }
        long            set_keep_retrytimes(long ARetryTimes) { return conn_obj_->SetKeepRetryTimes(ARetryTimes); }
        bool            set_id(DWORD AId);
        
        // ����
        int             open();
        // �ر�
        void            close(bool ANeedWait);

        // ˽��ĳ�û�
        int             private_chat(DWORD AId, const KYString& AStr);
        // �㲥
        DWord           broadcast(const KYString& AStr);
        // ��ѯ��ǰ�����û�
        bool            query(DWord ABegin, DWord AEnd, TKYList& AList);

        // �����յ�˽���¼�
        void            set_on_recv_private(TDORecvMsg ADoRecPriv, void* AObject);
        // �����յ��㲥�¼�
        void            set_on_recv_broadcast(TDORecvMsg ADoRecBroad, void* AObject);

    public:
        TOnDisConn      on_dis_conn;

    private:
        // ִ�д�����
        int             do_open();
                        
        // ִ�йر�����
        void            do_close(bool ANendClose);
        // ִ�е�¼
        int             do_login();

        // ������ܻ�����
        void            deal_rec();

        // ��ȡ��Ϣ��Ӧ
        int             get_msg_reply(Longword ASerial, TDataCtl* ADataCtl);
        // ������յ��Ϸ�����Ϣ
        bool            deal_msg(const TDataHead& AHead, Byte* AMsg);

        // ����ظ���Ϣ ��¼ ˽��
        void            deal_reply_msg(const TReplyMsg& AMsg);
        // �����ѯ�ظ�
        void            deal_reply_query(const TReplyQuery& AMsg);
        // ����㲥�ظ�
        void            deal_reply_broadcast(const TReplyBroadcast& AMsg);

        // ת����Ϣ�ظ�
        bool            reply_trans_private(const TSendMsg& AMsg);

        // ������Ϣ����
        bool            add_msg_ctl(Longword ASerial, TDataCtl& ACtl);

        // ״̬��
        void            lock() { state_lock_->Enter(); }
        void            unlock() { state_lock_->Leave(); }

        // ��Ϣ������
        void            map_lock() { map_lock_->Enter(); }
        void            map_unlock() { map_lock_->Leave(); }

        // �Ӽ����ü���
        bool            inc_ref_count(); 
        bool            inc_opening_ref_count();
        void            dec_ref_count() { InterlockedDecrement(&ref_count_); } 

        // ɾ����Ϣ����
        void            clear_ctl_map();
        void            do_del_ctl(TDataCtl* ACtl);

        // ��������
        void            do_recv_data(TTCPConnObj* AConnObj, void* AData, long ASize);
        // �Ͽ�����
        void            do_disconn(TTCPConnObj* AConnObj);

        // �յ�˽���¼�
        TOnRecvMsg      on_recv_private;
        // �յ��㲥�¼�
        TOnRecvMsg      on_recv_broad;

    private:
        TKYCritSect*    state_lock_;          // ״̬��
        DWord           state_;               // ״̬
        DWord           ref_count_;           // ���ü���

        DWORD           id_;                  // �ͻ���id
        Longword        serial_num_;          // ���ݰ����
        Longword        close_thread_id_;     // ���ڹرյ��߳�id

        bool            bool_head_;           //�Ƿ���ͷ
        TDataHead       rec_head_;            //��Ϣͷ

        TTCPConnObj*    conn_obj_;            // ����
        TKYCycBuffer*   rec_buffer_;          // �������ݵĻ�����
        TKYCritSect*    map_lock_;            // ��Ϣ������
        TDataCtlMap*    msg_ctl_map_;         // ��Ϣ����
        TKYEvent*       msg_ctl_notify_;      // ��Ϣ֪ͨ
    };
}


#endif