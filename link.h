#ifndef _LINK_H_
#define _LINK_H_

#include "KYLib.h"
#include "TCPObjs.h"
#include "protocol.h"
using namespace TCPUtils;

namespace _ghx
{
    class TLink;

    // TObject - ����
    typedef KYLib::TObject TObject;

    // ��¼�¼�
    typedef char (TObject::*TDoLogin)(TLink* ALink, DWord AId);
    typedef struct
    {
        TDoLogin Method;
        void* Object;
    } TOnLogin;

    // �ǳ��¼�
    typedef void (TObject::*TDoLoginOut)(TLink* ALink);
    typedef struct
    {
        TDoLoginOut Method;
        void* Object;
    } TOnLoginOut;

    // ˽���¼�
    typedef char (TObject::*TDoPrivate)(TLink* ALink, DWord AId, DWord ASeriNum, const KYString& AData);
    typedef struct
    {
        TDoPrivate Method;
        void* Object;
    } TOnPrivate;

    // �㲥�¼�
    typedef int (TObject::*TDoBroadcast)(TLink* ALink, const KYString& AData);
    typedef struct
    {
        TDoBroadcast Method;
        void* Object;
    } TOnBroadcast;

    // �ظ�ת��˽���¼� ���յ�˽�Ļظ�ʱ������
    typedef bool (TObject::*TDoReply)(TLink* ALink, DWord AId, DWord ASeriNum, char AReply);
    typedef struct
    {
        TDoReply Method;
        void* Object;
    } TOnReply;

    // ��ѯ�б��¼�
    typedef char (TObject::*TDoQuery)(TLink* ALink, DWord Begin, DWord AEnd, TKYList& AList);
    typedef struct
    {
        TDoQuery Method;
        void* Object;
    } TOnQuery;

    class TLink
    {
    public:
        // ״̬
        enum TState
        {
            kClosed  = 0,   // �ѹر�
            kClosing = 1,   // ���ڹر�
            kOpening = 2,   // ���ڴ�
            kOpened  = 3,   // �Ѵ�
        };

        // ˽����Ϣ����
        typedef struct 
        {
            DWord  msg_serial_num; 
            DWord  id;
            time_t time;
        } TPrivateCtl;

    public:
        TLink(TTCPConnObj* AConn);
        ~TLink();

    public:
        // ����
        DWord                         id() const { return id_; };
        TKYTimer*                     timer() const { return timer_; }
        TTCPConnObj*                  conn_obj() const { return conn_obj_; }
        DWORD                         conn_tick() const { return conn_tick_; }
        char*                         perr_addr() const { return  conn_obj_->PeerAddr(); }

        // ��������
        void                          set_id(DWord AId) { id_ = AId; }
        bool                          set_timer(TKYTimer* ATimer);
        void                          set_conn_tick(DWORD ATimet) { conn_tick_ = ATimet; }
        long                          set_perr_addr(const char* Addr) { return conn_obj_->SetPeerAddr(Addr); }

        // �����¼�
        void                          set_on_login(TDoLogin AMethod, void* AObj);
        void                          set_on_login_out(TDoLoginOut AMethod, void* AObj);
        void                          set_on_private(TDoPrivate AMethod, void* AObj);
        void                          set_on_broadcast(TDoBroadcast AMethod, void* AObj);
        void                          set_on_reply(TDoReply AMethod, void* AObj);
        void                          set_on_query(TDoQuery AMethod, void* AObj);

        // �������ر�����
        bool                          open();
        void                          close(bool ANeedWait);
       
        // ת��˽��  
        bool                          send_trans_private(DWord ASeriNum, DWord AId, const KYString& AData)
                                      { return send_trans_msg(kTransPrivateChat, ASeriNum, AId, AData); }
        //  ת���㲥 
        bool                          send_trans_broadcast(DWord ASeriNum, DWord AId, const KYString& AData)
                                      { return send_trans_msg(kTransBroadCast, ASeriNum, AId, AData); }
        // �ظ�˽��
        bool                          send_reply_msg(DWord ASeriNum, char AReplyType);

        // ��������
        void                          do_recv(void* AData, long ASize);
        // �Ͽ������¼�����
        void                          do_disconn();

    protected:
        // ״̬��
        void                          lock() { state_lock_->Enter(); }
        void                          unlock() { state_lock_->Leave(); }

        bool                          inc_ref_count(); // ���ü�����һ
        void                          dec_ref_count() { InterlockedDecrement(&ref_count_); } // ���ü�����һ

    private:
        // ��ʼ��
        void                          init_link();
        // ִ�п������ر�
        bool                          do_open();
        void                          do_close(bool ANeedClose, bool ANeedEvent);
                
        // ���������������
        unsigned                      deal_rec(TKYThread* AThread, void* AParam);
        // ������յ��Ϸ�����Ϣ
        bool                          deal_msg(const TDataHead& AHead, Byte* AMsg);

        // �����¼����
        bool                          deal_login(const TLoginMsg& AMsg);
        // ����˽������
        bool                          deal_private(const TSendMsg& AMsg);
        // ����㲥����
        bool                          deal_broadcast(const TBroadcastdMsg& AMsg);
        // �����ѯ����
        bool                          deal_query(const TQueryMsg& AMsg);
        // ����ת��˽�Ļظ�
        bool                          deal_reply_trans_private(const TReplyMsg& AMsg);

        // �ظ�˽��
        bool                          do_reply_private(DWord ASeriNum, char AReplyType);
        // ת����Ϣ
        bool                          send_trans_msg(TMsgTypeTag AType, DWord ASeriNum, DWord AId, const KYString& AData);

        
        // ��ʱ���¼����������Ƴ�ʱ��
        void                          do_timer(void* Sender, void* AParam, long ATimerID);
        // �б�ɾ���¼�
        void                          do_dele_list(void* AItem);

        // �б���
        void                          lock_map() { private_ctl_map_lock_->Enter(); }
        void                          unlock_map() { private_ctl_map_lock_->Leave(); }
    private:
        // �¼�
        TOnLogin                      on_login;
        TOnLoginOut                   on_login_out;
        TOnPrivate                    on_private;
        TOnReply                      on_reply;
        TOnBroadcast                  on_broadcast;
        TOnQuery                      on_query;

    private:
        TKYCritSect*                  state_lock_;      // ״̬��
        TState                        state_;           // ״̬
        long                          ref_count_;        // ���ü���
        DWord                         link_serial_num_; // ���ӵ���Ϣ���
        TKYTimer*                     timer_;           // ��ʱ������
        long                          timer_id_;        // ��ʱ��id
        bool                          is_set_timeout_;  // �Ƿ������ó�ʱ
        Longword                      close_thread_id_;  // ���ڹرյ��߳�id
        DWORD                         conn_tick_;

        // ���ӵĿͻ���id
        DWord                         id_;
        // �������ݻ�����
        TKYCycBuffer*                 rec_buffer_;
        // ����
        TTCPConnObj*                  conn_obj_;
        // �����߳�
        TKYThread*                    rec_thread_;
        // �߳�֪ͨ����
        TKYEvent*                     notify_;

        // ˽�Ŀ�����Ϣ�б�
        TKYMapIntKey<TPrivateCtl>*    private_ctl_map_;
        // ˽�Ŀ�����Ϣ�б���
        TKYCritSect*                  private_ctl_map_lock_;
    };
}


#endif