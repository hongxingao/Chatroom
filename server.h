#ifndef _server_H_
#define _server_H_

#include "KYLib.h"
#include "TCPObjs.h"
#include "link.h"
#include "protocol.h"

using namespace TCPUtils;

namespace _ghx
{
    typedef TKYMapIntKeyEx<TLink>      TUserMap; // 用户map
    typedef TKYMapObjKey<TLink>        TConnMap; // 连接map

    // 登录事件
    typedef void (TObject::*TDoLoginSucess)(const KYString& AIp, DWord AId);
    typedef struct
    {
        TDoLoginSucess Method;
        void* Object;
    } TOnLoginSucess;

    class TServer
    {
    public:
        // 状态
        typedef enum 
        {
            kClosed  = 0,   // 已关闭
            kClosing = 1,   // 正在关闭
            kOpening = 2,   // 正在打开
            kOpened  = 3,   // 已打开
        } TState;

        TServer(const KYString& AIp = "127.0.0.1", Longword APort = 5000);
        // 析构函数
        ~TServer();

        // 属性
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

        // 设置属性
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
    
        // 设置断开监听事件
        long            set_disocnnect(TDoSrvEvent AMethod, void* AObject)
                        { return obj_server_->SetOnDisconnect(AMethod, AObject); }
    
        // 开启、关闭服务器
        bool            open();
        void            close(bool ANeedWait);

        // 删除一个用户 
        bool            delete_user(DWord AId, TLink* ALink = NULL);
        // 删除一个连接 
        bool            delete_conn(TLink* ALink);

        // 当前在线信息
        bool            query_users(DWord ABegin, DWord AEnd, TKYStringList& AStrList);

        // 设置用户登录事件
        bool            set_loign_sucess(TDoLoginSucess AMethod, void* Object);

    private:
        // 执行打开、关闭
        bool            do_open();
        void            do_close();

        // 引用计数加、减一
        bool            inc_ref_count(); 
        void            dec_ref_count() { InterlockedDecrement(&ref_count_); } 
        
       
        // 设置连接对象的事件
        void            set_link_event(TLink* ALink);
        // 需要绑定的事件方法
        char            link_login(TLink* ALink, DWord AId);
        void            link_login_out(TLink* ALink);
        char            link_private(TLink* ALink, DWord AId, DWord ASeriNum, const KYString& AData);
        int             link_broadcast(TLink* ALink, const KYString& AData);
        bool            link_reply(TLink* ALink, DWord AId, DWord ASeriNum, DWord AReply);
        char            link_query(TLink* ALink, DWord ABegin, DWord AEnd, TKYList& AList);

        // 定时器事件方法（控制超时）
        void            do_timer(void* Sender, void* AParam, long ATimerID);

        // 接受连接
        void            do_accept(TTCPServer* AServer, TTCPConnObj* AConnObj, bool& AIsRefused);
        // 接收数据
        void            do_recv_data(TTCPConnObj* AConnObj, void* AData, long ASize);
        // 释放连接
        void            do_free_ctl(TTCPConnObj* AConnObj);

        //状态锁打开、关闭
        void            lock() { state_lock_->Enter(); }
        void            unlock() { state_lock_->Leave(); }

        // 列表读写锁
        bool            map_lock_read()     { return map_lock_->LockRead(); }
        bool            map_lock_write()    { return map_lock_->LockWrite(); }
        void            map_unlock_read()   { map_lock_->UnlockRead(); }
        void            map_unlock_write()  { map_lock_->UnlockWrite(); }

        // 列表项删除事件方法
        void            delete_list(TLink* AItem);

        // 删除并释放map项
        void            clear_map();

        // 用户登录事件
        TOnLoginSucess  on_login_sucess;

    private:
        TKYCritSect*    state_lock_;         // 状态锁
        DWord           state_;              // 状态
        long            ref_count_;          // 引用计数
        Longword        close_thread_id_;

        long            timer_id_;           // 定时器id
        bool            is_set_timeout_;     // 是否已设置超时

        TKYLockRW*      map_lock_;           // 列表锁
        TUserMap*       user_map_;           // 在线列表
        TConnMap*       conn_map_;           // 连接列表

        TKYRunThread*   destory_link_thread_;
        TKYTimer*       timer_;
        TTCPServer*     obj_server_;
    };

}

#endif