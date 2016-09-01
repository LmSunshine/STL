#pragma once

#include "alloc.h"
#include "construct.h"
#include "uninitialized.h"
 
template<class T, class Alloc = alloc>
class Vector
{
public:
	typedef T* Iterator;
	typedef const T* ConstIterator;

	typedef ReverseIterator<Iterator> ReverseIterator;
	typedef ReverseIterator<ConstIterator> ConstReverseIterator;

	typedef SimpleAlloc<T, Alloc> DataAllocator;

public:
	Vector()
		: _start(NULL)
		, _finish(NULL)
		, _endOfStorage(NULL)
	{}

	~Vector()
	{
		if (_start)
		{
			Destroy(Begin(), End());
			DataAllocator::Deallocate(_start, _endOfStorage - _start);
		}
	}

	void PushBack(const T& x)
	{
		_CheckStorage();

		*_finish = x;
		++_finish;
	}

	void PopBack();
	void Insert(Iterator pos, const T& x);
	Iterator Erase(Iterator pos);

	T& operator[](const size_t index)
	{
		assert(index < Size());
		return _start[index];
	}

	size_t Size()
	{
		return _finish - _start;
	}

	Iterator Begin()
	{
		return _start;
	}

	Iterator End()
	{
		return _finish;
	}

	ConstIterator Begin() const
	{
		return _start;
	}

	ConstIterator End() const
	{
		return _finish;
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

protected:
	void _CheckStorage()
	{
		if (_finish == _endOfStorage)
		{
			size_t size = Size();
			size_t newStorage = size * 2 + 3;
			/*T* tmp = new T [newStorage];
			if (_start)
			{
			for (size_t i = 0; i < size; ++i)
			{
			tmp[i] = _start[i];
			}

			delete[] _start;
			}*/
			T* tmp = DataAllocator::Allocate(newStorage);
			if (_start)
			{
				// 拷贝旧数据到增容的新空间
				UninitializedCopy(Begin(), End(), tmp);

				// 释放旧空间的数据
				Destroy(Begin(), End());
				DataAllocator::Deallocate(_start, Size());
			}

			_start = tmp;
			_finish = _start + size;
			_endOfStorage = _start + newStorage;
		}
	}

protected:
	Iterator _start;
	Iterator _finish;
	Iterator _endOfStorage;
};

void PrintVector1(Vector<int>& v)
{
	Vector<int>::ReverseIterator it = v.RBegin();
	while (it != v.REnd())
	{
		cout << *it << " ";
		++it;
	}
	cout << endl;
}

void PrintVector2(const Vector<int>& v)
{
	Vector<int>::ConstIterator it = v.Begin();
	while (it != v.End())
	{
		//*it = 20;
		cout << *it << " ";
		++it;
	}
	cout << endl;
}

void TestVector()
{
	Vector<int> v;
	v.PushBack(1);
	v.PushBack(2);
	v.PushBack(3);
	v.PushBack(4);

	PrintVector1(v);
	PrintVector2(v);
}