// Single Programmer Affidavit
// I the undersigned promise that the attached assignment is my own work.
// While I was free to discuss ideas with others, the work contained is my own.
// I recognize that should this not be the case, I will be subject to penalties
// as outlined in the course syllabus. - Richard Sutherland

#include <iostream>
#include <vector>
#include <string>
#include <fstream>

struct OpCode
{
    int code;
    std::string mnemonic;
    bool format2;
};

const int OPTABLE_LEN = 43;

const struct OpCode OP_TABLE[] = {{0x18, "ADD", false}, {0x58, "ADDF", false}, {0x40, "AND", false}, {0xB4, "CLEAR", true}, {0x28, "COMP", false}, {0x88, "COMPF", false}, {0x24, "DIV", false}, {0x64, "DIVF", false}, {0x3C, "J", false}, {0x30, "JEQ", false}, {0x34, "JGT", false}, {0x38, "JLT", false}, {0x48, "JSUB", false}, {0x00, "LDA", false}, {0x68, "LDB", false}, {0x50, "LDCH", false}, {0x70, "LDF", false}, {0x08, "LDL", false}, {0x6C, "LDS", false}, {0x74, "LDT", false}, {0x04, "LDX", false}, {0xD0, "LPS", false}, {0x20, "MUL", false}, {0x60, "MULF", false}, {0x44, "OR", false}, {0xD8, "RD", false}, {0x4C, "RSUB", false}, {0xEC, "SSK", false}, {0x0C, "STA", false}, {0x78, "STB", false}, {0x54, "STCH", false}, {0x80, "STF", false}, {0xD4, "STI", false}, {0x14, "STL", false}, {0x7C, "STS", false}, {0xE8, "STSW", false}, {0x84, "STT", false}, {0x10, "STX", false}, {0x1C, "SUB", false}, {0x5C, "SUBF", false}, {0xE0, "TD", false}, {0x2C, "TIX", false}, {0xDC, "WD", false}};

const std::string REGISTER_TABLE[] = {"A", "X", "L", "B", "S", "T", "F"};

OpCode getOpCode(int code)
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

struct ListRow
{
    int addr;
    std::string label;
    std::string op;
    std::string opPrefix;
    std::string operand;
    std::string operandPrefix;
    std::string objCode;
};

// reads number and fills the nixbpe array
void getFlagBits(int code, bool (&nixbpe)[6])
{
    for (int i = 0; i < 6; i++)
    {
        nixbpe[5 - i] = (code >> i) & 1;
    }
}

// calculates the operand from the disp/address
std::string getOperand(std::string disp, const bool nixbpe[6], int pc, int base)
{
    int ta = (int)strtol(disp.c_str(), NULL, 16);
    std::string operand;

    if (nixbpe[3])
    {
        // base relative
        ta += base;
    }
    if (nixbpe[4])
    {
        // pc relative

        // negative
        if(ta > 0x7FF){
            ta = -1 * ((0xFFF - ta) + 1);
        }

        ta += pc;
        std::cout << ta;


        // TODO: right now we need to handle the case where disp is negative.
        // need to look into twos compliment/negative hex numbers.
        // TA=(PC) + disp (-2048 <= disp <= 2047)
        // EXAMPLE: obj code 53AFEC
        // disp = FEC
        // PC = 852
        // TA should be 0x083E (2110) but instead we get 0x183E (6206)
    }
    return operand;
}

struct ListRow buildRow(int &addr, int &cursor, const std::string txtRecord)
{
    ListRow ret = ListRow();
    ret.addr = addr;
    OpCode op = getOpCode((int)strtol(txtRecord.substr(cursor, 2).c_str(), NULL, 16));
    int instructionBytes = 0;
    if (!op.format2)
    {
        // set flag bits
        int flagCode = (int)strtol(txtRecord.substr(cursor + 1, 2).c_str(), NULL, 16);

        bool nixbpe[] = {0, 0, 0, 0, 0, 0};

        getFlagBits(flagCode, nixbpe);

        if (nixbpe[5]) // this is a format 4 instruction
        {
            ret.op = op.mnemonic;
            instructionBytes = 4;
        }
        else // this is a format 3 instruction
        {
            ret.op = op.mnemonic;
        }
    }
    else
    {
        ret.op = op.mnemonic;
        ret.operand = REGISTER_TABLE[(int)strtol(txtRecord.substr(cursor + 2, 1).c_str(), NULL, 16)];
    }

    cursor += (2 * instructionBytes);
    addr += instructionBytes;

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

int main(int argc, char **argv)
{
    std::vector<std::string> objVect;

    if (readObjFile("test.obj", objVect))
    {
        for (int i = 0; i < objVect.size(); i++)
        {
            std::cout << objVect[i] << std::endl;
        }
    }

    bool nixbpe[] = {1, 1, 1, 0, 1, 0};
    getOperand("7FF", nixbpe, 0x0852, 0);
}
