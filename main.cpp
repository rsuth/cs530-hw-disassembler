// CS530 Hw1 - SIC/XE Disassembler
// Richard Sutherland, RED ID: 8122192

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

const int OPTABLE_LEN = 43;

// Opcode datatype
struct OpCode
{
    int code;
    std::string mnemonic;
    bool format2;
};

const struct OpCode OP_TABLE[] = {{0x18, "ADD", false}, {0x58, "ADDF", false}, {0x40, "AND", false}, {0xB4, "CLEAR", true}, {0x28, "COMP", false}, {0x88, "COMPF", false}, {0x24, "DIV", false}, {0x64, "DIVF", false}, {0x3C, "J", false}, {0x30, "JEQ", false}, {0x34, "JGT", false}, {0x38, "JLT", false}, {0x48, "JSUB", false}, {0x00, "LDA", false}, {0x68, "LDB", false}, {0x50, "LDCH", false}, {0x70, "LDF", false}, {0x08, "LDL", false}, {0x6C, "LDS", false}, {0x74, "LDT", false}, {0x04, "LDX", false}, {0xD0, "LPS", false}, {0x20, "MUL", false}, {0x60, "MULF", false}, {0x44, "OR", false}, {0xD8, "RD", false}, {0x4C, "RSUB", false}, {0xEC, "SSK", false}, {0x0C, "STA", false}, {0x78, "STB", false}, {0x54, "STCH", false}, {0x80, "STF", false}, {0xD4, "STI", false}, {0x14, "STL", false}, {0x7C, "STS", false}, {0xE8, "STSW", false}, {0x84, "STT", false}, {0x10, "STX", false}, {0x1C, "SUB", false}, {0x5C, "SUBF", false}, {0xE0, "TD", false}, {0x2C, "TIX", false}, {0xDC, "WD", false}};
const std::string REGISTER_TABLE[] = {"A", "X", "L", "B", "S", "T", "F"};

// List file row datatype
struct ListRow
{
    unsigned int addr;
    std::string label;
    std::string op;
    std::string opPrefix;
    std::string operand;
    std::string operandPrefix;
    std::string operandSuffix;
    std::string objCode;
    bool needLTORG;
};

struct SymbolEntry
{
    std::string name;
    unsigned int value;
    std::string flag;

    bool operator<(const SymbolEntry &s) const
    {
        return (value < s.value);
    }
};

struct LitEntry
{
    std::string name;
    std::string lit;
    int length;
    unsigned int address;
};

std::string checkSymbolTable(unsigned int value, std::vector<SymbolEntry> symVec)
{
    std::string ret = "";
    for (int i = 0; i < symVec.size(); i++)
    {
        if (symVec[i].value == value)
        {
            ret = symVec[i].name;
        }
    }
    return ret;
}

struct LitEntry checkLitTable(unsigned int addr, std::vector<LitEntry> litVec)
{
    LitEntry ret = {"", "", -1, 0};
    for (int i = 0; i < litVec.size(); i++)
    {
        if (litVec[i].address == addr)
        {
            ret = litVec[i];
        }
    }
    return ret;
}

OpCode getOpCode(unsigned int code)
{
    // use bitmask to get the first 6 bits
    int firstSixBits = code & 0xFC;

    // search optable for code
    for (int i = 0; i < OPTABLE_LEN; i++)
    {
        if (firstSixBits == OP_TABLE[i].code)
        {
            return OP_TABLE[i];
        }
    }
    return {-1, "ERROR", false};
}

// reads number and fills the nixbpe array
void getFlagBits(int code, bool (&nixbpe)[6])
{
    for (int i = 0; i < 6; i++)
    {
        nixbpe[5 - i] = (code >> i) & 1;
    }
}

