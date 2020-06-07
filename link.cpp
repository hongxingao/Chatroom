#include "link.h"

namespace _ghx
{
    // ���캯��
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

    // ��������
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

    // ��ʼ��
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
        // ��ʼ�������¼�
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

    // ִ�п���
    bool TLink::do_open()
    { 
        bool result = false;
    
        if (on_login.Method != NULL && on_login_out.Method != NULL && on_private.Method != NULL
            && on_reply.Method != NULL && on_broadcast.Method != NULL && on_query.Method != NULL
            && timer_ != NULL && conn_obj_ != NULL && conn_obj_->Open() == TU_trSuccess)
        {
            // �����߳�
            rec_thread_ = new TKYThread((TKYThread::TDoExecute)&TLink::deal_rec, this, NULL, false);

            if (rec_thread_ != NULL)
                result = true;
            else
                conn_obj_->Close();
        }

        return result;
    }

    // ִ�йر�
    void TLink::do_close(bool ANeedClose, bool ANeedEvent)
    {
        rec_thread_->Terminate();
        notify_->Set();

        if (ANeedClose)
            conn_obj_->Close();
       
        // �ȴ����ü���Ϊ0
        while (ref_count_ > 0)
            Sleep(10);

        if (ANeedEvent)
            ((TObject*)on_login_out.Object->*on_login_out.Method)(this);
        FreeKYThread(rec_thread_);
    }

