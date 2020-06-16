#ifndef _LINK_H_
#define _LINK_H_

#include "KYLib.h"
#include "TCPObjs.h"
#include "protocol.h"
using namespace TCPUtils;

namespace _ghx
{
    class TLink;

    // TObject - 空类
    typedef KYLib::TObject TObject;

    // 登录事件
    typedef char (TObject::*TDoLogin)(TLink* ALink, DWord AId);
    typedef struct
    {
        TDoLogin Method;
        void* Object;
    } TOnLogin;

    // 登出事件
    typedef void (TObject::*TDoLoginOut)(TLink* ALink);
    typedef struct
    {
        TDoLoginOut Method;
        void* Object;
    } TOnLoginOut;

    // 私聊事件
    typedef char (TObject::*TDoPrivate)(TLink* ALink, DWord AId, DWord ASeriNum, const KYString& AData);
    typedef struct
    {
        TDoPrivate Method;
        void* Object;
    } TOnPrivate;

    // 广播事件
    typedef int (TObject::*TDoBroadcast)(TLink* ALink, const KYString& AData);
    typedef struct
    {
        TDoBroadcast Method;
        void* Object;
    } TOnBroadcast;

    // 回复转发私聊事件 （收到私聊回复时触发）
    typedef bool (TObject::*TDoReply)(TLink* ALink, DWord AId, DWord ASeriNum, char AReply);
    typedef struct
    {
        TDoReply Method;
        void* Object;
    } TOnReply;

    // 查询列表事件
    typedef char (TObject::*TDoQuery)(TLink* ALink, DWord Begin, DWord AEnd, TKYList& AList);
    typedef struct
    {
        TDoQuery Method;
        void* Object;
    } TOnQuery;

    class TLink
    {
    public:
        // 状态
        enum TState
        {
            kClosed  = 0,   // 已关闭
            kClosing = 1,   // 正在关闭
            kOpening = 2,   // 正在打开
            kOpened  = 3,   // 已打开
        };

        // 私聊消息控制
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
        // 属性
        DWord                         id() const { return id_; };
        TKYTimer*                     timer() const { return timer_; }
        TTCPConnObj*                  conn_obj() const { return conn_obj_; }
        DWORD                         conn_tick() const { return conn_tick_; }
        char*                         perr_addr() const { return  conn_obj_->PeerAddr(); }

        // 设置属性
        void                          set_id(DWord AId) { id_ = AId; }
        bool                          set_timer(TKYTimer* ATimer);
        void                          set_conn_tick(DWORD ATimet) { conn_tick_ = ATimet; }
        long                          set_perr_addr(const char* Addr) { return conn_obj_->SetPeerAddr(Addr); }

        // 设置事件
        void                          set_on_login(TDoLogin AMethod, void* AObj);
        void                          set_on_login_out(TDoLoginOut AMethod, void* AObj);
        void                          set_on_private(TDoPrivate AMethod, void* AObj);
        void                          set_on_broadcast(TDoBroadcast AMethod, void* AObj);
        void                          set_on_reply(TDoReply AMethod, void* AObj);
        void                          set_on_query(TDoQuery AMethod, void* AObj);

        // 开启、关闭连接
        bool                          open();
        void                          close(bool ANeedWait);
       
        // 转发私聊  
        bool                          send_trans_private(DWord ASeriNum, DWord AId, const KYString& AData)
                                      { return send_trans_msg(kTransPrivateChat, ASeriNum, AId, AData); }
        //  转发广播 
        bool                          send_trans_broadcast(DWord ASeriNum, DWord AId, const KYString& AData)
                                      { return send_trans_msg(kTransBroadCast, ASeriNum, AId, AData); }
        // 回复私聊
        bool                          send_reply_msg(DWord ASeriNum, char AReplyType);

        // 接收数据
        void                          do_recv(void* AData, long ASize);
        // 断开连接事件方法
        void                          do_disconn();

    protected:
        // 状态锁
        void                          lock() { state_lock_->Enter(); }
        void                          unlock() { state_lock_->Leave(); }

        bool                          inc_ref_count(); // 引用计数加一
        void                          dec_ref_count() { InterlockedDecrement(&ref_count_); } // 引用计数减一

    private:
        // 初始化
        void                          init_link();
        // 执行开启、关闭
        bool                          do_open();
        void                          do_close(bool ANeedClose, bool ANeedEvent);
                
        // 处理缓冲区里的数据
        unsigned                      deal_rec(TKYThread* AThread, void* AParam);
        // 处理接收到合法的消息
        bool                          deal_msg(const TDataHead& AHead, Byte* AMsg);

        // 处理登录请求
        bool                          deal_login(const TLoginMsg& AMsg);
        // 处理私聊请求
        bool                          deal_private(const TSendMsg& AMsg);
        // 处理广播请求
        bool                          deal_broadcast(const TBroadcastdMsg& AMsg);
        // 处理查询请求
        bool                          deal_query(const TQueryMsg& AMsg);
        // 处理转发私聊回复
        bool                          deal_reply_trans_private(const TReplyMsg& AMsg);

        // 回复私聊
        bool                          do_reply_private(DWord ASeriNum, char AReplyType);
        // 转发消息
        bool                          send_trans_msg(TMsgTypeTag AType, DWord ASeriNum, DWord AId, const KYString& AData);

        
        // 定时器事件方法（控制超时）
        void                          do_timer(void* Sender, void* AParam, long ATimerID);
        // 列表删除事件
        void                          do_dele_list(void* AItem);

        // 列表锁
        void                          lock_map() { private_ctl_map_lock_->Enter(); }
        void                          unlock_map() { private_ctl_map_lock_->Leave(); }
    private:
        // 事件
        TOnLogin                      on_login;
        TOnLoginOut                   on_login_out;
        TOnPrivate                    on_private;
        TOnReply                      on_reply;
        TOnBroadcast                  on_broadcast;
        TOnQuery                      on_query;

    private:
        TKYCritSect*                  state_lock_;      // 状态锁
        TState                        state_;           // 状态
        long                          ref_count_;        // 引用计数
        DWord                         link_serial_num_; // 连接的消息序号
        TKYTimer*                     timer_;           // 定时器对象
        long                          timer_id_;        // 定时器id
        bool                          is_set_timeout_;  // 是否已设置超时
        Longword                      close_thread_id_;  // 正在关闭的线程id
        DWORD                         conn_tick_;

        // 连接的客户端id
        DWord                         id_;
        // 接收数据缓冲区
        TKYCycBuffer*                 rec_buffer_;
        // 链接
        TTCPConnObj*                  conn_obj_;
        // 接收线程
        TKYThread*                    rec_thread_;
        // 线程通知对象
        TKYEvent*                     notify_;

        // 私聊控制消息列表
        TKYMapIntKey<TPrivateCtl>*    private_ctl_map_;
        // 私聊控制消息列表锁
        TKYCritSect*                  private_ctl_map_lock_;
    };
}


#endif