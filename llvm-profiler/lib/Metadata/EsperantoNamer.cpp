/*
 *
 * EsperantoNamer.cpp
 *
 * Generate metadata files to notify the description of device information
 * for each functions. 
 *
 */

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"

#include "corelab/Metadata/LoadNamer.h"
#include "corelab/Metadata/NamedMetadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

namespace corelab {
	char EsperantoNamer::ID = 0;

	static RegisterPass<EsperantoNamer> RP("esperanto-namer", "Get the information of esperanto pragma", false, false);

	EsperantoNamer::EsperantoNamer() : ModulePass(ID) { }

	void EsperantoNamer::getAnalysisUsage(AnalysisUsage &AU) const {
		AU.addRequired< LoadNamer >();
		AU.setPreservesAll();
	}

	bool EsperantoNamer::runOnModule(Module &M) {
		// initialize member fields
		this->pM = &M;

		checkFunction(); 
		//initUseDefChains();
		//while(checkUseDefChains());
		addMetadata();
		//generateFunctionTableProfile();
		setMaps();

		printSpecs();
		return false;
	}

	void EsperantoNamer::checkFunction() {
		LoadNamer& loadNamer = getAnalysis< LoadNamer >();

		typedef Module::iterator FF;
		for(FF FI = pM->begin(), FE = pM->end(); FI != FE; ++FI) {
			Function *F = (Function*)&*FI;
			if (F->isDeclaration()) continue;
			int functionId = loadNamer.getFunctionId(*F);
			if(functionId == 0) continue;
				
			DeviceEntry* dev = getDeviceInfo(F);
			if(dev!=NULL) {
				checkCallInsts(F, dev);
			}
		}
	}
	
	void EsperantoNamer::checkCallInsts(Function* F, EsperantoNamer::DeviceEntry* dev) {	
		printf("DEBUG :: checkCallInsts\n");
		LLVMContext &Ctx = F->getParent()->getContext();
		std::vector<Value*> callInsts;
		std::vector<Value*> callInstsBuild;
		std::vector<Value*> temp;
		
		typedef Value::user_iterator U;
		typedef std::vector<Value*>::iterator V;
		for(U ui = F->user_begin(), ue = F->user_end(); ui != ue; ++ui) {
			User* u = (User*) *ui;
			Value* v = (Value*) u;
			isCheckedOrEnroll(v);
			callInstsBuild.push_back(v);
		}
		
		bool isChanged = true;

		while(isChanged) {
			isChanged = false;
			
			for(V vi = callInstsBuild.begin(), ve = callInstsBuild.end(); vi != ve; ++vi) {
				Value* vv = (Value*) *vi;
				for(U ui = vv->user_begin(), ue = vv->user_end(); ui != ue; ++ui) {
					User* u = (User*) *ui;
					Value* v = (Value*) u;
					bool isChecked = isCheckedOrEnroll(v);
					isChanged |= !isChecked;
					if(!isChecked) {
						temp.push_back(v);
					}
				}
			}
			callInsts.insert(callInsts.end(), callInstsBuild.begin(), callInstsBuild.end());
			callInstsBuild.clear();
			callInstsBuild.insert(callInstsBuild.end(), temp.begin(), temp.end());
			temp.clear();
		}
			
		StringRef name(dev->name);
		StringRef type(getPlatformAsString(dev->platform));
		NamedMDNode* namedMDNode = pM->getOrInsertNamedMetadata("esperanto.comp");
		Metadata* componentNode[] = {MDString::get(Ctx, name), MDString::get(Ctx, type)};
		MDNode* mdNode = MDNode::get(Ctx, componentNode);
		namedMDNode->addOperand(mdNode);

		for(V vi = callInsts.begin(), ve = callInsts.end(); vi != ve; ++vi) {
			Value* v = (Value*) *vi;
			if(!isa<Instruction>(v)) continue;
			Instruction* I = (Instruction*) v;
			I->setMetadata("comp", mdNode);
		}

		return;
	}
	
	

