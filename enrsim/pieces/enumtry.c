/*try enum to characterize material type*/

#include <stdio.h>

int main(void)	{

	enum MAT	{
		NU,
		LEU,
		LEU197,
		HEU50,
		HEU90,
		SF,
		PU
	};
	
	printf("%d\n",LEU197);
}