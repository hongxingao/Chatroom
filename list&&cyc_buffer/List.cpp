#include "List.h"

namespace KYLib
{
    // 构造函数
    TList::TList()
    {
        FCapacity = 0;
        FCount = 0;
        FDelta = 0;
        FSorted = false;
        FHasSorted = false;
        FCanDuplicate = true;
        FItems = NULL;
        OnDeletion.Method = NULL;
        OnDeletion.Object = NULL;
        OnCompare.Method = NULL;
        OnCompare.Object = NULL;
    }

    // 析构函数
    TList::~TList()
    {
        Clear();
    }

    // 根据索引查找某一项
    void* TList::Item(long AIndex) const
    {
        void* result = NULL;
        if (AIndex >= 0 && AIndex < FCount)
            result = FItems[AIndex];

        return result;
    }

    // 根据索引设置某一项
    bool TList::SetItem(long AIndex, void* AItem)
    {
        bool result = true;
        if (FCount > 0 && AIndex >= 0 && AIndex < FCount)
        {
            if (FSorted)
            {
                if (Compare(FItems[AIndex], AItem) == 0)
                    FItems[AIndex] = AItem;
                else
                {
                    long des_index;
                    FindNearestItem(0, FCount - 1, AItem, des_index);
                    FItems[AIndex] = AItem;

                    // 调整改变的项， 使重新有序
                    if (AIndex < des_index)
                        des_index--;
                    if (AIndex != des_index)
                        SimpleMove(AIndex, des_index);
                }
                
            }
            else
            {
                FItems[AIndex] = AItem;
                FHasSorted = false;
            }

        }
        else
            result = false;

        return result;
    }

    // 在AFrom索引处 向后ACount个 激发 OnDeletion 事件
    void TList::OnDeleteItems(Pointer* Aarr, long AFrom, long ACount)
    {
        TOnDeletion tmp_ondeleteion = OnDeletion;
        if (tmp_ondeleteion.Method != NULL && Aarr != NULL && ACount > 0)
        {
            if (AFrom < 0)
                AFrom = 0;

            Pointer* end = Aarr + ACount;
            for (; Aarr < end; Aarr++)
                ((TObject*)tmp_ondeleteion.Object->*tmp_ondeleteion.Method)(*Aarr);
        }

    }

    // 更改容量, 若 Count() 大于更改的容量值则自动释放项, 并激发 OnDeletion 事件
    void TList::ChangeCapacity(long ACapacity)
    {
        if (ACapacity == FCapacity)
            ;
        else if (ACapacity <= 0)
        {
            if (FItems != NULL)
            {
                // 先清除数据 再激发 OnDeletion 事件
                long tmp_count = FCount;
                Pointer* tmp = FItems;

                FCount = 0;
                FCapacity = 0;
                FItems = NULL;

                OnDeleteItems(tmp, 0, tmp_count);
                free(tmp);
            }
        }
        else
        {
            Pointer* tmp = (Pointer*)malloc(ACapacity * sizeof(Pointer));
            if (tmp != NULL)
            {
                Pointer* old = FItems;
                long tmp_count = FCount;
                FCapacity = ACapacity;

                if (FCount > 0)
                {
                    // 项数大于分配的容量
                    if (FCount > ACapacity)
                        FCount = ACapacity;
                    memcpy(tmp, FItems, FCount * sizeof(Pointer));
                }

                FItems = tmp;

                // 原列表不为空 一定需要释放
                if (old != NULL)
                {
                    //  激发 OnDeletion 事件
                    long del_count = tmp_count - ACapacity;
                    if (del_count > 0)
                        OnDeleteItems(old, ACapacity, del_count);

                    free(old);
                }
            }
        }

    }

    // 扩容，返回容量大小
    int TList::AddCapacity() const
    {
        int intNew = FCapacity;
        if (FCapacity <= 16)
            intNew += 16;
        else if (FCapacity <= 64)
            intNew += FCapacity;
        else if (FCapacity <= 256)
            intNew += FCapacity >> 1;
        else
            intNew += FCapacity >> 2;

        return intNew;
    }

