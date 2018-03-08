// myfind.cpp : Defines the entry point for the console application.
//


#include <sys/types.h>
#include <dirent.h>

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

void do_file(const char * file_name, const char * const * parms);

void do_dir(const char * dir_name, const char * const * parms);

int main(int argc, char* argv[]) {
	do_dir(argv[1], NULL);
	
}

void do_dir(const char * dir_name, const char * const * parms) {
	DIR* dirp = opendir(dir_name);

	if (dirp == NULL) {
		exit(1);
	}

	struct dirent* file = readdir(dirp);

	if (file == NULL) {
		closedir(dirp);
		return;
	}


	printf("%s/%s\n", dir_name, file->d_name);

	/*if (file->d_type != 4) {
		do_file(dir_name, parms);
	}*/

	if (file->d_type == 4) {
		char* new_name = (char*)dir_name;
		strcat(new_name, file->d_name);
		do_dir((const char*)new_name, parms);
	}

	/*while ((file = readdir(file_addr))) {

		printf("%s/%s\n", dir_name, file->d_name);

		if (file->type != 4) {
			do_file(dir_name, parms);
		}
		dir_name = "%s/%s\n", file_name, file->d_name;

	}*/
}

void do_file(const char * file_name, const char * const * parms) {
	do_dir(file_name, parms);

}