#include "program.h"

#include <sstream>
#include <iostream>

bool Program::operator==(const Program& rhs) const {
  return NumChecked == rhs.NumChecked && First == rhs.First && std::equal(begin(), end(), rhs.begin());
}

std::string Program::marshall() const {
  std::stringstream buf;
  buf.write((char*)&NumChecked, sizeof(NumChecked));
  buf.write((char*)&First, sizeof(First));
  for (auto instruction: *this) {
    buf.write((char*)&instruction, sizeof(instruction));
  }
  std::streampos len = buf.tellp();
  std::string ret = buf.str();
//  ret.erase(ret.begin() + len, ret.end());
  return ret;
}

ProgramPtr Program::unmarshall(const std::string& s) {
  ProgramPtr p(new Program);
  std::stringstream buf(s);
  buf.read((char*)&p->NumChecked, sizeof(uint32));
  buf.read((char*)&p->First, sizeof(ByteSet));
  Instruction i;
  while (buf.read((char*)&i, sizeof(Instruction))) {
    p->push_back(i);
  }
  std::cerr << "unmarshalled program of size " << s.size() << std::endl;
  return p;
}