    // ��������
    bool TLink::open()
    {
        // ��ʼ��
        bool result = false;
        bool next = false;
        Longword curr_thread_id = GetCurrentThreadId();

        // ����״̬
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

    // �ر�����
    void TLink::close(bool ANeedWait)
    {
        // ���״̬
        if (state_ == kClosed)
            return;

        // ��ʼ��
        bool     boolNext = false;
        bool     boolWait = false;
        Longword curr_thread_id = GetCurrentThreadId();

        // ����״̬
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

        // �ж��Ƿ����
        if (boolNext)
        {
            // ִ�йر�
            do_close(true, false);
            state_ = kClosed;
        }
        else if (boolWait)   // �ȴ� Close ����
            while (state_ == kClosing)
                Sleep(10);

    }

    // ���ö�ʱ��
    bool TLink::set_timer(TKYTimer* ATimer)
    {
        // ��ʼ��
        bool        result = false;
        long        intID = 0;
        TKYTimer*   objTimer = NULL;

        // ����
        lock();
        if (state_ != kClosed)
            ;
        else if (timer_ == ATimer)
            result = true;
        else
        {
            // ����
            objTimer = timer_;
            intID = timer_id_;

            // �ж��Ƿ�Ϊ��
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

        // �ж��Ƿ�ɹ�
        if (result && (objTimer != NULL))
            objTimer->Delete(intID);

        // ���ؽ��
        return result;
    }

    // �б�ɾ���¼�
    void TLink::do_dele_list(void* AItem)
    {
        ((TObject*)on_reply.Object->*on_reply.Method)(this, ((TPrivateCtl*)AItem)->id, ((TPrivateCtl*)AItem)->msg_serial_num, TU_trTimeout);
        delete (TPrivateCtl*)AItem;
    }

    // ��ʱ���¼����������Ƴ�ʱ��
    void TLink::do_timer(void* Sender, void* AParam, long ATimerID)
    {
        // ��ʼ����ʱlist
        TKYList tmp_list(false);
        tmp_list.OnDeletion.Method = (TKYList::TDoDeletion)&TLink::do_dele_list;
        tmp_list.OnDeletion.Object = this;
    
        // ��map��
        lock_map();
        void* tmp_node = private_ctl_map_->First();

        // ������鳬ʱ
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

        // ÿ��������һ�γ�ʱ
        if (timer_ != NULL)
            timer_->AddEvent(ATimerID, 3000);
        
    }

    // ���ü�����һ
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

    // ���õ�¼�¼�
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

    // ���õǳ��¼�
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

    // ����˽���¼�
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

    // ���ù㲥�¼�
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

    // ���ûظ��¼�
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

    // ���ò�ѯ�¼�
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

    // ��������
    void TLink::do_recv(void* AData, long ASize)
    {
        rec_buffer_->Push(AData, ASize);
        notify_->Set();
    }

    // ������յ��Ϸ�����Ϣ
    bool TLink::deal_msg(const TDataHead& AHead, Byte* AMsg)
    {
        bool result = false;
        if (AHead.msg_type == kLogin)
        {
            TLoginMsg msg;
            result = BufferToLogin(AHead, AMsg, msg) && deal_login(msg);
        }
        else if (id_ > 0) //�ѵ�¼
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

    // ���������������
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
                if (bool_head) // �Ѿ��õ���Ϣͷ
                {
                    if (rec_buffer_->Size() >= rec_head.len)
                    {
                        // ��Ϣ����� ����
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

                        // ȡ��Ϣ��
                        if (rec_buffer_->Pop(rec_msg, rec_head.len) != rec_head.len
                            || !deal_msg(rec_head, (Byte*)rec_msg ))
                        {
                            need_close = true;
                            break;
                        }
                        // �������һ����Ϣ ��bool_head����Ϊfalse
                        bool_head = false;
                    }
                    else
                        notify_->Wait(1000);
                }
                else if (rec_buffer_->Size() >= kHeadLen) // δ�õ���Ϣͷ
                {
                    int rec_size = rec_buffer_->Pop(rec_msg, kHeadLen);
                    if (rec_size != kHeadLen || !BufferToHead((Byte*)rec_msg, rec_head))
                    {
                        need_close = true;
                        break;
                    }
                    // bool_head��Ϊfalse
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

    // �Ͽ������¼�����
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

    //  ת����Ϣ
    bool TLink::send_trans_msg(TMsgTypeTag AType, DWord ASeriNum, DWord AId, const KYString& AData)
    {
        bool result = false;
        if (AData.Length() > 0 && inc_ref_count())
        {
            bool is_send = true; // �Ƿ���Ҫ����
            DWord msg_len = kHeadLen + sizeof(DWord)+AData.Length(); //Ӧ�÷��͵ĳߴ�
            DWord send_len;  // ʵ�ʷ��͵ĳߴ�

            KYString msg_str((int)msg_len);
            char* msg = (char*)msg_str;

            DWord tmp_serial_num = InterlockedIncrement(&link_serial_num_); // ���ӵ���Ϣ���к�

            if (msg_str.Length() == msg_len)
            {
                // ת�����ֽ���
                TSendMsg send_msg;
                send_msg.head.len = sizeof(DWord)+AData.Length();
                send_msg.head.msg_type = AType;
                send_msg.head.serial_num = tmp_serial_num;
                send_msg.id = AId;
                send_msg.data = AData;
                SendToBuffer(send_msg, (Byte*)msg);

                // �����˽�� ���Ƴ�ʱ
                if (AType == kTransPrivateChat)
                {
                    TPrivateCtl* tmp_ctl = new TPrivateCtl;
                    if (tmp_ctl != NULL)
                    {
                        tmp_ctl->id = AId;
                        tmp_ctl->msg_serial_num = ASeriNum;
                        tmp_ctl->time = time(NULL);

                        // ���ʧ���򲻷���
                        lock_map();
                        if (private_ctl_map_->Add(tmp_serial_num, tmp_ctl) == NULL)
                        {
                            is_send = false;
                            delete tmp_ctl;
                        }
                        else if (!is_set_timeout_) // ��ӳɹ����Կ�����ʱ��
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
                else if (AType == kTransPrivateChat)// ��Ҫ����&&����ʧ��&&��˽����Ϣ ����Ҫ�޳���ʱ����
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

    // �ظ�˽��
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

    // �ظ�˽�� (˽��)
    bool TLink::do_reply_private(DWord ASeriNum, char AReplyType)
    {
        bool result = false;
        int msg_len = kHeadLen + sizeof(char);
        KYString msg_str(msg_len);

        if (msg_str.Length() == msg_len)
        {
            // ��װ��Ϣ
            char* re_msg = (char*)msg_str;
            TReplyMsg reply_msg;
            reply_msg.reply_type = AReplyType;
            reply_msg.head.len = sizeof(char);
            reply_msg.head.msg_type = kPrivateChatReply;
            reply_msg.head.serial_num = ASeriNum;

            ReplyMsgToBuffer(reply_msg, (Byte*)re_msg);

            // ������Ϣ
            result = (msg_len == conn_obj_->Send(re_msg, msg_len));
        }
        
        return result;
    }

    // �����¼����
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

            // ��װ��Ϣ
            TReplyMsg reply_msg;
            reply_msg.reply_type = reply;
            reply_msg.head.len = sizeof(char);
            reply_msg.head.msg_type = kLoginReply;
            reply_msg.head.serial_num = AMsg.head.serial_num;

            ReplyMsgToBuffer(reply_msg, (Byte*)re_msg);
           
            // ������Ϣ
            result = (msg_len == conn_obj_->Send(re_msg, msg_len));
        }

        return result;
    }

    // ����˽������
    bool TLink::deal_private(const TSendMsg& AMsg)
    {
        bool result = true;
        int res = ((TObject*)on_private.Object->*on_private.Method)(this, AMsg.id, AMsg.head.serial_num, AMsg.data);

        if (res <= 0) // ʧ����ֱ�ӻظ�
            result = do_reply_private(AMsg.head.serial_num, res);

        return result;
    }

    // ����㲥����
    bool TLink::deal_broadcast(const TBroadcastdMsg& AMsg)
    {
        bool result = false;
        int count = ((TObject*)on_broadcast.Object->*on_broadcast.Method)(this, AMsg.data);

        int msg_len = kHeadLen + sizeof(char)+sizeof(DWord);
        KYString msg_str(msg_len);
        
        if (msg_str.Length() == msg_len)
        {
            char* re_msg = (char*)msg_str;

            // ��װ��Ϣ
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

            // ������Ϣ
            result = (msg_len == conn_obj_->Send(re_msg, msg_len));
        }

        return result;
    }

    // �����ѯ����
    bool TLink::deal_query(const TQueryMsg& AMsg)
    {
        bool result = false;
        TKYList list;
        char res = ((TObject*)on_query.Object->*on_query.Method)(this, AMsg.begin, AMsg.end, list);

        DWord count = list.Count();// ��ѯ�����û�����
        int data_len = count * sizeof(DWord);// �û��������ݳ���
        int msg_len = kHeadLen + sizeof(char)+sizeof(DWord)+data_len; // ��Ϣ����

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

    // ����ת��˽�Ļظ�
    bool TLink::deal_reply_trans_private(const TReplyMsg& AMsg)
    {
        bool result = false;
        bool flag = false;
        TPrivateCtl tmp_ctl;

        lock_map();
        // �ҵ��������ظ� �Ҳ���˵���Ѿ���ʱɾ��
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

        if (flag)// �ظ��ɹ�
            result = ((TObject*)on_reply.Object->*on_reply.Method)(this, tmp_ctl.id, tmp_ctl.msg_serial_num, AMsg.reply_type);

        return result;
    }

}
