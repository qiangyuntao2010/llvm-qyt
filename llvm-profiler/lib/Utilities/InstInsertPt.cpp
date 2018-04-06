#include <utility>
#include <ostream>
#include <sstream>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "corelab/Utilities/InstInsertPt.h"

namespace corelab {

  using namespace llvm;

	Instruction *getNextInstruction(Instruction *i){
		BasicBlock::iterator I(i);
		if (++I == i->getParent()->end())
			return nullptr;
		return (Instruction *)&*I;
	}

  void InstInsertPt::print(llvm::raw_ostream &out) const{
    if( invalid ) {
      out << "Invalid InstInsertPt\n";
      return;
    }

    out << "InstInsertPt @ fcn "
        << pos->getParent()->getParent()->getName().str()
        << " block "
        << pos->getParent()->getName().str()
        << '\n';

    if( _before )
      out << "Before inst: ";
    else
      out << "After inst: ";
    pos->print(out);

    out << "Basic Block:\n";
    pos->getParent()->print(out);
  }

  void InstInsertPt::dump() const {
    print(errs());
  }

  void InstInsertPt::insert(Instruction *i) {
    assert( !invalid );

    if( _before ) {
      // This error would be caught eventually by the module verifier
      // but its easier if we know right when it happens
      if( isa<PHINode>(pos) && ! isa<PHINode>(i) ) {
        errs() << "Attempting to insert a non-PHI before a PHI!\n"
             << "non-PHI is:\n";
        i->dump();
        errs() << "Position is:\n";
        dump();
        assert(0);
      }

      i->insertBefore(pos);
    } else {
      // This error would be caught eventually by the module verifier
      // but its easier if we know right when it happens
      if( !isa<PHINode>(pos) && isa<PHINode>(i) ) {
        errs() << "Attempting to insert a PHI after a non-PHI!\n"
             << "PHI is:\n";
        i->dump();
        errs() << "Position is:\n";
        dump();
        assert(0);
      }

      // Attempting to insert a non-PHI after a PHI is
      // valid, but pos might not be the last PHI in the block.
      // If so, skip...
      if( !isa<PHINode>(i) ) {
        // We are not inserting a PHI

        // Make sure we are past all of the PHIs in this block.
       // while( PHINode *next = dyn_cast<PHINode>( ilist_nextprev_traits<Instruction>::getNext(pos) ) ) {
				while( PHINode *next = dyn_cast<PHINode>( getNextInstruction(pos) )){
				
          errs() << "InstInsertPt: skipping PHIs\n";
          pos = next;
        } 
      }

      i->insertAfter(pos);
      pos = i;
    }
  }


}
