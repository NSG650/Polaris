/* tell linker we cant the symbol _start to be the entry point*/

ENTRY(_start)

SECTIONS {
	/* the kernel wants to be places in the higher half 2mb above 0 PM 
	 * since we are going to use PIE this is the base load address but the bootloader
	 * will be able to relocate af it sees fit */
	
	. = 0xffffffff80200000;

	/* we place the stivale2hdr section conataining the header in its own section
	 * and we use the KEEP directive on it to make sure it doesnt get discarded */

	.stivale2hdr : {
		KEEP(*(.stivale2hdr))
	}

	/* place the traditional excutable sections later */

	.text : {
		*(.text*)
	}

	.rodata : {
		*(.rodata*)
	}

	.data : {
		*(.data*)
	}

	.bss : {
		*(COMMON)
		*(.bss*)
	}
}
