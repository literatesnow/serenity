// The MemLeakCheck source code is by Robert Walker
// support@tunesmithy.co.uk

// To check for updates visit
// http://tunesmithy.co.uk/memleakcheck/index.htm

// You are free to do what you like with it including
// using it in commercial programs, and no need to publish
// changes you make. 

// Please keep this notice in your source code if practical
// so other developers who happen on it know where to go to 
// find out more and check for updates.

// I'll be interested to hear about any improvements
// you make :-).

// Robert

#include "MemLeakCheck.h"

#ifdef _DEBUG

#undef malloc
#undef calloc
#undef strdup
#undef realloc
#undef free
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
typedef unsigned char byte;

#ifdef cl_mem_leak_diagnostics

struct _malloc_info
{
 int nrecord;// needs to be first
 const char *szFile;
 int iLine;
 int size;
};
typedef struct _malloc_info MallocInfo;

#else

typedef int MallocInfo;

#endif

#define MALLOC_INFO_SIZE sizeof(MallocInfo)

#define POINTER_ALLOC_INCR 1024

#ifdef MLC_USE_THREAD_LOCAL_VARIABLES
__declspec(thread) MallocInfo **pointers_allocated;
__declspec(thread) int npointers_allocated;
__declspec(thread) int max_pointers_allocated;
__declspec(thread) int ntotal_allocations;
__declspec(thread) double dmem_alloc;
 #ifdef _DEBUG 
  #ifdef cl_mem_leak_diagnostics
   __declspec(thread) static MallocInfo was_MI;
  #endif
 #endif
#else
MallocInfo **pointers_allocated;
int npointers_allocated;
int max_pointers_allocated;
int ntotal_allocations;
double dmem_alloc; // in case it goes out of range for longs
                   // - this is the total amount in all allocations made - 
                   // so if memory is allocted, freed, re-allocated again it adds to this each time

 #ifdef _DEBUG 
  #ifdef cl_mem_leak_diagnostics
   static MallocInfo was_MI; // so you can look at it in debugger 
  #endif
 #endif
#endif

#ifdef _DEBUG
 // #define DEBUG_P // Define this if you want to make sure that
 // the position of the pointer in the pointers_allocated[] array
 // remains unchanged throughout all the allocations
 // - means this array will gradually get larger as you
 // free and allocate more pointers - only 
 // needed if you step into this code to find out more
 // about what is happening in your allocations and frees
#endif

static int ntest_compactify_pointers_at=1024;

#ifndef cl_mem_leak_check

 #undef MemLeakCheckAndFree
 #undef SetMemLeakWarnProc
 #undef SetMemLeakLogProc
 #undef SetMemLeakAnticipateFreeProc

#endif

#ifndef cl_mem_leak_diagnostics

 #undef SetMemLeakWarnProc
 #undef SetMemLeakLogProc
 #undef SetMemLeakAnticipateFreeProc

#endif

#ifdef BUILD_MEMLEAK_CHECK_AS_DLL

// calling proc can set these to sidestep the problems involved
// in passing pointers to a dll that uses a different version of the
// C Run Time (CRT)
MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B set_mlc_malloc
(void *(__cdecl *new_mlc_malloc)(size_t size));

MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B set_mlc_calloc
(void *(__cdecl *new_mlc_calloc)(size_t num,size_t size));

MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B set_mlc_realloc
(void *(__cdecl *new_mlc_realloc)(void *p,size_t size));

MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B set_mlc_free
(void (__cdecl *new_mlc_free)(void *p));

MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B set_mlc_memcpy
(void *(__cdecl *new_mlc_memcpy)(void *dest,void *src,size_t count));

MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B set_mlc_strlen
(size_t (__cdecl *new_mlc_strlen)(const char *string));

MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B set_mlc_strcpy
(char *(__cdecl *new_mlc_strcpy)(char *strDestination,const char *strSource ));


