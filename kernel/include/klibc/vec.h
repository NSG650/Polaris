//
//  vec.h
//
//  Created by Mashpoe on 2/26/19.
//

#ifndef vec_h
#define vec_h

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef void* vector; // you can't use this to store vectors, it's just used
		      // internally as a generic type
typedef size_t vec_size_t;	  // stores the number of elements
typedef unsigned char vec_type_t; // stores the number of bytes for a type

typedef int* vec_int;
typedef char* vec_char;

#ifndef _MSC_VER

// shortcut defines

// vec_addr is a vector* (aka type**)
#define vector_add_asg(vec_addr)                                               \
	((typeof(*vec_addr))(                                                  \
	    _vector_add((vector*)vec_addr, sizeof(**vec_addr))))
#define vector_insert_asg(vec_addr, pos)                                       \
	((typeof(*vec_addr))(                                                  \
	    _vector_insert((vector*)vec_addr, sizeof(**vec_addr), pos)))

#define vector_add(vec_addr, value) (*vector_add_asg(vec_addr) = value)
#define vector_insert(vec_addr, pos, value)                                    \
	(*vector_insert_asg(vec_addr, pos) = value)

#else

#define vector_add_asg(vec_addr, type)                                         \
	((type*)_vector_add((vector*)vec_addr, sizeof(type)))
#define vector_insert_asg(vec_addr, type, pos)                                 \
	((type*)_vector_insert((vector*)vec_addr, sizeof(type), pos))

#define vector_add(vec_addr, type, value)                                      \
	(*vector_add_asg(vec_addr, type) = value)
#define vector_insert(vec_addr, type, pos, value)                              \
	(*vector_insert_asg(vec_addr, type, pos) = value)

#endif

// vec is a vector (aka type*)
#define vector_erase(vec, pos, len)                                            \
	(_vector_erase((vector*)vec, sizeof(*vec), pos, len))
#define vector_remove(vec, pos)                                                \
	(_vector_remove((vector*)vec, sizeof(*vec), pos))

#define vector_copy(vec) (_vector_copy((vector*)vec, sizeof(*vec)))

vector vector_create(void);

void vector_free(vector vec);

vector _vector_add(vector* vec_addr, vec_type_t type_size);

vector _vector_insert(vector* vec_addr, vec_type_t type_size, vec_size_t pos);

void _vector_erase(vector* vec_addr, vec_type_t type_size, vec_size_t pos,
		   vec_size_t len);

void _vector_remove(vector* vec_addr, vec_type_t type_size, vec_size_t pos);

void vector_pop(vector vec);

vector _vector_copy(vector vec, vec_type_t type_size);

vec_size_t vector_size(vector vec);

vec_size_t vector_get_alloc(vector vec);

#endif /* vec_h */

