#include "CycBuffer.h"

namespace KYLib
{
    // 构造函数
    TCycBuffer::TCycBuffer(Word AMaxCache, Byte ASizeType)
    {
        size_type_ = ASizeType > stSize2_16 ? stSize2_16 : ASizeType;
        block_size_ = 1 << (size_type_ + 8);
        block_mask_ = block_size_ - 1;
        push_size_ = 0;
        pop_size_ = 0;
        max_size_ = 0xFFFFFFFF;
        cache_ = new TKYCache(block_size_ + sizeof(TIndex), AMaxCache);
        head_ = tail_ = (TIndex*)cache_->New();
        if (head_ != NULL)
        {
            head_->next = NULL;
            head_->PushSize = head_->PopSize = 0;
        }
    }

    //析构函数
    TCycBuffer::~TCycBuffer()
    {
        TIndex* p_tmp1;
        TIndex* p_tmp2 = head_;
        head_ = tail_ = NULL;
        cache_->SetMaxCount(0);
        while (p_tmp2 != NULL)
        {
            p_tmp1 = p_tmp2;
            p_tmp2 = (TIndex*)p_tmp2->next;
            cache_->Delete(p_tmp1);
        }
        delete cache_;
    }

    //压入数据到一个数据块中
    Longword TCycBuffer::DoPush(TIndex* AIndex, const void* AData, Longword ASize) const
    {
        Longword result = 0;
        Longword used_size = AIndex->PushSize - AIndex->PopSize;
        if (used_size < block_size_)
        {
            Longword res_size = block_size_ - used_size;
            if (ASize > res_size)
                ASize = res_size;
            Longword right_pos = AIndex->PushSize & block_mask_;
            Longword right_size = block_size_ - right_pos;
            char* tmp_arr = (char*)AIndex + sizeof(TIndex);
            if (ASize > right_size)
            {
                Longword letf_need = ASize - right_size;
                memcpy(tmp_arr + right_pos, AData, right_size);
                memcpy(tmp_arr, (char*)AData + right_size, letf_need);
            }
            else        
                memcpy(tmp_arr + right_pos, AData, ASize);
            AIndex->PushSize += ASize;
            result = ASize;
        }

        return result;
    }

    //压入数据
    Longword TCycBuffer::Push(const void* AData, Longword ASize)
    {
        Longword result = 0;
        Longword cur_max_size = max_size_;
        Longword used_size = push_size_ - pop_size_;
        if (AData == NULL || ASize == 0 || used_size >= cur_max_size || tail_ == NULL)
            ;
        else
        {
            cur_max_size -= used_size;
            if (ASize > cur_max_size)
                ASize = cur_max_size;
            char* data_ptr = (char*)AData;
            for (; ;)
            {
                Longword ret = DoPush(tail_, data_ptr, ASize);
                ASize -= ret;
                data_ptr += ret;
                if (ASize > 0)
                {
                    TIndex* tmp = (TIndex*)cache_->New();
                    if (tmp == NULL)
                        break;
                    tmp->next = NULL;
                    tmp->PushSize = tmp->PopSize = 0;
                    tail_->next = tmp;
                    tail_ = tmp;
                }
                else
                    break;
            } 
            result = data_ptr - (char*)AData;
            push_size_ += result;
        }

        return result;
    }

    //压入数据到一个数据块中
    Longword TCycBuffer::DoPush(TIndex* AIndex, TDoRead ADoRead, void* AObject, Longword ASize) const
    {
        Longword result = 0;
        Longword right_pos = AIndex->PushSize & block_mask_;
        Longword right_size = block_size_ - right_pos;
        char* tmp_arr = (char*)AIndex + sizeof(TIndex);

        if (ASize > right_size)
        {
            Longword letf_need = ASize - right_size;
            long ret = ((TObject*)AObject->*ADoRead)(tmp_arr + right_pos, right_size);
            if (ret == right_size)
            {
                result = ret;
                ret = ((TObject*)AObject->*ADoRead)(tmp_arr, letf_need);
                if (ret > 0)
                    result += ret;
            }
            else if (ret > 0)
                result = ret;
                   
        }
        else
        {
            long ret = ((TObject*)AObject->*ADoRead)(tmp_arr + right_pos, ASize);
            if (ret > 0)       
                result = ret;
        }
        AIndex->PushSize += result;
   
        return result;
    }
    
