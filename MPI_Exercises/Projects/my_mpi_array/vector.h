//Vector
#ifndef VECTORS_H_INCLUDED
#define VECTORS_H_INCLUDED



/*https://www.happybearsoftware.com/implementing-a-dynamic-array*/


// Define a vector type
typedef struct {
  int size;      // slots used so far
  int capacity;  // total available slots
  float *data;     // array of integers we're storing
} Vector;

void vector_init(Vector *vector);
void vector_append(Vector *vector, float value);
float vector_get(Vector *vector, int index);
float * vector_get_pointer(Vector *vector, int index);
void vector_set(Vector *vector, int index, float value);
void vector_double_capacity_if_full(Vector *vector);
void vector_free(Vector *vector);

#endif