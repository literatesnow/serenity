/* The MemLeakCheck source code is by Robert Walker
 * support@tunesmithy.co.uk
 *
 * To check for updates visit
 * http://tunesmithy.co.uk/memleakcheck/index.htm
 */

#ifndef _DEBUG

#define _malloc_ malloc
#define _strdup_ strdup
#define _realloc_ realloc
#define _free_ free

#else

void MemLeakLog(char *szmsg);

#ifndef MEMLEAKCHECK_H_INCLUDED
#define MEMLEAKCHECK_H_INCLUDED

/* Include this header after the stdio, stdlib malloc etc ones in your program.
 * If you use the MSVC memory leak check normally and are temporarily overriding
 * it, include this after the #include <crtdbg.h> 
 *
 * Uncomment these as needed - for info about them see below or the web site.
 */

#define cl_mem_leak_check 
#define cl_mem_leak_diagnostics

// #define cl_qsort_pointers // sorts the diagnostic reports by file, line and size
                             // - may be slow if you have many pointers 

// #define LINK_MEMLEAK_CHECK_AS_DLL
//#define BUILD_MEMLEAK_CHECK_AS_DLL 
//#define MLC_MAKE_WINDOWS_THREAD_SAFE
// #define MLC_USE_THREAD_LOCAL_VARIABLES
// #define code_uses_redefined_malloc

#define MLC_WARN_ON_ATTEMPT_TO_FREE_WILD_POINTER 
 /* In diagnostic mode, this gives a warning if you free a pointer which you never
  * allocated, or allocated and have already freed
  */


#ifdef cl_mem_leak_diagnostics
 #define cl_mem_leak_check 
#endif


#ifdef MLC_USE_THREAD_LOCAL_VARIABLES
 #undef MLC_MAKE_WINDOWS_THREAD_SAFE 
 // not needed as each thread has its own version of the pointers array 
#endif


#ifdef BUILD_MEMLEAK_CHECK_AS_DLL
 #undef LINK_MEMLEAK_CHECK_AS_DLL 
#endif

#ifdef INCLUDE_MEMLEAK_CHECK_AS_DLL // older version
 #define LINK_MEMLEAK_CHECK_AS_DLL 
#endif