    //压入数据
    Longword TCycBuffer::Push(TDoRead ADoRead, void* AObject, Longword ASize)
    {
        Longword result = 0;
        Longword cur_max_size = max_size_;
        Longword used_size = push_size_ - pop_size_;

        if (ADoRead == NULL ||  ASize == 0 || used_size >= cur_max_size || tail_ == NULL)
            ;
        else
        {
            cur_max_size -= used_size;
            if (ASize > cur_max_size)
                ASize = cur_max_size;
            for (; ;)
            {
                Longword block_res_size = block_size_ - tail_->PushSize + tail_->PopSize;
                if (block_res_size > ASize)
                    block_res_size = ASize;
                Longword ret = DoPush(tail_, ADoRead, AObject, block_res_size);
                result += ret;
                //压入成功
                if (ret == block_res_size)
                    ASize -= ret;
                else  //压入出错 未压入期望长度
                    break;

                if (ASize > 0)
                {
                    TIndex* tmp = (TIndex*)cache_->New();
                    if (tmp == NULL)
                        break;
                    tmp->next = NULL;
                    tmp->PopSize = tmp->PopSize = 0;
                    tail_->next = tmp;
                    tail_ = tmp;
                }
                else
                    break;
            } 
            push_size_ += result;
        }

        return result;
    }

    //从一个数据块弹出数据
    Longword TCycBuffer::DoPop(TIndex* AIndex, void* AData, Longword ASize) const
    {
        Longword result = 0;
        Longword block_used_size = AIndex->PushSize - AIndex->PopSize;
        if (block_used_size > 0)
        {
            if (ASize > block_used_size)
                ASize = block_used_size;
            Longword pop_pos = AIndex->PopSize & block_mask_;
            Longword right_pop_size = block_size_ - pop_pos;
            char* tmp_arr = (char*)AIndex + sizeof(TIndex);
            if (right_pop_size >= ASize)
                memcpy(AData, tmp_arr + pop_pos, ASize);
            else
            {
                Longword left_pop_size = ASize - right_pop_size;
                memcpy(AData, tmp_arr + pop_pos, right_pop_size);
                memcpy((char*)AData + right_pop_size, tmp_arr, left_pop_size);
            }
            AIndex->PopSize += ASize;
            result = ASize;
        }

        return result;
    }

    //弹出数据
    Longword TCycBuffer::Pop(void* AData, Longword ASize)
    {
        Longword result = 0;
        Longword used_size = push_size_ - pop_size_;
        if (AData == NULL || head_ == NULL || ASize == 0 || used_size == 0 )
            ;
        else 
        {
            if (ASize > used_size)
                ASize = used_size;
            char* data_ptr = (char*)AData;
            for (; ;)
            {
                Longword ret = DoPop(head_, data_ptr, ASize);
                ASize -= ret;
                data_ptr += ret;
                if (ASize == 0 || head_->next == NULL)
                    break;
                
                TIndex* tmp = head_;
                head_ = (TIndex*)head_->next;
                cache_->Delete(tmp);
            } 
            result = data_ptr - (char*)AData;
            pop_size_ += result;
        }

        return result;
    }

    //从一个数据块弹出数据
    Longword TCycBuffer::DoPop(TIndex* AIndex, TDoWrite ADoWrite, void* AObject, Longword ASize) const
    {
        Longword result = 0;
        Longword pop_pos = AIndex->PopSize & block_mask_;
        Longword right_pop_size = block_size_ - pop_pos;
        char* tmp_arr = (char*)AIndex + sizeof(TIndex);

        if (right_pop_size >= ASize)
        {
            long ret = ((TObject*)AObject->*ADoWrite)(tmp_arr + pop_pos, ASize);
            if (ret > 0)
                result = ret;
        }
        else
        {
            Longword left_pop_size = ASize - right_pop_size;
            long ret = ((TObject*)AObject->*ADoWrite)(tmp_arr + pop_pos, right_pop_size);
            if (ret == right_pop_size)
            {
                result  = ret;
                ret = ((TObject*)AObject->*ADoWrite)(tmp_arr, left_pop_size);
                if (ret >0)
                    result += ret;
            }
            else if (ret > 0)
                result = ret;
        }
        AIndex->PopSize += result;
        
        return result;
    }

    //弹出数据
    Longword TCycBuffer::Pop(TDoWrite ADoWrite, void* AObject, Longword ASize)
    {
        Longword result = 0;
        Longword used_size = push_size_ - pop_size_;
        if (ADoWrite == NULL || head_ == NULL || ASize == 0 || used_size == 0)
            ;
        else
        {
            if (ASize > used_size)
                ASize = used_size;
            for (; ;)
            {
                Longword block_used_size = head_->PushSize  -head_->PopSize;
                if (block_used_size > ASize)
                    block_used_size = ASize;

                Longword ret = DoPop(head_, ADoWrite, AObject, block_used_size);
                result += ret;
                if (ret == block_used_size) //弹出成功              
                    ASize -= ret;
                else //弹出错误 未弹出期望长度               
                    break;
                
                if (ASize == 0 || head_->next == NULL)
                    break;
                TIndex* tmp = head_;
                head_ = (TIndex*)head_->next;
                cache_->Delete(tmp);
            } 
            pop_size_ += result;
        }

        return result;
    }

