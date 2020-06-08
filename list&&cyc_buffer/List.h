#ifndef _List_H_
#define _List_H_
#include "KYSyncObj.h"

// KYLib 2.0 ��ʼʹ�� KYLib �����ռ�
namespace KYLib
{

    class TList
    {
    public:
        // TOnCompare ��Ƚ��¼�
        // 1. �� AItem1 ���� AItem2 �� ACompare == 0
        // 2. �� AItem1 ���� AItem2 �� ACompare > 0
        // 3. �� AItem1 С�� AItem2 �� ACompare < 0
        typedef void (TObject::* TDoCompare)(const void* AItem1,
                                             const void* AItem2, int& ACompare);
        typedef struct
        {
            TDoCompare  Method;
            void*       Object;
        } TOnCompare;

        // TOnDeletion ��ɾ���¼�
        typedef void (TObject::* TDoDeletion)(void* AItem);
        typedef struct
        {
            TDoDeletion Method;
            void*       Object;
        } TOnDeletion;

    public:
        TList();
        TList(const TList& AList);
        virtual ~TList();

        // �����б�, ͬʱҲ���� OnCompare �¼�����, ����� OnDeletion �¼�����
        TList& operator=(const TList& AList);

        // ����������ȡ��, AIndex ȡֵ��Χ: [0..Count()-1]
        void* operator[](long AIndex) const { return Item(AIndex); }
        void* Item(long AIndex) const;

        // ����
        long           Capacity() const { return FCapacity; }      // default: 0
        long           Count() const { return FCount; }         // default: 0
        long           Delta() const { return FDelta; }         // default: Delta_Auto
        bool           Sorted() const { return FSorted; }        // default: false
        bool           CanDuplicate() const { return FCanDuplicate; }  // default: true
        
        // ��������
        bool           SetItem(long AIndex, void* AItem);
        void           SetDelta(long ADelta);
        void           SetSorted(bool ASorted);
        void           SetCanDuplicate(bool ACanDuplicate);

        // �����б�, ͬʱҲ���� OnCompare �¼�����, ����� OnDeletion �¼�����
        void           Assign(const TList& AList);

        // ��������, �� Count() ���ڸ��ĵ�����ֵ���Զ��ͷ���, ������ OnDeletion �¼�
        void           ChangeCapacity(long ACapacity);

        // �����
        // 1. �� Sorted ����Ϊ false ����ӵ�ĩβ��, �������ֵ��С���뵽ָ��λ��
        // 2. ������ֵ >= 0, ������������, ���򷵻����ʧ�ܵĴ�����
        long           Add(void* AItem);

        // ����ָ��������
        // 1. �� Sorted ����Ϊ true ��������� Insert ����, ������� Add ���������
        // 2. �� AIndex <= 0 ������һ��, �� AIndex >= Count() �����ĩβ��
        bool           Insert(long AIndex, void* AItem);

        // ɾ��ָ��������, ���� OnDeletion �¼�
        void           Delete(long AIndex);

        // ɾ��<��������>�ĵ�һ��, ���� OnDeletion �¼�
        void           Delete(void* AItem);

        // ɾ��<��ָ����ͬ>�ĵ�һ��, ������ɾ��ǰ������, ���� OnDeletion �¼�
        long           Remove(void* AItem);

        // ���������, ���� OnDeletion �¼�
        void           Clear();

        // ���� OnCompare ��С��������
        // ��Ҫ��������, ��ֻҪ�� OnCompare �ķ�������ֵ ACompare = -ACompare ����
        void           Sort();

        //��������
        void           QuickSort(long AFrom, long ATo);

        // �ƶ����� AIndex � ANewIndex ����, �� Sorted ����Ϊ true ��ʧ��
        bool           MoveTo(long AIndex, long ANewIndex);

        // �������� AIndex1 �� AIndex2 ��, �� Sorted ����Ϊ true ��ʧ��
        bool           Exchange(long AIndex1, long AIndex2);

        // ����ֵΪ AItem �� Compare(...) == 0 ��һ�������
        long           IndexOf(const void* AItem) const;

        // �� AFrom ������ʼ����ֵΪ AItem �� Compare(...) == 0 ��ͬ����
        long           SameCount(const void* AItem, long AFrom = 0) const;

        // �� AFrom ������ʼ����ֵΪ AItem �� Compare(...) == 0 ��һ�������
        long           Search(const void* AItem, long AFrom = 0) const;
        long           Search(const void* AItem, Pointer& ARetItem, long AFrom = 0) const;

        // �������һ������
        // 1. �� ANearest == -1 ���ʾδ����, ����ʧ��;
        // 2. ������ֵΪ true, ���ʾ�ҵ�������, ������ֵ�� ANearest ����֮ǰ
        bool           FindNearest(const void* AItem, long& ANearest, long AFrom = 0) const;

        // �¼�
        TOnCompare     OnCompare;
        TOnDeletion    OnDeletion;
    protected:
        // FindItem �������б���ֲ���, FindByOrder ������˳�����  IsPtr�Ƿ�Ƚ�ָ��(�� ��compare)
        bool           FindItem(long AFrom, long ATo, const void* AItem, long& ANearest) const;
        long           FindByOrder(long AFrom, long ATo, const void* AItem) const;
        // ���ֲ���
        void           FindNearestItem(long AFrom, long ATo, const void* AItem, long& ANearest) const;

        // �Ƚ� AItem1 > AItem2 ���ش���0����ȷ���0�����򷵻�С��0
        int            Compare(TOnCompare AParaCompare, const void* AItem1, const void* AItem2) const;
        int            Compare(const void* AItem1, const void* AItem2) const;

        // ��AFrom������ ���ACount�� ���� OnDeletion �¼�
        void           OnDeleteItems(Pointer* Aarr, long AFrom, long ACount);

        void           DoSort0(TOnCompare ACompare, long AFrom, long ATo);
        void           DoSort1(long AFrom, long ATo);
    private:
        // ����
        int            AddCapacity() const;
        // �ƶ����� AIndex � ANewIndex ����
        void           SimpleMove(long AIndex, long ANewIndex);

    private:
        Pointer*       FItems;              // ���б�
        long           FCapacity;           // �б�����
        long           FCount;              // �����
        long           FDelta;              // �������Զ�����

        bool           FSorted;             // �Ƿ�����, Ĭ��ֵΪ false
        bool           FHasSorted;          // �Ƿ�������, Ĭ��ֵΪ false
        bool           FCanDuplicate;       // �Ƿ������ظ�, Ĭ��ֵΪ true
    };

}

#endif