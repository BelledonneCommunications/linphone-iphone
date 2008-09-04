#include "lpc10_wrapper.c"
#include <stdio.h>



int main()
{
	FILE *input;
	int i;
	unsigned char buffer[10];
	INT32 bits[54];
	
	input=fopen("/tmp/dam9.bits","r");
	if (input==NULL) printf("error opening file\n");
	fread(buffer,7,1,input);
	read_bits(buffer,bits,54);
	for (i=0;i<54;i++) printf("%i ",bits[i]);
	fclose(input);
	return(0);
}