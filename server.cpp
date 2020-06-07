#include <stdio.h>
#include <conio.h>
#include "server.h"

namespace _ghx
{
    // 构造函数
    TServer::TServer(const KYString& AIp, Longword APort)
    {
        obj_server_          = new TTCPServer;
        state_lock_          = new TKYCritSect;
        map_lock_            = new TKYLockRW;
        user_map_            = new TUserMap;
        conn_map_            = new TConnMap;

        destory_link_thread_ = NULL;
        timer_               = NULL;
        state_               = kClosed;
        close_thread_id_     = 0;
        timer_id_            = 0;
        is_set_timeout_      = false;

        obj_server_->SetOnAccept((TDoAccept)&TServer::do_accept, this);
        obj_server_->SetOnCltRecvData((TDoRecvData)&TServer::do_recv_data, this);
        obj_server_->SetOnFreeClt((TDoConnEvent)&TServer::do_free_ctl, this);

        set_addr(AIp);
        set_port(APort);
    }

    // 析构函数
    TServer::~TServer()
    {
        close(true);

        if (timer_ != NULL)
        {
            timer_->Delete(timer_id_);
            timer_ = NULL;
            timer_id_ = 0;
        }

        // 释放
        FreeAndNil(obj_server_);
        FreeAndNil(state_lock_);
        FreeAndNil(map_lock_);
        FreeAndNil(user_map_);
        FreeAndNil(conn_map_);
        
    }

    // 执行开启
    bool TServer::do_open()
    {
        bool result = false;
        if (timer_ != NULL && obj_server_->Open() == TU_trSuccess)
        {
            destory_link_thread_ = new TKYRunThread;
            if (destory_link_thread_ != NULL)
                result = true;
            else
                obj_server_->Close();
        }
           
        return result;
    }

    // 执行关闭
    void TServer::do_close()
    {
        obj_server_->Close();

        while (ref_count_ > 0)
            Sleep(9);

        // 清除列表项
        clear_map();
        // 释放线程
        while (destory_link_thread_->Count() > 0)
            Sleep(9);
        FreeKYRunThread(destory_link_thread_);
    }

    // 开启服务器
    bool TServer::open()
    {
        bool result = false;
        bool next = false;
        Longword cur_thread_id = GetCurrentThreadId();
        lock();
        if (state_ >= kOpened)
            result = true;
        else if (state_ == kClosed)
        {
            next = true;
            ref_count_ = 0;
            state_ = kOpening;
            cur_thread_id = cur_thread_id;
        }
        unlock();

        if (next)
        {
            next = false;
            result = do_open();

            lock();
            if (state_ != kOpening)
                next = true;
            else if (result)
                state_ = kOpened;
            else
                state_ = kClosed;
            unlock();

            if (next)
            {
                if (result)
                {
                    do_close();
                    result = false;
                }
                state_ = kClosed;
            }
        }

        return result;
    }

    // 关闭服务器
    void TServer::close(bool ANeedWait)
    {
        // 检查状态
        if (state_ == kClosed)
            return;

        // 初始化
        bool     boolNext = false;
        bool     boolWait = false;
        Longword cur_thread_id = GetCurrentThreadId();

        // 更改状态
        lock();
        if (state_ >= kOpened)
        {
            state_ = kClosing;
            boolNext = true;
            close_thread_id_ = cur_thread_id;
        }
        else if (state_ == kOpening)
        {
            state_ = kClosing;
            boolWait = ANeedWait && (close_thread_id_ != cur_thread_id);
        }
        else if (state_ == kClosing)
            boolWait = ANeedWait && (close_thread_id_ != cur_thread_id);
        unlock();

        // 判断是否继续
        if (boolNext)
        {
            // 执行关闭
            do_close();
            state_ = kClosed;
        }
        else if (boolWait)   // 等待 Close 结束
            while (state_ == kClosing)
                Sleep(10);

    }

