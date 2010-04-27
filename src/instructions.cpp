#include "instructions.h"

#include <stdexcept>
#include <iomanip>

class HexCode {
public:
  HexCode(byte b): MyByte(b) {}

  byte MyByte;
};

std::ostream& operator<<(std::ostream& out, const HexCode& hex) {
  out << std::hex << std::setfill('0') << std::setw(2) << (unsigned short)hex.MyByte;
  return out;
}

std::string Instruction::toString() const {
  std::stringstream buf;
  switch (OpCode) {
    case LIT_OP:
      buf << "Literal 0x" << HexCode(Op.Literal) << "/'" << Op.Literal << '\'';
      break;
    case EITHER_OP:
      buf << "Either 0x" << HexCode(Op.Range.First) << "/'" << Op.Range.First << "', 0x" << HexCode(Op.Range.Last) << "/'" << Op.Range.Last << '\'';
      break;
    case JUMP_OP:
      buf << "Jump 0x" << std::hex << std::setfill('0') << std::setw(8) << Op.Offset << '/' << std::dec << Op.Offset;
      break;
    case MATCH_OP:
      buf << "Match";
      break;
    case SAVE_LABEL_OP:
      buf << "SaveLabel " << Op.Offset;
      break;
    default:
      buf << "* UNRECOGNIZED *";
  };
  return buf.str();
}

Instruction Instruction::makeLit(byte b) {
  Instruction i;
  i.OpCode = LIT_OP;
  i.Size = 0;
  i.Op.Literal = b;
  return i;
}

Instruction Instruction::makeEither(byte one, byte two) {
  Instruction i;
  i.OpCode = EITHER_OP;
  i.Size = 0;
  i.Op.Range.First = one;
  i.Op.Range.Last = two;
  return i;
}

Instruction Instruction::makeJump(uint32 relativeOffset) {
  // we should have a two-word version for if relativeOffset >= 2^24
  // for now, we'll just throw a runtime error
  if (relativeOffset >= (1 << 24)) {
    THROW_WITH_OUTPUT(std::overflow_error, "jump offsets are 24 bit; specified offset was " << relativeOffset);
  }
  Instruction i;
  i.OpCode = JUMP_OP;
  i.Size = 0;
  i.Op.Offset = relativeOffset;
  return i;
}

Instruction Instruction::makeMatch() {
  Instruction i;
  i.OpCode = MATCH_OP;
  i.Size = 0;
  return i;
}

Instruction Instruction::makeSaveLabel(uint32 label) {
  Instruction i = makeJump(label);
  i.OpCode = SAVE_LABEL_OP;
  return i;
}

std::ostream& operator<<(std::ostream& out, const Instruction& instr) {
  out << instr.toString();
  return out;
}