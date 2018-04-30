#include <stdio.h>
#include <iostream>
#include <malloc.h>
#include <math.h>
#include <fstream> //to file IO
#include <bitset> //to output binary format
#include <iomanip> //to output hex format
#include <cstring>
#include <string>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>

#include "objTracetraceRuntime.h"
#include "ShadowMemory.hpp"
#include "x86timer.hpp"

/*static uint64_t objcount;
static uint64_t ctxcount;
static uint64_t ctxobjcount;*/
static IterStack globalIterStack;
static int globalIterStackIdx;
static int globalIterStackIdx_overflow;
static uint32_t currentCtx;

#define MAX_LINE_SIZE 128
#define MAX_PAIR (1024*1024*128)

static uint16_t disableCxtChange; // enabled when 0
Ctxtime *ctx_time;
static clock_t start;
static clock_t  end;
static double dur;
static uint64_t clocks_per_nano;
x86timer ctx_t;

#define getFullId(instrID) ((uint64_t)currentCtx << 32)|((uint64_t)instrID & 0x00000000FFFFFFFF)
#define getCtxId(fullId) (uint32_t)(fullId >> 32)
#define getInstrId(fullId) (uint32_t)(fullId & 0x00000000FFFFFFFF)
#define GET_BLOCK_ID(instrID) (uint16_t)(instrID >> 16 )
#define GET_INSTR_ID(instrID) (uint16_t)(instrID & 0x0000FFFF)

uint64_t def_clocks_per_nano()
{
	uint64_t bench1=0;
	uint64_t bench2=0;
	clocks_per_nano=0;

	bench1=qyt_time();

	#ifdef unix
	usleep(250000); // 1/4 second
	#else
	SDL_Delay(250);
	#endif

	bench2=qyt_time();

	clocks_per_nano=bench2-bench1;
	clocks_per_nano*=4.0e-9;
	return clocks_per_nano*(1.0e+9);
}

uint64_t qyt_time()
{
       	uint64_t lo,hi;


        __asm__ __volatile__
        (
         "rdtsc":"=a"(lo),"=d"(hi)
        );
        return (uint64_t)hi<<32|lo;
}



void objTraceShadowMemorySetting (void *addr, size_t size, FullID fullId){
	uint64_t flipAddr = GET_SHADOW_ADDR_MALLOC_MAP(addr);

	for (unsigned int i = 0; i < (size-1)/8 + 1; i++)
	{	
		*( (uint64_t *)(flipAddr + (uint64_t)(i*8)) ) = fullId;
	}
}



// Initializer/Finalizer
extern "C"
void objTraceInitialize () {
	ctx_time=new Ctxtime();
	start=0;
	end=0;
	dur=0.0;
	clocks_per_nano=def_clocks_per_nano();
	globalIterStackIdx=-1; // null
	globalIterStackIdx_overflow = -1;
	disableCxtChange = 0;
	currentCtx = 0;
	// STK_MAX_SIZE_DIV_BY_8 is 2
	for (int i = 0; i < STK_MAX_SIZE_DIV_BY_8; ++i)
		globalIterStack.i64[i] = 0;

	//memory setting
	ShadowMemoryManager::initialize();
	ctx_t.start();
}


extern "C"
void objTraceFinalize () {
	std::ofstream outfile("ctx_object_trace.data", std::ios::out | std::ofstream::binary);
	if(outfile.is_open()){
		for(auto it:*ctx_time)
		{
			outfile<<it.first<<" "<<it.second<<"\n";
		}
	}
	delete ctx_time;
}

// Normal Context Event
extern "C"
void objTraceLoopBegin (CntxID cntxID) { //arg is LocId and CntxID is uint16_t
	if(disableCxtChange != 0) return;
	end=clock();
	dur=ctx_t.stop() * (1.0e-9);
	Ctxtime::iterator it=ctx_time->find(currentCtx);
	if (it==ctx_time->end())
	{
		ctx_time->insert(std::pair<uint64_t,double>(currentCtx,dur));
	}
	else
	{
		it->second += dur;
	}
	
	currentCtx += cntxID;
	
	ctx_t.start();
	
	if(globalIterStackIdx >= STK_MAX_SIZE-1)
		globalIterStackIdx_overflow++;
	else{
		globalIterStackIdx++;
		globalIterStack.i8[globalIterStackIdx] = 0;
	}
}

extern "C"
void objTraceLoopNext () {
	if(disableCxtChange != 0) return;
	globalIterStack.i8[globalIterStackIdx] = (globalIterStack.i8[globalIterStackIdx]+1) & 0x7f;  // 0111 1111b
}

extern "C"
void objTraceLoopEnd (CntxID cntxID) {
	if(disableCxtChange!= 0) return;
	end=clock();
	dur=ctx_t.stop() * (1.0e-9);
	Ctxtime::iterator it=ctx_time->find(currentCtx);
	if (it==ctx_time->end())
	{
		ctx_time->insert(std::pair<uint64_t,double>(currentCtx,dur));
	}
	else
	{
		it->second += dur;
	}
	
	currentCtx -= cntxID;
	
	ctx_t.start();
	
	if(globalIterStackIdx_overflow > -1)
		globalIterStackIdx_overflow--;
	else{
		globalIterStack.i8[globalIterStackIdx] = 0;
		globalIterStackIdx--;
	}

}

