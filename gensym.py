"""Generates symbol table"""
# Orignal author V0ID-NULL
# Modified to work with Polaris

import subprocess
import sys

PARSED = "kernel/kernel/symbols.c"
KERNEL = sys.argv[1]
writer = open(PARSED, "w", encoding="utf-8")

SKELETON = """#include "symbols.h"

sym_table_t symbol_table[] = {
"""

END = """
static sym_table_t lookup(uint64_t address) {
	size_t i;
	for (i = 0; symbol_table[i].address != 0xffffffff; i++)
		if ((symbol_table[i].address << 52) == (address << 52))
			return symbol_table[i];
	return symbol_table[i];
}

const char *symbols_return_function_name(uint64_t address) {
	sym_table_t table = lookup(address);
	if (table.address == 0xffffffff)
		return "UNKNOWN";
	return table.function_name;
}
"""

def init_writer():
    """Initialize writer by writing the skeleton"""
    try:
        assert writer.mode == "w"
    except AssertionError:
        print("[x] Cannot open file in write mode, check your permissions and try again")
        sys.exit(1)

    writer.write(SKELETON)

def symbol_template(address, function_name):
    """Return an entry in the symbol array: {0xdeadbeef, "function"},"""
    return "\t{0x" + address + ", " + "\"" + function_name + "\"" + "},\n"

def write_data(address, function_name):
    """Write an address and a function name to the symbol table"""
    if not function_name:
        return
    writer.write(symbol_template(address=address, function_name=function_name))

def destroy_writer():
    """Finish by writing the final of the skeleton"""
    writer.write("\t{0xffffffff, \"\"}")
    writer.write("};\n")
    writer.write(END)
    writer.close()

# Input: A single line from objdump
def is_function_symbol(objdump_line):
    """Returns true if it's a function, returns false otherwise"""
    if not ".text" in objdump_line:
        return False
    return True

def function_symbol_extract(objdump_line):
    """Extracts the address and symbol name from the objdump output"""
    objdump_line = str(objdump_line).split()
    return (str(objdump_line[0]), str(objdump_line[5:]).replace("]", "")\
        .replace("'","").replace("[", "").replace(",,", ","))

def parse_symbol_tables(unparsed_sym_table):
    """Parses the symbol table from the objdump output"""
    unparsed_sym_table = unparsed_sym_table.splitlines()

    for i in range(len(unparsed_sym_table) - 1):
        if not is_function_symbol(unparsed_sym_table[i]):
            continue
        addr, name = function_symbol_extract(unparsed_sym_table[i])
        write_data(str(addr), name)

if __name__ == "__main__":
    init_writer()
    parse_symbol_tables(subprocess.run(["objdump", "-C", "-t", KERNEL],
                        stdout=subprocess.PIPE, check=True).stdout.decode())
    destroy_writer()
    print("[*] Wrote symbol table")