/* If using MemLeakCheck as a dll, define
 * LINK_MEMLEAK_CHECK_AS_DLL
 *
 * The settings must also match the dll.
 *
 * for memleakcheckta.dll (thread safe auto free only dll)
 * define cl_mem_leak_check 
 *
 * for memleakcheckt.dll (thread safe diagnostics dll)
 * you must also define cl_mem_leak_diagnostics
 *
 *
 * define code_uses_redefined_malloc to use the 
 * version where you 
 * do a search and replace of source code
 * of malloc to _malloc_, calloc to _calloc_
 * realloc to _realloc_ and strdup to _strdup_
 * - advantage, don't need to make sure this header
 * is included after all the standard libraries
 * when including it as source code in your build.
 *
 * define cl_MSVC_debug_mem_leak_tests 
 * if you want to revert to the MSVC debug version
 * - not needed if you use it as a dll
 *
 * define cl_mlc_qsort_pointers if you want the pointers
 * sorted according to the files and line numbers where
 * the allocation occured.
 * the qsort is done just before the diagnostics report (if in diagnostic mode)
 * It will be slow if you have many unfreed pointers, e.g. thousands
 * of them. Sorts the pointers by file, then line number, then size
 *
 *
 * To switch on the automatic freeing of memory on exit
 * define cl_mem_leak_check 
 *
 * To also show diagnostics, 
 * define cl_mem_leak_diagnostics
 *
 * To free any remaining unfreed memory (e.g. on exit)
 * call MemLeakCheckAndFree  * in your program. 
 * This is also the routine that hows the diagnostics report if
 * any memory needed to be freed.
 *
 * The default diagnostics warnings proc writes to stderr, which is probably going to
 * be fine for many Console apps but it's not suitable for Windows programs
 *
 * To set your own warnings proc, use  SetMemLeakWarnProc(proc)
 *
 * Here is a windows version which just shows the warnings in your debugger output window:

#ifdef cl_mem_leak_diagnostics

MemLeakWarnProc(char *szmsg)
{
 OutputDebugString(szmsg); 
}

#endif
...
SetMemLeakWarnProc(MemLeakWarnProc);
...

 *
 *And here is a version which shows a message box
 *
 
#ifdef cl_mem_leak_diagnostics

void MemLeakWarnProc(char *szmsg)
{
 MessageBox(NULL,szmsg,"Memory leak detected!",MB_ICONEXCLAMATION);
}
#endif
...
SetMemLeakWarnProc(MemLeakWarnProc);
...

 *
 * When the pointers get freed at the end you may get a whole succession
 * of warnings. Again that works fine for a console app though if there 
 * are many one may want to log them to a file instead. It doesn't work in 
 * well in Windows gui programs and you will probably want to log them to a file
 * in that case, or send them to your debugger output window (see above).
 *
 * To log them to a file, set a MemLeakLog proc which could be done like this for a windows app:
 *

#ifdef cl_mem_leak_diagnostics
void MemLeakLogProc(char *szmsg)
{
 static char init;
 char *szLog="MemLeakLog.txt";
 FILE *fp=fopen(szLog,"a");
 if(!init)
 {
  char szinfo[1024];
  sprintf(szinfo,"Memory leak(s) detected and logged to %s - see this for the details"
         ,szLog
         );
  MessageBox(NULL,szinfo,"Memory leak(s) detected!",MB_ICONEXCLAMATION);
 }
 if(fp)
 {
  if(!init)
  {
   // start a new section in the memory diagnostics log - captioned 
   // with the current date and time
   struct tm *today;
   char tmpbuf[128];
   time_t time_now;
   time( &time_now );
   today = localtime( &time_now );
   strftime( tmpbuf, 128,"%#I %p %A, %B %d, %Y %Z",today);
   fprintf
    (fp
    ,"\n"
     "************************************************************\n"
     "%s\n"
     "************************************************************\n"
     "\n\n"
    ,tmpbuf
    );
  }
  fprintf(fp,"%s",szmsg);
  fclose(fp);
 }
 init=1;
}
#endif

...
SetMemLeakLogProc(MemLeakLogProc);
...

 * If you don't set a MemLeakLogProc then your MemLeakWarnProc gets used for it
 *
 * You can also use SetMemLeakAnticipateFreeProc
 * - this just gets called with the file and line number
 * before every free that occurs in your program
 * - but may be useful sometimes in debugging
 * - can be used to print a log of all the frees
 * so you can easily find the last one  that 
 * was just about to be called before a crash
 *
 * As it is this code isn't thread safe. However if you are using it
 * in windows you can make it thread safe by defining
 * MAKE_WINDOWS_THREAD_SAFE
 * and link with kernel32.lib - as you will do anyway probably 
 * if you build it in a Windows compiler.
 *
 * This works by using a busy wait on InterlockedExchange(..)
 * This is fast if you normally expect to be able to gain access
 * or to wait only a short time. Even if there is thread contention
 * e.g. because one thread is allocating a large area of memory,
 * the busy wait only does a few assembly language isntructions before
 * ceding the rest of its time slice back to the operating system
 * using Sleep(0);
 *
 * If you want to not use this any more and change to
 * the MSVC memory leak diagnostics with malloc_dbg
 * etc, but retain this header, then use define cl_MSVC_debug_mem_leak_tests
 *
 * You can also use it as a dll - which doesn't require
 * any changes to your source code apart from including
 *.the header and linking to the dll.
 *
 * To use it as a dll, you need to define
 * LINK_MEMLEAK_CHECK_AS_DLL
 * in the version of the header used in your program.
 *
 * The effect of this is that in your program
 * wherever it has a call to malloc, calloc, realloc
 * strdup or free, then this will be replaced by calls
 * to the routines xz_malloc etc in the dll. Those then
 * in turn will call the real malloc, calloc etc.
 * as needed
 *
 * You can also set your own routines for allocation and
 * reallocation etc.
 *
 * Place the following code at the end of one of your source files
 * where the #undefs won't affect any other memory
 * allocation calls you make, and call it before you
 * do any allocations
 *

#ifdef cl_mem_leak_diagnostics

 // If you need to call the standard 
 // malloc or calloc etc here then undefine them first
 // as they got redefined to _malloc_ etc in the header
#undef malloc
#undef calloc
#undef realloc
#undef memcpy
#undef free
#include "malloc.h"
void init_memleakcheck_alloc(void)
{
 // now here set whatever allocation functions you actually
 // use
  set_mlc_malloc(malloc);
  set_mlc_calloc(calloc);
  set_mlc_realloc(realloc);
  set_mlc_free(free);
  set_mlc_memcpy(memcpy);
  set_mlc_strlen(strlen);
  set_mlc_strcpy(strcpy);
}
#endif

 * and call it when your program starts, or first needs to use
 * MemLeakCheck
 *
 * It may be an idea to do this anyway in case your app and the dll
 * don't use the same version of the C run time library
 * (they only use the same version if they are both the same as regards
 * single / multi-threaded and release / debug, and also the same
 * in terms of whether they are made in MSVC 4.2 or later or not.
 *
 * You should  use a dll built with the same settings for diagnostics
 * and auto free as your app.
 *
 * Zip includes memleakcheckdmt for diagnostics - compiled to be thread 
 * safe and can be used with multi-threaded apps as well as single 
 * threaded ones - and uses the CRT for multithreaded debug apps.
 * You don't need to do the init_memleakcheck_alloc(..)
 * if using it with a multithreaded debug build made in a version of
 * MSVC later than 4.0 as you will be using the same CRT.. 
 *
 * For auto free it includes memleakcheckat - thread safe
 * multi-threaded release build.
 *
 * To build the dll - in MSVC you go to File | New | Projects
 * | Win 32 dynamic link library, make an empty DLL, then add the MemLeakCheck
 * files to your project
 *
 * To build it you use:
 * 
 * #define BUILD_MEMLEAK_CHECK_AS_DLL
 *
 * Suggestion for dll version names:
 *
 * If windows thread safe add a t at the end
 * memleakcheckt.dll.
 * if it is for
 * auto free only, add an a.
 * memleakcheckat.dll.
 * If the variables are thread local, so each thread
 * needs to call the auto free separately, add an l
 * memleakcheckalt.dll.
 * If it is a debug build add a d
 * memleakcheckadlt.dll etc.
 * order
 *
 * The zip you can download from the home page
 * has memleakcheckt.dll and memleakcheckat.dll
 *
 * If you include this source code as part of a larger dll
 * then be sure that you don't export MemLeakCheckAndFree under its 
 * original name, or indeed any of the other routines in 
 * this header (unless your dll is intended as a replacemnt for this
 * one).
 *
 * Instead, if you want to
 * export one of the routines, first provide a wrapper routine
 * for it with some other name perhaps prefixed by the name
 * of your dll - e.g. WinMing_MemLeakCheckAndFree
 * which will call your local version of MemLeakCheckAndFree 
 * to check for memory leaks and free memory for your dll .
 *
 * This will ensure that users of your dll can continue to
 * use any of the original MemLeakCheckAndFree dlls 
 * in their own apps as well as the version for freeing memory in
 * your dll, and the two won't get mixed up.
 */

