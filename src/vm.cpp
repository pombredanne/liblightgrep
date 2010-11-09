#include "vm.h"

#include <iostream>
#include <algorithm>
#include <cctype>
#include <iomanip>

static uint64 UNALLOCATED = std::numeric_limits<uint64>::max();

std::ostream& operator<<(std::ostream& out, const Thread& t) {
  out << "{ \"pc\":" << std::hex << t.PC << ", \"Label\":" << std::dec << t.Label << ", \"Start\":" << t.Start << ", \"End\":"
    << t.End << " }";
  return out;
}

void Thread::output(std::ostream& out, const Instruction* base) const {
  out << "{ \"pc\":" << (PC ? PC - base: -1) << ", \"Label\":" << Label << ", \"Start\":" << Start << ", \"End\":" << End << " }";
}

void printThreads(const Vm::ThreadList& list, uint64 offset, const Instruction* base) { // can only be called if list is not empty
  std::cerr << "{\"offset\":" << offset << ", \"num\":" << list.size() << ", \"list\":[";
  Vm::ThreadList::const_iterator threadIt = list.begin();
  threadIt->output(std::cerr, base);
  while (++threadIt != list.end()) {
    std::cerr << ", ";
    threadIt->output(std::cerr, base);
  }
  std::cerr << "]}\n";  
}




Vm::Vm() : BeginDebug(UNALLOCATED), EndDebug(UNALLOCATED) {}

Vm::Vm(ProgramPtr prog): 
  BeginDebug(UNALLOCATED), EndDebug(UNALLOCATED)
{
  init(prog);
}

void Vm::init(ProgramPtr prog) {
  Prog = prog;
  Active.resize(Prog->size());
  Next.resize(Prog->size());
  uint32 numPatterns = 0,
         numCheckedStates = 0;
  Program& p(*Prog);
  for (uint32 i = 0; i < p.size(); ++i) {
    if (p[i].OpCode == MATCH_OP && numPatterns < p[i].Op.Offset) {
      numPatterns = p[i].Op.Offset;
    }
    if (p[i].OpCode == CHECK_BRANCH_OP || p[i].OpCode == CHECK_HALT_OP) {
      numCheckedStates = std::max(numCheckedStates, p[i].Op.Offset);
    }
  }
  ++numPatterns;
  numCheckedStates += 2; // bit 0 reserved for whether any bits were flipped
  Matches.resize(numPatterns);
  CheckStates.resize(numCheckedStates);
  reset();
}

void Vm::reset() {
  Active.clear();
  Next.clear();
  CheckStates.assign(CheckStates.size(), false);
  Matches.assign(Matches.size(), std::pair<uint64, uint64>(UNALLOCATED, 0));
}

inline bool Vm::_execute(Thread& t, const byte* cur) {
  // std::string instr;
  // std::cerr << t << std::endl;
  // instr = t.PC->toString(); // for some reason, toString() is corrupting the stack... maybe?
  // std::cerr << instr << std::endl;
  // std::cout << t << ": " << *t.PC << std::endl;
  Thread nextT;
  register Instruction instr = *t.PC;
  switch (instr.OpCode) {
    case LIT_OP:
      // std::cerr << "Lit " << t.PC->Op.Literal << std::endl;
      if (*cur == instr.Op.Literal) {
        t.advance();
        return true;
      }
      break;
    case EITHER_OP:
      if (*cur == instr.Op.Range.First || *cur == instr.Op.Range.Last) {
        t.advance();
        return true;
      }
      break;
    case RANGE_OP:
      if (instr.Op.Range.First <= *cur && *cur <= instr.Op.Range.Last) {
        t.advance();
        return true;
      }
      break;
    case BIT_VECTOR_OP:
      {
        const ByteSet* setPtr = reinterpret_cast<const ByteSet*>(t.PC + 1);
        if ((*setPtr)[*cur]) {
          t.advance();
          return true;
        }
      }
      break;
    case JUMP_TABLE_OP:
      t.jump(t.PC, 1 + *cur);
      if (t.PC->OpCode != HALT_OP) {
        return true;
      }
      break;
    case JUMP_TABLE_RANGE_OP:
      if (instr.Op.Range.First <= *cur && *cur <= instr.Op.Range.Last) {
        t.jump(t.PC, 1 + (*cur - instr.Op.Range.First));
        if (t.PC->OpCode != HALT_OP) {
          return true;
        }
      }
      break;
  }
  t.PC = 0;
  return false;
}

// while base is always == &Program[0], we pass it in because it then should get inlined away
inline bool Vm::_executeEpsilon(const Instruction* base, Thread& t, uint64 offset) {
  Thread f;
  register Instruction instr = *t.PC;
  switch (instr.OpCode) {
    case JUMP_OP:
      t.jump(base, instr.Op.Offset);
      return true;
    case FORK_OP:
      f.fork(t, base, instr.Op.Offset);
      Active.push_back(f);
      t.advance();
      return true;
    case CHECK_HALT_OP:
      if (CheckStates[instr.Op.Offset]) {
        t.PC = 0;
        return false;
      }
      else {
        CheckStates[instr.Op.Offset] = true;
        CheckStates[0] = true;
        t.advance();
        return true;
      }
    case MATCH_OP:
      t.Label = instr.Op.Offset;
      t.End = offset;
      t.advance();
      return true;
    case HALT_OP:
      t.PC = 0;
      return false;
    default:
      Next.push_back(t);
      return false;
  }
}