    // 引用计数加一
    bool TServer::inc_ref_count()
    {
        bool result = false;

        InterlockedIncrement(&ref_count_);
        if (state_ >= kOpened)
            result = true;
        else
            InterlockedDecrement(&ref_count_);

        return result;
    }

    // 设置定时器
    bool TServer::set_timer(TKYTimer* ATimer)
    {
        // 初始化
        bool        result = false;
        long        intID = 0;
        TKYTimer*   objTimer = NULL;

        // 操作
        lock();
        if (state_ != kClosed)
            ;
        else if (timer_ == ATimer)
            result = true;
        else
        {
            // 备份
            objTimer = timer_;
            intID = timer_id_;

            // 判断是否为空
            if (ATimer == NULL)
            {
                timer_ = NULL;
                timer_id_ = 0;
                result = true;
            }
            else
            {
                long intNew = ATimer->New((TKYTimer::TDoTimer)&TServer::do_timer, this, NULL);
                if (intNew > 0)
                {
                    timer_ = ATimer;
                    timer_id_ = intNew;
                    result = true;
                }
            }
        }
        unlock();

        // 判断是否成功
        if (result && (objTimer != NULL))
            objTimer->Delete(intID);

        // 返回结果
        return result;
    }

    // 列表项删除事件方法
    void TServer::delete_list(TLink* AItem)
    {
        AItem->close(false);
        FreeAndNil(AItem);
    }

    // 删除并释放map项
    void TServer::clear_map()
    {
        TKYList tmp_list;
        tmp_list.OnDeletion.Method = (TKYList::TDoDeletion)&TServer::delete_list;
        tmp_list.OnDeletion.Object = this;

        map_lock_write();
        tmp_list.ChangeCapacity(conn_map_->Count() + user_map_->Count());
        void* node = conn_map_->First();
        while (node != NULL)
        {
            TLink* tmp_link = conn_map_->Value(node);
            node = conn_map_->Next(node);
            tmp_list.Add(tmp_link);
        }
        conn_map_->Clear();

        node = user_map_->First();
        while (node != NULL)
        {
            TLink* tmp_link = user_map_->Value(node);
            node = user_map_->Next(node);
            tmp_list.Add(tmp_link);
        }
        user_map_->Clear();
        map_unlock_write(); 

        tmp_list.Clear();
    }

    // 定时器事件方法（控制超时）
    void TServer::do_timer(void* Sender, void* AParam, long ATimerID)
    {
        if (inc_ref_count())
        {
            TKYList tmp_list;
            tmp_list.OnDeletion.Method = (TKYList::TDoDeletion)&TServer::delete_list;
            tmp_list.OnDeletion.Object = this;

            map_lock_write();
            void* tmp_node = conn_map_->First();

            // 遍历检查超时
            while (tmp_node != NULL)
            {
                void* tmp_node2 = tmp_node;
                tmp_node = conn_map_->Next(tmp_node);

                TLink* tmp_link = conn_map_->Value(tmp_node2);
                // 如果超时
                if (tmp_link != NULL && GetTickCount() - tmp_link->Conn_tick() >= 30000 && tmp_list.Add(tmp_link) >= 0)
                    conn_map_->Remove(tmp_node2);
                
            }
            map_unlock_write();
            
            tmp_list.Clear();

            // 每隔1秒检查一次超时
            if (timer_ != NULL)
                timer_->AddEvent(ATimerID, 1000);

            dec_ref_count();
        }
        
    }

    // 接受连接
    void TServer::do_accept(TTCPServer* AServer, TTCPConnObj* AConnObj, bool& AIsRefused)
    {
        AIsRefused = false;
        if (AConnObj == NULL)
            ;
        else if (inc_ref_count())
        {
            TLink* new_link = new TLink(AConnObj);
            set_link_event(new_link);
            new_link->set_timer(timer_);

            if (new_link->open())
            {
                bool is_succ_add = false;

                map_lock_write();
                if (conn_map_->Add(new_link, new_link) != NULL)
                {
                    new_link->set_conn_tick(GetTickCount());
                    is_succ_add = true;
                }
                map_unlock_write();

                if (!is_succ_add) // 添加失败
                {
                    AIsRefused = true;
                    delete new_link;
                }
                else if (!is_set_timeout_) // 设置超时
                {
                    timer_->AddEvent(timer_id_, 30000);
                    is_set_timeout_ = true;
                }
            }
            else
            {
                AIsRefused = true;
                delete new_link;
            }
                
            dec_ref_count();
        }
        else
            AIsRefused = true;

    }

