/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "btAlignedAllocator.h"

#if defined(WII)
namespace ork {
namespace wii {
void* MEM2Alloc( int isize );
}}
#endif

int gNumAlignedAllocs = 0;
int gNumAlignedFree = 0;
int gTotalBytesAlignedAllocs = 0;//detect memory leaks


#if defined (BT_HAS_ALIGNED_ALLOCATOR)
#include <malloc.h>
static void *btAlignedAllocDefault(size_t size, int alignment)
{
	return _aligned_malloc(size, (size_t)alignment);
}

static void btAlignedFreeDefault(void *ptr)
{
	_aligned_free(ptr);
}
#elif defined(__CELLOS_LV2__)
#include <stdlib.h>

static inline void *btAlignedAllocDefault(size_t size, int alignment)
{
	return memalign(alignment, size);
}

static inline void btAlignedFreeDefault(void *ptr)
{
	free(ptr);
}
#else
static inline void *btAlignedAllocDefault(size_t size, int alignment)
{
  void *ret;
  char *real;
  unsigned long offset;

  real = (char *)malloc(size + sizeof(void *) + (alignment-1));
  if (real) {
    offset = (alignment - (unsigned long)(real + sizeof(void *))) & (alignment-1);
    ret = (void *)((real + sizeof(void *)) + offset);
    *((void **)(ret)-1) = (void *)(real);
  } else {
    ret = (void *)(real);
  }
  return (ret);
}

static inline void btAlignedFreeDefault(void *ptr)
{
  void* real;

  if (ptr) {
    real = *((void **)(ptr)-1);
    free(real);
  }
}
#endif

#if defined(WII)

#include <revolution/mem.h>


static MEMHeapHandle GetBulletHeap()
{
	static MEMHeapHandle BulletHeap = 0;

	static bool binit = true;

	if( binit )
	{
		static const int ksize = 16<<20;
		void* pbulletheap = ork::wii::MEM2Alloc( ksize );
		BulletHeap = MEMCreateExpHeap(pbulletheap,ksize);
		binit = false;
	}

	return BulletHeap;
}
#endif




static void *btAllocDefault(size_t size)
{
#if defined(WII)
	return MEMAllocFromExpHeap( GetBulletHeap(), size );
#else
	return malloc(size);
#endif
}

static void btFreeDefault(void *ptr)
{
#if defined(WII)
	MEMFreeToExpHeap( GetBulletHeap(), ptr );
#else
	free(ptr);
#endif
}

static btAlignedAllocFunc *sAlignedAllocFunc = btAlignedAllocDefault;
static btAlignedFreeFunc *sAlignedFreeFunc = btAlignedFreeDefault;
static btAllocFunc *sAllocFunc = btAllocDefault;
static btFreeFunc *sFreeFunc = btFreeDefault;

void btAlignedAllocSetCustomAligned(btAlignedAllocFunc *allocFunc, btAlignedFreeFunc *freeFunc)
{
  sAlignedAllocFunc = allocFunc ? allocFunc : btAlignedAllocDefault;
  sAlignedFreeFunc = freeFunc ? freeFunc : btAlignedFreeDefault;
}

void btAlignedAllocSetCustom(btAllocFunc *allocFunc, btFreeFunc *freeFunc)
{
  sAllocFunc = allocFunc ? allocFunc : btAllocDefault;
  sFreeFunc = freeFunc ? freeFunc : btFreeDefault;
}

#ifdef BT_DEBUG_MEMORY_ALLOCATIONS
//this generic allocator provides the total allocated number of bytes
#include <stdio.h>

void*   btAlignedAllocInternal  (size_t size, int alignment,int line,char* filename)
{
 void *ret;
 char *real;
 unsigned long offset;

 gTotalBytesAlignedAllocs += size;
 gNumAlignedAllocs++;

 
 real = (char *)sAllocFunc(size + 2*sizeof(void *) + (alignment-1));
 if (real) {
   offset = (alignment - (unsigned long)(real + 2*sizeof(void *))) &
(alignment-1);
   ret = (void *)((real + 2*sizeof(void *)) + offset);
   *((void **)(ret)-1) = (void *)(real);
       *((int*)(ret)-2) = size;

 } else {
   ret = (void *)(real);//??
 }

 printf("allocation#%d at address %x, from %s,line %d, size %d\n",gNumAlignedAllocs,real, filename,line,size);

 int* ptr = (int*)ret;
 *ptr = 12;
 return (ret);
}

void    btAlignedFreeInternal   (void* ptr,int line,char* filename)
{

 void* real;
 gNumAlignedFree++;

 if (ptr) {
   real = *((void **)(ptr)-1);
       int size = *((int*)(ptr)-2);
       gTotalBytesAlignedAllocs -= size;

	   printf("free #%d at address %x, from %s,line %d, size %d\n",gNumAlignedFree,real, filename,line,size);

   sFreeFunc(real);
 } else
 {
	 printf("NULL ptr\n");
 }
}

#else //BT_DEBUG_MEMORY_ALLOCATIONS

void*	btAlignedAllocInternal	(size_t size, int alignment)
{
	gNumAlignedAllocs++;
  void* ptr;
#if defined (BT_HAS_ALIGNED_ALLOCATOR) || defined(__CELLOS_LV2__)
	ptr = sAlignedAllocFunc(size, alignment);
#else
  char *real;
  unsigned long offset;

  real = (char *)sAllocFunc(size + sizeof(void *) + (alignment-1));
  if (real) {
    offset = (alignment - (unsigned long)(real + sizeof(void *))) & (alignment-1);
    ptr = (void *)((real + sizeof(void *)) + offset);
    *((void **)(ptr)-1) = (void *)(real);
  } else {
    ptr = (void *)(real);
  }
#endif  // defined (BT_HAS_ALIGNED_ALLOCATOR) || defined(__CELLOS_LV2__)
//	printf("btAlignedAllocInternal %d, %x\n",size,ptr);
	return ptr;
}

void	btAlignedFreeInternal	(void* ptr)
{
	if (!ptr)
	{
		return;
	}

	gNumAlignedFree++;
//	printf("btAlignedFreeInternal %x\n",ptr);
#if defined (BT_HAS_ALIGNED_ALLOCATOR) || defined(__CELLOS_LV2__)
	sAlignedFreeFunc(ptr);
#else
  void* real;

  if (ptr) {
    real = *((void **)(ptr)-1);
    sFreeFunc(real);
  }
#endif  // defined (BT_HAS_ALIGNED_ALLOCATOR) || defined(__CELLOS_LV2__)
}

#endif //BT_DEBUG_MEMORY_ALLOCATIONS