#if defined(BUILD_MEMLEAK_CHECK_AS_DLL)||defined(LINK_MEMLEAK_CHECK_AS_DLL)

 #ifdef __cplusplus
  #define MEMLEAK_CHECK_WM_DLL_A extern "C" __declspec (dllexport)
 #else
  #define MEMLEAK_CHECK_WM_DLL_A __declspec (dllexport)
 #endif

 #define MEMLEAK_CHECK_WM_DLL_B    __stdcall

#else // ndef compile_as_DLL

 #define MEMLEAK_CHECK_WM_DLL_A
 #define MEMLEAK_CHECK_WM_DLL_B

#endif


#ifdef LINK_MEMLEAK_CHECK_AS_DLL

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
#endif

// If  one builds the library without cl_mem_leak_check defined
// code that calls these routines will still link okay but do nothing

#ifndef cl_mem_leak_check

 #define MemLeakCheckAndFree() 0
 #define SetMemLeakWarnProc()
 #define SetMemLeakLogProc()
 #define SetMemLeakAnticipateFreeProc()

#endif

#ifndef cl_mem_leak_diagnostics

 #define SetMemLeakLogProc()
 #define SetMemLeakWarnProc()
 #define SetMemLeakAnticipateFreeProc()

#endif

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#ifdef cl_mem_leak_check

 MEMLEAK_CHECK_WM_DLL_A int MEMLEAK_CHECK_WM_DLL_B MemLeakCheckAndFree(void);

 #ifdef cl_mem_leak_diagnostics

  MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B SetMemLeakWarnProc(void (__cdecl *NewMemLeakWarnProc)(char *szmsg));
  MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B SetMemLeakLogProc(void (__cdecl *NewMemLeakLogProc)(char *szmsg));

  MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B SetMemLeakAnticipateFreeProc(void( __cdecl *NewMemLeakAnticipateFreeProc)(char *szFile,int nLine));
  MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_realloc(void *p,size_t size,char *szFile,int iLine);
  MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_malloc(size_t size,char *szFile,int iLine);
  MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_calloc(size_t num,size_t size,char *szFile,int iLine);
  MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B xz_free(void *p,char *szFile,int iLine);
  MEMLEAK_CHECK_WM_DLL_A char * MEMLEAK_CHECK_WM_DLL_B xz_strdup(const char *src,char *szFile,int iLine);
  #define cl_xz_strdup 

  #define _malloc_(size) xz_malloc(size,__FILE__,__LINE__)
  #define _calloc_(num,size) xz_calloc(num,size,__FILE__,__LINE__)
  #define _realloc_(p,size) xz_realloc(p,size,__FILE__,__LINE__)
  #define _strdup_(src) xz_strdup(src,__FILE__,__LINE__)
  #define _free_(p) xz_free(p,__FILE__,__LINE__)

 #else //  ndef cl_mem_leak_diagnostics
  // This is the version that auto frees unfreed memory but with no diagnostics

  MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_realloc(void *p,size_t size);
  MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_malloc(size_t size);
  MEMLEAK_CHECK_WM_DLL_A void * MEMLEAK_CHECK_WM_DLL_B xz_calloc(size_t num,size_t size);
  MEMLEAK_CHECK_WM_DLL_A void MEMLEAK_CHECK_WM_DLL_B xz_free(void *p);
  MEMLEAK_CHECK_WM_DLL_A char * MEMLEAK_CHECK_WM_DLL_B xz_strdup(const char *src); 
  #define cl_xz_strdup

  #define _malloc_ xz_malloc
  #define _free_ xz_free
  #define _calloc_ xz_calloc
  #define _realloc_ xz_realloc

  #define _strdup_ xz_strdup

 #endif // ndef cl_mem_leak_diagnostics

 #ifndef code_uses_redefined_malloc

  #define  malloc _malloc_
  #define  free  _free_ 
  #define  calloc _calloc_
  #define  realloc _realloc_
  #define  strdup _strdup_

  // if you want to use the redefined malloc, then in your code you should
  // use _malloc_ etc instead of malloc
  // - useful if it is inconvenient to place this
  // header after all the standard libraries and before
  // the source code that uses allocation.

 #endif

