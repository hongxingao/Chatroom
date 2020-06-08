#ifndef _List_H_
#define _List_H_
#include "KYSyncObj.h"

// KYLib 2.0 开始使用 KYLib 命名空间
namespace KYLib
{

    class TList
    {
    public:
        // TOnCompare 项比较事件
        // 1. 若 AItem1 等于 AItem2 则 ACompare == 0
        // 2. 若 AItem1 大于 AItem2 则 ACompare > 0
        // 3. 若 AItem1 小于 AItem2 则 ACompare < 0
        typedef void (TObject::* TDoCompare)(const void* AItem1,
                                             const void* AItem2, int& ACompare);
        typedef struct
        {
            TDoCompare  Method;
            void*       Object;
        } TOnCompare;

        // TOnDeletion 项删除事件
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

        // 拷贝列表, 同时也复制 OnCompare 事件方法, 并清空 OnDeletion 事件方法
        TList& operator=(const TList& AList);

        // 根据索引读取项, AIndex 取值范围: [0..Count()-1]
        void* operator[](long AIndex) const { return Item(AIndex); }
        void* Item(long AIndex) const;

        // 属性
        long           Capacity() const { return FCapacity; }      // default: 0
        long           Count() const { return FCount; }         // default: 0
        long           Delta() const { return FDelta; }         // default: Delta_Auto
        bool           Sorted() const { return FSorted; }        // default: false
        bool           CanDuplicate() const { return FCanDuplicate; }  // default: true
        
        // 设置属性
        bool           SetItem(long AIndex, void* AItem);
        void           SetDelta(long ADelta);
        void           SetSorted(bool ASorted);
        void           SetCanDuplicate(bool ACanDuplicate);

        // 拷贝列表, 同时也复制 OnCompare 事件方法, 并清空 OnDeletion 事件方法
        void           Assign(const TList& AList);

        // 更改容量, 若 Count() 大于更改的容量值则自动释放项, 并激发 OnDeletion 事件
        void           ChangeCapacity(long ACapacity);

        // 添加项
        // 1. 若 Sorted 属性为 false 则添加到末尾项, 否则根据值大小插入到指定位置
        // 2. 若返回值 >= 0, 则添加项的索引, 否则返回添加失败的错误码
        long           Add(void* AItem);

        // 插入指定索引项
        // 1. 若 Sorted 属性为 true 则不允许调用 Insert 方法, 必须调用 Add 方法添加项
        // 2. 若 AIndex <= 0 则插入第一项, 若 AIndex >= Count() 则插入末尾项
        bool           Insert(long AIndex, void* AItem);

        // 删除指定索引项, 激发 OnDeletion 事件
        void           Delete(long AIndex);

        // 删除<满足条件>的第一项, 激发 OnDeletion 事件
        void           Delete(void* AItem);

        // 删除<项指针相同>的第一项, 并返回删除前的索引, 激发 OnDeletion 事件
        long           Remove(void* AItem);

        // 清除所有项, 激发 OnDeletion 事件
        void           Clear();

        // 根据 OnCompare 从小到大排序
        // 若要反向排序, 则只要在 OnCompare 的方法返回值 ACompare = -ACompare 即可
        void           Sort();

        //快速排序
        void           QuickSort(long AFrom, long ATo);

        // 移动索引 AIndex 项到 ANewIndex 索引, 若 Sorted 属性为 true 则失败
        bool           MoveTo(long AIndex, long ANewIndex);

        // 交换索引 AIndex1 和 AIndex2 项, 若 Sorted 属性为 true 则失败
        bool           Exchange(long AIndex1, long AIndex2);

        // 搜索值为 AItem 的 Compare(...) == 0 第一项的索引
        long           IndexOf(const void* AItem) const;

        // 从 AFrom 索引开始搜索值为 AItem 的 Compare(...) == 0 相同项数
        long           SameCount(const void* AItem, long AFrom = 0) const;

        // 从 AFrom 索引开始搜索值为 AItem 的 Compare(...) == 0 第一项的索引
        long           Search(const void* AItem, long AFrom = 0) const;
        long           Search(const void* AItem, Pointer& ARetItem, long AFrom = 0) const;

        // 查找最近一个索引
        // 1. 若 ANearest == -1 则表示未排序, 查找失败;
        // 2. 若返回值为 true, 则表示找到项索引, 否则项值在 ANearest 索引之前
        bool           FindNearest(const void* AItem, long& ANearest, long AFrom = 0) const;

        // 事件
        TOnCompare     OnCompare;
        TOnDeletion    OnDeletion;
    protected:
        // FindItem 已排序列表二分查找, FindByOrder 是逐项顺序查找  IsPtr是否比较指针(否 则compare)
        bool           FindItem(long AFrom, long ATo, const void* AItem, long& ANearest) const;
        long           FindByOrder(long AFrom, long ATo, const void* AItem) const;
        // 二分插入
        void           FindNearestItem(long AFrom, long ATo, const void* AItem, long& ANearest) const;

        // 比较 AItem1 > AItem2 返回大于0，相等返回0，否则返回小于0
        int            Compare(TOnCompare AParaCompare, const void* AItem1, const void* AItem2) const;
        int            Compare(const void* AItem1, const void* AItem2) const;

        // 在AFrom索引处 向后ACount个 激发 OnDeletion 事件
        void           OnDeleteItems(Pointer* Aarr, long AFrom, long ACount);

        void           DoSort0(TOnCompare ACompare, long AFrom, long ATo);
        void           DoSort1(long AFrom, long ATo);
    private:
        // 扩容
        int            AddCapacity() const;
        // 移动索引 AIndex 项到 ANewIndex 索引
        void           SimpleMove(long AIndex, long ANewIndex);

    private:
        Pointer*       FItems;              // 项列表
        long           FCapacity;           // 列表容量
        long           FCount;              // 项个数
        long           FDelta;              // 容量的自动增量

        bool           FSorted;             // 是否排序, 默认值为 false
        bool           FHasSorted;          // 是否已排序, 默认值为 false
        bool           FCanDuplicate;       // 是否允许重复, 默认值为 true
    };

}

#endif