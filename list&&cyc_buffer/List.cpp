#include "List.h"

namespace KYLib
{
    // ���캯��
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

    // ��������
    TList::~TList()
    {
        Clear();
    }

    // ������������ĳһ��
    void* TList::Item(long AIndex) const
    {
        void* result = NULL;
        if (AIndex >= 0 && AIndex < FCount)
            result = FItems[AIndex];

        return result;
    }

    // ������������ĳһ��
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

                    // �����ı��� ʹ��������
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

    // ��AFrom������ ���ACount�� ���� OnDeletion �¼�
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

    // ��������, �� Count() ���ڸ��ĵ�����ֵ���Զ��ͷ���, ������ OnDeletion �¼�
    void TList::ChangeCapacity(long ACapacity)
    {
        if (ACapacity == FCapacity)
            ;
        else if (ACapacity <= 0)
        {
            if (FItems != NULL)
            {
                // ��������� �ټ��� OnDeletion �¼�
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
                    // �������ڷ��������
                    if (FCount > ACapacity)
                        FCount = ACapacity;
                    memcpy(tmp, FItems, FCount * sizeof(Pointer));
                }

                FItems = tmp;

                // ԭ�б�Ϊ�� һ����Ҫ�ͷ�
                if (old != NULL)
                {
                    //  ���� OnDeletion �¼�
                    long del_count = tmp_count - ACapacity;
                    if (del_count > 0)
                        OnDeleteItems(old, ACapacity, del_count);

                    free(old);
                }
            }
        }

    }

    // ���ݣ�����������С
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

    // ���ֲ��� 
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
        // ���ȽϷ���Ϊ�� ��ֻ�ܽ���ָ��Ƚ�
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

    // �����
    // 1. �� Sorted ����Ϊ false ����ӵ�ĩβ��, �������ֵ��С���뵽ָ��λ��
    // 2. ������ֵ >= 0, ������������, ���򷵻����ʧ�ܵĴ�����
    long TList::Add(void* AItem)
    {
        long result = 0;

        if (FCount == FCapacity) //����
            ChangeCapacity(AddCapacity());

        if (FCount == FCapacity) // �п�������ʧ��
            result = -1;
        else if (!FSorted) // ���������б�
        {
            FItems[FCount] = AItem;
            result = FCount++;
            FHasSorted = false;
        }
        else
        {
            // ���ֲ���
            FindNearestItem(0, FCount - 1, AItem, result);

            if (result != FCount)
                memmove(FItems + result + 1, FItems + result, (FCount - result) * sizeof(Pointer));
            FItems[result] = AItem;
            FCount++;

        }

        return result;
    }

    // ����ָ��������
    // 1. �� Sorted ����Ϊ true ��������� Insert ����, ������� Add ���������
    // 2. �� AIndex <= 0 ������һ��, �� AIndex >= Count() �����ĩβ��
    bool TList::Insert(long AIndex, void* AItem)
    {
        bool result = false;
        if (!FSorted)
        {
            if (FCount == FCapacity) //����
                ChangeCapacity(AddCapacity());

            if (FCount != FCapacity)
            {
                if (AIndex >= FCount)
                    FItems[FCount] = AItem;
                else
                {
                    if (AIndex < 0)
                        AIndex = 0;
                    // ������±�֮���Ԫ��ȫ������
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

    // ɾ��ָ��������, ���� OnDeletion �¼�
    void TList::Delete(long AIndex)
    {
        if (AIndex >= 0 && AIndex < FCount)
        {
            // ȡ����Ҫɾ������
            void* tmp = FItems[AIndex];

            FCount--;
            if (AIndex != FCount)
                memmove(FItems + AIndex, FItems + AIndex + 1, (FCount - AIndex) * sizeof(Pointer));

            if (OnDeletion.Method != NULL)
                ((TObject*)OnDeletion.Object->*OnDeletion.Method)(tmp);
        }
    }

    // �ƶ����� AIndex � ANewIndex ����, �� Sorted ����Ϊ true ��ʧ��
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

    // ����Sorted����Ϊtrueʱ�������Ǹ��ı���ʹ�������� (�ڲ�����)
    void TList::SimpleMove(long AIndex, long ANewIndex)
    {
        Pointer tmp = FItems[AIndex];

        // [ANewIndex, AIndex) �� (AIndex, ANewIndex] ֮���Ԫ�أ�����ȫ�����һ������ƶ�һ����λ
        if (AIndex > ANewIndex)
            memmove(FItems + ANewIndex + 1, FItems + ANewIndex, (AIndex - ANewIndex) * sizeof(Pointer));
        else
            memmove(FItems + AIndex, FItems + AIndex + 1, (ANewIndex - AIndex) * sizeof(Pointer));

        FItems[ANewIndex] = tmp;
    }

    // �������� AIndex1 �� AIndex2 ��, �� Sorted ����Ϊ true ��ʧ��
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

    // ���ֲ��� ����ҵ� ANearest����һ����ȷ���� ���Ҳ��� ANearest����Ҫ�����λ��
    // IsPtr �Ƿ�Ƚ�ָ�� Ϊfalse�� compare
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

        // ���ȽϷ���Ϊ�� ��ֻ�ܽ���ָ��Ƚ�
        TOnCompare tmp_compare = OnCompare;
        if (tmp_compare.Method == NULL)
        {
            while (AFrom <= ATo)
            {
				// ��ӳ��Զ����ܻ���������Խ��
                mid = AFrom + ((ATo - AFrom) >> 1);
                if (AItem > FItems[mid])
                    AFrom = mid + 1;
                else
                    ATo = mid - 1;
            }

            // �����Ϸ���ֵ��ͬ ���ҵ�
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

            // �����Ϸ���ֵ��ͬ ���ҵ�
            if (AFrom < FCount && Compare(tmp_compare, AItem, FItems[AFrom]) == 0)
                result = true;
        }
        ANearest = AFrom;

        return result;
    }

    //��˳�����
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

        // ���ȽϷ���Ϊ�� ��ֻ�ܽ���ָ��Ƚ�
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

    // ����ֵ��������
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

    // ɾ��<��������>�ĵ�һ��, ���� OnDeletion �¼�
    void TList::Delete(void* AItem)
    {
        if (FCount > 0)
        {
            long index = IndexOf(AItem);
            if (index >= 0 && index < FCount)
                Delete(index);
        }

    }

    // ɾ��<��ָ����ͬ>�ĵ�һ��, ������ɾ��ǰ������, ���� OnDeletion �¼�
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

    // ���������, ���� OnDeletion �¼�
    void TList::Clear()
    {
        // ��������Ϊ 0 �����������Ч����ͬ
        ChangeCapacity(0);

    }

    // ����
    void TList::Sort()
    {
        if (!FHasSorted)
        {
            if (FCount > 1)
                QuickSort(0, FCount - 1);

            FHasSorted = true;
        }

    }

    // compare ����
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

    // ָ������
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

    //��������
    void TList::QuickSort(long AFrom, long ATo)
    {
        // ���ȽϷ���Ϊ�� ��ֻ�ܽ���ָ��Ƚ�
        TOnCompare tmp_compare = OnCompare;

        if (tmp_compare.Method != NULL)
            DoSort0(tmp_compare, AFrom, ATo);
        else
            DoSort1(AFrom, ATo);

    }

    // �Ƚ� AItem1 > AItem2 ���ش���0����ȷ���0�����򷵻�С��0 (����ǰ���������)
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

    // �� AFrom ������ʼ����ֵΪ AItem �� Compare(...) == 0 ��һ�������
    long TList::Search(const void* AItem, long AFrom) const
    {
        long result = -1;

        // ʹ�����Ϸ�
        if (AFrom < 0)
            AFrom = 0;
        else if (AFrom >= FCount)
            AFrom = FCount - 1;

        if (FCount <= 0)
            ;
        else if (!FHasSorted)  // �������б�
            result = FindByOrder(AFrom, FCount - 1, AItem);
        else if (!FindItem(AFrom, FCount - 1, AItem, result))
            result = -1;

        return result;
    }

    // �� AFrom ������ʼ����ֵΪ AItem �� Compare(...) == 0 ��һ�������
    long TList::Search(const void* AItem, Pointer& ARetItem, long AFrom) const
    {
        long result = Search(AItem, AFrom);

        if (result >= 0)
            ARetItem = FItems[result];

        return result;
    }

    // �������һ������
    // 1. �� ANearest == -1 ���ʾδ����, ����ʧ��;
    // 2. ������ֵΪ true, ���ʾ�ҵ�������, ������ֵ�� ANearest ����֮ǰ
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