inline void Vm::_executeFrame(const ByteSet& first, ThreadList::iterator& threadIt, const Instruction* base, const byte* cur, uint64 offset, HitCallback& hitFn) {
  while (threadIt != Active.end()) {
    if (_execute(*threadIt, cur)) {
      while (_executeEpsilon(base, *threadIt, offset)) ;
      if (threadIt->End == offset) {
        doMatch(threadIt, hitFn);
      }
    }
    ++threadIt;
  }
  if (first[*cur]) {
    Active.addBack().init(base, offset);
    do {
      if (_execute(*threadIt, cur)) {
        while (_executeEpsilon(base, *threadIt, offset)) ;
        if (threadIt->End == offset) {
          doMatch(threadIt, hitFn);
        }
      }
    } while (++threadIt != Active.end());
  }
}

bool Vm::execute(Thread& t, const byte* cur) {
  return _execute(t, cur);
}

bool Vm::executeEpsilon(Thread& t, uint64 offset) {
  return _executeEpsilon(&(*Prog)[0], t, offset);
}

void Vm::executeFrame(const byte* cur, uint64 offset, HitCallback& hitFn) {
  ThreadList::iterator threadIt = Active.begin();
  _executeFrame(Prog->First, threadIt, &(*Prog)[0], cur, offset, hitFn);
}

void Vm::doMatch(register ThreadList::iterator threadIt, HitCallback& hitFn) {
  // std::cerr << "had a match" << std::endl;
  SearchHit  hit;
  std::pair< uint64, uint64 > lastHit = Matches[threadIt->Label];
  if (lastHit.first == UNALLOCATED || (lastHit.first == threadIt->Start && lastHit.second < threadIt->End)) {
    Matches[threadIt->Label] = std::make_pair(threadIt->Start, threadIt->End);
  }
  else if (lastHit.second <= threadIt->Start) {
    hit.Offset = lastHit.first;
    hit.Length = lastHit.second - lastHit.first;
    hit.Label = threadIt->Label;
    hitFn.collect(hit);
    // std::cerr << "emitting hit " << hit << std::endl;
    Matches[threadIt->Label] = std::make_pair(threadIt->Start, threadIt->End);
  }
}

inline void Vm::cleanup() {
  Active.swap(Next);
  Next.clear();
  if (CheckStates[0]) {
    CheckStates.assign(CheckStates.size(), false);
  }
}

bool Vm::search(register const byte* beg, register const byte* end, uint64 startOffset, HitCallback& hitFn) {
  const Instruction* base = &(*Prog)[0];
  SearchHit  hit;
  register uint64     offset = startOffset;
  // register uint32     window = Prog->Skip ? Prog->Skip->l_min() - 1: 1;
  // register uint32     curDiff;
  // const std::vector<uint32>* skipTbl = Prog->Skip ? &Prog->Skip->skipVec(): SkipTblPtr.get();
  // if (!skipTbl) {
  //   SkipTblPtr.reset(new std::vector<uint32>(256, 0));
  //   skipTbl = SkipTblPtr.get();
  // }
  // register const byte* guard;
  // register byte value;
  ByteSet first = Prog->First;
  register ThreadList::iterator threadIt = Active.begin();
  for (register const byte* cur = beg; cur < end; ++cur) {
    while (threadIt != Active.end()) {
      while (_execute(*threadIt, cur)) ;
      if (threadIt->End == offset) {
        doMatch(threadIt, hitFn);
      }
      ++threadIt;
    }
    // guard   = cur + window >= end ? end - 1: cur + window;
    // curDiff = guard - cur;
    // value   = *guard;
    // while (curDiff > 0 && (*skipTbl)[value] <= curDiff) {
    //   --curDiff;
    //   value = *(--guard);
    // }
    // if (guard == cur && (*skipTbl)[value] == 0) {
    if (first[*cur]) {
      Active.addBack().init(base, offset);
      do {
        while (_execute(*threadIt, cur)) ;
        if (threadIt->End == offset) {
          doMatch(threadIt, hitFn);
        }
      } while (++threadIt != Active.end());
    }
    if (threadIt != Active.begin()) {
      #ifdef LBT_TRACE_ENABLED
      if (BeginDebug <= offset && offset < EndDebug) {
        printThreads(Active, offset, base);
      }
      #endif
      cleanup();
      threadIt = Active.begin();
    }
    // else {
    //   offset += curDiff;
    //   cur = guard;
    // }
    ++offset;
  }
  // this flushes out last char matches
  // and leaves us only with comparison instructions (in next)
  for (threadIt = Active.begin(); threadIt != Active.end(); ++threadIt) {
    while (_executeEpsilon(base, *threadIt, offset)) ;
    if (threadIt->End == offset) {
      doMatch(threadIt, hitFn);
    }
  }
  for (uint32 i = 0; i < Matches.size(); ++i) {
    if (Matches[i].first < UNALLOCATED) {
      hit.Offset = Matches[i].first;
      hit.Length = Matches[i].second - hit.Offset;
      hit.Label = i;
      hitFn.collect(hit);
      Matches[i] = std::make_pair(UNALLOCATED, 0ul);
    }
  }
  cleanup();
  // std::cerr << "Max number of active threads was " << maxActive << ", average was " << total/(end - beg) << std::endl;
  return Active.size() > 0; // potential hits, if there's more data
}
