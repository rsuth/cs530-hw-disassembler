# CS530 HW1 - SIC/XE Disassembler
## Richard Sutherland, RED ID: 8122192

usage:
`./disassem <objfile> <symfile>`

This program reads an object code file into a vector of strings, with each string representing a record from the object code. It also reads a symbol table file containing symbols and literals.

The program goes through each record, analyzing each instruction. Each instruction is used to populate a class called ListRow, which contains all information needed to create one line of the list file (out.lst). 

Each instruction includes a check to the symbol table and literal table. When an address or operand matches an entry in these tables, the symbol or literal is used in place of the operand target address or when it matches the instruction address itself, it is used as a label. 

The special cases like literal "\*" declarations are handled as well, when the instructions address matches an address in the literals table.

All addresses and disp fields are parsed as unsigned integers, for ease of comparison. The NIXBPE field is stored as an array of booleans. 

To handle RESW directives, the program detects gaps between the starting addresses of consecutive text records, and searches for addressses in the symbol table that fall within those gaps. Once the symbols that are within those gaps are found, we work backwards from the upper bound address to figure out how many words are reserved by each RESW.

The Header and End records are simple and just involve simple extraction and print formatting.

## TODO:
[] refactor into seperate files
[] update literals table read to not use regex
[x] turn some stuff into classes?
[] #FIRST should be #0 in one case, check that out
[] write some automated tests