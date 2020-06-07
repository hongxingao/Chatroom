#include "client0.h"

namespace _ghx
{
    // 构造函数
    TClient::TClient(const KYString& AIp, long APort)
    {
        conn_obj_               = new TTCPConnObj;
        rec_buffer_             = new TKYCycBuffer;
        msg_ctl_map_            = new TDataCtlMap;
        state_lock_             = new TKYCritSect;
        map_lock_               = new TKYCritSect;
        msg_ctl_notify_         = new TKYEvent(NULL, TKYEvent::erPulse);

        on_recv_broad.Method    = NULL;
        on_recv_broad.Object    = NULL;
        on_recv_private.Method  = NULL;
        on_recv_private.Object  = NULL;

        on_dis_conn.Method        = NULL;
        on_dis_conn.Object        = NULL;

        id_                     = 0;
        state_                  = kClosed;
        ref_count_              = 0;
        serial_num_             = 0;
        close_thread_id_        = 0;
        bool_head_              = false;
        // 绑定事件
        conn_obj_->SetOnRecvData((TDoRecvData)&TClient::do_recv_data, this);
        conn_obj_->SetOnDisconnect((TDoConnEvent)&TClient::do_disconn, this);

        set_curr_addr(AIp);
        set_curr_port(APort);
    }

    // 析构函数
    TClient::~TClient()
    {
        close(true);

        // 释放资源
        FreeAndNil(conn_obj_);
        FreeAndNil(rec_buffer_);
        FreeAndNil(msg_ctl_map_);
        FreeAndNil(state_lock_);
        FreeAndNil(map_lock_);
        FreeAndNil(msg_ctl_notify_);
    }

    // 执行打开连接
    int TClient::do_open()
    {
        int result = TU_trFailure;
        if (conn_obj_->Open() == TU_trSuccess)
        {
            result = do_login();
            // 登陆失败-》关闭
            if (result != TU_trSuccess)
                conn_obj_->Close();
        }

        return result;
    }

    // 开启
    int TClient::open()
    {
        bool result = TU_trFailure;
        bool next = false;
        Longword curr_thread_id = GetCurrentThreadId();

        // 更改状态
        lock();
        if (state_ == kOpened)      
            result = TU_trSuccess;
        else if (state_ == kClosed) 
        {
            next = true;
            state_ = kOpening;
            close_thread_id_ = curr_thread_id;
        }
        unlock();

        // 是否继续
        if (next)
        {
            next = false;
            result = do_open();

            lock();
            if (state_ != kOpening)
                next = true;
            else if (result == TU_trSuccess)
                state_ = kOpened;
            else
                state_ = kClosed;
            unlock();

            // 关闭
            if (next)
            {
                if ((result == TU_trSuccess))
                    do_close(true);
                state_ = kClosed;
            }
        }

        return result;
    }

    // 关闭
    void TClient::close(bool ANeedWait)
    {
        // 检查状态
        if (state_ == kClosed)
            return;

        // 初始化
        bool     boolNext = false;
        bool     boolWait = false;
        Longword curr_thread_id = GetCurrentThreadId();

        // 更改状态
        lock();
        if (state_ == kOpened)
        {
            state_ = kClosing;
            boolNext = true;
            close_thread_id_ = curr_thread_id;
        }
        else if (state_ == kOpening)
        {
            state_ = kClosing;
            boolWait = ANeedWait && (close_thread_id_ != curr_thread_id);
        }
        else if (state_ == kClosing)
            boolWait = ANeedWait && (close_thread_id_ != curr_thread_id);
        unlock();

        // 判断是否继续
        if (boolNext)
        {
            // 执行关闭
            do_close(true);
            state_ = kClosed;
        }
        else if (boolWait)   // 等待 Close 结束
            while (state_ == kClosing)
                Sleep(10);

    }

    // 关闭连接
    void TClient::do_close(bool ANendClose)
    {

        if (ANendClose)
            conn_obj_->Close();

        // 等待引用计数归0
        while (ref_count_ > 0)
            Sleep(10);

        bool_head_ = false;
        // 清除列表
        clear_ctl_map();
    }

