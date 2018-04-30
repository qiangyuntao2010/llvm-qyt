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

#include "ctxObjtraceRuntime.h"
#include "ShadowMemory.hpp"
#include "x86timer.hpp"

/*static uint64_t objcount;
static uint64_t ctxcount;
static uint64_t ctxobjcount;*/
static IterStack globalIterStack;
static int globalIterStackIdx;
static int globalIterStackIdx_overflow;
static uint32_t currentCtx;

#define MAX_PAIR (1024*1024*128)

static unsigned nestingDepth;
static uint16_t disableCxtChange; // enabled when 0
static uint32_t tmpctx;
static uint64_t tmpobj;
static uint64_t paircount;
AllocMap *ctxAllocIdMap;
static ctx_list* ctxpair;
CtxObjCountMap* COCMap;


#define getFullId(instrID) ((uint64_t)currentCtx << 32)|((uint64_t)instrID & 0x00000000FFFFFFFF)
#define getCtxId(fullId) (uint32_t)(fullId >> 32)
#define getInstrId(fullId) (uint32_t)(fullId & 0x00000000FFFFFFFF)
#define GET_BLOCK_ID(instrID) (uint16_t)(instrID >> 16 )
#define GET_INSTR_ID(instrID) (uint16_t)(instrID & 0x0000FFFF)

void ctxObjShadowMemorySetting (void *addr, size_t size, FullID fullId){
	uint64_t flipAddr = GET_SHADOW_ADDR_MALLOC_MAP(addr);

	for (unsigned int i = 0; i < (size-1)/8 + 1; i++)
	{	
		*( (uint64_t *)(flipAddr + (uint64_t)(i*8)) ) = fullId;
	}
}
/*
extern "C"
void pair_clean()
{
	for(uint64_t icount=0;icount<=paircount;icount++)
	{
		outfile<<ctxpair[icount].ctx<<" "<<ctxpair[icount].obj<<" "<<ctxpair[icount].mcount<<"\n";
	}
	paircount=0;
}*/

// Initializer/Finalizer
extern "C"
void ctxObjInitialize () {
/*	ctxpair=(ctx_list*)malloc(sizeof(ctx_list)*MAX_PAIR);*/
	COCMap=new CtxObjCountMap();
	globalIterStackIdx=-1; // null
	globalIterStackIdx_overflow = -1;
	disableCxtChange = 0;
	nestingDepth=0;
	currentCtx = 0;
	tmpctx=0;
	tmpobj=0;
	paircount=0;
//	ctxcount=0;
	// STK_MAX_SIZE_DIV_BY_8 is 2
	for (int i = 0; i < STK_MAX_SIZE_DIV_BY_8; ++i)
		globalIterStack.i64[i] = 0;

	//memory setting
	ShadowMemoryManager::initialize();
	ctxAllocIdMap = new AllocMap();
}


extern "C"
void ctxObjFinalize () {

std::ofstream outfile("ctx-object_count.data", std::ios::out | std::ofstream::binary);
	
	if(outfile.is_open())
	{
		for(auto e : *COCMap)
		{
			outfile<<e.first<<" "<<e.second<<"\n";
		}
	}
	outfile.close();
	delete COCMap;
}

// Normal Context Event
extern "C"
void ctxObjLoopBegin (CntxID cntxID, UcID uniID) { //arg is LocId and CntxID is uint16_t
	if(disableCxtChange != 0) return;

	currentCtx += cntxID;
	
	if(globalIterStackIdx >= STK_MAX_SIZE-1)
		globalIterStackIdx_overflow++;
	else{
		globalIterStackIdx++;
		globalIterStack.i8[globalIterStackIdx] = 0;
	}
}

extern "C"
void ctxObjLoopNext () {
	if(disableCxtChange != 0) return;
	globalIterStack.i8[globalIterStackIdx] = (globalIterStack.i8[globalIterStackIdx]+1) & 0x7f;  // 0111 1111b
}

extern "C"
void ctxObjLoopEnd (CntxID cntxID) {
	if(disableCxtChange!= 0) return;
	currentCtx -= cntxID;

	if(globalIterStackIdx_overflow > -1)
		globalIterStackIdx_overflow--;
	else{
		globalIterStack.i8[globalIterStackIdx] = 0;
		globalIterStackIdx--;
	}

}

extern "C"
void ctxObjCallSiteBegin (CntxID cntxID, UcID uniID) {
	if(disableCxtChange == 0)
	currentCtx += cntxID;	

}

extern "C"
void ctxObjCallSiteEnd  (CntxID cntxID) {
	if(disableCxtChange == 0)
       	currentCtx -= cntxID;
	
}

extern "C" void ctxObjDisableCtxtChange(){
	disableCxtChange++;
}

extern "C" void ctxObjEnableCtxtChange(){
	disableCxtChange--;
}


//TODO :: how to handle data seg access && where is the exact start address of heap space. 


extern "C"
void ctxObjLoadInstr (void* addr, InstrID instrId) {

		uint64_t flipAddr = GET_SHADOW_ADDR_MALLOC_MAP(addr);
		FullID fullAllocId = *( (uint64_t *)(flipAddr) );
		if (fullAllocId > 0 )
		{
			std::string tmp_str=std::to_string(currentCtx)+"_"+std::to_string(fullAllocId);
			CtxObjCountMap::iterator it=COCMap->find(tmp_str);
			if(it==COCMap->end())
			{
				COCMap->insert(std::pair<std::string,uint64_t>(tmp_str,1));
			}
			else
			{
				it->second += 1;
			}
		}
	/*		FullID fullId = getFullId(instrId);
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
			}*/
		
}

extern "C"
void ctxObjStoreInstr (void* addr, InstrID instrId) {

		uint64_t flipAddr = GET_SHADOW_ADDR_MALLOC_MAP(addr);
		FullID fullAllocId = *( (uint64_t *)(flipAddr) );

		if (fullAllocId > 0 )
		{
			std::string tmp_str=std::to_string(currentCtx)+"_"+std::to_string(fullAllocId);
			CtxObjCountMap::iterator it=COCMap->find(tmp_str);
			if(it==COCMap->end())
			{
				COCMap->insert(std::pair<std::string,uint64_t>(tmp_str,1));
			}
			else
			{
				it->second += 1;
			}
		//	FullID fullId = getFullId(instrId);
	/*		if(paircount==MAX_PAIR)
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
			}*/
		}
}

extern "C" 
void* ctxObjMalloc (size_t size, InstrID instrId){

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
	
	ctxObjShadowMemorySetting ( addr, size, fullId );

	return addr;
}

extern "C" 
void* ctxObjCalloc (size_t num, size_t size, InstrID instrId){
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

	ctxObjShadowMemorySetting ( addr, (size*num), fullId );

  return addr;
}

extern "C" 
void* ctxObjRealloc (void* addr, size_t size, InstrID instrId){
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

	ctxObjShadowMemorySetting ( naddr, size, fullId );

  return naddr;
}