// calculates the operand from the disp/address
unsigned int getOperandAddress(std::string disp, const bool nixbpe[6], const int pc, const int base)
{
    unsigned int ta = std::stoi(disp, NULL, 16);

    if (nixbpe[3]) // base relative
    {
        ta += base;
    }
    if (nixbpe[4]) // pc relative
    {
        // negative
        if (ta > 0x7FF)
        {
            ta = -1 * ((0xFFF - ta) + 1);
        }
        ta += pc;
    }
    return ta;
}

std::string getOperandPrefix(const bool nixbpe[6])
{
    if (nixbpe[0] == nixbpe[1])
    {
        // simple addressing
        return " ";
    }
    else if (nixbpe[0])
    {
        // indirect
        return "@";
    }
    else if (nixbpe[1])
    {
        // immediate
        return "#";
    }
    else
    {
        return " ";
    }
}

std::string getOperandSuffix(const bool nixbpe[6])
{
    if (nixbpe[2])
    {
        return ",X";
    }
    else
    {
        return "  ";
    }
}

std::string intToHexString(unsigned int num, int digits)
{
    // https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
    std::stringstream sstream;
    if (digits > 0)
    {
        sstream << std::setfill('0')
                << std::setw(digits)
                << std::hex
                << std::uppercase
                << num;
    }
    else
    {
        sstream << std::hex << num;
    }
    return sstream.str();
}

struct ListRow readInstruction(unsigned int &locctr, unsigned int &pc, unsigned int &base,
                               int &cursor, const std::string txtRecord,
                               const std::vector<SymbolEntry> symVec,
                               const std::vector<LitEntry> litvec)
{
    struct ListRow ret =
        {
            0,
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            false};

    ret.addr = locctr;

    struct LitEntry lit = checkLitTable(locctr, litvec);
    if (lit.length > 0)
    {
        // this is not an instruction but a literal
        ret.opPrefix = " ";
        ret.op = "*";
        ret.operandPrefix = "=X'";
        ret.operand = lit.lit;
        ret.operandSuffix = "'";
        ret.objCode = txtRecord.substr(cursor, (lit.length));
        ret.needLTORG = true;

        // move cursor to start of next instruction
        cursor += lit.length;

        // update address counter
        locctr += lit.length / 2;

        return ret;
    }

    ret.label = checkSymbolTable(ret.addr, symVec);

    OpCode op = getOpCode(std::stoi(txtRecord.substr(cursor, 2), NULL, 16));
    int instructionBytes = 0;
    if (!op.format2)
    {
        // set flag bits
        bool nixbpe[] = {0, 0, 0, 0, 0, 0};
        getFlagBits(std::stoi(txtRecord.substr(cursor + 1, 2), NULL, 16), nixbpe);
        ret.operandPrefix = getOperandPrefix(nixbpe);
        ret.operandSuffix = getOperandSuffix(nixbpe);

        if (nixbpe[5]) // this is a format 4 instruction
        {
            ret.op = op.mnemonic;
            instructionBytes = 4;
            pc += instructionBytes;
            ret.opPrefix = "+";
            std::string s = checkSymbolTable(std::stoi(txtRecord.substr(cursor + 3, 5), NULL, 16), symVec);
            if (!s.empty())
            {
                ret.operand = s;
            }
            else
            {
                // check literals table to see if a literal is referenced.
                struct LitEntry l = checkLitTable(std::stoi(txtRecord.substr(cursor + 3, 5), NULL, 16), litvec);
                if (l.length > 0)
                {
                    // found a match in the literals table.
                    ret.operandPrefix = "=X'";
                    ret.operand = l.lit;
                    ret.operandSuffix = "'";
                }
                else
                {
                    // its a constant or immediate.
                    ret.operand = intToHexString(std::stoi(txtRecord.substr(cursor + 3, 5), NULL, 16), -1);
                }
            }

            // if LDB we need to update base:
            if (ret.op == "LDB")
            {
                base = std::stoi(txtRecord.substr(cursor + 3, 5), NULL, 16);
            }
        }
        else // this is a format 3 instruction
        {
            ret.op = op.mnemonic;
            instructionBytes = 3;
            pc += instructionBytes;
            ret.opPrefix = " ";

            unsigned int addr = getOperandAddress(txtRecord.substr(cursor + 3, 3), nixbpe, pc, base);

            // check symbol table for address.
            std::string s = checkSymbolTable(addr, symVec);
            if (!s.empty())
            {
                // found a match in the symbol table.
                ret.operand = s;
            }
            else
            {
                // check literals table to see if a literal is referenced.
                struct LitEntry l = checkLitTable(addr, litvec);
                if (l.length > 0)
                {
                    // found a match in the literals table.
                    ret.operandPrefix = "=X'";
                    ret.operand = l.lit;
                    ret.operandSuffix = "'";
                }
                else
                {
                    // its a constant or immediate.
                    ret.operand = intToHexString(addr, -1);
                }
            }
            // if LDB we need to update base:
            if (ret.op == "LDB")
            {
                base = addr;
            }
        }
    }
    else
    {
        // format 2 (CLEAR)
        instructionBytes = 2;
        pc += instructionBytes;

        ret.opPrefix = " ";
        ret.operandPrefix = " ";
        ret.operandSuffix = " ";

        ret.op = op.mnemonic;
        ret.operand = REGISTER_TABLE[std::stoi(txtRecord.substr(cursor + 2, 1), NULL, 16)];
    }

    ret.objCode = txtRecord.substr(cursor, (instructionBytes * 2));

    // move cursor to start of next instruction
    cursor += (2 * instructionBytes);

    // update address counter
    locctr += instructionBytes;

    return ret;
}

