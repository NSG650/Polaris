#include <stdint.h>
#include <stddef.h>
#include <stivale2.h>

//tell the bootloader where we want out stack to be 
//we are going to allocate the stack as an uninitalized array in .bss

static uint8_t stack[4096];

//stivale2 offers a runtime terminal service whcih can be diteched any time 
//it provides an easy way to print out to the graphical terminal
//especially during early boot

static struct stivale2_header_tag_terminal terminal_hdr_tag = {
	//all tags need to begin with an identifier and pointer to next tag
	.tag = {
		.identifier = STIVALE2_HEADER_TAG_TERMINAL_ID,
		//if the .next is 0 it ends the linked llist of the header tags
		.next = 0
	},
	//terminal header tag possesses a flags fielld leave it 0 now
	.flags = 0
};

//time to define a frame buffer header tag which is manadatory when using the stivale2 terminal
//thistag tells the bootlaoder we want a graphical framebuffer
//and not a CGA compatible text mode one

static struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
	.tag = {
		.identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        	//instead of 0 we now point to the previous header tag
        	//which header tags are linked does not matter
        	.next = (uint64_t)&terminal_hdr_tag
    	},
    	//we set all the framebuffer specifics to 0 as we want the bootloader
    	//to pick the best it can.
    	.framebuffer_width  = 0,
    	.framebuffer_height = 0,
    	.framebuffer_bpp    = 0
};

//we need to define a header structure too
//this structure needs to reside in thhe .stivale2hde elf header section
//for the bootloader to find it
//we use __attribute__ directive to tell the compiler
//to put in there

__attribute__((section(".stivale2hdr"), used))
static struct stivale2_header stivale_hdr = {
    //the entry_point member is used to specify an alternative entry
    //point that the bootloader should jump to instead of the executable's
    //ELF entry point. We do not care about that so we leave it zeroed.
    .entry_point = 0,
    //tell the bootloader where our stack is.
    //need to add the sizeof(stack) since in x86(_64) the stack grows downwards.
    .stack = (uintptr_t)stack + sizeof(stack),
    //no flags are currently defined as per spec and should be left to 0.
    .flags = 0,
    //this header structure is the root of the linked list of header tags and
    //points to the first one in the linked list.
    .tags = (uintptr_t)&framebuffer_hdr_tag
};

//now write a heloer function which will allow us to scan for the tags
//that we need from the bootloader (structure tags)

void *stivale2_get_tag(struct stivale2_struct *stivale2_struct, uint64_t id) {
    struct stivale2_tag *current_tag = (void *)stivale2_struct->tags;
    for (;;) {
        //if the tag pointer is NULL (end of linked list), we did not find
        //the tag. Return NULL to signal this.
        if (current_tag == NULL) {
            return NULL;
        }
 
        //check whether the identifier matches. If it does, return a pointer
        //to the matching tag.
        if (current_tag->identifier == id) {
            return current_tag;
        }
 
        //get a pointer to the next tag in the linked list and repeat.
        current_tag = (void *)current_tag->next;
    }
}

//kernel's entry point
 
void _start(struct stivale2_struct *stivale2_struct) {
	//lets get the terminal structure tag from bootloader
	 struct stivale2_struct_tag_terminal *term_str_tag;
    	term_str_tag = stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_TERMINAL_ID);
	
	//check if the tag was actually found
	if (term_str_tag == NULL) {
		for(;;)
			__asm__("hlt");
	}

	//get address of the terminal write function
	
	void *term_write_prt = (void*)term_str_tag->term_write;

	//simple function prototype for printing to the screen
	void (*term_write)(const char *string, size_t length) = term_write_prt;

	//hello world !
	term_write("Hello World!", 12);
	
	//alright bro thats it give your phone back
	for (;;)
		__asm__("hlt");	
}