#else // ndef cl_mem_leak_check

 #ifdef cl_MSVC_debug_mem_leak_tests 

  #define _malloc_(size) _malloc_dbg(size, _CLIENT_BLOCK, __FILE__, __LINE__ )
  #define _free_(p) _free_dbg(p,_CLIENT_BLOCK);
  #define _calloc_(num,size) _calloc_dbg(num,size, _CLIENT_BLOCK, __FILE__, __LINE__ )
  #define _realloc_(userData,size) _realloc_dbg(userData,size, _CLIENT_BLOCK, __FILE__, __LINE__ )
  #define _strdup_(src) xz_strdup(src)
  MEMLEAK_CHECK_WM_DLL_A char * MEMLEAK_CHECK_WM_DLL_B xz_strdup(const char *src); // still need it for the malloc debug
  #define cl_xz_strdup

 #else // ndef cl_MSVC_debug_mem_leak_tests and also ndef cl_mem_leak_check
       // i.e. just use normal malloc etc

  #ifdef code_uses_redefined_malloc

   #define  _malloc_ malloc
   #define  _free_  free
   #define  _calloc_ calloc
   #define  _realloc_ realloc
   #define  _strdup_ strdup

  #endif


 #endif // ndef cl_MSVC_debug_mem_leak_tests 

#endif // ndef cl_mem_leak_check

#endif /* MEMLEAKCHECK_H_INCLUDED */

#endif