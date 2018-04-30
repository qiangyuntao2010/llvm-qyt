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
typedef std::string String;

//extern std::ofstream outfile;
typedef uint16_t CntxID;
typedef uint32_t UcID;
typedef uint32_t InstrID;
typedef uint64_t FullID;
typedef struct _INPUT
{
	uint32_t ctxid;
	char objcount[1024*1024];
} inputstr;
	
typedef struct _TIME_CTX
{
	double time;
	FullID ctx;
} tc_kv;

typedef union IterStack{
	uint8_t i8[STK_MAX_SIZE];
	uint64_t i64[STK_MAX_SIZE_DIV_BY_8];
} IterStack;

extern "C" void in_read(inputstr* instr, FILE *input);

//extern "C" double timecal(clock_t starttmp,clock_t endtmp); 
// Initializer/Finalizer
extern "C" void QproInitialize ();

extern "C" void QproFinalize ();
		
// Normal Context Event
extern "C" void QproLoopBegin (CntxID cntxID, UcID uniID);
extern "C" void QproLoopNext ();
extern "C" void QproLoopEnd (CntxID cntxID);

extern "C" void QproCallSiteBegin (CntxID cntxID, UcID uniID);
extern "C" void QproCallSiteEnd  (CntxID cntxID);


extern "C" void* cxtObjMalloc (size_t size, InstrID instrId);
extern "C" void* cxtObjCalloc (size_t num, size_t size, InstrID instrId);
extern "C" void* cxtObjRealloc (void* addr, size_t size, InstrID instrId);

extern "C" void QproDisableCtxtChange();
extern "C" void QproEnableCtxtChange();


#endif
//CORELAB_CAMP_RUNTIME_H
