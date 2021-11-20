# orignal author V0ID-NULL
# modified to work with polaris

from os import popen, system
import sys

PARSED =  "kernel/kernel/symbols.c"
KERNEL =  sys.argv[1]
#system(f"touch {PARSED}")
writer = open(PARSED, "w")

symbol_table_naked = """
#include "symbols.h"

sym_table_t symbol_table[] = {
"""

END = """
static sym_table_t lookup(uint64_t address) {
	size_t i;
	for(i = 0; symbol_table[i].address != 0xFFFFFFFF; i++)
		if((symbol_table[i].address << 52) == (address << 52))
			return symbol_table[i];
	return symbol_table[i];
}

const char *symbols_return_function_name(uint64_t address) {
	sym_table_t table = lookup(address);
	if(table.address == 0xFFFFFFFF)
		return "UNKNOWN";
	return table.function_name;
}

"""

def init_writer():
    try:
        assert(writer.mode == "w")
    except AssertionError:
        print("[x] Cannot open file in write mode, check your permissions and try again")
        exit(1)
    
    writer.write(symbol_table_naked)

# return an entry in the symbol array: {0xdeadbeef, "function"},
def symbol_template(address, function_name):
    return '\t{0x' + address + ', ' + '\"' + function_name + '\"' + '},\n'

def write_data(address, function_name):
    if function_name == "":
        return
    writer.write(symbol_template(address=address, function_name=function_name))


def destroy_writer():
    writer.write("\t{0xFFFFFFFF, \"\"}\n")
    writer.write("};\n")
    writer.write(END)
    writer.close()

# Input: A single line from objdump, returns true if it's a function, otherwise it returns false since we don't need variables or function names
def is_function_symbol(objdump_line):
    if not ".text" in objdump_line:
        return False
    return True

def function_symbol_extract(objdump_line):
    objdump_line = str(objdump_line).split()
    return (str(objdump_line[0]), str(objdump_line[5:]).replace("]", "").replace("'","").replace("[", "").replace(",,", ","))

def parse_symbol_tables(unparsed_sym_table):
    unparsed_sym_table = unparsed_sym_table.splitlines()

    for x in range(len(unparsed_sym_table)-1):
        if not is_function_symbol(unparsed_sym_table[x]):
            continue
        else:
            addr, name = function_symbol_extract(unparsed_sym_table[x])
            write_data(str(addr), name)

if __name__ == '__main__':
    init_writer()
    parse_symbol_tables(popen(f"objdump -C -t {KERNEL}").read())
    destroy_writer()
    print("[*] Wrote symbol table")