	void EsperantoNamer::buildCalledFunctionList(){
		typedef Module::iterator FF;
		typedef Function::iterator BB;
		typedef BasicBlock::iterator II;
		LoadNamer& loadNamer = getAnalysis< LoadNamer >();
		for(FF fi = pM->begin(), fe = pM->end(); fi != fe; ++fi) {
			Function* f = (Function*) &*fi;
			if (f->isDeclaration()) continue;
			int functionid = loadNamer.getFunctionId(*f);
			if(functionid == 0) continue;

			for(BB bi = f->begin(), be = f->end(); bi != be; ++bi) {
				BasicBlock* b = (BasicBlock*) &*bi;

				for(II ii = b->begin(), ie = b->end(); ii != ie; ++ii) {
					Instruction* i = (Instruction*) &*ii;
					if (isa<CallInst>(i)){
						
						CallInst* cI = cast<CallInst>(i);
						Function* calledFunction = cI->getCalledFunction();
						if(loadNamer.getFunctionId(*calledFunction) ==0) continue;

						calledFunctionList.push_back(calledFunction);

					}
				}
			}
		}
	}

	 std::string EsperantoNamer::getClassNameInFunction(StringRef functionName){
		StringRef fName = functionName.substr(3,functionName.size()-3);
		//printf("fName = %s\n",fName.data());
		std::string ret = "";
		int classNameLength=0;
		int count = 0;
		for(int i=0;i<(int)fName.size();i++){
			if(47 < fName[i] && fName[i] <58){
				classNameLength *= 10;
				classNameLength += atoi(fName.data()+i);
				count++;
			}
			else
				break;
		}
		if(classNameLength !=0){
			for(int i=0;i<classNameLength;i++){
				ret.push_back(fName[count+i]);
			}
			return ret;
		}
		return ret;
	}

	void EsperantoNamer::isExistOrInsert(std::string cName){
		for(int i=0;i<(int)classNameList.size();i++){
			if(classNameList[i].compare(cName) == 0)
				return;
		}
		//printf("insert cName is %s\n",cName.c_str());
		classNameList.push_back(cName);
	}

	void EsperantoNamer::buildClassNameList(){
		typedef Module::iterator FF;
		typedef Function::iterator BB;
		typedef BasicBlock::iterator II;
		LoadNamer& loadNamer = getAnalysis< LoadNamer >();
		for(FF fi = pM->begin(), fe = pM->end(); fi != fe; ++fi) {
			Function* f = (Function*) &*fi;
			if (f->isDeclaration()) continue;
			int functionid = loadNamer.getFunctionId(*f);
			if(functionid == 0) continue;

			for(BB bi = f->begin(), be = f->end(); bi != be; ++bi) {
				BasicBlock* b = (BasicBlock*) &*bi;

				for(II ii = b->begin(), ie = b->end(); ii != ie; ++ii) {
					Instruction* i = (Instruction*) &*ii;
					if (isa<CallInst>(i)){
						
						CallInst* cI = cast<CallInst>(i);
						Function* calledFunction = cI->getCalledFunction();
						std::string className = getClassNameInFunction(calledFunction->getName());
						if(className.size() ==0) continue;
						//printf("class : %s\n",className.c_str());
						if(loadNamer.getFunctionId(*calledFunction) ==0) continue;
						if(pM->getNamedMetadata(className.c_str())){
							isExistOrInsert(className);	
						}
					}
				}
			}
		}
		/*std::vector<StructType*> classList = pM->getIdentifiedStructTypes();
		printf("DEBUG :: class number : %d\n",(int)classList.size());
		unsigned it = 0;
		for(it = 0; it<classList.size();it++){
			StringRef className = (classList[it]->getName()).substr(6,(classList[it]->getName()).size()-6);
			classNameList.push_back(className);
			printf("DEBUG :: className : %s\n",className.data());
		}*/
	}

	void EsperantoNamer::buildRemoteCallFunctionTable(){
		typedef std::vector<std::string>::iterator NI;
		typedef std::vector<Function*>::iterator FI;
	
		//printf("size of classNameList : %d\n",(int)classNameList.size());	
		
		for(NI ni = classNameList.begin(), ne = classNameList.end(); ni != ne; ++ni){
			std::string className = *ni;
			std::string size = std::to_string((int)(className.size()));
			StringRef classNameSize = StringRef(size.c_str());
			for(FI fi = calledFunctionList.begin(), fe = calledFunctionList.end(); fi != fe; ++fi){
				Function* calledFunction = (Function*) *fi;
				StringRef functionName = calledFunction->getName().substr(3,calledFunction->getName().size() -3);
				if(!functionName.startswith(classNameSize)) continue;
				StringRef functionNameWithoutSize = functionName.substr(size.size(),functionName.size()-size.size());
				if(!functionNameWithoutSize.startswith(StringRef(className))) continue;
				
				remoteCallFunctionTable[calledFunction] = true;
				remoteCallFunctions.push_back(calledFunction);
			}
		}
	
	}