    // 接收数据的方法
    void TServer::do_recv_data(TTCPConnObj* AConnObj, void* AData, long ASize)
    {
        TLink* cur_link = (TLink*)AConnObj->Data();
        if (cur_link != NULL && cur_link->Conn_obj() == AConnObj)
            cur_link->do_recv(AData, ASize);
    }

    // 释放连接
    void TServer::do_free_ctl(TTCPConnObj* AConnObj)
    {
        TLink* cur_link = (TLink*)AConnObj->Data();
        if (cur_link != NULL && cur_link->Conn_obj() == AConnObj)
        {
            // 触发断开连接
            cur_link->do_disconn();

            // 从列表删除连接
            DWord cur_id = cur_link->Id();
            if (cur_id > 0)
                delete_user(cur_id, cur_link);
            else
                delete_conn(cur_link);
           
        }
           
    }

    // 设置连接的事件方法
    void TServer::set_link_event(TLink* ALink)
    {
        if (ALink != NULL)
        {
            ALink->set_on_login((TDoLogin)&TServer::link_login, this);
            ALink->set_on_login_out((TDoLoginOut)&TServer::link_login_out, this);
            ALink->set_on_private((TDoPrivate)&TServer::link_private, this);
            ALink->set_on_broadcast((TDoBroadcast)&TServer::link_broadcast, this);
            ALink->set_on_reply((TDoReply)&TServer::link_reply, this);
            ALink->set_on_query((TDoQuery)&TServer::link_query, this);
        }
    }

    // 登录
    char TServer::link_login(TLink* ALink, DWord AId)
    {
        char result = TU_trFailure;
        if (inc_ref_count())
        {
            if (AId == 0)
                result = TU_trIsIllegal;
            else
            {
                map_lock_write();
                if (user_map_->Find(AId) != NULL)
                    result = TU_trIsExisted;
                else
                {
                    void* node = conn_map_->Find(ALink);
                    if (node != NULL && user_map_->Add(AId, ALink) != NULL)
                    {
                        conn_map_->Remove(node);
                        result = TU_trSuccess;
                        if (on_login_sucess.Method != NULL)
                            ((TObject*)on_login_sucess.Object->*on_login_sucess.Method)(ALink->Conn_obj()->PeerAddr(), AId);

                    }
                }
                map_unlock_write();
            }
            dec_ref_count();
        }

        return result;
    }

    // 登出
    void TServer::link_login_out(TLink* ALink)
    {
        if (inc_ref_count())
        {
            map_lock_write();
            void* tmp_node = user_map_->Find(ALink->Id());
            if (tmp_node != NULL && ALink == user_map_->Value(tmp_node))
            {
                user_map_->Remove(tmp_node);
                destory_link_thread_->AddOfP1((TKYRunThread::TDoP1)&TServer::delete_list, this, ALink);
            }
            map_unlock_write();
            dec_ref_count();
        }
    }

    // 私信
    char TServer::link_private(TLink* ALink, DWord AId, DWord ASeriNum, const KYString& AData)
    {
        char result = TU_trFailure;

        if (inc_ref_count())
        {
            if (AId == 0)
                result = TU_trIsIllegal;
            else
            {
                map_lock_read();
                void* des_node = user_map_->Find(AId);
                if (des_node == NULL)
                    result = TU_trNotExist;
                else
                {
                    TLink* des_link = user_map_->Value(des_node);
                    if (des_link->send_trans_private(ASeriNum, ALink->Id(), AData))
                        result = TU_trSuccess;
                    
                }
                map_unlock_read();
            }
            dec_ref_count();
        }

        return result;
    }

