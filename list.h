#pragma once

#include "iterator.h"
#include "alloc.h"
#include "construct.h"

template<class T>
struct __ListNode
{
	T _data;
	__ListNode<T>* _prev;
	__ListNode<T>* _next;

	__ListNode(const T& data = T())
		: _data(data)
		, _prev(NULL)
		, _next(NULL)
	{}
};

//List迭代器
template<class T, class Ref, class Ptr>
struct __ListIterator<T, Ref, Ptr>
{
	typedef __ListIterator<T, Ref, Ptr> Self;

	typedef BidirectionalIteratorTag IteratorCategory;
	typedef T ValueType;
	typedef ptrdiff_t DifferenceType;
	typedef Ptr Pointer;
	typedef Ref Reference;

	__ListNode<T>* _node;

	__ListIterator()
	{}

	__ListIterator(__ListNode<T>* node)
		:_node(node)
	{}

	Reference operator*()
	{
		return _node->_data;
	}

	Pointer operator->()
	{
		return &(_node->_data);
	}

	bool operator==(const Self& s)
	{
		return _node == s._node;
	}

	bool operator!=(const Self& s)
	{
		return _node != s._node;
	}
	//前置++
	Self& operator++()
	{
		_node = _node->_next;
		return *this;
	}
	//后置++
	Self operator++(int)
	{
		Self tmp(*this);
		_node = _node->_next;
		return tmp;
	}

	Self& operator--()
	{
		_node = _node->_prev;
		return *this
	}

	Self operator--(int)
	{
		Self tmp(*this);
		_node = _node->_prev;
		return tmp;
	}
};

template<class T, class Alloc = alloc>
class List
{
	typedef __ListNode<T> Node;
public:
	typedef __ListIterator<T, T&, T*> Iterator;
	typedef __ListIterator<T, const T&, const T*> ConstIterator;
	typedef ReverseIterator<ConstIterator> ConstReverseIterator;
	typedef ReverseIterator<Iterator> ReverseIterator;
	typedef SimpleAlloc<__ListNode<T>, Alloc> ListNodeAllocator;

	__ListNode<T>* BuyNode(const T& x)
	{
		// 调用空间配置器分配空间
		// 显示的调用构造函数初始化
		__ListNode<T>* node = ListNodeAllocator();
		Construct(node, x);
		return node;
	}
	void DestoryNode(__ListNode<T>* node)
	{
		Destory(node);
		ListNodeAllocator::Deallocate();
	}
	List()
		:_head(BuyNode(T()))
	{
		//双向链表
		_head->_next = _head;
		_head->_prev = _head;
	}
	~List()
	{
		// 调用clear()释放空间
		// 删除头节点
		Clear();
		DestoryNode(_head);
		_head = NULL;
	}
	void Clear()
	{
		Iterator begin = Begin();
		while (begin != End())
		{
			__ListNode<T>* cur = begin._node;
			++begin;
			DestoryNode(cur);
		}
	}
	void Insert(Iterator pos, const T& x)
	{
		Node* cur = pos._node;
		Node* prev = cur->_prev;

		Node* tmp = BuyNode(x);

		tmp->_next = cur;
		cur->_prev = tmp;

		prev->_next = tmp;
		tmp->_prev = prev;
	}

	Iterator Erase(Iterator pos)
	{
		assert(pos != End());

		Node* cur = pos._node;
		Node* prev = cur->_prev;
		Node* next = cur->_next;

		prev->_next = next;
		next->_prev = prev;

		//delete cur;
		DestoryNode(cur);

		return Iterator(next);
	}
	void PushBack(const T& x)
	{
		Insert(End(), x);
	}

	void PushFront(const T& x)
	{
		Insert(Begin(), x);
	}

	void PopBack()
	{
		Erase(--End());
	}

	void PopFront()
	{
		Erase(Begin());
	}

	Iterator Begin()
	{
		return Iterator(_head->_next);
	}

	Iterator End()
	{
		//return Iterator(_head);
		return _head; // 强制类型 explict
	}

	ConstIterator Begin() const
	{
		return _head->_next;
	}

	ConstIterator End() const
	{
		//return Iterator(_head);
		return _head; // 强制类型 explict
	}

	ReverseIterator RBegin()
	{
		return ReverseIterator(End());
		//return End();
	}

	ReverseIterator REnd()
	{
		return ReverseIterator(Begin());
	}

	ConstReverseIterator RBegin() const
	{
		return ConstReverseIterator(End());
		//return End();
	}

	ConstReverseIterator REnd() const
	{
		return ConstReverseIterator(Begin());
	}
protected:
	Node* _head;
};

void PrintList1(List<int>& l)
{
	List<int>::Iterator it = l.Begin();
	while (it != l.End())
	{
		*it = 10;
		cout << *it << " ";
		++it;
	}
	cout << endl;
}
//const
void PrintList2(const List<int>& l)
{
	List<int>::ConstIterator it = l.Begin();
	while (it != l.End())
	{
		//*it = 10;
		cout << *it << " ";
		++it;
	}
	cout << endl;
}

//反向迭代器
void PrintList3(const List<int>& l)
{
	//List<int>::
	List<int>::ConstReverseIterator it = l.RBegin();
	while (it != l.REnd())
	{
		cout << *it << " ";
		++it;
	}
	cout << endl;
}

void TestList()
{
	List<int> l;
	l.PushBack(1);
	l.PushBack(2);
	l.PushBack(3);
	l.PushBack(4);

	//PrintList2(l);
	//PrintList1(l);
	PrintList3(l);
}