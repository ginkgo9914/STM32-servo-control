#include "TypeExtend.h"


/**
 * @function TypeExtend_vector_add_vector
 * @brief Adds two vectors
 */
Vector TypeExtend_vector_add_vector(Vector v1, Vector v2){
    return (Vector){v1.x + v2.x, v1.y + v2.y};
}

Vector TypeExtend_vector_sub_vector(Vector v1, Vector v2){
    return (Vector){v1.x - v2.x, v1.y - v2.y};
}