	void EsperantoNamer::addMetadata() {
		LLVMContext &Ctx = pM->getContext();
		
		typedef std::vector<MetadataNode*>::iterator DL;
		typedef Module::iterator FF;
		typedef Function::iterator BB;
		typedef BasicBlock::iterator II;
		buildClassNameList();
		LoadNamer& loadNamer = getAnalysis< LoadNamer >();
		for(FF fi = pM->begin(), fe = pM->end(); fi != fe; ++fi) {
			Function* f = (Function*) &*fi;
			if (f->isDeclaration()) continue;
			int functionid = loadNamer.getFunctionId(*f);
			if(functionid == 0) continue;

			for(BB bi = f->begin(), be = f->end(); bi != be; ++bi) {
				BasicBlock* b = (BasicBlock*) &*bi;

				for(II ii = b->begin(), ie = b->end(); ii != ie; ++ii) {
					Instruction* i = (Instruction*) &*ii;
					if (isa<CallInst>(i)){
						
						CallInst* cI = cast<CallInst>(i);
						Function* calledFunction = cI->getCalledFunction();
						std::string className = getClassNameInFunction(calledFunction->getName());
						if(className.size() ==0) continue;
						//printf("class : %s\n",className.c_str());
						if(loadNamer.getFunctionId(*calledFunction) ==0) continue;
						NamedMDNode* espNMD = pM->getOrInsertNamedMetadata("esperanto.comp");
						NamedMDNode* nmd = pM->getNamedMetadata(className.c_str());
						if(nmd == nullptr) continue;
						
						MDNode* mdNode = nmd->getOperand(0);
						espNMD->addOperand(mdNode);
						i->setMetadata("comp",mdNode);
						StringRef deviceName = cast<MDString>(mdNode->getOperand(0).get())->getString();
						
						/*if(pM->getNamedMetadata(className.c_str()) != nullptr ){

						  isExistOrInsert(className);	
						  }*/
					}
				}
			}
		}
		deviceMap.print();
		/*
		for(DL mi = mdList.begin(), me = mdList.end(); mi != me; ++mi) {
			MetadataNode* M = (MetadataNode*) *mi;
			DeviceEntry* D = M->dev;
			Instruction* I = M->inst;

			MDNode* md = I->getMetadata("comp");
			if(md) continue;
			if(isa<CallInst>(I)){	
				CallInst* callInst = cast<CallInst>(I);
				if(!remoteCallFunctionTable[callInst->getCalledFunction()]) continue;
				StringRef name(D->name);
				StringRef type(getPlatformAsString(D->platform));
								

				NamedMDNode* namedMDNode = pM->getOrInsertNamedMetadata("esperanto.comp");
				Metadata* componentNode[] = {MDString::get(Ctx, name), MDString::get(Ctx, type)};
				MDNode* mdNode = MDNode::get(Ctx, componentNode);
				namedMDNode->addOperand(mdNode);
				I->setMetadata("comp", mdNode);
			}
		}*/
		
	}

	/*void EsperantoNamer::generateFunctionTableProfile(){
		
		FILE* output = fopen("functionTable.profile","w");
		typedef std::vector<Function*>::iterator FI;
		LoadNamer& loadNamer = getAnalysis< LoadNamer >();
		for(FI fi = remoteCallFunctions.begin(), fe = remoteCallFunctions.end(); fi != fe; ++fi){
			Function* func = (Function*)*fi;
			int functionID =  loadNamer.getFunctionId(*func);
			fprintf(output,"%d ",functionID);
		}	
		fclose(output);
	}*/
	
	bool EsperantoNamer::isCheckedOrEnroll(Value* v) {
		typedef std::vector<Value*>::iterator II;

		for(II ii = checkedValueInFunction.begin(), ie = checkedValueInFunction.end(); ii != ie; ++ii) {
			Value* V = (Value*) *ii;
			if (V == v) return true;
		}

		checkedValueInFunction.push_back(v);
		return false;
	}

