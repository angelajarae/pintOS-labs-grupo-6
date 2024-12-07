#ifndef __THREAD_FIXED_POINT_H
#define __THREAD_FIXED_POINT_H

typedef int fixed_point;

#define F (1 << 14)

// Converts an integer N to a fixed-point representation by shifting left.
#define INT_TO_FP(N) ((fixed_point)(N *F)) 

// Converts a fixed-point value A to an integer by dividing by the scaling factor.
#define FP_TO_INT(A) (A / F) 

// Converts a fixed-point value A to an integer, rounding correctly based on the sign.
#define FP_TO_INT_ROUND(A) ((A) >= 0 ? ((A) + F / 2) / F: ((A) - F / 2) / F)

// Adds two fixed-point values A and B.
#define FP_ADD(A, B) (A + B) 

// Subtracts fixed-point value B from A.
#define FP_SUB(A, B) (A - B) 

// Multiplies two fixed-point values A and B, adjusting for scaling.
#define FP_MULT(A, B) ((fixed_point)(((int64_t)A) * B / F)) 

// Divides fixed-point value A by B, adjusting for scaling.
#define FP_DIV(A, B) ((fixed_point)((((int64_t)A)*F) / B)) 

// Adds an integer N to a fixed-point value A by converting N to fixed-point.
#define FP_ADD_INT(A, N) ((A) + (INT_TO_FP(N))) 

// Subtracts an integer N from a fixed-point value A by converting N to fixed-point.
#define FP_SUB_INT(A, N) ((A) - (INT_TO_FP(N)))

// Multiplies a fixed-point value A by an integer N.
#define FP_MULT_INT(A, N) ((fixed_point)((A) * (N))) 

// Divides a fixed-point value A by an integer N, without adjusting for scaling.
#define FP_DIV_INT(A, N) ((fixed_point)((A) / (N))) 

#endif /* threads/fixed_point.h */