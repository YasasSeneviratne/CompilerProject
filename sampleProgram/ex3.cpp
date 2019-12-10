#include<stdio.h>
#include<stdlib.h>

int main(){
	int * a;
	int b,c;
	c = 20;
	a = (int *) calloc(sizeof(int), b);
	for(int i=0;i<64;i++){
		a[rand()%64] = b*c;
	}
	return 0;
}
