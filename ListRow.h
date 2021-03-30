#ifndef LISTROW_H
#define LISTROW_H
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

class ListRow
{
public:
    ListRow();
    ListRow(unsigned int addr, std::string label, std::string op, 
    std::string opPrefix, std::string operand, std::string operandPrefix, 
    std::string operandSuffix, std::string objCode);
    void print(std::ofstream &outfile);
    static std::string intToHexString(unsigned int num, int digits);
    unsigned int addr;
    std::string label;
    std::string op;
    std::string opPrefix;
    std::string operand;
    std::string operandPrefix;
    std::string operandSuffix;
    std::string objCode;
};
#endif