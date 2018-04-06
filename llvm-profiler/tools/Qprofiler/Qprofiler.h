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
typedef uint32_t InstrID;
typedef uint64_t FullID;
typedef struct _INPUT
{
	uint32_t ctxid;
	char objcount[1024*1024];
} inputstr;

typedef struct _MERGED_STACK
{
	uint32_t ctxid_array[2];
	char merged_objcount[128];
} Merged_Array;
	
typedef struct _TIME_CTX
{
	double time;
	FullID ctx;
} tc_kv;
//typedef std::set<size_t> AllocInfo;
//typedef std::set<FullID> LoadStoreInfo;

//typedef std::map<FullID, AllocInfo> AllocMap;
/*typedef std::map<FullID, LoadStoreInfo> LoadStoreMap;
typedef std::map<FullID,uint64_t> ObjectCountMap;
typedef std::map<FullID,uint64_t> CtxCountMapType;*/
//typedef std::map<std::string,uint64_t> CtxObjCountMapType;

typedef union IterStack{
	uint8_t i8[STK_MAX_SIZE];
	uint64_t i64[STK_MAX_SIZE_DIV_BY_8];
} IterStack;
void obj_find(uint32_t ctx);
bool is_exist(uint32_t ctx);

int ismergestack_full();

void push_mergestack(uint32_t ctx);

bool empty_mergestack();

bool mergestack_merge();

void obj_find(uint32_t ctx);

bool is_exist(uint32_t ctx);

extern "C" void in_read(inputstr* instr, FILE *input);

//extern "C" double timecal(clock_t starttmp,clock_t endtmp); 
// Initializer/Finalizer
extern "C" void QproInitialize ();

extern "C" void QproFinalize ();
		
// Memory Event
extern "C" void QproLoadInstr (void* addr, InstrID instrId);
extern "C" void QproStoreInstr (void* addr, InstrID instrId);
		
// Normal Context Event
extern "C" void QproLoopBegin (CntxID cntxID);
extern "C" void QproLoopNext ();
extern "C" void QproLoopEnd (CntxID cntxID);

extern "C" void QproCallSiteBegin (CntxID cntxID);
extern "C" void QproCallSiteEnd  (CntxID cntxID);


//extern "C" void* cxtObjMalloc (size_t size, InstrID instrId);
//extern "C" void* cxtObjCalloc (size_t num, size_t size, InstrID instrId);
//extern "C" void* cxtObjRealloc (void* addr, size_t size, InstrID instrId);

extern "C" void QproDisableCtxtChange();
extern "C" void QproEnableCtxtChange();


#endif
//CORELAB_CAMP_RUNTIME_H