	bool EsperantoNamer::checkUseDefChains() {
		typedef std::vector<MetadataNode*>::iterator DL;
		bool isChanged = false;

		std::vector<MetadataNode*> temp;

		for(DL ii = mdListBuild.begin(), ie = mdListBuild.end(); ii != ie; ++ii) {
			MetadataNode* M = (MetadataNode*) *ii;
			Instruction* I = M->inst;

			typedef Value::user_iterator U;
			for(U ui = I->user_begin(), ue = I->user_end(); ui != ue; ++ui) {
				User* u = (User*) *ui;
				if(isa<Instruction>(u)) {
					Instruction* i = (Instruction*) u;
					bool isChecked = isCheckedOrEnroll(i);
					isChanged |= !isChecked;

					if(!isChecked) {
						MetadataNode* newM = new MetadataNode();
						newM->dev = M->dev;
						newM->inst = i;
						temp.push_back(newM);
					}
				}
			}
		}
		mdList.insert(mdList.end(), mdListBuild.begin(), mdListBuild.end());
		mdListBuild.clear();
		mdListBuild.insert(mdListBuild.end(), temp.begin(), temp.end());
		temp.clear();

		return isChanged;
	}
	void EsperantoNamer::initUseDefChains() {
		typedef Module::iterator FF;
		typedef Function::iterator BB;
		typedef BasicBlock::iterator II;
		LoadNamer& loadNamer = getAnalysis< LoadNamer >();
		
		for(FF FI = pM->begin(), FE = pM->end(); FI != FE; ++FI) {
			Function* F = (Function*) &*FI;
			if (F->isDeclaration()) continue;
			int functionId = loadNamer.getFunctionId(*F);
			if(functionId == 0) continue;

			for(BB BI = F->begin(), BE = F->end(); BI != BE; ++BI) {
				BasicBlock* B = (BasicBlock*) &*BI;

				for(II Ii = B->begin(), Ie = B->end(); Ii != Ie; ++Ii) {
					Instruction* I = (Instruction*) &*Ii;
					MDNode* md = I->getMetadata("comp");
					if (md == NULL) continue;

					StringRef compName = cast<MDString>(md->getOperand(0).get())->getString();
					StringRef compType = cast<MDString>(md->getOperand(1).get())->getString();
					
					MetadataNode* mdn = new MetadataNode();
					mdn->dev = getOrInsertDeviceList(compName.data(), compType.data());
					mdn->inst = I;
					isCheckedOrEnroll(I);
					mdListBuild.push_back(mdn);
				}
			}
		}
	}

	bool EsperantoNamer::isCheckedOrEnroll(Instruction* target) {
		typedef std::vector<Instruction*>::iterator II;

		for(II ii = alreadyChecked.begin(), ie = alreadyChecked.end(); ii != ie; ++ii) {
			Instruction* I = (Instruction*) *ii;
			if (I == target) return true;
		}

		alreadyChecked.push_back(target);
		return false;
	}

	EsperantoNamer::EsperantoPlatform EsperantoNamer::getPlatform(const char* compType) {
		// Case that metadata node matched
		if(strcmp("x86", compType) == 0)
			return X86;
		if(strcmp("arm", compType) == 0)
			return ARM;
		if(strcmp("avr", compType) == 0)
			return AVR;

		// return default case
		assert(compType != NULL && "Error: There is no certain platform in system");
		return defaultPlatform;
	}
	
	const char* EsperantoNamer::getPlatformAsString(EsperantoNamer::EsperantoPlatform E) {
		switch(E) {
			case EsperantoPlatform::X86:
			return "x86";
			case EsperantoPlatform::ARM:
			return "arm";
			case EsperantoPlatform::AVR:
			return "avr";
		}
		return "";
	}

	EsperantoNamer::DeviceEntry* EsperantoNamer::getDeviceInfo(Function* F) {
		/// Match the named metadata to device. Default device(no marked) is gateway, 
		/// assuming that gateway has x86 platform defaultely.
		typedef Module::named_metadata_iterator NM;
		for(NM NMI = pM->named_metadata_begin(), NME = pM->named_metadata_end(); NMI != NME; ++NMI) {
			NamedMDNode* NMD = &*NMI;

			// Case that metadata node matched
			if(F->getName().equals(NMD->getName())) {
				if(NMD->getNumOperands() == 0) return NULL;
				MDNode* MN = NMD->getOperand(0);

				if(MN->getNumOperands() == 0) return NULL;
				StringRef name = cast<MDString>(MN->getOperand(0))->getString();
				StringRef type = cast<MDString>(MN->getOperand(1))->getString();
				DeviceEntry* dev = getOrInsertDeviceList(name.data(), type.data());
				return dev;
				break;
			}
		}
		return NULL;
	}

