#ifndef CORELAB_CAMP_RUNTIME_H
#define CORELAB_CAMP_RUNTIME_H
#include <inttypes.h>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <ctime>
#define STK_MAX_SIZE 16
#define STK_MAX_SIZE_DIV_BY_8 2

#if STK_MAX_SIZE == 32
typedef uint64_t IterRelation;
#elif STK_MAX_SIZE == 16
typedef uint32_t IterRelation;
#else /* X != 2 and X != 1*/
#endif 

typedef uint16_t CntxID;
typedef uint32_t InstrID;
typedef uint64_t FullID;

typedef std::map<uint64_t,double> Ctxtime;

typedef union IterStack{
	uint8_t i8[STK_MAX_SIZE];
	uint64_t i64[STK_MAX_SIZE_DIV_BY_8];
} IterStack;

uint64_t def_clocks_per_nano();
uint64_t qyt_time();
// Initializer/Finalizer
extern "C" void objTraceInitialize ();
extern "C" void objTraceFinalize ();
		
// Memory Event
extern "C" void objTraceLoadInstr (void* addr, InstrID instrId);
extern "C" void objTraceStoreInstr (void* addr, InstrID instrId);
		
// Normal Context Event
extern "C" void objTraceLoopBegin (CntxID cntxID);
extern "C" void objTraceLoopNext ();
extern "C" void objTraceLoopEnd (CntxID cntxID);

extern "C" void objTraceCallSiteBegin (CntxID cntxID);
extern "C" void objTraceCallSiteEnd  (CntxID cntxID);

/*
extern "C" void* cxtObjMalloc (size_t size, InstrID instrId);
extern "C" void* cxtObjCalloc (size_t num, size_t size, InstrID instrId);
extern "C" void* cxtObjRealloc (void* addr, size_t size, InstrID instrId);

extern "C" void objTraceDisableCtxtChange();
extern "C" void objTraceEnableCtxtChange();
*/

#endif
//CORELAB_CAMP_RUNTIME_H