    // 二分插入 
    void TList::FindNearestItem(long AFrom, long ATo, const void* AItem, long& ANearest) const
    {
        if (AFrom < 0)
            AFrom = 0;
        else if (AFrom > FCount)
            AFrom = FCount;

        if (ATo < 0)
            ATo = 0;
        else if (ATo >= FCount)
            ATo = FCount - 1;

        int mid;
        // 若比较方法为空 则只能进行指针比较
        TOnCompare tmp_compare = OnCompare;
        if (tmp_compare.Method == NULL)
            while (AFrom <= ATo)
            {
                mid = AFrom + ((ATo - AFrom) >> 1);
                if (AItem < FItems[mid])
                    ATo = mid - 1;
                else
                    AFrom = mid + 1;

            }
        else
            while (AFrom <= ATo)
            {
                mid = AFrom + ((ATo - AFrom) >> 1);
                if (Compare(tmp_compare, AItem, FItems[mid]) < 0)
                    ATo = mid - 1;
                else
                    AFrom = mid + 1;

            }

        ANearest = AFrom;
    }

    // 添加项
    // 1. 若 Sorted 属性为 false 则添加到末尾项, 否则根据值大小插入到指定位置
    // 2. 若返回值 >= 0, 则添加项的索引, 否则返回添加失败的错误码
    long TList::Add(void* AItem)
    {
        long result = 0;

        if (FCount == FCapacity) //扩容
            ChangeCapacity(AddCapacity());

        if (FCount == FCapacity) // 有可能扩容失败
            result = -1;
        else if (!FSorted) // 不是排序列表
        {
            FItems[FCount] = AItem;
            result = FCount++;
            FHasSorted = false;
        }
        else
        {
            // 二分插入
            FindNearestItem(0, FCount - 1, AItem, result);

            if (result != FCount)
                memmove(FItems + result + 1, FItems + result, (FCount - result) * sizeof(Pointer));
            FItems[result] = AItem;
            FCount++;

        }

        return result;
    }

    // 插入指定索引项
    // 1. 若 Sorted 属性为 true 则不允许调用 Insert 方法, 必须调用 Add 方法添加项
    // 2. 若 AIndex <= 0 则插入第一项, 若 AIndex >= Count() 则插入末尾项
    bool TList::Insert(long AIndex, void* AItem)
    {
        bool result = false;
        if (!FSorted)
        {
            if (FCount == FCapacity) //扩容
                ChangeCapacity(AddCapacity());

            if (FCount != FCapacity)
            {
                if (AIndex >= FCount)
                    FItems[FCount] = AItem;
                else
                {
                    if (AIndex < 0)
                        AIndex = 0;
                    // 插入的下标之后的元素全部后移
                    memmove(FItems + AIndex + 1, FItems + AIndex, (FCount - AIndex) * sizeof(Pointer));
                    FItems[AIndex] = AItem;
                }
                FCount++;
                FHasSorted = false;
                result = true;
            }
        }

        return result;
    }

    // 删除指定索引项, 激发 OnDeletion 事件
    void TList::Delete(long AIndex)
    {
        if (AIndex >= 0 && AIndex < FCount)
        {
            // 取到需要删除的项
            void* tmp = FItems[AIndex];

            FCount--;
            if (AIndex != FCount)
                memmove(FItems + AIndex, FItems + AIndex + 1, (FCount - AIndex) * sizeof(Pointer));

            if (OnDeletion.Method != NULL)
                ((TObject*)OnDeletion.Object->*OnDeletion.Method)(tmp);
        }
    }

    // 移动索引 AIndex 项到 ANewIndex 索引, 若 Sorted 属性为 true 则失败
    bool TList::MoveTo(long AIndex, long ANewIndex)
    {
        bool result = false;
        if (!FSorted && AIndex != ANewIndex && AIndex >= 0 && AIndex < FCount && ANewIndex >= 0 && ANewIndex < FCount)
        {
            SimpleMove(AIndex, ANewIndex);
            FHasSorted = false;
            result = true;
        }

        return result;
    }

    // 用在Sorted属性为true时，调整那个改变的项，使重新有序 (内部调用)
    void TList::SimpleMove(long AIndex, long ANewIndex)
    {
        Pointer tmp = FItems[AIndex];

        // [ANewIndex, AIndex) 或 (AIndex, ANewIndex] 之间的元素，整体全部向右或向左移动一个单位
        if (AIndex > ANewIndex)
            memmove(FItems + ANewIndex + 1, FItems + ANewIndex, (AIndex - ANewIndex) * sizeof(Pointer));
        else
            memmove(FItems + AIndex, FItems + AIndex + 1, (ANewIndex - AIndex) * sizeof(Pointer));

        FItems[ANewIndex] = tmp;
    }