    // 清除列表
    void TClient::clear_ctl_map()
    {
        TKYList tmp_list;
        try
        {
            map_lock();
            int cnt = msg_ctl_map_->Count();
            tmp_list.ChangeCapacity(cnt);
            void* node = msg_ctl_map_->First();

            // 添加到删除列表
            while (node != NULL)
            {
                tmp_list.Add(msg_ctl_map_->Value(node));
                node = msg_ctl_map_->Next(node);
            }
            msg_ctl_map_->Clear();
            map_unlock();

            // 绑定删除事件
            tmp_list.OnDeletion.Method = (TKYList::TDoDeletion)&TClient::do_del_ctl;
            tmp_list.OnDeletion.Object = this;
        }
        catch (...) { }

        tmp_list.Clear();
    }

    // 加入消息管理
    bool TClient::add_msg_ctl(Longword ASerial, TDataCtl& ACtl)
    {
        // 初始化
        ACtl.is_reply = false;
        ACtl.reply_type = TU_trFailure;
        ACtl.serial_num = ASerial;
        ACtl.time = GetTickCount();

        // 加入
        map_lock();
        void* add_ctl_succ = msg_ctl_map_->Add(ASerial, &ACtl);
        map_unlock();

        return add_ctl_succ != NULL;
    }

    // 执行登录
    int TClient::do_login()
    {
        char result = TU_trFailure;
        int msg_len = kHeadLen + sizeof(DWord);
        KYString msg_str(msg_len);
        
        // 判断是否合法
        if (id_ > 0 && msg_str.Length() == msg_len)
        {
            // 组包
            char* msg = (char*)msg_str;
            Longword serial_num = InterlockedIncrement(&serial_num_);
            TLoginMsg login_msg;
            login_msg.head.len = sizeof(DWord);
            login_msg.head.msg_type = kLogin;
            login_msg.head.serial_num = serial_num;
            login_msg.id = id_;
            LoginToBuffer(login_msg, (Byte*)msg);

            TDataCtl msg_ctl;
            if (!add_msg_ctl(serial_num, msg_ctl))
                ;
            else if (conn_obj_->Send(msg, msg_len) == msg_len) // 发送成功
                result = get_msg_reply(serial_num, &msg_ctl);
            else
            {
                map_lock();
                msg_ctl_map_->Delete(serial_num);
                map_unlock();
            }
            
        }
            
        return result;
    }

    // 设置id
    bool TClient::set_id(DWORD AId)
    {
        bool result = false;
        if (state_ == kClosed)
        {
            id_ = AId;
            result = true;
        }

        return result;
    }

    // 设置收到私信事件
    void TClient::set_on_recv_private(TDORecvMsg ADoRecPriv, void* AObject)
    {
        lock();
        if (state_ == kClosed)
        {
            on_recv_private.Method = ADoRecPriv;
            on_recv_private.Object = AObject;
        }
        unlock();

    }

    // 设置收到广播事件
    void TClient::set_on_recv_broadcast(TDORecvMsg ADoRecBroad, void* AObject)
    {
        lock();
        if (state_ == kClosed)
        {
            on_recv_broad.Method = ADoRecBroad;
            on_recv_broad.Object = AObject;
        }
        unlock();

    }

    // 引用计数加一
    bool TClient::inc_ref_count()
    {
        bool result = false;

        InterlockedIncrement(&ref_count_);
        if (state_ == kOpened)
            result = true;
        else
            InterlockedDecrement(&ref_count_);

        return result;
    }

    // 引用计数加一
    bool TClient::inc_opening_ref_count()
    {
        bool result = false;

        InterlockedIncrement(&ref_count_);
        if (state_ >= kOpening)
            result = true;
        else
            InterlockedDecrement(&ref_count_);

        return result;
    }

