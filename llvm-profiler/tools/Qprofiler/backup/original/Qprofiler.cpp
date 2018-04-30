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
static int32_t guard_ctxid;
x86timer ctx_t;
FILE *input;
std::set<int> testset;
Merged_Array *merged_arr;
static uint32_t *mergestack;
#define getFullId(instrID) ((uint64_t)currentCtx << 32)|((uint64_t)instrID & 0x00000000FFFFFFFF)
#define getCtxId(fullId) (uint32_t)(fullId >> 32)
#define getInstrId(fullId) (uint32_t)(fullId & 0x00000000FFFFFFFF)
#define GET_BLOCK_ID(instrID) (uint16_t)(instrID >> 16 )
#define GET_inputSTR_ID(instrID) (uint16_t)(instrID & 0x0000FFFF)

int ismergestack_full()
{
	if((mergestack[1]==0)&&(mergestack[0]==0))
	{
		return 0;
	}
	else if((mergestack[0]==0)&&(mergestack[1]!=0))
	{
		return 1;
	}
	else if((mergestack[0]!=0)&&(mergestack[1]!=0))
	{
		return 2;
	}
	else
	{
		printf("error:ismergestack\n");
		exit(1);
	}
}

void push_mergestack(uint32_t ctx)
{
	int i=ismergestack_full();
	if(i==0)
	{
		mergestack[1]=ctx;
	}
	else if(i==1)
	{
		mergestack[0]=mergestack[1];
		mergestack[1]=ctx;
	}
	else
	{
		mergestack[0]=mergestack[1];
		mergestack[1]=ctx;
	}
}

bool empty_mergestack()
{
	memset(mergestack,0,sizeof(uint32_t)*2);
	if(mergestack[0] || mergestack[1])
	{
		return false;
	}
	return true;
}

bool mergestack_merge()
{
	int tmpc=0;
	int x=ismergestack_full();
	int diff=mergestack[0]-mergestack[1];
	if(x==0 || x==1 || x==-1)
	{
		empty_mergestack();
		return true;
	}
	else
	{		
		if(diff==0)
		{
			empty_mergestack();
			return true;
		}
		for(int c=0;c<=MERGED_MAX;c++)
		{
			if(merged_arr[c].ctxid_array[0]==mergestack[0] || merged_arr[c].ctxid_array[1]==mergestack[0] || merged_arr[c].ctxid_array[0]==mergestack[1] || merged_arr[c].ctxid_array[1]==mergestack[1])
			{
				empty_mergestack();
				return true;
			}
			if(merged_arr[c].ctxid_array[0]==0)
			{
				tmpc=c;
				break;
			}
		}	
		merged_arr[tmpc].ctxid_array[0]=mergestack[0];
		merged_arr[tmpc].ctxid_array[1]=mergestack[1];
		for(int i=0;i<CTXCOUNT;i++)
		{
			if((instr[i].ctxid==mergestack[0]) || (instr[i].ctxid==mergestack[1]))
			{
				strcat(merged_arr[tmpc].merged_objcount,instr[i].objcount);
			}
		}
		empty_mergestack();
		return true;
	}
	return false;
}
		
	


void obj_find(uint32_t ctx)
{
		
/*	for(int a=0;merged_arr[a].ctxid_array[0]!=0;a++)
	{
		if(merged_arr[a].ctxid_array[0]==ctx || merged_arr[a].ctxid_array[1]==ctx)
		{
			if(a!=guard_mergedid)
			{
//				printf("%s",merged_arr[a].merged_objcount);
				guard_mergedid=a;
				return;
			}
			else
			{
				return;
			}
		}
	}	*/	
	for(int i=0;i<CTXCOUNT;i++)
	{
		if(ctx==instr[i].ctxid)
		{
//			printf("%s",instr[i].objcount);
		//	guard_mergedid=MERGED_MAX;
			printf("%d_%s",ctx,instr[i].objcount);
			return;
		}
	}
}
	