    // 交换索引 AIndex1 和 AIndex2 项, 若 Sorted 属性为 true 则失败
    bool TList::Exchange(long AIndex1, long AIndex2)
    {
        bool result = false;
        if (!FSorted && AIndex1 != AIndex2 && AIndex1 >= 0 && AIndex1 < FCount && AIndex2 >= 0 && AIndex2 < FCount)
        {
            Pointer tmp = FItems[AIndex1];
            FItems[AIndex1] = FItems[AIndex2];
            FItems[AIndex2] = tmp;

            FHasSorted = false;
            result = true;
        }

        return result;
    }

    // 二分查找 如果找到 ANearest即第一个正确索引 若找不到 ANearest即是要插入的位置
    // IsPtr 是否比较指针 为false则 compare
    bool TList::FindItem(long AFrom, long ATo, const void* AItem, long& ANearest) const
    {
        if (AFrom < 0)
            AFrom = 0;
        else if (AFrom > FCount)
            AFrom = FCount;

        if (ATo < 0)
            ATo = 0;
        else if (ATo >= FCount)
            ATo = FCount - 1;

        bool result = false;
        long mid;

        // 若比较方法为空 则只能进行指针比较
        TOnCompare tmp_compare = OnCompare;
        if (tmp_compare.Method == NULL)
        {
            while (AFrom <= ATo)
            {
				// 相加除以二可能会数据类型越界
                mid = AFrom + ((ATo - AFrom) >> 1);
                if (AItem > FItems[mid])
                    AFrom = mid + 1;
                else
                    ATo = mid - 1;
            }

            // 索引合法且值相同 则找到
            if (AFrom < FCount && AItem == FItems[AFrom])
                result = true;
        }
        else
        {
            while (AFrom <= ATo)
            {
                mid = AFrom + ((ATo - AFrom) >> 1);
                if (Compare(tmp_compare, AItem, FItems[mid]) > 0)
                    AFrom = mid + 1;
                else
                    ATo = mid - 1;
            }

            // 索引合法且值相同 则找到
            if (AFrom < FCount && Compare(tmp_compare, AItem, FItems[AFrom]) == 0)
                result = true;
        }
        ANearest = AFrom;

        return result;
    }

    //按顺序查找
    long TList::FindByOrder(long AFrom, long ATo, const void* AItem) const
    {
        if (AFrom < 0)
            AFrom = 0;
        else if (AFrom > FCount)
            AFrom = FCount;

        if (ATo < 0)
            ATo = 0;
        else if (ATo >= FCount)
            ATo = FCount - 1;

        long result = -1;

        // 若比较方法为空 则只能进行指针比较
        TOnCompare tmp_compare = OnCompare;
        if (tmp_compare.Method == NULL)
        {
            for (; AFrom <= ATo; AFrom++)
                if (FItems[AFrom] == AItem)
                {
                    result = AFrom;
                    break;
                }
        }
        else
        {
            for (; AFrom <= ATo; AFrom++)
                if (Compare(tmp_compare, FItems[AFrom], AItem) == 0)
                {
                    result = AFrom;
                    break;
                }
        }

        return result;
    }

    // 根据值返回索引
    long TList::IndexOf(const void* AItem) const
    {
        long result = -1;
        if (FCount <= 0)
            ;
        else if (!FHasSorted)
            result = FindByOrder(0, FCount - 1, AItem);
        else if (!FindItem(0, FCount - 1, AItem, result))
            result = -1;

        return result;
    }

    // 删除<满足条件>的第一项, 激发 OnDeletion 事件
    void TList::Delete(void* AItem)
    {
        if (FCount > 0)
        {
            long index = IndexOf(AItem);
            if (index >= 0 && index < FCount)
                Delete(index);
        }

    }

    // 删除<项指针相同>的第一项, 并返回删除前的索引, 激发 OnDeletion 事件
    long TList::Remove(void* AItem)
    {
        long result = -1;
        if (FCount > 0)
        {
            result = IndexOf(AItem);
            while (FItems[result] != AItem && result < FCount)
            {
                if (Compare(FItems[result++], AItem) != 0)
                {
                    result = -1;
                    break;
                }
            }

            if (result >= 0 && result < FCount)
                Delete(result);
        }

        return result;
    }

    // 清除所有项, 激发 OnDeletion 事件
    void TList::Clear()
    {
        // 设置容量为 0 和清除所有项效果相同
        ChangeCapacity(0);

    }

