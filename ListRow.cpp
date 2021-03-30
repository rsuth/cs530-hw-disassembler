#include "ListRow.h"

/**
 * Default constructor - created an empty list row
 */
ListRow::ListRow()
{
    addr = 0;
    label = "";
    op = "";
    opPrefix = "";
    operand = "";
    operandPrefix = "";
    operandSuffix = "";
    objCode = "";
}

/**
 * Constructor with all parameters
 */
ListRow::ListRow(unsigned int _addr, std::string _label, std::string _op,
                 std::string _opPrefix, std::string _operand, std::string _operandPrefix,
                 std::string _operandSuffix, std::string _objCode)
{
    addr = _addr;
    label = _label;
    op = _op;
    opPrefix = _opPrefix;
    operand = _operand;
    operandPrefix = _operandPrefix;
    operandSuffix = _operandSuffix;
    objCode = _objCode;

}

/**
* Prints a row of the list file.
* 
* @param outfile the lst file output stream
*/
void ListRow::print(std::ofstream &outfile)
{
    if (op == "*") // Literal - Print LTORG line first
    {
        outfile
            << std::setw(17)
            << " "
            << std::setw(6)
            << " LTORG"
            << std::endl;
    }

    outfile
        << intToHexString(addr, 4)
        << std::setw(4)
        << " "
        << std::setw(6)
        << label
        << std::setw(3)
        << " "
        << std::setw(6)
        << opPrefix + op
        << std::setw(3)
        << " "
        << std::left
        << std::setw(10)
        << operandPrefix + operand + operandSuffix
        << std::setw(4)
        << " "
        << objCode
        << std::endl;

    if (op == "LDB") // Base - Print BASE directive line
    {
        outfile
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
}

/**
 * convert unsigned int to a hexadecimal string representation
 * 
 * @param num the int to be converted
 * @param digits the number of digits to use, with 0-padding. pasing a value of -1 does not pad at all.
 * @return hexadecimal string representation of the number.  
 */
std::string ListRow::intToHexString(unsigned int num, int digits)
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