// reads the object file into objVec
// return 1 for success, 0 for failure
bool readObjFile(const char *path, std::vector<std::string> &objVec)
{
    std::ifstream objFile(path);
    std::string tmp;

    if (!objFile)
    {
        return 0;
    }
    while (std::getline(objFile, tmp))
    {
        objVec.push_back(tmp);
    }
    return 1;
}

bool readSymFile(const char *path, std::vector<SymbolEntry> &symVec, std::vector<LitEntry> &litVec)
{
    std::ifstream symFile(path);
    std::vector<std::string> fileLines;
    std::string tmp;
    if (!symFile)
    {
        //problem opening file
        return 0;
    }
    while (std::getline(symFile, tmp))
    {
        // read all file lines into vector
        fileLines.push_back(tmp);
    }

    int i;
    for (i = 2; i < fileLines.size(); i++)
    // start at third line
    {
        if (fileLines[i].empty())
        // we've reached the end of the symbol table
        {
            i += 3;
            break;
        }
        else
        {
            struct SymbolEntry s;
            s.name = fileLines[i].substr(0, 6);
            s.value = std::stoi(fileLines[i].substr(8, 6), NULL, 16);
            s.flag = fileLines[i].substr(16, 1);
            symVec.push_back(s);
        }
    }
    for (int j = i; j < fileLines.size(); j++)
    {
        struct LitEntry l;
        l.name = fileLines[j].substr(0, 6);
        l.lit = fileLines[j].substr(11, 6);
        l.length = std::stoi(fileLines[j].substr(20, 1), NULL, 16);
        l.address = std::stoi(fileLines[j].substr(29, 6), NULL, 16);
        litVec.push_back(l);
    }
    return 1;
}

void printListRow(ListRow r)
{
    std::cout
        << intToHexString(r.addr, 4)
        << std::setw(4)
        << " "
        << std::setw(6)
        << r.label
        << std::setw(3)
        << " "
        << std::setw(6)
        << r.opPrefix + r.op
        << std::setw(4)
        << " "
        << std::left
        << std::setw(10)
        << r.operandPrefix + r.operand + r.operandSuffix
        << std::setw(4)
        << " "
        << r.objCode
        << std::endl;
}

void printBaseLine(std::string operand)
{
    std::cout
        << std::setw(17)
        << " "
        << std::setw(6)
        << " BASE"
        << std::setw(4)
        << " "
        << std::setw(6)
        << " " + operand
        << std::endl;
}

