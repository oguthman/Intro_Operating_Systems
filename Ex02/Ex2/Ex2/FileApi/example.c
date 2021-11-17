#include <stdio.h>
#include "file.h"


void _main_example(int argc, char* argv[])
{
	// open files
	File* file = File_Open("Real\\Real1.txt", "r");
	File* write_file = File_Open("Res.txt", "w");

	// copy files content
	int grade;
	while (File_Scanf(file, 10, "%d", &grade))
	{
		printf("%d\n", grade);
		File_Printf(write_file, "Student Grade - %d\n", grade);
	}

	// close files
	File_Close(file);
	File_Close(write_file);
}