#define MAKE_PROC_DEC_AND_SET_PROC(type,proccname,args)\
type (__cdecl *mlc_##proccname)args=proccname;\
\
MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B set_mlc_##proccname\
(type (__cdecl *new_mlc_##proccname)args)\
{\
 mlc_##proccname=new_mlc_##proccname;\
}\

MAKE_PROC_DEC_AND_SET_PROC(void *,malloc,(size_t size))
MAKE_PROC_DEC_AND_SET_PROC(void *,calloc,(size_t num,size_t size))
MAKE_PROC_DEC_AND_SET_PROC(void *,realloc,(void *p,size_t size))
MAKE_PROC_DEC_AND_SET_PROC(void,free,(void *p))
MAKE_PROC_DEC_AND_SET_PROC(void *,memcpy,(void *psrc,void *pdest,size_t size))
MAKE_PROC_DEC_AND_SET_PROC(char *,strcpy,(char *strDestination,const char *strSource))
MAKE_PROC_DEC_AND_SET_PROC(size_t,strlen,(const char *string))

#else

#define mlc_malloc malloc
#define mlc_free free
#define mlc_calloc calloc
#define mlc_realloc realloc
#define mlc_strdup strdup
#define mlc_strcpy strcpy
#define mlc_strlen strlen
#define mlc_memcpy memcpy

#endif


#ifdef MLC_MAKE_WINDOWS_THREAD_SAFE

#include <windows.h>

long mlc_thread_lock;

#define SPIN_AND_GET_ACCESS \
{\
 for(;;)\
 {\
  if(InterlockedExchange(&mlc_thread_lock,1)!=0)\
   /* prior value for mlc_thread_lock was 1 so another thread*/\
   /*still has access*/\
   Sleep(0);/*sleep for the rest of this time slice - efficient busy wait\
  else\
   /*prior value was 0 and it is now set to 1 to*/\
   /*indicate that this thread has access*/\
   /*Now everything we do is atomic*/\
   break;\
 }\
}

#define CEDE_ACCESS  {InterlockedExchange(&mlc_thread_lock, 0);}

#else
#define SPIN_AND_GET_ACCESS
#define CEDE_ACCESS
#endif


void DefaultMemLeakWarnProc(char *szmsg)
{
 fprintf(stderr,"%s",szmsg);
}

void (__cdecl *MemLeakWarnProc)(char *szmsg)=DefaultMemLeakWarnProc;

MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B SetMemLeakWarnProc(void (__cdecl *NewMemLeakWarnProc)(char *szmsg))
{
 MemLeakWarnProc=NewMemLeakWarnProc;
}

void (__cdecl *MemLeakLogProc)(char *szmsg)=NULL;

MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B SetMemLeakLogProc(void (__cdecl *NewMemLeakLogProc)(char *szmsg))
{
 // This is called if one has a potentially long list of messages
 // to show - an appropriate implementation in a Windows program
 // would be to log them to disk
 MemLeakLogProc=NewMemLeakLogProc;
}


void DefaultMemLeakAnticipateFreeProc(char *szFile,int nLine)
{
 int n=nLine;// just to remove the level 4 warning errors if they are
 // unreferenced
 char *sz=szFile;
 sz=sz;
 n=n;
 /** This is one way to do it if you need this feature: 
 {
  FILE *fp=fopen("most _recent_mem_free","w");
  if(fp)
  {
   fprintf(fp,"Most Recent Free at: File: %s, Line %d",szFile,nLine);
   fclose(fp);
  }
 }
 **/
}

void (__cdecl *MemLeakAnticipateFreeProc)(char *szFile,int nLine)=DefaultMemLeakAnticipateFreeProc;

MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B SetMemLeakAnticipateFreeProc(void( __cdecl *NewMemLeakAnticipateFreeProc)(char *szFile,int nLine))
{
 MemLeakAnticipateFreeProc=NewMemLeakAnticipateFreeProc;
}

#ifdef cl_mem_leak_check

#ifdef cl_mem_leak_diagnostics
 #ifdef cl_qsort_pointers
int CompareMallocInfo(const MallocInfo * *ppmi1
                            , const MallocInfo * *ppmi2)
{
 if(ppmi1==NULL||*ppmi1==NULL) return 2;
 if(ppmi2==NULL||*ppmi2==NULL) return 1;
 if((*ppmi1)->szFile[0]!=(*ppmi2)->szFile[0])
  return (*ppmi1)->szFile[0]-(*ppmi2)->szFile[0];
 if(strcmpi((*ppmi1)->szFile,(*ppmi2)->szFile))
  return strcmpi((*ppmi1)->szFile,(*ppmi2)->szFile);
 if((*ppmi1)->iLine!=(*ppmi2)->iLine)
  return (*ppmi1)->iLine-(*ppmi2)->iLine;
 return (*ppmi1)->size-(*ppmi2)->size;
}
 #endif
#endif

MEMLEAK_CHECK_WM_DLL_A int MEMLEAK_CHECK_WM_DLL_B MemLeakCheckAndFree(void)
{
 // returns number of pointers freed, or 0 if no leak is found
 int i=0;
 int npointers_freed=0;
#ifdef cl_mem_leak_diagnostics
 char info_line_shown=0;
 char *szFile=NULL;
 long lmem_leak=0;
 long lmem_leak_debug_info=0;
 int iLine=-1,size=-1,nidentical=0;
 void (__cdecl *LocalMemLeakLogProc)(char *szmsg)=MemLeakLogProc?MemLeakLogProc:MemLeakWarnProc;
#endif
 SPIN_AND_GET_ACCESS
 if(!pointers_allocated)
  return 0;
#ifdef cl_mem_leak_diagnostics
 #ifdef cl_qsort_pointers
 // Sort the pointer array according to file, line number then size
 qsort (&pointers_allocated[0], npointers_allocated,sizeof(MallocInfo *),CompareMallocInfo) ;
 #endif
#endif
 for(i=0;i<npointers_allocated;i++)
  if(pointers_allocated[i])
  {
#ifdef cl_mem_leak_diagnostics
   MallocInfo *pmi=(MallocInfo *)pointers_allocated[i];
   char szmsg[10240];
   sprintf(szmsg,"File: %s, line %d, size %d\n"
          ,pmi->szFile,pmi->iLine,pmi->size
          );
   lmem_leak_debug_info+=MALLOC_INFO_SIZE;
   lmem_leak+=pmi->size;
   if(!info_line_shown)
    LocalMemLeakLogProc("Memory Leak!\n");
   info_line_shown=1;
   if(szFile==pmi->szFile&&iLine==pmi->iLine&&size==pmi->size)
    nidentical++;
   else
   {
    if(nidentical)
    {
     char szmsg2[100];
     sprintf(szmsg2,"Another %d pointers - same line, and size\n"
            ,nidentical
            );
     LocalMemLeakLogProc(szmsg2);
    }
    nidentical=0;
    LocalMemLeakLogProc(szmsg);
    szFile=(char *)pmi->szFile;
    iLine=pmi->iLine;
    size=pmi->size;
   }
#endif
   mlc_free(pointers_allocated[i]);
   pointers_allocated[i]=NULL;
   npointers_freed++;
  }
#ifdef cl_mem_leak_diagnostics
  if(nidentical)
  {
   char szmsg2[100];
   sprintf(szmsg2,"Another %d pointers allocated - same line, and same size\n"
          ,nidentical
          );
   LocalMemLeakLogProc(szmsg2);
  }
  if(info_line_shown)
  {
   char szmsg[1024];
   lmem_leak_debug_info+=max_pointers_allocated*sizeof(void *);
   sprintf
    (szmsg,"Memory leak detected!:\n"
    "%d pointers, %d bytes (%.2f Kb)\n%s"
    "Extra memory for debug info: %d bytes (%.2f Kb)\nAverage per pointer %g bytes\n\n"
    "Total allocations in session including ones freed normally:\n"
    "%d pointers, %.0f bytes (%.2f Kb)\n" 
    ,npointers_freed,lmem_leak,(float)lmem_leak/1024.0
    ,LocalMemLeakLogProc==MemLeakWarnProc?"":"See the memory leak log for details\n\n"
    ,lmem_leak_debug_info,(float)lmem_leak_debug_info/1024.0,(float)lmem_leak_debug_info/npointers_freed
    ,ntotal_allocations,dmem_alloc,dmem_alloc/1024.0
    );
   MemLeakWarnProc(szmsg);
  }
#endif
 npointers_allocated=max_pointers_allocated=0;
 mlc_free(pointers_allocated);
 pointers_allocated=NULL;
 CEDE_ACCESS
 return npointers_freed;
}

void CompactifyPointersArray(void)
{
 int i=0;
 int ipos=0;
#ifdef DEBUG_P
 return;
#endif
 for(i=0;i<npointers_allocated;i++)
 {
  if(pointers_allocated[i])
  {
   if(ipos==i)
    ipos++;
   else
   {
    mlc_memcpy(pointers_allocated[i],&ipos,sizeof(int));
     // works for cl_mem_leak_diagnostics too, 
    // because nrecord is the first element of the struct
    pointers_allocated[ipos++]=pointers_allocated[i];
   }
  }
 }
 npointers_allocated=ipos;
 ntest_compactify_pointers_at=ntotal_allocations+max(1024,npointers_allocated/2);
}

#ifdef cl_mem_leak_diagnostics
void AddPointerAllocated(int new_record_number,void *pinfo,size_t size,const char *szFile,int iLine)
#else
void AddPointerAllocated(int new_record_number,void *pinfo,size_t size)
#endif
{
 if(!pinfo)
  return;
 SPIN_AND_GET_ACCESS
 {
  int record_number=new_record_number>=0?new_record_number:npointers_allocated;
  record_number=min(record_number,npointers_allocated);
  if(record_number==npointers_allocated)
  {
   if(npointers_allocated+1>max_pointers_allocated)
   {
    int new_max_pointers_allocated=max_pointers_allocated+POINTER_ALLOC_INCR;
    MallocInfo **new_pointers_allocated=(MallocInfo**)mlc_realloc((void *)pointers_allocated,new_max_pointers_allocated*sizeof(void *));
    if(new_pointers_allocated)
    {
     pointers_allocated=new_pointers_allocated;
     max_pointers_allocated=new_max_pointers_allocated;
    }
    else
     return;
   }
  }
  if(record_number<npointers_allocated)
  if(pointers_allocated[record_number])
   record_number=record_number;// shouldn't happen
 #ifdef cl_mem_leak_diagnostics
  {
   MallocInfo mi;
   mi.nrecord=record_number;
   mi.szFile=szFile;
   mi.iLine=iLine;
   mi.size=size;
   mlc_memcpy(pinfo,&mi,sizeof(MallocInfo));
  #ifdef _DEBUG 
   was_MI=mi;
  #endif
  }
 #else
  mlc_memcpy(pinfo,&record_number,sizeof(int));
 #endif
  pointers_allocated[record_number]=pinfo;
  if(record_number==npointers_allocated)
   npointers_allocated++;
  ntotal_allocations++;
  dmem_alloc+=size;
  if(ntotal_allocations>=ntest_compactify_pointers_at)
  {
   CompactifyPointersArray();
  }
 }
 CEDE_ACCESS
}

#ifdef cl_mem_leak_diagnostics
char RemovePointerAllocated(void *pinfo,char *szFile,int nLine)
#else
char RemovePointerAllocated(void *pinfo)
#endif
{
 int i=0;
 if(!pinfo)
 {
  char szmsg[1024];
  sprintf
   (szmsg,"Attempt to free Null pointer - ignored.\n"
#ifdef cl_mem_leak_diagnostics
   "At: %s: Line %d\n"
   ,szFile,nLine
#endif
   );
  MemLeakWarnProc(szmsg);
  return 0;
 }
#ifdef cl_mem_leak_diagnostics
 MemLeakAnticipateFreeProc(szFile,nLine);
 #ifdef _DEBUG 
  mlc_memcpy(&was_MI,pinfo,sizeof(MallocInfo));
 #endif
#endif
 mlc_memcpy(&i,pinfo,sizeof(int));
 SPIN_AND_GET_ACCESS
 if(i>=0&&i<npointers_allocated&&pointers_allocated[i]==pinfo)
 {
  pointers_allocated[i]=NULL;
#ifndef DEBUG_P
  if(i==npointers_allocated-1)
  {

   for(;i>=0;i--)
   if(pointers_allocated[i])
   {npointers_allocated=i+1;break;}
   
  }
#endif
  CEDE_ACCESS
  return 1;
 }
 CEDE_ACCESS
#ifdef MLC_WARN_ON_ATTEMPT_TO_FREE_WILD_POINTER
 {
  char szmsg[1024];
  sprintf
   (szmsg,"Wild pointer!\n"
   "Not allocated, or freed already\n"
   "Attempt made to free it - ignored.\n"
#ifdef cl_mem_leak_diagnostics
   "At: %s: Line %d\n"
   ,szFile,nLine
#endif
   );
  MemLeakWarnProc(szmsg);
 }
#endif
 return 0;
}

#ifdef cl_mem_leak_diagnostics
MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_realloc(void *p,size_t size,char *szFile,int iLine)
#else
MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_realloc(void *p,size_t size)
#endif
{
 void *pret=NULL;
 void *pinfo=p==NULL?NULL:((byte *)p-MALLOC_INFO_SIZE);
 MallocInfo *pmi=(MallocInfo *)pinfo;
#ifdef cl_mem_leak_diagnostics
 int i=pmi?pmi->nrecord:-1;
 #ifdef _DEBUG
 was_MI=pmi?*pmi:was_MI;
 #endif
#else
 int i=pmi?(int)*((int *)pmi):-1;
#endif
 {
  void *pnew=mlc_realloc(pinfo,size+MALLOC_INFO_SIZE);
  if(pnew)
  {
   if(pmi)
   {
    // Note - pmi is no longer a valid pointer, so we can't use RemovePointerAllocated(..)
    // so remove it here using the record number obtained before it was reallocated
    SPIN_AND_GET_ACCESS
    if(i>=0&&i<npointers_allocated)
    {
     if(pointers_allocated[i]==pinfo)
      pointers_allocated[i]=NULL;
    }
    CEDE_ACCESS
   }
#ifdef cl_mem_leak_diagnostics
   AddPointerAllocated(i,pnew,size,szFile,iLine);
#else
   AddPointerAllocated(i,pnew,size);
#endif
   pret=(void *)((byte *)pnew+MALLOC_INFO_SIZE);
  }
 }
 return pret;
}


#ifdef cl_mem_leak_diagnostics
MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_malloc(size_t size,char *szFile,int iLine)
#else
MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_malloc(size_t size)
#endif
{
 void *pinfo=mlc_malloc(size+MALLOC_INFO_SIZE);
 void *pret=NULL;
 if(pinfo)
 {
#ifdef cl_mem_leak_diagnostics
  AddPointerAllocated(-1,pinfo,size,szFile,iLine);
#else
  AddPointerAllocated(-1,pinfo,size);
#endif
  pret=(byte *)pinfo+MALLOC_INFO_SIZE;
 }
 return pret;
}

#ifdef cl_mem_leak_diagnostics
MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_calloc(size_t num,size_t size,char *szFile,int iLine)
#else
MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_calloc(size_t num,size_t size)
#endif
{
 void *pinfo=mlc_calloc(1,size*num+MALLOC_INFO_SIZE);
 void *pret=NULL;
 if(pinfo)
 {
#ifdef cl_mem_leak_diagnostics
  AddPointerAllocated(-1,pinfo,size,szFile,iLine);
#else
  AddPointerAllocated(-1,pinfo,size);
#endif
  pret=(byte *)pinfo+MALLOC_INFO_SIZE;
 }
 return pret;
}

#ifdef cl_mem_leak_diagnostics
MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B xz_free(void *p,char *szFile,int iLine)
#else
MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B xz_free(void *p)
#endif
{
 if(p)
 {
  byte *pinfo=((byte *)p-MALLOC_INFO_SIZE);
#ifdef cl_mem_leak_diagnostics
  if(RemovePointerAllocated(pinfo,szFile,iLine))
#else
  if(RemovePointerAllocated(pinfo))
#endif
  mlc_free(pinfo);
 }
}

#ifdef cl_mem_leak_diagnostics
MEMLEAK_CHECK_WM_DLL_A char * MEMLEAK_CHECK_WM_DLL_B xz_strdup(const char *src,char *szFile,int iLine)
#else
MEMLEAK_CHECK_WM_DLL_A char * MEMLEAK_CHECK_WM_DLL_B xz_strdup(const char *src)
#endif
{\
 int len= mlc_strlen(src);
#ifdef cl_mem_leak_diagnostics
 byte *dest=xz_malloc(len+1,szFile,iLine);
#else
 byte *dest=xz_malloc(len+1);
#endif
 if(dest)
  mlc_strcpy((char *)dest,src);\
 return (char *)dest;
}

#endif

#endif