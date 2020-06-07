#include "link.h"

namespace _ghx
{
    // 构造函数
    TLink::TLink(TTCPConnObj* AConn)
    {
        conn_obj_             = AConn;
        rec_buffer_           = NULL;
        private_ctl_map_      = NULL;
        notify_               = NULL;
        private_ctl_map_lock_ = NULL;
        if (conn_obj_ != NULL)
        {
            conn_obj_->SetData(this);
          
            rec_buffer_ = new TKYCycBuffer(8, TKYCycBuffer::stSize2_13);
            private_ctl_map_ = new TKYMapIntKey<TPrivateCtl>(true); 
            notify_ = new TKYEvent;
            private_ctl_map_lock_ = new TKYCritSect;
        }

        state_lock_ = new TKYCritSect;
        init_link();
    }

    // 析构函数
    TLink::~TLink()
    {
        close(true);

        if (conn_obj_ != NULL)
            conn_obj_->SetData(NULL);
        
        if (timer_ != NULL)
        {
            timer_->Delete(timer_id_);
            timer_ = NULL;
            timer_id_ = 0;
        }

        FreeAndNil(rec_buffer_);
        FreeAndNil(state_lock_);
        FreeAndNil(private_ctl_map_lock_);
        FreeAndNil(private_ctl_map_);
        FreeAndNil(notify_);
    }

    // 初始化
    void TLink::init_link()
    {
        timer_               = NULL;
        timer_id_            = 0;
        is_set_timeout_      = false;
        rec_thread_          = NULL;

        conn_tick_             = 0;
        state_               = kClosed;
        ref_count_           = 0;
        link_serial_num_     = 0;
        id_                  = 0;
        close_thread_id_     = 0;
        // 初始化各种事件
        on_login.Method      = NULL;
        on_login.Object      = NULL;
        on_login_out.Method  = NULL;
        on_login_out.Object  = NULL;
        on_private.Method    = NULL;
        on_private.Object    = NULL;
        on_broadcast.Method  = NULL;
        on_broadcast.Object  = NULL;
        on_reply.Method      = NULL;
        on_reply.Object      = NULL;
        on_query.Method      = NULL;
        on_query.Object      = NULL;
        //on_ctl_timeout.Method = NULL;
        //on_ctl_timeout.Object = NULL;
    }

    // 执行开启
    bool TLink::do_open()
    { 
        bool result = false;
    
        if (on_login.Method != NULL && on_login_out.Method != NULL && on_private.Method != NULL
            && on_reply.Method != NULL && on_broadcast.Method != NULL && on_query.Method != NULL
            && timer_ != NULL && conn_obj_ != NULL && conn_obj_->Open() == TU_trSuccess)
        {
            // 创建线程
            rec_thread_ = new TKYThread((TKYThread::TDoExecute)&TLink::deal_rec, this, NULL, false);

            if (rec_thread_ != NULL)
                result = true;
            else
                conn_obj_->Close();
        }

        return result;
    }

    // 执行关闭
    void TLink::do_close(bool ANeedClose, bool ANeedEvent)
    {
        rec_thread_->Terminate();
        notify_->Set();

        if (ANeedClose)
            conn_obj_->Close();
       
        // 等待引用计数为0
        while (ref_count_ > 0)
            Sleep(10);

        if (ANeedEvent)
            ((TObject*)on_login_out.Object->*on_login_out.Method)(this);
        FreeKYThread(rec_thread_);
    }