bool is_exist(uint32_t ctx)
{
	for(int i=0;i<CTXCOUNT;i++)
	{
		if(ctx==instr[i].ctxid)
		{
			return true;
		}
	}
	return false;
}

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
/*		for(int i1=0;i1<i;i1++)
		{
			if(instr[i1].ctxid==instr[i].ctxid)
			{
				strcat(instr[i1].objcount, strtok(NULL," "));
			//	strcat(instr[i1].objcount," ");
				strcat(instr[i1].objcount, strtok(NULL," "));
			//	strcat(instr[i1].objcount, " ");
				goto outermost;
			}
		}
		strcpy(instr[i].objcount, strtok(NULL," "));
	//	strcat(instr[i].objcount," ");
		strcat(instr[i].objcount, strtok(NULL," "));
	//	strcat(instr[i].objcount, " ");
		i++;*/
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
//	start=clock();
//	mergestack=(uint32_t*)malloc(sizeof(uint32_t)*2);
//	merged_arr=(Merged_Array*)malloc(sizeof(Merged_Array)*MERGED_MAX);
//	memset(mergestack,0,sizeof(uint32_t)*2);
//	memset(merged_arr,0,sizeof(Merged_Array)*MERGED_MAX);
/*	for(int i=0;i<MERGED_MAX;i++)
	{
		merged_arr[i].merged_objcount[0]='\0';
	}*/
	dur=0.0;
	end=0;
	globalIterStackIdx=-1; // null
	globalIterStackIdx_overflow = -1;
	disableCxtChange = 0;
	nestingDepth=0;
	guard_ctxid=0;
//	guard_mergedid=0;
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
/*	std::set<int>::iterator iter_set=testset.begin();
	for(;iter_set!=testset.end();iter_set++)
	{
		std::cout<<*iter_set<<std::endl;
	}
	printf("size is %d\n",testset.size());
	for(int i=0;i<MERGED_MAX;i++)
	{
//		printf(" mergedarray ctxid 0 is %d and ctxid 1 is %d\n",merged_arr[i].ctxid_array[0],merged_arr[i].ctxid_array[1]);
	}
	free(mergestack);
	free(merged_arr);*/
//	delete instr;
}

// Normal Context Event
extern "C"
void QproLoopBegin (CntxID cntxID) { //arg is LocId and CntxID is uint16_t
	if(disableCxtChange != 0) return;
	currentCtx += cntxID;
	if(currentCtx==guard_ctxid)
	{	return;}
//	testset.insert(currentCtx);
for(int i=0;i<CTXCOUNT;i++)
{
	if(ctxarray[i]==currentCtx){
		guard_ctxid=currentCtx;
//	printf("%d",currentCtx);
	count++;}
}	
//	obj_find(currentCtx);
/*
	if(is_exist(currentCtx)&&(currentCtx!=guard_ctx)){
		guard_ctx=currentCtx;
		if(cntxID>1)
		{
			mergestack_merge();
		}
		if(currentCtx==merged_arr[guard_mergedid].ctxid_array[0] || currentCtx==merged_arr[guard_mergedid].ctxid_array[1])
		{
			return;
		}
		push_mergestack(currentCtx);
		end=clock();
		dur=timecal(start,end);
#ifdef CTXPRINT
		printf("%d\n",currentCtx);
#endif 	 
	//std::cout<<"start time: "<<dur<<" current Ctx:"<<currentCtx<<"\n"<<"info:"<<"\n";
		obj_find(currentCtx);
		}*/
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
//	testset.insert(currentCtx);

/*	if(is_exist(currentCtx)&&(currentCtx!=guard_ctx)){
		guard_ctx=currentCtx;
		if(cntxID>1)
		{
			mergestack_merge();
		}
		if(currentCtx==merged_arr[guard_mergedid].ctxid_array[0] || currentCtx==merged_arr[guard_mergedid].ctxid_array[1])
		{
			return;
		}
		push_mergestack(currentCtx);
		end=clock();
		dur=timecal(start,end);
#ifdef CTXPRINT
		printf("%d\n",currentCtx);
#endif
	 //	std::cout<<"start time: "<<dur<<" current Ctx:"<<currentCtx<<"\n"<<"info:"<<"\n";
		obj_find(currentCtx);
		}*/

	if(globalIterStackIdx_overflow > -1)
		globalIterStackIdx_overflow--;
	else{
		globalIterStack.i8[globalIterStackIdx] = 0;
		globalIterStackIdx--;
	}

}

