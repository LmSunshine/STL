#pragma once

using namespace std;
#include<iostream>
#include<stdlib.h>
#include<stdarg.h>
#define __DEBUG__

//GetFileName();
static string GetFileName(const string& path)
{
	char ch = '/';

#ifdef _WIN32
	ch = '\\';
#endif

	size_t pos = path.rfind(ch);
	if (pos == string::npos)
		return path;
	else
		return path.substr(pos + 1);   //substr?
}
//���ڵ���׷�ݵ�trace log
inline static void __trace_debug(const char* function, const char* filename, int line, char* format, ...)
{
	// ��ȡ�����ļ�

#ifdef __DEBUG__
	// ������ú�������Ϣ
	fprintf(stdout, "�� %s:%d��%s", GetFileName(filename).c_str(), line, function);

	// ����û����trace��Ϣ
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
#endif
}

#define __TRACE_DEBUG(...) __trace_debug(__FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);

///////////////////////////////////////////////////////////////////////////////
//һ���ռ�������
typedef void (*HANDLER_FUNC)();

template<int inst>
class __MallocAllocTemplate
{
private:
	//SGI��һ����������allocate()��realloc�����ڵ���malloc��realloc���ɹ��󣬸ĵ���OomMalloc()��OomRealloc()��
	//�����߶�����ѭ�������ϵ���"�ڴ治�㴦������"��������ĳ�ε���֮�󣬻���㹻���ڴ��Բ���������
	//���ķ�����������֣�
	//1.��������ڴ治�㴦�����̡���δ���Ͷ��趨��OomMalloc()��OomRealloc()�����_THROW_BAD_ALLOC, ����bad_alloc�쳣��Ϣ.
	//2.�����ڴ�ɹ����ء�
	static void* OomMalloc(size_t size)
	{
		while (1)
		{
			if (__MallocAllocOomHandler == 0)
				throw bad_alloc();
			__MallocAllocOomHandler();        //��
			void* ret = malloc(size);
			if (ret)
				return ret;
		}
	}
	static HANDLER_FUNC __MallocAllocOomHandler;
public:
	static void* Allocate(size_t n)
	{
		void* result = malloc(n);
		if (result == 0)
		{
			result=OomMalloc(n);
		}
		__TRACE_DEBUG("����һ���ռ�����������ptr:0x%p size : %u\n",result, n);
		return result;
	}

	static void Deallocate(void *p, size_t n)
	{
		__TRACE_DEBUG("����һ���ռ��������ͷ�ptr:0x%p size:%u\n",p, n);
		free(p);
	}

	//����ԭ�еĺ���ָ�룬���ڳ���
	static HANDLER_FUNC SetMallocHandler(HANDLER_FUNC f)
	{
		HANDLER_FUNC old = __MallocAllocOomHandler;
		__MallocAllocOomHandler = f;

		return(old);
	}
};
template<int inst>
HANDLER_FUNC __MallocAllocTemplate<inst>::__MallocAllocOomHandler = 0;