    // 排序
    void TList::Sort()
    {
        if (!FHasSorted)
        {
            if (FCount > 1)
                QuickSort(0, FCount - 1);

            FHasSorted = true;
        }

    }

    // compare 排序
    void TList::DoSort0(TOnCompare ACompare, long AFrom, long ATo)
    {
        int i, j;
        void* key;
        while (AFrom < ATo)
        {
            i = AFrom;
            j = ATo;
            key = FItems[AFrom];

            do
            {
                while (i < j && Compare(ACompare, FItems[j], key) >= 0)
                    j--;
                if (i != j)
                    FItems[i] = FItems[j];
                while (i < j && Compare(ACompare, FItems[i], key) <= 0)
                    i++;
                if (i != j)
                    FItems[j] = FItems[i];

            } while (i < j);

            if (i != AFrom)
                FItems[i] = key;
            if (AFrom < i - 1)
                DoSort0(ACompare, AFrom, i - 1);

            AFrom = i + 1;
        }
    }

    // 指针排序
    void TList::DoSort1(long AFrom, long ATo)
    {
        int i, j;
        void* key;
        while (AFrom < ATo)
        {
            i = AFrom;
            j = ATo;
            key = FItems[AFrom];

            do
            {
                while (i < j && FItems[j] >= key)
                    j--;
                if (i != j)
                    FItems[i] = FItems[j];
                while (i < j && FItems[i] <= key)
                    i++;
                if (i != j)
                    FItems[j] = FItems[i];

            } while (i < j);

            if (i != AFrom)
                FItems[i] = key;
            if (AFrom < i - 1)
                DoSort1(AFrom, i - 1);

            AFrom = i + 1;
        }
    }

    //快速排序
    void TList::QuickSort(long AFrom, long ATo)
    {
        // 若比较方法为空 则只能进行指针比较
        TOnCompare tmp_compare = OnCompare;

        if (tmp_compare.Method != NULL)
            DoSort0(tmp_compare, AFrom, ATo);
        else
            DoSort1(AFrom, ATo);

    }

    // 比较 AItem1 > AItem2 返回大于0，相等返回0，否则返回小于0 (调用前必须检查参数)
    int TList::Compare(TOnCompare ACompare, const void* AItem1, const void* AItem2) const
    {
        int result = -1;
        ((TObject*)ACompare.Object->*ACompare.Method)(AItem1, AItem2, result);

        return result;
    }

    int TList::Compare(const void* AItem1, const void* AItem2) const
    {
        int result = -1;
        if (OnCompare.Method != NULL)
            ((TObject*)OnCompare.Object->*OnCompare.Method)(AItem1, AItem2, result);
        else if (AItem1 > AItem2)
            result = -1;
        else if (AItem1 == AItem2)
            result = 0;

        return result;
    }

    // 从 AFrom 索引开始搜索值为 AItem 的 Compare(...) == 0 第一项的索引
    long TList::Search(const void* AItem, long AFrom) const
    {
        long result = -1;

        // 使索引合法
        if (AFrom < 0)
            AFrom = 0;
        else if (AFrom >= FCount)
            AFrom = FCount - 1;

        if (FCount <= 0)
            ;
        else if (!FHasSorted)  // 非排序列表
            result = FindByOrder(AFrom, FCount - 1, AItem);
        else if (!FindItem(AFrom, FCount - 1, AItem, result))
            result = -1;

        return result;
    }

    // 从 AFrom 索引开始搜索值为 AItem 的 Compare(...) == 0 第一项的索引
    long TList::Search(const void* AItem, Pointer& ARetItem, long AFrom) const
    {
        long result = Search(AItem, AFrom);

        if (result >= 0)
            ARetItem = FItems[result];

        return result;
    }

    // 查找最近一个索引
    // 1. 若 ANearest == -1 则表示未排序, 查找失败;
    // 2. 若返回值为 true, 则表示找到项索引, 否则项值在 ANearest 索引之前
    bool TList::FindNearest(const void* AItem, long& ANearest, long AFrom) const
    {
        ANearest = -1;
        bool result = false;

        if (FHasSorted)
            result = FindItem(AFrom, FCount - 1, AItem, ANearest);

        return result;
    }

    void TList::SetSorted(bool ASorted)
    {
        if (ASorted != FSorted)
        {
            if (!ASorted)
                Sort();

            FSorted = ASorted;
        }

    }

}