    // 断开连接
    void TClient::do_disconn(TTCPConnObj* AConnObj)
    {
        bool next = false;

        // 更改状态
        lock();
        if (state_ == kOpened)
        {
            next = true;
            state_ = kClosing;
        }
        else if (state_ == kOpening)
            state_ = kClosing;
        unlock();

        // 是否继续
        if (next)
        {
            do_close(false);
            state_ = kClosed;
        }
        if (on_dis_conn.Method != NULL)
            ((TObject*)on_dis_conn.Object->*on_dis_conn.Method)();
    }

    // 删除消息控制
    void TClient::do_del_ctl(TDataCtl* ACtl)
    {
        delete ACtl;
    }

    // 接收数据事件
    void TClient::do_recv_data(TTCPConnObj* AConnObj, void* AData, long ASize)
    {
        rec_buffer_->Push(AData, ASize);
        deal_rec();
    }

    // 处理接受缓冲区里的数据
    void TClient::deal_rec()
    {
        bool need_close = false;
 
        KYString rec_str(kMaxMsgLen);
        char* rec_msg = (char*)rec_str;

        if (rec_str.Length() == kMaxMsgLen && inc_opening_ref_count())
        {
            while (rec_buffer_->Size() >= kHeadLen || bool_head_)
            {
                if (!bool_head_) // 未得到消息头
                {
                    int rec_size = rec_buffer_->Pop(rec_msg, kHeadLen);
                    if (rec_size != kHeadLen || !BufferToHead((Byte*)rec_msg, rec_head_))
                    {
                        need_close = true;
                        break;
                    }
                    // bool_head置为false
                    bool_head_ = true;
                }
                else if (rec_buffer_->Size() >= rec_head_.len) // 已经得到消息头并且消息完整
                {
                    // 消息体过长 扩容
                    if (rec_head_.len > rec_str.Length())
                    {
                        KYString tmp_str(rec_head_.len);
                        if (tmp_str.Length() == rec_head_.len)
                        {
                            rec_str = tmp_str;
                            rec_msg = (char*)rec_str;
                        }
                        else
                        {
                            need_close = true;
                            break;
                        }
                    }

                    // 取消息体
                    if (rec_buffer_->Pop(rec_msg, rec_head_.len) != rec_head_.len
                        || !deal_msg(rec_head_, (Byte*)rec_msg))
                    {
                        need_close = true;
                        break;
                    }
                    // 处理完毕一个消息 则bool_head重新为false
                    bool_head_ = false;
                }
                else
                    break;

            }
            dec_ref_count();
        }

        if (need_close)
            conn_obj_->Close();
    }

    // 处理接收到合法的消息
    bool TClient::deal_msg(const TDataHead& AHead, Byte* AMsg)
    {
        bool result = false;
        switch (AHead.msg_type)
        {
            // 私信、登录回复
            case kLoginReply:
            case kPrivateChatReply:
                {
                    TReplyMsg msg;
                    if (BufferToReplyMsg(AHead, AMsg, msg))
                    {
                        result = true;
                        deal_reply_msg(msg);
                    }
                }
                break;
            // 收到私信
            case kTransPrivateChat:
                {
                    TSendMsg msg;
                    if (BufferToSend(AHead, AMsg, msg))
                    {
                        ((TObject*)on_recv_private.Object->*on_recv_private.Method)(msg.id, msg.data);
                        result = reply_trans_private(msg);
                    }
                           
                }
                break;

            // 收到广播
            case kTransBroadCast:
                {
                    TSendMsg msg;
                    if (BufferToSend(AHead, AMsg, msg))
                    {
                        ((TObject*)on_recv_broad.Object->*on_recv_broad.Method)(msg.id, msg.data);
                        result = true;
                    }
                }
                break;

            // 收到查询回复
            case kQueryReply:
                {
                    TReplyQuery msg;
                    int data_len = AHead.len - sizeof(char)-sizeof(DWord);
                    KYString data_str(data_len);
                    msg.data = (DWord*)(char*)data_str;

                    // 由 BufferToReplyQuery 来判断消息长度是否合法
                    if (data_str.Length() == data_len && BufferToReplyQuery(AHead, AMsg, msg))
                    {
                        result = true;
                        deal_reply_query(msg);
                    }    
                }
                break;

            // 收到广播回复
            case kBroadCaseReply:
                {
                    TReplyBroadcast msg;
                    if (BufferToReplyBroadcast(AHead, AMsg, msg))
                    {
                        result = true;
                        deal_reply_broadcast(msg);
                    }
                }
                break;
            default:
                break;
        }
        
        return result;
    }