    // 开启连接
    bool TLink::open()
    {
        // 初始化
        bool result = false;
        bool next = false;
        Longword curr_thread_id = GetCurrentThreadId();

        // 更改状态
        lock();
        if (state_ >= kOpened)
            result = true;
        else if (state_ == kClosed)
        {
            next = true;
            state_ = kOpening;
            close_thread_id_ = curr_thread_id;
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
                    do_close(true, false);
                    result = false;
                }
                state_ = kClosed;
            }
        }

        return result;
    }

    // 关闭连接
    void TLink::close(bool ANeedWait)
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
        if (state_ >= kOpened)
        {
            state_ = kClosing;
            close_thread_id_ = curr_thread_id;
            boolNext = true;
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
            do_close(true, false);
            state_ = kClosed;
        }
        else if (boolWait)   // 等待 Close 结束
            while (state_ == kClosing)
                Sleep(10);

    }

    // 设置定时器
    bool TLink::set_timer(TKYTimer* ATimer)
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
                long intNew = ATimer->New((TKYTimer::TDoTimer)&TLink::do_timer, this, NULL);
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

    // 列表删除事件
    void TLink::do_dele_list(void* AItem)
    {
        ((TObject*)on_reply.Object->*on_reply.Method)(this, ((TPrivateCtl*)AItem)->id, ((TPrivateCtl*)AItem)->msg_serial_num, TU_trTimeout);
        delete (TPrivateCtl*)AItem;
    }

    // 定时器事件方法（控制超时）
    void TLink::do_timer(void* Sender, void* AParam, long ATimerID)
    {
        // 初始化超时list
        TKYList tmp_list(false);
        tmp_list.OnDeletion.Method = (TKYList::TDoDeletion)&TLink::do_dele_list;
        tmp_list.OnDeletion.Object = this;
    
        // 加map锁
        lock_map();
        void* tmp_node = private_ctl_map_->First();

        // 遍历检查超时
        while (tmp_node != NULL)
        {
            void* tmp_node2 = tmp_node;
            tmp_node = private_ctl_map_->Next(tmp_node);

            TPrivateCtl* tmp_ctl = private_ctl_map_->Value(tmp_node2);
            if (tmp_ctl != NULL && difftime(tmp_ctl->time, time(NULL) >= 20))
            {
                if (tmp_list.Add(tmp_ctl) >= 0)
                    private_ctl_map_->SetValue(tmp_node2, NULL);
            
                private_ctl_map_->Remove(tmp_node2);
            }
                
        }
        unlock_map();

        tmp_list.Clear();

        // 每隔三秒检查一次超时
        if (timer_ != NULL)
            timer_->AddEvent(ATimerID, 3000);
        
    }

    // 引用计数加一
    bool TLink::inc_ref_count()
    {
        bool result = false;

        InterlockedIncrement(&ref_count_);
        if (state_ >= kOpened)
            result = true;
        else
            InterlockedDecrement(&ref_count_);

        return result;
    }

    // 设置登录事件
    void TLink::set_on_login(TDoLogin AMethod, void* AObj)
    {
        lock();
        if (state_ == kClosed)
        {
            on_login.Method = AMethod;
            on_login.Object = AObj;
        }
        unlock();
    }

    // 设置登出事件
    void TLink::set_on_login_out(TDoLoginOut AMethod, void* AObj)
    {
        lock();
        if (state_ == kClosed)
        {
            on_login_out.Method = AMethod;
            on_login_out.Object = AObj;
        }
        unlock();

    }

    // 设置私聊事件
    void TLink::set_on_private(TDoPrivate AMethod, void* AObj)
    {
        lock();
        if (state_ == kClosed)
        {
            on_private.Method = AMethod;
            on_private.Object = AObj;
        }
        unlock();
    }

    // 设置广播事件
    void TLink::set_on_broadcast(TDoBroadcast AMethod, void* AObj)
    {
        lock();
        if (state_ == kClosed)
        {
            on_broadcast.Method = AMethod;
            on_broadcast.Object = AObj;
        }
        unlock();
    }

    // 设置回复事件
    void TLink::set_on_reply(TDoReply AMethod, void* AObj)
    {
        lock();
        if (state_ == kClosed)
        {
            on_reply.Method = AMethod;
            on_reply.Object = AObj;
        }
        unlock();
    }

    // 设置查询事件
    void TLink::set_on_query(TDoQuery AMethod, void* AObj)
    {
        lock();
        if (state_ == kClosed)
        {
            on_query.Method = AMethod;
            on_query.Object = AObj;
        }
        unlock();
    }

    // 接收数据
    void TLink::do_recv(void* AData, long ASize)
    {
        rec_buffer_->Push(AData, ASize);
        notify_->Set();
    }

    // 处理接收到合法的消息
    bool TLink::deal_msg(const TDataHead& AHead, Byte* AMsg)
    {
        bool result = false;
        if (AHead.msg_type == kLogin)
        {
            TLoginMsg msg;
            result = BufferToLogin(AHead, AMsg, msg) && deal_login(msg);
        }
        else if (id_ > 0) //已登录
        {
            switch (AHead.msg_type)
            {
            case kPrivateChat:
                {
                    TSendMsg msg;
                    result = BufferToSend(AHead, AMsg, msg) && deal_private(msg);
                }
                break;
            case kBroadCast:
                {
                    TBroadcastdMsg msg;
                    result = BufferToBroadcast(AHead, AMsg, msg) && deal_broadcast(msg);
                }
                break;
            case kQuery:
                {
                    TQueryMsg msg;
                    result = BufferToQuery(AHead, AMsg, msg) && deal_query(msg);
                }
                break;
            case kPrivateChatReply:
                {
                    TReplyMsg msg;
                    result = BufferToReplyMsg(AHead, AMsg, msg) && deal_reply_trans_private(msg);
                }
                break;
            default:
                break;
            }
        }
    
        return result;
    }

    // 处理缓冲区里的数据
    unsigned TLink::deal_rec(TKYThread* AThread, void* AParam)
    {
        bool need_close = false;
        bool bool_head = false;
        TDataHead rec_head;
        
        KYString rec_str(kMaxMsgLen);
        char* rec_msg = (char*)rec_str;

        while (!rec_thread_->Terminated() && state_ == kOpening)
            notify_->Wait(1000);

        if (rec_str.Length() == kMaxMsgLen && inc_ref_count())
        {
            while (state_ >= kOpened)
            {
                if (bool_head) // 已经得到消息头
                {
                    if (rec_buffer_->Size() >= rec_head.len)
                    {
                        // 消息体过长 扩容
                        if (rec_head.len > rec_str.Length())
                        {
                            KYString tmp_str(rec_head.len);
                            if (tmp_str.Length() == rec_head.len)
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
                        if (rec_buffer_->Pop(rec_msg, rec_head.len) != rec_head.len
                            || !deal_msg(rec_head, (Byte*)rec_msg ))
                        {
                            need_close = true;
                            break;
                        }
                        // 处理完毕一个消息 则bool_head重新为false
                        bool_head = false;
                    }
                    else
                        notify_->Wait(1000);
                }
                else if (rec_buffer_->Size() >= kHeadLen) // 未得到消息头
                {
                    int rec_size = rec_buffer_->Pop(rec_msg, kHeadLen);
                    if (rec_size != kHeadLen || !BufferToHead((Byte*)rec_msg, rec_head))
                    {
                        need_close = true;
                        break;
                    }
                    // bool_head置为false
                    bool_head = true;
                }
                else
                    notify_->Wait(1000);
                     
            }    
            dec_ref_count();
        }

        if (need_close)
            conn_obj_->Close();

        return 0;
    }

    // 断开连接事件方法
    void TLink::do_disconn()
    {
        bool next = false;

        lock();
        if (state_ == kOpened)
        {
            next = true;
            state_ = kClosing;
        }
        else if (state_ == kOpening)
            state_ = kClosing;
        unlock();

        if (next)
        {
            do_close(false, true);
            state_ = kClosed;
        }
    }

    //  转发消息
    bool TLink::send_trans_msg(TMsgTypeTag AType, DWord ASeriNum, DWord AId, const KYString& AData)
    {
        bool result = false;
        if (AData.Length() > 0 && inc_ref_count())
        {
            bool is_send = true; // 是否需要发送
            DWord msg_len = kHeadLen + sizeof(DWord)+AData.Length(); //应该发送的尺寸
            DWord send_len;  // 实际发送的尺寸

            KYString msg_str((int)msg_len);
            char* msg = (char*)msg_str;

            DWord tmp_serial_num = InterlockedIncrement(&link_serial_num_); // 连接的消息序列号

            if (msg_str.Length() == msg_len)
            {
                // 转换好字节流
                TSendMsg send_msg;
                send_msg.head.len = sizeof(DWord)+AData.Length();
                send_msg.head.msg_type = AType;
                send_msg.head.serial_num = tmp_serial_num;
                send_msg.id = AId;
                send_msg.data = AData;
                SendToBuffer(send_msg, (Byte*)msg);

                // 如果是私聊 控制超时
                if (AType == kTransPrivateChat)
                {
                    TPrivateCtl* tmp_ctl = new TPrivateCtl;
                    if (tmp_ctl != NULL)
                    {
                        tmp_ctl->id = AId;
                        tmp_ctl->msg_serial_num = ASeriNum;
                        tmp_ctl->time = time(NULL);

                        // 添加失败则不发送
                        lock_map();
                        if (private_ctl_map_->Add(tmp_serial_num, tmp_ctl) == NULL)
                        {
                            is_send = false;
                            delete tmp_ctl;
                        }
                        else if (!is_set_timeout_) // 添加成功则尝试开启定时器
                        {
                            timer_->AddEvent(timer_id_, 20000);
                            is_set_timeout_ = true;
                        }

                        unlock_map();
                    }
                    else
                        is_send = false;
                }

            }
            else
                is_send = false;

            if (is_send)
            {
                send_len = conn_obj_->Send(msg, msg_len);
                if (send_len == msg_len)
                    result = true;
                else if (AType == kTransPrivateChat)// 需要发送&&发送失败&&是私聊消息 则需要剔除超时控制
                {
                    lock_map();
                    private_ctl_map_->Delete(tmp_serial_num);
                    unlock_map();
                }
            }
            dec_ref_count();
        }

        return result;
    }

    // 回复私聊
    bool TLink::send_reply_msg(DWord ASeriNum, char AReplyType)
    {
        bool result = false;
        if (inc_ref_count())
        {
            result = do_reply_private(ASeriNum, AReplyType);
            dec_ref_count();
        }

        return result;
    }

    // 回复私聊 (私有)
    bool TLink::do_reply_private(DWord ASeriNum, char AReplyType)
    {
        bool result = false;
        int msg_len = kHeadLen + sizeof(char);
        KYString msg_str(msg_len);

        if (msg_str.Length() == msg_len)
        {
            // 组装消息
            char* re_msg = (char*)msg_str;
            TReplyMsg reply_msg;
            reply_msg.reply_type = AReplyType;
            reply_msg.head.len = sizeof(char);
            reply_msg.head.msg_type = kPrivateChatReply;
            reply_msg.head.serial_num = ASeriNum;

            ReplyMsgToBuffer(reply_msg, (Byte*)re_msg);

            // 发送消息
            result = (msg_len == conn_obj_->Send(re_msg, msg_len));
        }
        
        return result;
    }

    // 处理登录请求
    bool TLink::deal_login(const TLoginMsg& AMsg)
    {
        bool result = false;
        char reply = ((TObject*)on_login.Object->*on_login.Method)(this, AMsg.id);
        if (reply == TU_trSuccess)
            id_ = AMsg.id;
        
        int msg_len = kHeadLen + sizeof(char);
        KYString msg_str(msg_len);

        if (msg_str.Length() == msg_len)
        {
            char* re_msg = (char*)msg_str;

            // 组装消息
            TReplyMsg reply_msg;
            reply_msg.reply_type = reply;
            reply_msg.head.len = sizeof(char);
            reply_msg.head.msg_type = kLoginReply;
            reply_msg.head.serial_num = AMsg.head.serial_num;

            ReplyMsgToBuffer(reply_msg, (Byte*)re_msg);
           
            // 发送消息
            result = (msg_len == conn_obj_->Send(re_msg, msg_len));
        }

        return result;
    }

    // 处理私聊请求
    bool TLink::deal_private(const TSendMsg& AMsg)
    {
        bool result = true;
        int res = ((TObject*)on_private.Object->*on_private.Method)(this, AMsg.id, AMsg.head.serial_num, AMsg.data);

        if (res <= 0) // 失败则直接回复
            result = do_reply_private(AMsg.head.serial_num, res);

        return result;
    }

    // 处理广播请求
    bool TLink::deal_broadcast(const TBroadcastdMsg& AMsg)
    {
        bool result = false;
        int count = ((TObject*)on_broadcast.Object->*on_broadcast.Method)(this, AMsg.data);

        int msg_len = kHeadLen + sizeof(char)+sizeof(DWord);
        KYString msg_str(msg_len);
        
        if (msg_str.Length() == msg_len)
        {
            char* re_msg = (char*)msg_str;

            // 组装消息
            TReplyBroadcast reply_msg;
            reply_msg.head.len = sizeof(char)+sizeof(DWord);
            reply_msg.head.msg_type = kBroadCaseReply;
            reply_msg.head.serial_num = AMsg.head.serial_num;
            
            if (count > 0)
            {
                reply_msg.reply_type = TU_trSuccess;
                reply_msg.count = count;
            }
            else
            {
                reply_msg.reply_type = count;
                reply_msg.count = 0;
            }
      
            ReplyBroadcastToBuffer(reply_msg, (Byte*)re_msg);

            // 发送消息
            result = (msg_len == conn_obj_->Send(re_msg, msg_len));
        }

        return result;
    }

    // 处理查询请求
    bool TLink::deal_query(const TQueryMsg& AMsg)
    {
        bool result = false;
        TKYList list;
        char res = ((TObject*)on_query.Object->*on_query.Method)(this, AMsg.begin, AMsg.end, list);

        DWord count = list.Count();// 查询到的用户个数
        int data_len = count * sizeof(DWord);// 用户数组数据长度
        int msg_len = kHeadLen + sizeof(char)+sizeof(DWord)+data_len; // 消息长度

        KYString msg_str(msg_len);
        KYString data_str(data_len);
        
        if (msg_str.Length() == msg_len && data_str.Length() == data_len)
        {
            TReplyQuery reply_msg;
            reply_msg.head.len = sizeof(char)+sizeof(DWord)+data_len;
            reply_msg.head.msg_type = kQueryReply;
            reply_msg.head.serial_num = AMsg.head.serial_num;
            reply_msg.data = (DWord*)(char*)data_str;
            char* re_msg = (char*)msg_str;

            reply_msg.reply_type = res;
            reply_msg.count = count;
        
            for (int i = 0; i < count; i++)
                reply_msg.data[i] = (DWord)list.Item(i);

            ReplyQueryToBuffer(reply_msg, (Byte*)re_msg);
            result = (msg_len == conn_obj_->Send(re_msg, msg_len));
        }
        
        return result;
    }

    // 处理转发私聊回复
    bool TLink::deal_reply_trans_private(const TReplyMsg& AMsg)
    {
        bool result = false;
        bool flag = false;
        TPrivateCtl tmp_ctl;

        lock_map();
        // 找到则正常回复 找不到说明已经超时删除
        void* tmp_node = private_ctl_map_->Find(AMsg.head.serial_num);
        if (tmp_node != NULL)
        {
            TPrivateCtl* tmp_private_ctl = private_ctl_map_->Value(tmp_node);
            if (tmp_private_ctl != NULL)
            {
                tmp_ctl = *tmp_private_ctl;
                if (difftime(tmp_private_ctl->time, time(NULL)) < 20)  
                    flag = true;
            }
            private_ctl_map_->Remove(tmp_node);
        }
        unlock_map();

        if (flag)// 回复成功
            result = ((TObject*)on_reply.Object->*on_reply.Method)(this, tmp_ctl.id, tmp_ctl.msg_serial_num, AMsg.reply_type);

        return result;
    }

}