///////////////////////////////////////////////////////////////////////////////
//�����ռ�������(�����ռ���������ΪƵ������С�ڴ������һ���㷨��������һ���ռ�������������Ƭ���⡣)
template <bool threads, int inst>
class __DefaultAllocTemplate
{
	enum { __ALIGN = 8 };               //С��������ϵ��߽�
	enum { __MAX_BYTES = 128 };         //С�����������
	enum { __NFREELISTS = __MAX_BYTES / __ALIGN };  //��������ĸ���
public:
	static char* ChunkAlloc(size_t size, size_t& nobjs)
	{
		char* result;
		size_t totalBytes = size*nobjs;
		size_t bytesLeft = _endFree - _startFree;

		//1.�ڴ�����㹻��С�Ŀռ䣬���������Ŀռ䣻
		if (bytesLeft > totalBytes)
		{
			__TRACE_DEBUG("���ڴ�����㹻nobjs:%u������\n", nobjs);

			result = _startFree;
			_startFree += totalBytes;
			return result;
		}
		//2.�ڴ��û���㹻��С�Ŀռ䣬�������ٻ��ܷ���һ���ڵ�Ŀռ䣬���ֶܷ��ٷֶ��٣� 
		else if (bytesLeft > size)
		{
			nobjs = bytesLeft / size;
			totalBytes = nobjs*size;

			__TRACE_DEBUG("���ڴ��ֻ��nobjs:%u������\n", nobjs);

			result = _startFree;
			_startFree += totalBytes;
			return result;
		}
		//3.�ڴ��һ���ڵ㶼�ڲ���������ϵͳ��heap����2����Ҫ���С�Ŀռ�����ѷ����_heapSize/8���ڴ浽�ڴ�� ��
		//��ʱ������ڴ��ʣ���пռ䣬��ŵ�free-list��ȥ��
		else
		{
			size_t bytesToGet = 2 * totalBytes
				+ ROUND_UP(_heapSize >> 4);

			// ���ڴ��ʣ��Ŀռ�ҵ���������
			if (bytesLeft > 0)
			{
				size_t index = FREELIST_INDEX(bytesLeft);
				((Obj*)_startFree)->_freeListLink = _freeList[index];
				_freeList[index] = (Obj*)_startFree;
			}

			_startFree = (char*)malloc(bytesToGet);
			__TRACE_DEBUG("�ڴ��û���㹻�Ŀռ䣬��ϵͳ����%u���ֽ�\n", bytesToGet);

			//4.�����heap����ռ�ʧ�ܣ���ôֻ�ܿ�free-list�и���Ľڵ��Ƿ��п��ÿռ��ˣ�
			//������֮��ͬʱ�ݹ������������ChunkAlloc(size,nobjs)�� 
			if (_startFree == NULL)
			{
				// ���������������
				for (size_t i = size; i <= __MAX_BYTES;i += __ALIGN)
				{
					size_t index = FREELIST_INDEX(i);
					if (_freeList[index])
					{
						_startFree = (char*)_freeList[index];
						_freeList[index] = ((Obj*)_startFree)->_freeListLink;
						_endFree = _startFree + i;

						return ChunkAlloc(size, nobjs);
					}
				}

				// 5.���free-listҲû�п��ýڵ��ˣ���ôת���һ���ռ�����������ռ�,
				//�ٲ��У���һ���ռ����������׳�bad_alloc�쳣.
				_endFree = 0;
				_startFree = (char*)__MallocAllocTemplate<0>::Allocate(bytesToGet);
			}

			_heapSize += bytesToGet;
			_endFree = _startFree + bytesToGet;
			return ChunkAlloc(size, nobjs);
		}
	}

	static void* Refill(size_t size)
	{
		// ���ڴ�������ڴ棬�з��ڴ��ҵ���������
		size_t nobjs = 20;     //Ĭ���������
		char* chunk = ChunkAlloc(size, nobjs);

		//1.ֻ��һ�鷵�ظ������ߣ��������½ڵ�
		if (nobjs == 1)
		{
			return chunk;
		}

		//2.��ֹһ��
		Obj* cur = (Obj*)(chunk + size);           //curָ��������ĵڶ���С�����飨��һ���������ߣ���sizeΪһ��С������Ĵ�С
		size_t index = FREELIST_INDEX(size);      //�ҵ���������
		_freeList[index] = cur;                   //���µ����齨����
		for (int i = 0; i < nobjs - 2; ++i)       //��������֮��Ľڵ㴮������
		{
			Obj* next = (Obj*)((char*)cur + size);
			cur->_freeListLink = next;

			cur = next;
		}
		cur->_freeListLink = NULL;

		return chunk;       //���ظ�������
	}

	static void* Allocate(size_t n)
	{
		__TRACE_DEBUG("���ö����ռ����÷����ڴ棺%u\n", n);

		//1. ����128�������һ���ռ�������
		if (n > __MAX_BYTES)
		{
			return __MallocAllocTemplate<0>::Allocate(n);
		}
		//Ѱ��16���������ʵ�����������ָ���С������
		size_t index = FREELIST_INDEX(n);
		//2.����û�п��õĿ����Refill����
		if (_freeList[index] == NULL)
		{
			return Refill(ROUND_UP(n));
		}
		//3.�к��ʵ�.��������ָ��
		else
		{
			Obj* first = _freeList[index];
			_freeList[index] = first->_freeListLink;

			__TRACE_DEBUG("����������ȡ�ڴ�飺0x%p\n", first);

			return first;
		}
	}

