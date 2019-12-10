//===- Project.cpp - CS6620 class project ---------------===//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
//#include "llvm/Analysis/OrderedBasicBlock.h"
#include <map> 

using namespace llvm;
using namespace std;

#define DEBUG_TYPE "compproject"

STATISTIC(ProjectCounter, "Counts number of functions greeted");

namespace {

	
  	// Project - The first implementation, without getAnalysisUsage.
  	class Project : public FunctionPass {
		//maps to store input/output data size to a basic block
		map< BasicBlock *, int> inputDataSize;
		map< BasicBlock *, bool> inputDataSizeConfidence;
		map< BasicBlock *, int> outputDataSize;
		map< BasicBlock *, bool> outputDataSizeConfidence;
		map< Value *, int> pointersize;
	
		
		public:
  	  	static char ID; // Pass identification, replacement for typeid
  	  	Project() : FunctionPass(ID) {}

  	  	bool runOnFunction(Function &F) override {
			//TODO tobe used in future 
  	  	  	DependenceInfo *DI = &getAnalysis<DependenceAnalysisWrapperPass>().getDI();
			MemoryDependenceResults *MD = &getAnalysis<MemoryDependenceWrapperPass>().getMemDep();
			pass(F,*DI,MD);
			print();
  	  	  	return false;
  	  	}

		void getAnalysisUsage(AnalysisUsage &AU) const override {
     			AU.setPreservesCFG();
     			AU.addRequired<DependenceAnalysisWrapperPass>();
     			AU.addRequired<MemoryDependenceWrapperPass>();
   		}

		void pass(Function &F, DependenceInfo &DI,MemoryDependenceResults *MD){
			//Data layout used to get the allocated bitsize of scalers
			const Module* M = F.getParent();	
			const DataLayout DL(M);	
			//initialize maps
			//Note using two arrays might be more efficient
			for (BasicBlock &BB : F){
				inputDataSize[&BB] = 0;
				inputDataSizeConfidence[&BB] = true;
				outputDataSize[&BB] = 0;
				outputDataSizeConfidence[&BB] = true;
			}

			for (BasicBlock &BB : F){
				//iterate over basic block for allocation def use chain for stores
				for (BasicBlock::iterator BBI = BB.begin(), BBE = BB.end(); BBI != BBE; ) {
 
     					Instruction *Inst = &*BBI++;
					//if not a allocate instruction skip
					if(!(isa<AllocaInst>(Inst)))
						continue;
					for (User *U : Inst->users()) {
						//cast to instruction type
  						if (Instruction *use = dyn_cast<Instruction>(U)) {
							//if store instruction, add store size to outputSize map
							if(isa<StoreInst>(use)){
								//TODO need to handle different operand types
								//Handling store to pointer
								Value * val = cast<StoreInst>(use)->getOperand(1);
								Type *t = cast<PointerType>(val->getType())->getElementType();
								if(isa<PointerType>(t)){
									//Find calloc  and return size otherwise return -1
									//TODO extend to malloc and arrays
									pointersize[Inst] = findCallocSize(use);
								}
								//handling of a basic store
								else{
									Optional< uint64_t > a = (cast<AllocaInst>(Inst))->getAllocationSizeInBits(DL);
									outputDataSize[use->getParent()] += a.getValueOr(0);

								}
							}					
						}
  					}			
				}
				//iterate over basic block for allocation def use chain for loads
				for (BasicBlock::iterator BBI = BB.begin(), BBE = BB.end(); BBI != BBE; ) {
 
     					Instruction *Inst = &*BBI++;
					//if not a allocate instruction skip
					if(!(isa<AllocaInst>(Inst)))// || isa<StoreInst>(Inst)))
						continue;
					for (User *U : Inst->users()) {
						//cast to instruction type
  						if (Instruction *use = dyn_cast<Instruction>(U)) {	
							////if load instruction, add load size to inputSize map
							if(isa<LoadInst>(use)){
								//if allocation and use in same basic block skip
  					 			if(use->getParent() == Inst->getParent())
									continue;
								//get operand of load instruction
								Value * val = cast<LoadInst>(use)->getPointerOperand ();//getOperand();
								Type *t = cast<PointerType>(val->getType())->getElementType();
								if(isa<PointerType>(t)){
									//if size can be determined add to inputsize
									//if not change confidence value
									if(pointersize[Inst]>0)
										inputDataSize[use->getParent()] += pointersize[Inst];
									else
										inputDataSizeConfidence[use->getParent()] = false;
								}
								else{
									Optional< uint64_t > a = (cast<AllocaInst>(Inst))->getAllocationSizeInBits(DL);
									inputDataSize[use->getParent()] += a.getValueOr(0);
								}
							}

						}
  					}			
				}
			}

		}

		int findCallocSize(Instruction *I){
			for (Use &U : I->operands()) {
			      if(Instruction *def = dyn_cast<Instruction>(U)){
				//if bitcast continue to find call
			      	if(isa<BitCastInst>(def)){
					for (Use &U1 : def->operands()) {
			      			if(Instruction *def1 = dyn_cast<Instruction>(U1)){				
							if(isa<CallInst>(def1)){
								Function *F = dyn_cast<CallInst>(def1)->getCalledFunction();
                						StringRef fn_name = F->getName();
                						//errs() << fn_name << " : " << dyn_cast<CallInst>(def)->getArgOperand(2) << "\n";
								//for(auto arg = dyn_cast<CallInst>(def1)->arg_begin(); arg != dyn_cast<CallInst>(def1)->arg_end(); ++arg){
								//get second argument of calloc
								
								auto arg = dyn_cast<CallInst>(def1)->arg_begin();
								if(auto* ci = dyn_cast<ConstantInt>(arg)){
									//argument 1 number of bytes
									int num_bytes = (ci->getValue()).getLimitedValue();
									arg++;
									if(auto* ci1 = dyn_cast<ConstantInt>(arg))
										return num_bytes*(ci1->getValue()).getLimitedValue()*8 ;
									else
										return -1;
								}
								//errs() << ci->getValue() << "\n";
								else
									return -1;
								//errs() << *arg << "\n";
								//}
							}
						}
					}
				}
				else
					return -1;
			      }
			}	
		}
		void print(){
			errs()<<"Basic block pointer\tinput in bits\tConfidence\n";
			for ( map< BasicBlock *, int>::iterator IPI = inputDataSize.begin(), IPE = inputDataSize.end(); IPI != IPE; ){
				errs()<<IPI->first<<"\t\t"<<IPI->second<<"\t\t"<<inputDataSizeConfidence[IPI->first]<<"\n";
				IPI++;
			}
			
			errs()<<"\nBasic block pointer\toutput in bits\tConfidence\n";
			for ( map< BasicBlock *, int>::iterator OPI = outputDataSize.begin(), OPE = outputDataSize.end(); OPI != OPE; ){
				errs()<<OPI->first<<"\t\t"<<OPI->second<<"\t\t"<<outputDataSizeConfidence[OPI->first]<<"\n";
				OPI++;
			}
		}

  	};
}

char Project::ID = 0;
static RegisterPass<Project> X("project", "Project World Pass");


