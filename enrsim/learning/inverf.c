#include <stdio.h>
#include <math.h>

int main(void)	{

float x = 0.99;
float y = 0.8385;

printf("erf(%.2f) = %.4f\n",x,erf(x));
printf("inverf(%.4f) = %.2f\n",y,inverf(y));

}