/*
 * FixedPoint.h
 *
 *  Created on: 2015. 8. 19.
 *      Author: scpark
 */

#include <sys/types.h>

#ifndef FIXEDPOINT_H_
#define FIXEDPOINT_H_

#define BASE_ 24
#define BASE_NUMBER 16777216

#define FXP_E 45605201
#define FXP_PI 52707178

typedef float FXP;
/*
FXP float2FXP(float var)
{
	return (FXP) (var * BASE_NUMBER);
}

float FXP2float(FXP var)
{
	return (float) (var / BASE_NUMBER);
}

FXP add(FXP var1, FXP var2)
{
	return var1 + var2;
}

FXP substract(FXP var1, FXP var2)
{
	return var1 - var2;
}

FXP multiply(FXP var1, FXP var2)
{
	return var1 * var2 >> BASE_;
}

FXP divide(FXP var1, FXP var2)
{
	return var1 / var2 << BASE_;
}
*/
#endif /* FIXEDPOINT_H_ */
