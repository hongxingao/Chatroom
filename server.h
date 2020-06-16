#ifndef _server_H_
#define _server_H_

#include "KYLib.h"
#include "TCPObjs.h"
#include "link.h"
#include "protocol.h"

using namespace TCPUtils;

namespace _ghx
{
    typedef TKYMapIntKeyEx<TLink>      TUserMap; // �û�map
    typedef TKYMapObjKey<TLink>        TConnMap; // ����map

    // ��¼�¼�
    typedef void (TObject::*TDoLoginSucess)(const KYString& AIp, DWord AId);
    typedef struct
    {
        TDoLoginSucess Method;
        void* Object;
    } TOnLoginSucess;

    class TServer
    {
    public:
        // ״̬
        typedef enum 
        {
            kClosed  = 0,   // �ѹر�
            kClosing = 1,   // ���ڹر�
            kOpening = 2,   // ���ڴ�
            kOpened  = 3,   // �Ѵ�
        } TState;

        TServer(const KYString& AIp = "127.0.0.1", Longword APort = 5000);
        // ��������
        ~TServer();

        // ����
        TKYTimer*       timer() const                             { return timer_; }
        char*           addr() const                              { return obj_server_->Addr(); }
        long            port() const                              { return obj_server_->Port(); }
        long            send_queue() const                        { return obj_server_->SendQueue(); }
        long            listen_queue() const                      { return obj_server_->ListenQueue(); }
        long            recv_threads() const                      { return obj_server_->RecvThreads(); }
        long            cache_threads() const                     { return obj_server_->CacheThreads(); }
        long            max_clients() const                       { return obj_server_->MaxClients(); }
        long            client_count() const                      { return obj_server_->ClientCount(); }
        long            keep_timeout() const                      { return obj_server_->KeepTimeout(); }
        long            keep_interval() const                     { return obj_server_->KeepInterval(); }
        long            keep_retryTimes() const                   { return obj_server_->KeepRetryTimes(); }

        // ��������
        bool            set_timer(TKYTimer* ATimer);
        bool            set_addr(const KYString& AIp)            { return obj_server_->SetAddr(AIp); }
        bool            set_port(Longword APort)                 { return obj_server_->SetPort(APort); }
        long            set_send_queue(long AQueueSize)          { return obj_server_->SetSendQueue(AQueueSize); }
        long            set_listen_queue(long AListenQueue)      { return obj_server_->SetListenQueue(AListenQueue); }
        long            set_recv_threads(long ACount)            { return obj_server_->SetRecvThreads(ACount); }
        long            set_cache_threads(long ACount)           { return obj_server_->SetCacheThreads(ACount); }
        long            set_max_clients(long AMaxCount)          { return obj_server_->SetMaxClients(AMaxCount); }
        long            set_keep_timeout(long ATimeout)          { return obj_server_->SetKeepTimeout(ATimeout); }
        long            set_keep_interval(long AInterval)        { return obj_server_->SetKeepInterval(AInterval); }
        long            set_keep_petrytimes(long ARetryTimes)    { return obj_server_->SetKeepRetryTimes(ARetryTimes); }
    
        // ���öϿ������¼�
        long            set_disocnnect(TDoSrvEvent AMethod, void* AObject)
                        { return obj_server_->SetOnDisconnect(AMethod, AObject); }
    
        // �������رշ�����
        bool            open();
        void            close(bool ANeedWait);

        // ɾ��һ���û� 
        bool            delete_user(DWord AId, TLink* ALink = NULL);
        // ɾ��һ������ 
        bool            delete_conn(TLink* ALink);

        // ��ǰ������Ϣ
        bool            query_users(DWord ABegin, DWord AEnd, TKYStringList& AStrList);

        // �����û���¼�¼�
        bool            set_loign_sucess(TDoLoginSucess AMethod, void* Object);

    private:
        // ִ�д򿪡��ر�
        bool            do_open();
        void            do_close();

        // ���ü����ӡ���һ
        bool            inc_ref_count(); 
        void            dec_ref_count() { InterlockedDecrement(&ref_count_); } 
        
       
        // �������Ӷ�����¼�
        void            set_link_event(TLink* ALink);
        // ��Ҫ�󶨵��¼�����
        char            link_login(TLink* ALink, DWord AId);
        void            link_login_out(TLink* ALink);
        char            link_private(TLink* ALink, DWord AId, DWord ASeriNum, const KYString& AData);
        int             link_broadcast(TLink* ALink, const KYString& AData);
        bool            link_reply(TLink* ALink, DWord AId, DWord ASeriNum, DWord AReply);
        char            link_query(TLink* ALink, DWord ABegin, DWord AEnd, TKYList& AList);

        // ��ʱ���¼����������Ƴ�ʱ��
        void            do_timer(void* Sender, void* AParam, long ATimerID);

        // ��������
        void            do_accept(TTCPServer* AServer, TTCPConnObj* AConnObj, bool& AIsRefused);
        // ��������
        void            do_recv_data(TTCPConnObj* AConnObj, void* AData, long ASize);
        // �ͷ�����
        void            do_free_ctl(TTCPConnObj* AConnObj);

        //״̬���򿪡��ر�
        void            lock() { state_lock_->Enter(); }
        void            unlock() { state_lock_->Leave(); }

        // �б��д��
        bool            map_lock_read()     { return map_lock_->LockRead(); }
        bool            map_lock_write()    { return map_lock_->LockWrite(); }
        void            map_unlock_read()   { map_lock_->UnlockRead(); }
        void            map_unlock_write()  { map_lock_->UnlockWrite(); }

        // �б���ɾ���¼�����
        void            delete_list(TLink* AItem);

        // ɾ�����ͷ�map��
        void            clear_map();

        // �û���¼�¼�
        TOnLoginSucess  on_login_sucess;

    private:
        TKYCritSect*    state_lock_;         // ״̬��
        DWord           state_;              // ״̬
        long            ref_count_;          // ���ü���
        Longword        close_thread_id_;

        long            timer_id_;           // ��ʱ��id
        bool            is_set_timeout_;     // �Ƿ������ó�ʱ

        TKYLockRW*      map_lock_;           // �б���
        TUserMap*       user_map_;           // �����б�
        TConnMap*       conn_map_;           // �����б�

        TKYRunThread*   destory_link_thread_;
        TKYTimer*       timer_;
        TTCPServer*     obj_server_;
    };

}

#endif