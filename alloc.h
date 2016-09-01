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
//用于调试追溯的trace log
inline static void __trace_debug(const char* function, const char* filename, int line, char* format, ...)
{
	// 读取配置文件

#ifdef __DEBUG__
	// 输出调用函数的信息
	fprintf(stdout, "【 %s:%d】%s", GetFileName(filename).c_str(), line, function);

	// 输出用户打的trace信息
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
#endif
}

#define __TRACE_DEBUG(...) __trace_debug(__FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);

///////////////////////////////////////////////////////////////////////////////
//一级空间配置器
typedef void (*HANDLER_FUNC)();

template<int inst>
class __MallocAllocTemplate
{
private:
	//SGI第一级配置器的allocate()和realloc都是在调用malloc和realloc不成功后，改调用OomMalloc()和OomRealloc()，
	//后两者都有内循环，不断调用"内存不足处理例程"，期望在某次调用之后，获得足够的内存而圆满完成任务。
	//它的返回情况有两种：
	//1.但如果“内存不足处理例程“并未被客端设定，OomMalloc()和OomRealloc()便调用_THROW_BAD_ALLOC, 丢出bad_alloc异常信息.
	//2.开辟内存成功返回。
	static void* OomMalloc(size_t size)
	{
		while (1)
		{
			if (__MallocAllocOomHandler == 0)
				throw bad_alloc();
			__MallocAllocOomHandler();        //？
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
		__TRACE_DEBUG("调用一级空间配置器分配ptr:0x%p size : %u\n",result, n);
		return result;
	}

	static void Deallocate(void *p, size_t n)
	{
		__TRACE_DEBUG("调用一级空间配置器释放ptr:0x%p size:%u\n",p, n);
		free(p);
	}

	//保存原有的函数指针，用于撤销
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
//二级空间配置器(二级空间配置器是为频繁分配小内存而生的一种算法——消除一级空间配置器的外碎片问题。)
template <bool threads, int inst>
class __DefaultAllocTemplate
{
	enum { __ALIGN = 8 };               //小型区块的上调边界
	enum { __MAX_BYTES = 128 };         //小型区块的上限
	enum { __NFREELISTS = __MAX_BYTES / __ALIGN };  //自由链表的个数
public:
	static char* ChunkAlloc(size_t size, size_t& nobjs)
	{
		char* result;
		size_t totalBytes = size*nobjs;
		size_t bytesLeft = _endFree - _startFree;

		//1.内存池有足够大小的空间，则分配申请的空间；
		if (bytesLeft > totalBytes)
		{
			__TRACE_DEBUG("在内存池有足够nobjs:%u个对象\n", nobjs);

			result = _startFree;
			_startFree += totalBytes;
			return result;
		}
		//2.内存池没有足够大小的空间，但是至少还能分配一个节点的空间，则能分多少分多少； 
		else if (bytesLeft > size)
		{
			nobjs = bytesLeft / size;
			totalBytes = nobjs*size;

			__TRACE_DEBUG("在内存池只有nobjs:%u个对象\n", nobjs);

			result = _startFree;
			_startFree += totalBytes;
			return result;
		}
		//3.内存池一个节点都腾不出来，向系统的heap申请2倍于要求大小的空间加上已分配的_heapSize/8的内存到内存池 ，
		//此时，如果内存池剩余有空间，则放到free-list中去；
		else
		{
			size_t bytesToGet = 2 * totalBytes
				+ ROUND_UP(_heapSize >> 4);

			// 将内存池剩余的空间挂到自由链表
			if (bytesLeft > 0)
			{
				size_t index = FREELIST_INDEX(bytesLeft);
				((Obj*)_startFree)->_freeListLink = _freeList[index];
				_freeList[index] = (Obj*)_startFree;
			}

			_startFree = (char*)malloc(bytesToGet);
			__TRACE_DEBUG("内存池没有足够的空间，到系统申请%u个字节\n", bytesToGet);

			//4.如果向heap申请空间失败，那么只能看free-list中更大的节点是否有可用空间了，
			//有则用之，同时递归调用自身修正ChunkAlloc(size,nobjs)； 
			if (_startFree == NULL)
			{
				// 到更大的自由链表
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

				// 5.如果free-list也没有可用节点了，那么转向第一级空间配置器申请空间,
				//再不行，第一级空间配置器就抛出bad_alloc异常.
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
		// 到内存池申请内存，切分内存块挂到自由链表
		size_t nobjs = 20;     //默认申请块数
		char* chunk = ChunkAlloc(size, nobjs);

		//1.只有一块返回给调用者，链表无新节点
		if (nobjs == 1)
		{
			return chunk;
		}

		//2.不止一块
		Obj* cur = (Obj*)(chunk + size);           //cur指向新申请的第二个小额区块（第一个给申请者），size为一个小额区块的大小
		size_t index = FREELIST_INDEX(size);      //找到所属链表
		_freeList[index] = cur;                   //对新的区块建链表
		for (int i = 0; i < nobjs - 2; ++i)       //将新链表之间的节点串接起来
		{
			Obj* next = (Obj*)((char*)cur + size);
			cur->_freeListLink = next;

			cur = next;
		}
		cur->_freeListLink = NULL;

		return chunk;       //返回给申请者
	}

	static void* Allocate(size_t n)
	{
		__TRACE_DEBUG("调用二级空间配置分配内存：%u\n", n);

		//1. 大于128，则调用一级空间配置器
		if (n > __MAX_BYTES)
		{
			return __MallocAllocTemplate<0>::Allocate(n);
		}
		//寻找16个链表中适当的自由链表指向的小额区块
		size_t index = FREELIST_INDEX(n);
		//2.链表没有可用的块调用Refill申请
		if (_freeList[index] == NULL)
		{
			return Refill(ROUND_UP(n));
		}
		//3.有合适的.调整链表指向
		else
		{
			Obj* first = _freeList[index];
			_freeList[index] = first->_freeListLink;

			__TRACE_DEBUG("在自由链表取内存块：0x%p\n", first);

			return first;
		}
	}

	static void Deallocate(void* p, size_t size)
	{
		__TRACE_DEBUG("二级空间配置器释放空间0x%p, %u\n", p, size);

		//1.大于128使用第一级配置器
		if (size > __MAX_BYTES)
		{
			__MallocAllocTemplate<0>::Deallocate(p, size);
			return;
		}

		//2.找到所属链表
		size_t index = FREELIST_INDEX(size);
		//调整指针指向
		((Obj*)p)->_freeListLink = _freeList[index];
		_freeList[index] = (Obj*)p;
	}

protected:
	//把分配的内存大小提升一个数量级（+7，每间隔8为一个数量级），
	//然后除以8，可以算到要找的内存块是第几块了，数组从0开始所以再减一
	static  size_t FREELIST_INDEX(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) / __ALIGN - 1);
	}

	//得到所需内存块大小的向上对齐数。在自由链表中，我们的内存块大小总是8的倍数，
	//但是并不是每次所需内存大小都是8的倍数。所以我们就要取比所需大小大或相等的内存块，
	//这就是向上取整。&~(ALIGN - 1)相当于将低8位置0，只取高8位，高8位总是8的倍数，
	static size_t ROUND_UP(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) & ~(__ALIGN - 1));
	}

	//自由链表
	//这个结构可以看做是从一个内存块中抠出4个字节大小来，
	//当这个内存块空闲时，它存储了下个空闲块，
	//当这个内存块交付给用户时，它存储的时用户的数据
	union Obj
	{
		union Obj* _freeListLink;  //指向下一个内存对象
		char client_data[1];      //
	};
	static Obj* _freeList[__NFREELISTS];
	
	//内存池
	static char* _startFree;    //水位线
	static char* _endFree;      //池底
	static size_t _heapSize;    //二级空间配置器总共申请的多少字节的内存
};

template<bool threads, int inst>
char* __DefaultAllocTemplate<threads, inst>::_startFree = NULL;

template<bool threads, int inst>
char* __DefaultAllocTemplate<threads, inst>::_endFree = NULL;

template<bool threads, int inst>
size_t __DefaultAllocTemplate<threads, inst>::_heapSize = 0;

//typename：告诉编译器DefaultAllocTemplate<threads, inst>是一个类型
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















////函数指针
//#include<stdio.h>
//void hello(void) 
//{ 
//	printf("你好!\n");
//}
//void bye(void) 
//{ 
//	printf("再见！\n"); 
//}
//void ok(void) 
//{ 
//	printf("好的！\n"); 
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