	static void Deallocate(void* p, size_t size)
	{
		__TRACE_DEBUG("�����ռ��������ͷſռ�0x%p, %u\n", p, size);

		//1.����128ʹ�õ�һ��������
		if (size > __MAX_BYTES)
		{
			__MallocAllocTemplate<0>::Deallocate(p, size);
			return;
		}

		//2.�ҵ���������
		size_t index = FREELIST_INDEX(size);
		//����ָ��ָ��
		((Obj*)p)->_freeListLink = _freeList[index];
		_freeList[index] = (Obj*)p;
	}

protected:
	//�ѷ�����ڴ��С����һ����������+7��ÿ���8Ϊһ������������
	//Ȼ�����8�������㵽Ҫ�ҵ��ڴ���ǵڼ����ˣ������0��ʼ�����ټ�һ
	static  size_t FREELIST_INDEX(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) / __ALIGN - 1);
	}

	//�õ������ڴ���С�����϶������������������У����ǵ��ڴ���С����8�ı�����
	//���ǲ�����ÿ�������ڴ��С����8�ı������������Ǿ�Ҫȡ�������С�����ȵ��ڴ�飬
	//���������ȡ����&~(ALIGN - 1)�൱�ڽ���8λ��0��ֻȡ��8λ����8λ����8�ı�����
	static size_t ROUND_UP(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) & ~(__ALIGN - 1));
	}

	//��������
	//����ṹ���Կ����Ǵ�һ���ڴ���пٳ�4���ֽڴ�С����
	//������ڴ�����ʱ�����洢���¸����п飬
	//������ڴ�齻�����û�ʱ�����洢��ʱ�û�������
	union Obj
	{
		union Obj* _freeListLink;  //ָ����һ���ڴ����
		char client_data[1];      //
	};
	static Obj* _freeList[__NFREELISTS];
	
	//�ڴ��
	static char* _startFree;    //ˮλ��
	static char* _endFree;      //�ص�
	static size_t _heapSize;    //�����ռ��������ܹ�����Ķ����ֽڵ��ڴ�
};

template<bool threads, int inst>
char* __DefaultAllocTemplate<threads, inst>::_startFree = NULL;

template<bool threads, int inst>
char* __DefaultAllocTemplate<threads, inst>::_endFree = NULL;

template<bool threads, int inst>
size_t __DefaultAllocTemplate<threads, inst>::_heapSize = 0;

//typename�����߱�����DefaultAllocTemplate<threads, inst>��һ������
template<bool threads, int inst>
typename __DefaultAllocTemplate<threads, inst>::Obj* __DefaultAllocTemplate<threads, inst>::_freeList[__NFREELISTS] = { 0 };

#ifdef __USE_MALLOC
typedef __MallocAllocTemplate<0> alloc;
#else
typedef	__DefaultAllocTemplate<0, 0> alloc;
#endif

template<class T, class Alloc>
class SimpleAlloc 
{
public:
	static T* Allocate(size_t n)
	{
		return 0 == n ? 0 : (T*)Alloc::Allocate(n * sizeof (T));
	}

	static T* Allocate(void)
	{
		return (T*)Alloc::Allocate(sizeof (T));
	}

	static void Deallocate(T *p, size_t n)
	{
		if (0 != n)
			Alloc::Deallocate(p, n * sizeof (T));
	}

	static void Deallocate(T *p)
	{
		Alloc::Deallocate(p, sizeof (T));
	}
};















////����ָ��
//#include<stdio.h>
//void hello(void) 
//{ 
//	printf("���!\n");
//}
//void bye(void) 
//{ 
//	printf("�ټ���\n"); 
//}
//void ok(void) 
//{ 
//	printf("�õģ�\n"); 
//}
//typedef void(*funcptr)(void);
//
//void speak(int id)
//{
//	funcptr words[3] = { &hello, &bye, &ok };
//	funcptr fun = words[id]; 
//	(*fun)();
//}
//void Test()
//{
//	speak(0);
//	speak(1);
//	speak(2);
//}