    // 广播
    int TServer::link_broadcast(TLink* ALink, const KYString& AData)
    {
        DWord result = 0;

        if (inc_ref_count())
        {
            map_lock_read();
            void* node = user_map_->First();
            while (node != NULL)
            {
                TLink* tmp_link = user_map_->Value(node);
                if (tmp_link != ALink && tmp_link->send_trans_broadcast(0, ALink->Id(), AData))
                    result++;
                
                node = user_map_->Next(node);
            }
            map_unlock_read();

            dec_ref_count();
        }

        return result;
    }

    // 回复
    bool TServer::link_reply(TLink* ALink, DWord AId, DWord ASeriNum, DWord AReply)
    {
        bool result = false;
        
        if (inc_ref_count())
        {
            map_lock_read();
            void* des_node = user_map_->Find(AId);
            if (des_node != NULL)
            {
                TLink* des_link = user_map_->Value(des_node);
                result = des_link->send_reply_msg(ASeriNum, AReply);
            }
            map_unlock_read();

            dec_ref_count();
        }

        return result;
    }

    // 查询
    char TServer::link_query(TLink* ALink, DWord ABegin, DWord AEnd, TKYList& AList)
    {
        char result = TU_trFailure;
        
        if (inc_ref_count())
        {
            map_lock_read();
            // 保证索引合法
            int count = user_map_->Count();
            if (AEnd >= count)
                AEnd = count - 1;

            if (ABegin <= AEnd)
            {
                void* node = user_map_->Node(ABegin);
                for (int i = ABegin; i <= AEnd; i++)
                {
                    AList.Add((void*)user_map_->Key(node));
                    node = user_map_->Next(node);
                }
            }
            map_unlock_read();
            dec_ref_count();
        }

        if (AList.Count() > 0)
            result = TU_trSuccess;

        return result;
    }

    // 删除一个用户
    bool TServer::delete_user(DWord AId, TLink* ALink)
    {
        bool result = false;
        if (inc_ref_count())
        {
            TLink* tmp_link = NULL;
            map_lock_write();
            void* tmp_node = user_map_->Find(AId);
            if (tmp_node != NULL 
                && (ALink == NULL || ALink == user_map_->Value(tmp_node)))
            {
                tmp_link = user_map_->Value(tmp_node);
                user_map_->Remove(tmp_node);
                result = true;
                 
            }
            map_unlock_write();

            dec_ref_count();
            if (result)
                delete tmp_link;
        }
        
        return result;
    }

    // 删除一个连接 
    bool TServer::delete_conn(TLink* ALink)
    {
        bool result = false;
        if (inc_ref_count())
        {
            map_lock_write();
            void* tmp_node = conn_map_->Find(ALink);
            if (tmp_node != NULL)
            {
                conn_map_->Remove(tmp_node);
                result = true;
            }
            map_unlock_write();
            dec_ref_count();

            if (result)
                delete ALink;
        }
        
        return result;
    }

    // 设置用户登录事件
    bool TServer::set_loign_sucess(TDoLoginSucess AMethod, void* Object)
    {
        bool result = false;
        if (state_ == kClosed)
        {
            on_login_sucess.Method = AMethod;
            on_login_sucess.Object = Object;
        }

        return result;
    }

    // 当前在线信息
    bool TServer::query_users(DWord ABegin, DWord AEnd, TKYStringList& AStrList)
    {
        bool result = false;

        map_lock_read();
        // 保证索引合法
        int cnt = user_map_->Count();
        if (AEnd > cnt && cnt > 0)
            AEnd = cnt - 1;

        if (ABegin <= AEnd && cnt > 0)
        {
            void* node = user_map_->Node(ABegin);
            for (DWord i = ABegin; i <= AEnd; i++)
            {
                AStrList.Add(user_map_->Value(node)->Perr_addr(), (void*)user_map_->Key(node));
                node = user_map_->Next(node);
            }
    
            result = (AStrList.Count() > 0);
        }
        map_unlock_read();

        return result;
    }

}
