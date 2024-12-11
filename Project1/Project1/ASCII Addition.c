#define _CRT_SECURE_NO_WARNINGS
// ASCII Addition

#include<stdio.h>

int main()
{
	char char1, char2;
	printf("Enter char1: ");
	scanf("%c", &char1);

	getchar();

	printf("Enter char2: ");
	scanf("%c", &char2);

	int char3 = (int)char1 + (int)char2;
	printf("\nchar3 value is %c", char3);
	return 0;
}
