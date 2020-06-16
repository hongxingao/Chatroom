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

    // 收到私信或者广播事件
    typedef void (TObject::*TDORecvMsg)(DWord ASourId, const KYString& AData);
    typedef struct
    {
        TDORecvMsg Method;
        void* Object;
    } TOnRecvMsg;

    // 断开连接
    typedef void (TObject::*TDODisConn)();
    typedef struct
    {
        TDODisConn Method;
        void* Object;
    } TOnDisConn;

    class TClient
    {
    public:
        // 状态
        typedef enum TState
        {
            kClosed        = 0,   // 已关闭
            kClosing       = 1,   // 正在关闭
            kOpening       = 2,   // 正在打开
            kOpened        = 3,   // 已打开
        } TState;

        ~TClient();
        TClient(const KYString& AIp = "127.0.0.1", long APort = 8886);
        // 属性
        DWORD           id() const            { return id_; }
        long            send_queue() const    { return conn_obj_->SendQueue(); }
        char*           curr_addr() const     { return conn_obj_->CurrAddr(); }
        long            curr_port() const     { return conn_obj_->CurrPort(); }
        char*           peer_addr() const     { return conn_obj_->PeerAddr(); }
        long            peer_port() const     { return conn_obj_->PeerPort(); }
        long            keep_timeout() const  { return conn_obj_->KeepTimeout(); }
        long            keep_interval() const { return conn_obj_->KeepInterval(); }
        long            keep_retryTimes()const{ return conn_obj_->KeepRetryTimes(); }

        // 设置属性
        long            set_send_queue(long AQueueSize)       { return conn_obj_->SetSendQueue(AQueueSize); }
        long            set_curr_addr(const char* Addr)       { return conn_obj_->SetCurrAddr(Addr); }
        long            set_curr_port(long APort)             { return conn_obj_->SetCurrPort(APort); }
        long            set_peer_addr(const char* Addr)       { return conn_obj_->SetPeerAddr(Addr); }
        long            set_peer_port(long APort)             { return conn_obj_->SetPeerPort(APort); }
        long            set_keep_timeout(long ATimeout)       { return conn_obj_->SetKeepTimeout(ATimeout); }
        long            set_keep_interval(long AInterval)     { return conn_obj_->SetKeepInterval(AInterval); }
        long            set_keep_retrytimes(long ARetryTimes) { return conn_obj_->SetKeepRetryTimes(ARetryTimes); }
        bool            set_id(DWORD AId);
        
        // 开启
        int             open();
        // 关闭
        void            close(bool ANeedWait);

        // 私聊某用户
        int             private_chat(DWORD AId, const KYString& AStr);
        // 广播
        DWord           broadcast(const KYString& AStr);
        // 查询当前在线用户
        bool            query(DWord ABegin, DWord AEnd, TKYList& AList);

        // 设置收到私信事件
        void            set_on_recv_private(TDORecvMsg ADoRecPriv, void* AObject);
        // 设置收到广播事件
        void            set_on_recv_broadcast(TDORecvMsg ADoRecBroad, void* AObject);

    public:
        TOnDisConn      on_dis_conn;

    private:
        // 执行打开连接
        int             do_open();
                        
        // 执行关闭连接
        void            do_close(bool ANendClose);
        // 执行登录
        int             do_login();

        // 处理接受缓冲区
        void            deal_rec();

        // 获取消息回应
        int             get_msg_reply(Longword ASerial, TDataCtl* ADataCtl);
        // 处理接收到合法的消息
        bool            deal_msg(const TDataHead& AHead, Byte* AMsg);

        // 处理回复消息 登录 私聊
        void            deal_reply_msg(const TReplyMsg& AMsg);
        // 处理查询回复
        void            deal_reply_query(const TReplyQuery& AMsg);
        // 处理广播回复
        void            deal_reply_broadcast(const TReplyBroadcast& AMsg);

        // 转发消息回复
        bool            reply_trans_private(const TSendMsg& AMsg);

        // 加入消息管理
        bool            add_msg_ctl(Longword ASerial, TDataCtl& ACtl);

        // 状态锁
        void            lock() { state_lock_->Enter(); }
        void            unlock() { state_lock_->Leave(); }

        // 消息管理锁
        void            map_lock() { map_lock_->Enter(); }
        void            map_unlock() { map_lock_->Leave(); }

        // 加减引用计数
        bool            inc_ref_count(); 
        bool            inc_opening_ref_count();
        void            dec_ref_count() { InterlockedDecrement(&ref_count_); } 

        // 删除消息控制
        void            clear_ctl_map();
        void            do_del_ctl(TDataCtl* ACtl);

        // 接收数据
        void            do_recv_data(TTCPConnObj* AConnObj, void* AData, long ASize);
        // 断开连接
        void            do_disconn(TTCPConnObj* AConnObj);

        // 收到私信事件
        TOnRecvMsg      on_recv_private;
        // 收到广播事件
        TOnRecvMsg      on_recv_broad;

    private:
        TKYCritSect*    state_lock_;          // 状态锁
        DWord           state_;               // 状态
        DWord           ref_count_;           // 引用计数

        DWORD           id_;                  // 客户端id
        Longword        serial_num_;          // 数据包序号
        Longword        close_thread_id_;     // 正在关闭的线程id

        bool            bool_head_;           //是否是头
        TDataHead       rec_head_;            //消息头

        TTCPConnObj*    conn_obj_;            // 连接
        TKYCycBuffer*   rec_buffer_;          // 接受数据的缓冲区
        TKYCritSect*    map_lock_;            // 消息管理锁
        TDataCtlMap*    msg_ctl_map_;         // 消息管理
        TKYEvent*       msg_ctl_notify_;      // 消息通知
    };
}


#endif