    // 处理回复消息 登录 私聊
    void TClient::deal_reply_msg(const TReplyMsg& AMsg)
    {
        bool is_notify = false;
        map_lock();
        void* node = msg_ctl_map_->Find(AMsg.head.serial_num);

        if (node != NULL)
        {
            TDataCtl* tmp_ctl = msg_ctl_map_->Value(node);
            tmp_ctl->is_reply = true;
            tmp_ctl->reply_type = AMsg.reply_type;
            is_notify = true;
        }
        map_unlock();

        // 是否通知
        if (is_notify)
            msg_ctl_notify_->Pulse();
    }

    // 处理查询回复
    void TClient::deal_reply_query(const TReplyQuery& AMsg)
    {
        bool is_notify = false;

        // 把回复列表添加到本地消息控制
        map_lock();
        void* node = msg_ctl_map_->Find(AMsg.head.serial_num);
        if (node != NULL)
        {
            TDataCtl* tmp_ctl = msg_ctl_map_->Value(node);
            tmp_ctl->is_reply = true;
            tmp_ctl->reply_type = AMsg.reply_type;
            is_notify = true;
            if (tmp_ctl->reply_type == TU_trSuccess)
                for (int i = 0; i < AMsg.count; i++)
                    tmp_ctl->data.Add((void*)AMsg.data[i]);
        }
        map_unlock();

        // 是否通知
        if (is_notify)
            msg_ctl_notify_->Pulse();
    }

    // 处理广播回复
    void TClient::deal_reply_broadcast(const TReplyBroadcast& AMsg)
    {
        bool is_notify = false;

        map_lock();
        void* node = msg_ctl_map_->Find(AMsg.head.serial_num);
        if (node != NULL)
        {
            TDataCtl* tmp_ctl = msg_ctl_map_->Value(node);
            tmp_ctl->is_reply = true;
            tmp_ctl->reply_type = AMsg.reply_type;
            is_notify = true;

            // 如果成功
            if (tmp_ctl->reply_type == TU_trSuccess)
                tmp_ctl->data.Add((void*)AMsg.count);
        }
        map_unlock();

        // 是否通知
        if (is_notify)
            msg_ctl_notify_->Pulse();
    }

    // 获取消息回应
    int TClient::get_msg_reply(Longword ASerial, TDataCtl* ADataCtl)
    {
        char result = TU_trFailure;
        if (inc_opening_ref_count())
        {
            // 等待通知
            while (!ADataCtl->is_reply && state_ >= kOpening
                                       && GetTickCount() - ADataCtl->time < 30000)
                msg_ctl_notify_->Wait(100);

            // 删除元素
            map_lock();
            msg_ctl_map_->Delete(ASerial);
            map_unlock();

            // 得到返回值
            if (ADataCtl->is_reply)
                result = ADataCtl->reply_type;
            else if (state_ >= kOpening)
                result = TU_trTimeout;

            dec_ref_count();
        }

        return result;
    }

    // 私聊某用户
    int TClient::private_chat(DWORD AId, const KYString& AStr)
    {
        int result = TU_trFailure;
        int msg_len = kHeadLen + sizeof(DWord) + AStr.Length();
        KYString msg_str(msg_len);

        if (msg_str.Length() == msg_len && inc_ref_count())
        {
            // 组包
            char* msg = (char*)msg_str;
            Longword serial_num = InterlockedIncrement(&serial_num_);
            TSendMsg send_msg;
            send_msg.head.len = sizeof(DWord)+AStr.Length();
            send_msg.head.serial_num = serial_num;
            send_msg.head.msg_type = kPrivateChat;
            send_msg.id = AId;
            send_msg.data = AStr;
            SendToBuffer(send_msg, (Byte*)msg);

            // 发送
            TDataCtl msg_ctl;
            if (!add_msg_ctl(serial_num, msg_ctl))
                ;
            else if (conn_obj_->Send(msg, msg_len) == msg_len)  // 发送成功
                result = get_msg_reply(serial_num, &msg_ctl);
            else
            {
                map_lock();
                msg_ctl_map_->Delete(serial_num);
                map_unlock();
            }
        
            dec_ref_count();
        }

        return result;
    }