extern "C"
void QproCallSiteBegin (CntxID cntxID) {
	if(disableCxtChange == 0)
	currentCtx += cntxID;	
	if(currentCtx==guard_ctxid)
	{	return;}
//	testset.insert(currentCtx);
for(int i=0;i<CTXCOUNT;i++)
{
	if(ctxarray[i]==currentCtx)
	{
		guard_ctxid=currentCtx;
//	printf("%d",currentCtx);
	count++;}
}	
//	printf("%d\n",currentCtx);
//	obj_find(currentCtx);

/*	if(is_exist(currentCtx)&&(currentCtx!=guard_ctx)){
		guard_ctx=currentCtx;
		if(cntxID>1)
		{
			mergestack_merge();
		}
		if(currentCtx==merged_arr[guard_mergedid].ctxid_array[0] || currentCtx==merged_arr[guard_mergedid].ctxid_array[1])
		{
			return;
		}
		push_mergestack(currentCtx);
		end=clock();
		dur=timecal(start,end);
#ifdef	CTXPRINT
		printf("%d\n",currentCtx);
#endif
	 //	std::cout<<"start time: "<<dur<<" current Ctx:"<<currentCtx<<"\n"<<"info:"<<"\n";
		obj_find(currentCtx);*/
	//	}
}

extern "C"
void QproCallSiteEnd  (CntxID cntxID) {
	if(disableCxtChange == 0)
       	currentCtx -= cntxID;
//	testset.insert(currentCtx);

/*	if(is_exist(currentCtx)&&(currentCtx!=guard_ctx)){
		guard_ctx=currentCtx;
		if(cntxID>1)
		{
			mergestack_merge();
		}
		if(currentCtx==merged_arr[guard_mergedid].ctxid_array[0] || currentCtx==merged_arr[guard_mergedid].ctxid_array[1])
		{
			return;
		}
		push_mergestack(currentCtx);
		end=clock();
		dur=timecal(start,end);
#ifdef CTXPRINT
		printf("%d\n",currentCtx);
#endif 
	//	std::cout<<"start time: "<<dur<<" current Ctx:"<<currentCtx<<"\n"<<"info:"<<"\n";
		obj_find(currentCtx);
		}*/
}

extern "C" void QproDisableCtxtChange(){
	disableCxtChange++;
}

extern "C" void QproEnableCtxtChange(){
	disableCxtChange--;
}


//TODO :: how to handle data seg access && where is the exact start address of heap space. 