extern "C"
void objTraceCallSiteBegin (CntxID cntxID) {
	if(disableCxtChange == 0){
	end=clock();
	dur=ctx_t.stop() * (1.0e-9);
	Ctxtime::iterator it=ctx_time->find(currentCtx);
	if (it==ctx_time->end())
	{
		ctx_time->insert(std::pair<uint64_t,double>(currentCtx,dur));
	}
	else
	{
		it->second += dur;
	}
	currentCtx += cntxID;	
	ctx_t.stop();
	}
}

extern "C"
void objTraceCallSiteEnd  (CntxID cntxID) {
	if(disableCxtChange == 0){
	end=clock();
	dur=ctx_t.stop() *(1.0e-9);
	Ctxtime::iterator it=ctx_time->find(currentCtx);
	if (it==ctx_time->end())
	{
		ctx_time->insert(std::pair<uint64_t,double>(currentCtx,dur));
	}
	else
	{
		it->second += dur;
	}
       	currentCtx -= cntxID;
	ctx_t.start();
	}	
}

extern "C" void objTraceDisableCtxtChange(){
	disableCxtChange++;
}

extern "C" void objTraceEnableCtxtChange(){
	disableCxtChange--;
}


//TODO :: how to handle data seg access && where is the exact start address of heap space. 
/*

extern "C"
void objTraceLoadInstr (void* addr, InstrID instrId) {

		uint64_t flipAddr = GET_SHADOW_ADDR_MALLOC_MAP(addr);
		FullID fullAllocId = *( (uint64_t *)(flipAddr) );
		if (fullAllocId > 0 )
		{
			//FullID fullId = getFullId(instrId);
			if(paircount==MAX_PAIR)
			{
				pair_clean();
			}
			if(currentCtx==tmpctx)
			{
				if(fullAllocId==tmpobj)
				{
					ctxpair[paircount].mcount++;
				}
				else
				{
					tmpobj=fullAllocId;
					paircount++;
					ctxpair[paircount].ctx=currentCtx;
					ctxpair[paircount].obj=fullAllocId;
					ctxpair[paircount].mcount=1;
				}
			}
			else
			{
				paircount++;
				tmpctx=currentCtx;
				tmpobj=fullAllocId;
				ctxpair[paircount].ctx=currentCtx;
				ctxpair[paircount].obj=fullAllocId;
				ctxpair[paircount].mcount=1;
			}
		}
}

extern "C"
void objTraceStoreInstr (void* addr, InstrID instrId) {

		uint64_t flipAddr = GET_SHADOW_ADDR_MALLOC_MAP(addr);
		FullID fullAllocId = *( (uint64_t *)(flipAddr) );

		if (fullAllocId > 0 )
		{
		//	FullID fullId = getFullId(instrId);
			if(paircount==MAX_PAIR)
			{
				pair_clean();
			}
			if(currentCtx==tmpctx)
			{
				if(fullAllocId==tmpobj)
				{
					ctxpair[paircount].mcount++;
				}
				else
				{
					paircount++;
					tmpobj=fullAllocId;
					ctxpair[paircount].ctx=currentCtx;
					ctxpair[paircount].obj=fullAllocId;
					ctxpair[paircount].mcount=1;
				}
			}
			else
			{
				paircount++;
				tmpctx=currentCtx;
				tmpobj=fullAllocId;
				ctxpair[paircount].ctx=currentCtx;
				ctxpair[paircount].obj=fullAllocId;
				ctxpair[paircount].mcount=1;
			}
		}
}

extern "C" 
void* objTraceMalloc (size_t size, InstrID instrId){

	void *addr = malloc (size);
	const uint64_t addr_ = (uint64_t)addr;
	FullID fullId = getFullId(instrId);

	AllocMap::iterator it = ctxAllocIdMap->find(fullId);
	if( it == ctxAllocIdMap->end() ) // first alloc by this instruction
	{
		AllocInfo allocSet;
		allocSet.insert(size);
		ctxAllocIdMap->insert( std::pair<FullID, AllocInfo> ( fullId, allocSet ) );
	}
	else
	{
		(it->second).insert(size);
	}
	
	objTraceShadowMemorySetting ( addr, size, fullId );

	return addr;
}

extern "C" 
void* objTraceCalloc (size_t num, size_t size, InstrID instrId){
  void* addr = calloc (num, size);
	const uint64_t addr_ = (uint64_t)addr;
	FullID fullId = getFullId(instrId);

	AllocMap::iterator it = ctxAllocIdMap->find(fullId);
	if( it == ctxAllocIdMap->end() )
	{
		AllocInfo allocSet;
		allocSet.insert(size*num);
		ctxAllocIdMap->insert( std::pair<FullID, AllocInfo> (fullId, allocSet) );
	}
	else
	{
		(it->second).insert(size*num);
	}

	objTraceShadowMemorySetting ( addr, (size*num), fullId );

  return addr;
}

extern "C" 
void* objTraceRealloc (void* addr, size_t size, InstrID instrId){
  void* naddr = NULL;
  naddr = realloc (addr, size);
	const uint64_t addr_ = (uint64_t)naddr;
	FullID fullId = getFullId(instrId);

	AllocMap::iterator it = ctxAllocIdMap->find(fullId);
	if( it == ctxAllocIdMap->end() )
	{
		AllocInfo allocSet;
		allocSet.insert(size);
		ctxAllocIdMap->insert( std::pair<FullID, AllocInfo> (fullId, allocSet) );
	}
	else
	{
		(it->second).insert(size);
	}

	objTraceShadowMemorySetting ( naddr, size, fullId );

  return naddr;
}
*/