    // 广播
    DWord TClient::broadcast(const KYString& AStr)
    {
        DWord result = 0;
        int msg_len = kHeadLen + AStr.Length();
        KYString msg_str(msg_len);
        
        if (msg_str.Length() == msg_len && inc_ref_count())
        {
            // 组包
            char* msg = (char*)msg_str;
            Longword serial_num = InterlockedIncrement(&serial_num);
            TBroadcastdMsg broadcast_msg;
            broadcast_msg.head.len = AStr.Length();
            broadcast_msg.head.msg_type = kBroadCast;
            broadcast_msg.head.serial_num = serial_num;
            broadcast_msg.data = AStr;
            BroadcastToBuffer(broadcast_msg, (Byte*)msg);

            // 发送
            TDataCtl msg_ctl;
            if (!add_msg_ctl(serial_num, msg_ctl))
                ;
            else if (conn_obj_->Send(msg, msg_len) != msg_len)  //发送失败
            {
                map_lock();
                msg_ctl_map_->Delete(serial_num);
                map_unlock();
            }
            else if (get_msg_reply(serial_num, &msg_ctl) == TU_trSuccess)
                result = (DWord)msg_ctl.data.Item(0);
        
            dec_ref_count();
        }
    
        return result;
    }

    // 查询当前在线用户
    bool TClient::query(DWord ABegin, DWord AEnd, TKYList& AList)
    {
        bool result = false;
        int msg_len = kHeadLen + sizeof(DWord) * 2;
        KYString msg_str(msg_len);
        
        if (msg_str.Length() == msg_len && inc_ref_count())
        {
            // 组包
            char* msg = (char*)msg_str;
            Longword serial_num = InterlockedIncrement(&serial_num);
            TQueryMsg query_msg;
            query_msg.head.len = sizeof(DWord)* 2;
            query_msg.head.msg_type = kQuery;
            query_msg.head.serial_num = serial_num;
            query_msg.begin = ABegin;
            query_msg.end = AEnd;
            QueryToBuffer(query_msg, (Byte*)msg);

            // 发送
            TDataCtl msg_ctl;
            if (!add_msg_ctl(serial_num, msg_ctl))
                ;
            else if (conn_obj_->Send(msg, msg_len) != msg_len) //发送失败
            {
                map_lock();
                msg_ctl_map_->Delete(serial_num);
                map_unlock();
            }
            else if (get_msg_reply(serial_num, &msg_ctl) == TU_trSuccess)
            {
                AList = msg_ctl.data;
                result = true;
            }
            
            dec_ref_count();
        }
         
        return result;
    }

    // 转发消息回复
    bool TClient::reply_trans_private(const TSendMsg& AMsg)
    {
        bool result = false;
        if (inc_ref_count())
        {
            int msg_len = kHeadLen + sizeof(char);
            KYString msg_str(msg_len);
            if (msg_str.Length() == msg_len)
            {
                // 组包
                char* msg = (char*)msg_str;
                TReplyMsg reply_msg;
                reply_msg.head.len = sizeof(char);
                reply_msg.head.serial_num = AMsg.head.serial_num;
                reply_msg.head.msg_type = kPrivateChatReply;
                reply_msg.reply_type = TU_trSuccess;
                ReplyMsgToBuffer(reply_msg, (Byte*)msg);

                // 发送
                result = (conn_obj_->Send(msg, msg_len) == msg_len);
            }
            dec_ref_count();
        }

        return result;
    }

}