    //从一个数据块预读取数据
    Longword TCycBuffer::DoPeek(TIndex* AIndex, void* AData, Longword ASize) const
    {
        Longword result = 0;
        Longword block_used_size = AIndex->PushSize - AIndex->PopSize;
        if (block_used_size > 0)
        {
            if (ASize > block_used_size)
                ASize = block_used_size;
            Longword pop_pos = AIndex->PopSize & block_mask_;
            Longword right_pop_size = block_size_ - pop_pos;
            char* tmp_arr = (char*)AIndex + sizeof(TIndex);
            if (right_pop_size >= ASize)
                memcpy(AData, tmp_arr + pop_pos, ASize);
            else
            {
                Longword left_pop_size = ASize - right_pop_size;
                memcpy(AData, tmp_arr + pop_pos, right_pop_size);
                memcpy((char*)AData + right_pop_size, tmp_arr, left_pop_size);
            }
            result = ASize;
        }

        return result;
    }

    //预读取数据
    Longword TCycBuffer::Peek(void* AData, Longword ASize) const
    {
        Longword result = 0;
        Longword used_size = push_size_ - pop_size_;
        if (AData == NULL || head_ == NULL || ASize == 0 || used_size == 0)
            ;
        else
        {
            if (ASize > used_size)
                ASize = used_size;
            char* data_ptr = (char*)AData;
            TIndex* tmp = head_;
            for (; ;)
            {
                Longword ret = DoPeek(tmp, data_ptr, ASize);
                ASize -= ret;
                data_ptr += ret;
                if (ASize == 0 || tmp->next == NULL)
                    break;
                tmp = (TIndex*)tmp->next;
            }
            result = data_ptr - (char*)AData;
        }

        return result;
    }

    //从一个数据块预读取数据
    Longword TCycBuffer::DoPeek(TIndex* AIndex, TDoWrite ADoWrite, void* AObject, Longword ASize) const
    {
        Longword result = 0;
        Longword pop_pos = AIndex->PopSize & block_mask_;
        Longword right_pop_size = block_size_ - pop_pos;
        char* tmp_arr = (char*)AIndex + sizeof(TIndex);

        if (right_pop_size >= ASize)
        {
            long ret = ((TObject*)AObject->*ADoWrite)(tmp_arr + pop_pos, ASize);
            if (ret > 0)
                result = ret;
        }
        else
        {
            Longword left_pop_size = ASize - right_pop_size;
            long ret = ((TObject*)AObject->*ADoWrite)(tmp_arr + pop_pos, right_pop_size);
            if (ret == right_pop_size)
            {
                result = ret;
                ret = ((TObject*)AObject->*ADoWrite)(tmp_arr, left_pop_size);
                if (ret > 0)
                    result += ret;
            }
            else if (ret > 0)
                result = ret;
        }
        
        return result;
    }

    Longword TCycBuffer::Peek(TDoWrite ADoWrite, void* AObject, Longword ASize) const
    {
        Longword result = 0;
        Longword used_size = push_size_ - pop_size_;
        if (ADoWrite == NULL || head_ == NULL || ASize == 0 || used_size == 0)
            ;
        else
        {
            if (ASize > used_size)
                ASize = used_size;
            TIndex* tmp = head_;
            for (; ;)
            {
                Longword block_uesd_size = tmp->PushSize - tmp->PopSize;
                if (block_uesd_size > ASize)
                    block_uesd_size = ASize;
                Longword ret = DoPeek(tmp, ADoWrite, AObject, block_uesd_size);
                result += ret;
                if (ret == block_uesd_size)
                    ASize -= ret;
                else      
                    result += ret;
 
                if (ASize == 0 || tmp->next == NULL)
                    break;
                TIndex* tmp = (TIndex*)tmp->next;
            } 
        }

        return result;
    }

    //丢失指定尺寸的数据
    Longword TCycBuffer::Lose(Longword ASize)
    {
        Longword result = 0;
        Longword used_size = push_size_ - pop_size_;
        if (head_ == NULL || ASize == 0 || used_size == 0)
            ;
        else
        {
            if (ASize > used_size)
                ASize = used_size;
            for (; ;)
            {
                Longword block_used_size = head_->PushSize - head_->PopSize;
                if (ASize <= block_used_size)
                {
                    head_->PopSize += ASize;
                    result += ASize;
                    ASize = 0;
                }
                else
                {
                    head_->PopSize += block_used_size;
                    ASize -= block_used_size;
                    result += block_used_size;
                }
                if (ASize == 0 || head_->next == NULL)
                    break;
                
                TIndex* tmp = head_;
                head_ = (TIndex*)head_->next;
                cache_->Delete(tmp);
            }
            pop_size_ += result;
        }

        return result;
    }

}