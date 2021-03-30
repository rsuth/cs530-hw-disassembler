// CS530 Hw1 - SIC/XE Disassembler
// Richard Sutherland, RED ID: 8122192

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include "ListRow.h"

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

// SYMTAB entry datatype
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

// LITTAB entry datatype
struct LitEntry
{
    std::string name;
    std::string lit;
    int length;
    unsigned int address;
};

/**
 * Search symbol table for address
 * 
 * @param addr the address to look for
 * @param litVec vector containing the symbol entries in the SYMTAB.
 * @return string of the symbol name matching the given address.
 */
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

/**
 * Search literals table for address
 * 
 * @param addr the address to look for
 * @param litVec vector containing the literal entries in the LITTAB.
 * @return LitEntry matching the given address.
 */
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

/**
 * Opcode lookup function
 *  
 * @param code the integer representation of the first 2 hex digits (first 8 bits) of the instruction
 * @return OpCode object containing { code, mnemonic, LTORG flag } from table.
 */
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

/**
 * read flag bits into nixbpe flag array
 *  
 * @param code the integer representation of the 2nd and 3rd hex digits (bits 4-12) of the instruction
 * @param nixbpe the flag bits boolean array to be filled by this function.
 */
void getFlagBits(unsigned int code, bool (&nixbpe)[6])
{
    for (int i = 0; i < 6; i++)
    {
        nixbpe[5 - i] = (code >> i) & 1;
    }
}

/**
 * calculate the target address
 *  
 * @param disp string representing the hex digits that make up the disp field of the instruction.
 * @param nixbpe the flag bits as a boolean array.
 * @param pc the current value in the program counter
 * @param base the current value in the base register
 * @return string that should be prepended to the operand field in the lst file.  
 */
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

/**
 * get the correct prefix for a given operand
 * 
 * @param nixbpe the flag bits as a boolean array.
 * @return string that should be prepended to the operand field in the lst file.  
 */
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

/**
 * get the correct suffix for a given operand
 * 
 * @param nixbpe the flag bits as a boolean array.
 * @return string that should be appended to the operand field in the lst file.  
 */
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

/**
 * read an instruction from the object code file and get the corresponding list file row
 * 
 * @param locctr the current instruction's address, incremented by this function
 * @param pc  program counter variable, incremented by this function
 * @param base base register variable, possibly changed by this function
 * @param cursor the beginning half-byte position of the current instruction within the text record
 * @param txtRecord the current text record being read
 * @param symVec the symbol table vector
 * @param litvec the literals table vector
 * @return A ListRow struct containing all needed data for creating a row of the list file.
 */
ListRow readInstruction(unsigned int &locctr, unsigned int &pc, unsigned int &base,
                        int &cursor, const std::string txtRecord,
                        const std::vector<SymbolEntry> symVec,
                        const std::vector<LitEntry> litvec)
{
    ListRow ret = ListRow();
    ret.addr = locctr;

    int instructionBytes = 0;

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

        // move cursor to start of next instruction
        cursor += lit.length;

        // update address counter
        locctr += lit.length / 2;

        return ret;
    }

    ret.label = checkSymbolTable(ret.addr, symVec);

    OpCode op = getOpCode(std::stoi(txtRecord.substr(cursor, 2), NULL, 16));

    // special case of RSUB
    if (op.mnemonic == "RSUB")
    {
        ret.opPrefix = " ";
        ret.op = "RSUB";
        ret.operandPrefix = " ";
        ret.operand = " ";
        ret.operandSuffix = " ";
        ret.objCode = "4C0000";
        cursor += 6;
        locctr += 3;
        return ret;
    }

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
                    ret.operand = ListRow::intToHexString(std::stoi(txtRecord.substr(cursor + 3, 5), NULL, 16), -1);
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
                    ret.operand = ListRow::intToHexString(addr, -1);
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

/**
 * Read an .obj file
 * 
 * @param path path to the .obj file.
 * @param objVec Vector of strings representing each line of the object file.
 * @return 0 on failure, 1 on success.
 */
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

/**
 * Read a .sym Symbol/Literals table file.
 * 
 * @param path path to .sym file.
 * @param symVec a Vector of SymbolEnrys to be filled.
 * @param litVec a Vector of LitEntrys to be filled.
 * @return 0 on failure, 1 on success
 */
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
        // and the start of the literals table
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
        try
        {
            // TODO:
            // figure out how to read the literal table without regex because
            // <regex> does not work with the old g++ version on edoras.
            std::regex pattern(" +([A-Z]+)? +=[XC]'([0-9A-Fa-f]+)' +([0-9A-Fa-f]+) +([0-9A-Fa-f]+)");

            std::smatch matches;

            struct LitEntry l;

            if (std::regex_search(fileLines[j], matches, pattern))
            {
                // match found on this line
                l.name = matches[1];
                l.lit = matches[2];
                l.length = std::stoi(matches[3], NULL, 16);
                l.address = std::stoi(matches[4], NULL, 16);
                litVec.push_back(l);
            }
        }
        catch (const std::regex_error &e)
        {
            std::cout << "Regex Error caught: " << e.what() << std::endl;
            return 0;
        }
    }
    return 1;
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

    std::ofstream outfile("out.lst");
    if (!outfile.is_open())
    {
        std::cout << "Error opening output file.";
        return -1;
    }

    unsigned int base = 0;

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
                try
                {
                    ListRow row = readInstruction(locctr, pc, base,
                                                  cursor, txtRec, symVect, litVect);
                    row.print(outfile);
                }
                catch (...)
                {
                    std::cout << "Exception reading instruction";
                    break;
                }
            }

            if (objVect[i + 1][0] == 'T')
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
                        ListRow r = ListRow(resWs[k].value, resWs[k].name,
                                            "RESW", " ", " ", " ", " ", " ");

                        // get the gap between the last RESW and this one.
                        int gap = last - resWs[k].value;
                        r.operand = std::to_string(gap / 3);
                        newRows.push_back(r);
                        last = resWs[k].value;
                    }
                    std::reverse(newRows.begin(), newRows.end());
                    for (int q = 0; q < newRows.size(); q++)
                    {
                        newRows[q].print(outfile);
                    }
                }
            }
        }
        else if (objVect[i][0] == 'H') // header record
        {
            ListRow r = ListRow(0, objVect[i].substr(1, 6), " ", " ", "START",
                                " ", " ", "0");
            r.addr = std::stoi(objVect[i].substr(7, 6), NULL, 16);
            r.print(outfile);
        }
        else if (objVect[i][0] == 'E') // end record
        {
            outfile << "                  END       "
                    << checkSymbolTable(std::stoi(objVect[i].substr(1, 6), NULL, 16), symVect);
        }
    }
    outfile.close();
    return 0;
}