void printLTORGLine()
{
    std::cout << std::setfill(' ')
              << std::setw(18)
              << " "
              << std::setw(6)
              << " LTORG"
              << std::endl;
}

int main(int argc, char **argv)
{
    std::vector<std::string> objVect;
    std::vector<LitEntry> litVect;
    std::vector<SymbolEntry> symVect;

    if (argc < 3)
    {
        std::cout << "You have not supplied enough input files.";
        return -1;
    }

    if (!readObjFile(argv[1], objVect))
    {
        std::cout << "Cannot open supplied .obj file.";
        return -1;
    }
    if (!readSymFile(argv[2], symVect, litVect))
    {
        std::cout << "Cannot open supplied .sym file";
        return -1;
    };

    unsigned int base = 0;

    // redirect cout to a file
    // not good but it works
    std::ofstream out("out.lst");
    std::cout.rdbuf(out.rdbuf());

    for (int i = 0; i < objVect.size(); i++)
    {
        if (objVect[i][0] == 'T') // text record
        {
            std::string txtRec = objVect[i];
            int cursor = 9;
            unsigned int locctr = std::stoi(txtRec.substr(1, 6), NULL, 16);
            unsigned int pc = locctr;
            int recordLength = 2 * std::stoi(txtRec.substr(7, 2), NULL, 16);

            while (cursor < recordLength + 9)
            {
                unsigned int preBase = base;
                struct ListRow r = readInstruction(locctr, pc, base,
                                                   cursor, txtRec, symVect, litVect);

                if (r.needLTORG)
                {
                    printLTORGLine();
                }

                printListRow(r);

                if (base != preBase)
                {
                    printBaseLine(r.operand);
                }
            }

            if ((objVect[i + 1][0] == 'T'))
            {
                // check for gap in addresses
                int nextRecordAddr = std::stoi(objVect[i + 1].substr(1, 6), NULL, 16);
                if (nextRecordAddr != pc)
                {
                    // there is a gap
                    // create a vector of listRows to add
                    std::vector<ListRow> newRows;

                    // create a vector of Symbols that fall in this gap
                    std::vector<SymbolEntry> resWs;

                    for (int j = 0; j < symVect.size(); j++)
                    {
                        // find all the symbols that fall in this gap.
                        if (symVect[j].value >= pc && symVect[j].value < nextRecordAddr)
                        {
                            resWs.push_back(symVect[j]);
                        }
                    }

                    std::sort(resWs.begin(), resWs.end());

                    unsigned int last = nextRecordAddr;
                    for (int k = resWs.size() - 1; k >= 0; k--)
                    {
                        struct ListRow r =
                            {
                                resWs[k].value,
                                resWs[k].name,
                                "RESW",
                                " ",
                                " ",
                                " ",
                                " ",
                                " ",
                                false};
                        // get the gap between the last RESW and this one.
                        int gap = last - resWs[k].value;
                        r.operand = std::to_string(gap / 3);
                        newRows.push_back(r);
                        last = resWs[k].value;
                    }
                    std::reverse(newRows.begin(), newRows.end());
                    for (int q = 0; q < newRows.size(); q++)
                    {
                        printListRow(newRows[q]);
                    }
                }
            }
        }
        else if (objVect[i][0] == 'H') // header record
        {
            struct ListRow r =
                {
                    0,
                    objVect[i].substr(1, 6),
                    " ",
                    " ",
                    "START",
                    " ",
                    " ",
                    "0",
                    false};
            r.addr = std::stoi(objVect[i].substr(7, 6), NULL, 16);
            printListRow(r);
        }
        else if (objVect[i][0] == 'E')
        {
            std::cout << "                  END       "
                      << checkSymbolTable(std::stoi(objVect[i].substr(1, 6), NULL, 16), symVect);
        }
    }
    return 0;
}
