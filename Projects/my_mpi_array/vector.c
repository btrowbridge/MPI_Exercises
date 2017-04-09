
// vector.c

#include <stdio.h>
#include <stdlib.h>
#include "vector.h"

#ifndef VECTOR_INITIAL_CAPACITY
#define VECTOR_INITIAL_CAPACITY 100
#endif

#ifndef PRINT_STUB
#define PRINT_STUB(){printf ("Stub at %s, line %d.\n", __FILE__, __LINE__);}
#endif 

void vector_init(Vector *vector) {
  // initialize size and capacity
  vector->size = 0;
  vector->capacity = VECTOR_INITIAL_CAPACITY;
  
  // allocate memory for vector->data
  vector->data =(float *) malloc(sizeof(float) * vector->capacity);
}

void vector_append(Vector *vector, float value) {
  // make sure there's room to expand into
  vector_double_capacity_if_full(vector);

  // append the value and increment vector->size
  vector->data[vector->size++] = value;
}

float vector_get(Vector *vector, int index) {
  if (index >= vector->size || index < 0) {
    printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
    exit(1);
  }
  return vector->data[index];
}
float * vector_get_pointer(Vector *vector, int index) {
  if (index >= vector->size || index < 0) {
    printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
    exit(1);
  }
  float* ptr = &vector->data[index];
  return(ptr);
}

void vector_set(Vector *vector, int index, float value) {
  // zero fill the vector up to the desired index
	
  while (index >= vector->size) {
    vector_append(vector, 0);
  }
  // set the value at the desired index
  vector->data[index] = value;
}

void vector_double_capacity_if_full(Vector *vector) {
  if (vector->size >= vector->capacity) {
    // double vector->capacity and resize the allocated memory accordingly
    vector->capacity *= 2;
    vector->data = realloc(vector->data, sizeof(float) * vector->capacity);
  }
}

void vector_free(Vector *vector) {
  free(vector->data);
}