	EsperantoNamer::DeviceEntry* EsperantoNamer::getOrInsertDeviceList(const char* compName, const char* compType) {
		typedef std::vector<DeviceEntry*>::iterator D;
		DeviceEntry* d;

		for (D it = deviceList.begin(), ie = deviceList.end(); it != ie; ++it) {
			d = *it;
			if(strcmp(d->name, compName) == 0)
				return d;
		}

		// case undefined compName
		assert(compType != NULL && "Error: There is no enrolled device has compName and compType points NULL");

		d = new DeviceEntry();
		strcpy(d->name, compName);
		d->platform = getPlatform(compType);
		deviceList.push_back(d);

		return d;
	}

	int EsperantoNamer::getIdCount(DeviceEntry* dev) {
		assert(deviceIdMap.find(dev) != deviceIdMap.end() && "there is no mapping for device");
		return deviceIdMap[dev];
	}
	
	int EsperantoNamer::getIdCount(CallInst* call) {
		DeviceEntry* dev = callMap[call];
		return getIdCount(dev);
	}

	void EsperantoNamer::setMaps() {
		typedef Module::iterator FF;
		typedef Function::iterator BB;
		typedef BasicBlock::iterator II;
		LoadNamer& loadNamer = getAnalysis< LoadNamer >();
		for(FF FI = pM->begin(), FE = pM->end(); FI != FE; ++FI) {
			Function* F = (Function*) &*FI;
			if (F->isDeclaration()) continue;
			int functionId = loadNamer.getFunctionId(*F);
			if(functionId == 0) continue;

			for(BB BI = F->begin(), BE = F->end(); BI != BE; ++BI) {
				BasicBlock* B = (BasicBlock*) &*BI;

				for(II Ii = B->begin(), Ie = B->end(); Ii != Ie; ++Ii) {
					Instruction* I = (Instruction*) &*Ii;
					if(!isa<CallInst>(I)) continue;

					MDNode* md = I->getMetadata("comp");
					if (md == NULL) continue;
					CallInst* C = (CallInst*)I;

					StringRef compName = cast<MDString>(md->getOperand(0).get())->getString();
					StringRef compType = cast<MDString>(md->getOperand(1).get())->getString();

					DeviceEntry* dev = getOrInsertDeviceList(compName.data(), compType.data());
					
					callMap[C] = dev;
				}
			}
		}
	
		int idCount = 1; // index starts from 1
		typedef std::map<CallInst*, DeviceEntry*>::iterator MI;

		for(MI ii = callMap.begin(), ie = callMap.end(); ii != ie; ++ii) {
			DeviceEntry* dev = ii->second;
			if (deviceIdMap.find(dev) == deviceIdMap.end()) {
				deviceIdMap[dev] = idCount;
				++idCount;
			}
		}

		return;
	}


	void EsperantoNamer::printSpecs() {
		std::vector<MDNode*> mds;
		typedef Module::iterator FF;
		typedef Function::iterator BB;
		typedef BasicBlock::iterator II;
		int classID = 0;

		char specDoc[20] = "spec.doc";
		FILE* specFile = fopen(specDoc, "w");

		for(FF FI = pM->begin(), FE = pM->end(); FI != FE; ++FI) {
			Function* F = (Function*) &*FI;
			if (F->isDeclaration()) continue;

			for(BB BI = F->begin(), BE = F->end(); BI != BE; ++BI) {
				BasicBlock* B = (BasicBlock*) &*BI;

				for(II Ii = B->begin(), Ie = B->end(); Ii != Ie; ++Ii) {
					Instruction* targetInstruction = (Instruction*) &*Ii;
					if(!isa<AllocaInst>(targetInstruction)) continue;
					AllocaInst* allocaInst = (AllocaInst*)targetInstruction;
					MDNode* typeMD = targetInstruction->getMetadata("type");
					if (typeMD == NULL) continue;
					StringRef className = cast<MDString>(typeMD->getOperand(0).get())->getString();
					NamedMDNode* classNM = pM->getOrInsertNamedMetadata(className.data());
					MDNode* md = classNM->getOperand(0);
					if (std::find(mds.begin(), mds.end(), md) == mds.end()) {
						StringRef compName = cast<MDString>(md->getOperand(0).get())->getString();
						StringRef compType = cast<MDString>(md->getOperand(1).get())->getString();
						fprintf(specFile, "%s %s\n", compName.data(), compType.data());
						mds.push_back(md);
						DeviceMapEntry* temp = new DeviceMapEntry();
						temp->setName(compName.data());
						temp->setID(classID);
						if(deviceMap.getEntry(temp) == nullptr){
							deviceMap.insertEntry(temp);
							printf("DEBUG :: className = %s, classID : %d\n",temp->deviceName,classID);
							classID++;
						}

					}
				}
			}
		}
		fclose(specFile);
	}
}

