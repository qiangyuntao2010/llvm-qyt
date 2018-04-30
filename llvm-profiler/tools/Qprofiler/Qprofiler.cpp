#include <stdio.h>
#include <iostream>
#include <malloc.h>
#include <math.h>
#include <fstream> //to file IO
#include <bitset> //to output binary format
#include <iomanip> //to output hex format
#include <cstring>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <ctime>
#include <set>

#include "Qprofiler.h"
#include "ShadowMemory.hpp"
#include "x86timer.hpp"

typedef std::string String;

static IterStack globalIterStack;
static int globalIterStackIdx;
static int globalIterStackIdx_overflow;
static uint32_t currentCtx;
#define MERGED_MAX 543 
#define OBJSIZE 1024*1024
#define CTXCOUNT 94
#define MAX_VEC (1024*640*1024)
#define MAX_LinputE_SIZE 128
//#define CTXPRINT
static uint32_t ctxarray[CTXCOUNT];
static inputstr* instr;
static unsigned nestingDepth;
static uint16_t disableCxtChange; // enabled when 0
static double dur;
static uint64_t count;
static clock_t start;
static clock_t end;
static uint32_t guard_ctx;
static uint32_t guard_ctxid;
x86timer ctx_t;
FILE *input;
std::set<int> testset;
#define getFullId(instrID) ((uint64_t)currentCtx << 32)|((uint64_t)instrID & 0x00000000FFFFFFFF)
#define getCtxId(fullId) (uint32_t)(fullId >> 32)
#define getInstrId(fullId) (uint32_t)(fullId & 0x00000000FFFFFFFF)
#define GET_BLOCK_ID(instrID) (uint16_t)(instrID >> 16 )
#define GET_inputSTR_ID(instrID) (uint16_t)(instrID & 0x0000FFFF)

extern "C"
void in_read(inputstr* instr,FILE *input)
{
	int i=0;
	char a[OBJSIZE];
	while(!feof(input))
	{
		a[0]='\0';
		fgets(a,OBJSIZE,input);
		if (a[0]=='\0'){
		break;}
		instr[i].ctxid=atoi(strtok(a," "));
		strcpy(instr[i].objcount,strtok(NULL," "));
		ctxarray[i]=instr[i].ctxid;
		i++;
	}
	for (int c=0;c<CTXCOUNT;c++)
	{
		printf("%d\n",ctxarray[c]);
	}
	for(int t1=0;t1<CTXCOUNT;t1++)
	{
		for(int t2=0;t2<128;t2++)
		{
			if(instr[t1].objcount[t2]=='\n'||instr[t1].objcount[t2]=='\r')
			{
				instr[t1].objcount[t2]='n';
			}
		}
	}	
	delete instr;
}

extern "C"
double timecal(clock_t starttmp,clock_t endtmp)
{
	double duration=(double)((endtmp-starttmp)/CLOCKS_PER_SEC);
	return duration;
}


void QproShadowMemorySetting (void *addr, size_t size, FullID fullId){
	uint64_t flipAddr = GET_SHADOW_ADDR_MALLOC_MAP(addr);

	for (unsigned int i = 0; i < (size-1)/8 + 1; i++)
	{	
		*( (uint64_t *)(flipAddr + (uint64_t)(i*8)) ) = fullId;
	}
}

// Initializer/Finalizer
extern "C"
void QproInitialize () {

	input=fopen("./data/merging4","r");
	count=0;
	instr=new inputstr[CTXCOUNT];
	in_read(instr, input);
	fclose(input);
	dur=0.0;
	end=0;
	globalIterStackIdx=-1; // null
	globalIterStackIdx_overflow = -1;
	disableCxtChange = 0;
	nestingDepth=0;
	guard_ctxid=0;
	currentCtx = 0;
	// STK_MAX_SIZE_DIV_BY_8 is 2
	for (int i = 0; i < STK_MAX_SIZE_DIV_BY_8; ++i)
		globalIterStack.i64[i] = 0;

	ShadowMemoryManager::initialize();
	ctx_t.start();
}


extern "C"
void QproFinalize () {
	printf("==%lu==\n",count);
}

// Normal Context Event
extern "C"
void QproLoopBegin (CntxID cntxID, UcID uniID) 
{ //arg is LocId and CntxID is uint16_t
	if(disableCxtChange != 0) return;
	if (uniID!=0&&uniID!=guard_ctxid)
	{
		count++;
		printf("%d",uniID);
		guard_ctxid=uniID;
	}
	currentCtx += cntxID;
	if(globalIterStackIdx >= STK_MAX_SIZE-1)	//STK_MAX_SIZE is 16
		globalIterStackIdx_overflow++;
	else{
		globalIterStackIdx++;
		globalIterStack.i8[globalIterStackIdx] = 0;
	}
}

//QprofileNext will count the iteration of the loop
extern "C"
void QproLoopNext () {
	if(disableCxtChange != 0) return;
	globalIterStack.i8[globalIterStackIdx] = (globalIterStack.i8[globalIterStackIdx]+1) & 0x7f;  // 0111 1111b
}

extern "C"
void QproLoopEnd (CntxID cntxID) {
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
void QproCallSiteBegin (CntxID cntxID, UcID uniID) {
	if(disableCxtChange == 0)
	currentCtx += cntxID;	
	if (uniID!=0&&uniID!=guard_ctxid)
	{
		count++;
		printf("%d",uniID);
		guard_ctxid=uniID;
	}
}

extern "C"
void QproCallSiteEnd  (CntxID cntxID) {
	if(disableCxtChange == 0)
       	currentCtx -= cntxID;
}

extern "C" void QproDisableCtxtChange(){
	disableCxtChange++;
}

extern "C" void QproEnableCtxtChange(){
	disableCxtChange--;
}



extern "C" void* QproMalloc (size_t size, InstrID instrId){

	void *addr = malloc (size);
	const uint64_t addr_ = (uint64_t)addr;
	FullID fullId = getFullId(instrId);

/*	AllocMap::iterator it = ctxAllocIdMap->find(fullId);
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
	
	QproShadowMemorySetting ( addr, size, fullId );*/
	printf("alloc %p %d",addr,fullId);
	return addr;
}

extern "C" void* QproCalloc (size_t num, size_t size, InstrID instrId){
  void* addr = calloc (num, size);
	const uint64_t addr_ = (uint64_t)addr;
	FullID fullId = getFullId(instrId);

	/*AllocMap::iterator it = ctxAllocIdMap->find(fullId);
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

	QproShadowMemorySetting ( addr, (size*num), fullId );*/
	printf("alloc %p %d",addr,fullId);

  return addr;
}

extern "C" void* QproRealloc (void* addr, size_t size, InstrID instrId){
  void* naddr = NULL;
  naddr = realloc (addr, size);
	const uint64_t addr_ = (uint64_t)naddr;
	FullID fullId = getFullId(instrId);

	/*AllocMap::iterator it = ctxAllocIdMap->find(fullId);
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

	QproShadowMemorySetting ( naddr, size, fullId );*/
	printf("%p %d",addr,fullId);

  return naddr;
}