/*
extern "C"
void QproLoadInstr (void* addr, InstrID instrId) {

		uint64_t flipAddr = GET_SHADOW_ADDR_MALLOC_MAP(addr);
		FullID fullAllocId = *( (uint64_t *)(flipAddr) );
		if (fullAllocId > 0 )
		{
			FullID fullId = getFullId(instrId);
		//	LoadStoreMap::iterator it = ctxLoadIdMap->find(fullId);
			ObjectCountMap::iterator objcount_iter = objcountMap->find(fullAllocId);
			if(objcount_iter==objcountMap->end())
			{
				uint64_t objcount=1;
				objcountMap->insert(std::pair<FullID, uint64_t>(fullAllocId,objcount));
			}
			else
			{
				objcount_iter->second++;
			}
			
			CtxCountMapType::iterator ctxcount_iter = ctxcountMap->find(fullId);
			if(ctxcount_iter==ctxcountMap->end())
			{
				uint64_t ctxcount=1;
				ctxcountMap->insert(std::pair<FullID, uint64_t>(fullId,ctxcount));
			}
			else
			{
				ctxcount_iter->second++;
			}
			end=clock();
			dur=((double)(end-start)/CLOCKS_PER_SEC);
			std::string ctx=std::to_string(getCtxId(fullId));
			std::string symbol="_";
			std::string timectxobj=std::to_string(dur)+symbol+ctx+symbol+std::to_string(fullAllocId);
			CtxObjCountMapType::iterator ctxobjcount_iter = ctxobjcountMap->find(timectxobj);
			if (ctxobjcount_iter==ctxobjcountMap->end())
			{
				uint64_t ctxobjcount=1;
				ctxobjcountMap->insert(std::pair<std::string,uint64_t>(timectxobj,ctxobjcount));
			}
			else
			{
				ctxobjcount_iter->second++;
			}	
			if (it==ctxLoadIdMap->end())
			{
				LoadStoreInfo loadSet;
				loadSet.push_back (fullAllocId);
				ctxLoadIdMap->insert( std::pair<FullID, LoadStoreInfo> (fullId, loadSet));
			}
			else
			{
				(it->second).push_back( fullAllocId );
			}
		}
//	}	
}
*/
/*
extern "C"
void QproStoreInstr (void* addr, InstrID instrId) {

//	if((uint64_t)addr > 0x0000000040000000 && (uint64_t)addr < 0x0000400000000000 )//TODO :: how to handle data seg access
//	{
		uint64_t flipAddr = GET_SHADOW_ADDR_MALLOC_MAP(addr);
		FullID fullAllocId = *( (uint64_t *)(flipAddr) );

		if (fullAllocId > 0 )
		{
			FullID fullId = getFullId(instrId);
			ObjectCountMap::iterator objcount_iter = objcountMap->find(fullAllocId);
			if(objcount_iter==objcountMap->end())
			{
				uint64_t objcount=1;
				objcountMap->insert(std::pair<FullID, uint64_t>(fullAllocId,objcount));
			}
			else
			{
				objcount_iter->second++;
			}
			
			CtxCountMapType::iterator ctxcount_iter = ctxcountMap->find(fullId);
			if(ctxcount_iter==ctxcountMap->end())
			{
				uint64_t ctxcount=1;
				ctxcountMap->insert(std::pair<FullID, uint64_t>(fullId,ctxcount));
			}
			else
			{
				ctxcount_iter->second++;
			}
			end=clock();
			dur=((double)(end-start)/CLOCKS_PER_SEC);
			std::string ctx=std::to_string(getCtxId(fullId));
			std::string symbol="_";
			std::string timectxobj=std::to_string(dur)+symbol+ctx+symbol+std::to_string(fullAllocId);
			CtxObjCountMapType::iterator ctxobjcount_iter = ctxobjcountMap->find(timectxobj);
			if (ctxobjcount_iter==ctxobjcountMap->end())
			{
				uint64_t ctxobjcount=1;
				ctxobjcountMap->insert(std::pair<std::string,uint64_t>(timectxobj,ctxobjcount));
			}
			else
			{
				ctxobjcount_iter->second++;
			}
		      LoadStoreMap::iterator it = ctxStoreIdMap->find(fullId);
			if (it==ctxStoreIdMap->end())
			{
				LoadStoreInfo storeSet;
				storeSet.push_back (fullAllocId);
				ctxStoreIdMap->insert( std::pair<FullID, LoadStoreInfo> (fullId, storeSet));
			}
			else
			{
				(it->second).push_back( fullAllocId );
			}
			
		}

//	}

}*/

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
	printf("%p %d",addr,fullId);

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
	printf("%p %d",addr,fullId);

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


