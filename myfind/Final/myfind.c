/*
* @file myfind.c
*
* Beispiel 1
*
* @author Ralf Ziefuhs <ic17b065@technikum-wien.at>
* @author Christina Uhl <ic17b089@technikum-wien.at>
* @author Clemens Fritzsche <ic17b087@technikum-wien.at>
*
* @date 2018/03/08
*
* @version 1
*
*/

/*
* -------------------------------------------------------------- includes --
*/
#include <dirent.h> // readdir, opendir...
#include <stdio.h> // fprintf
#include <errno.h> // errno
#include <sys/stat.h> // lstat, stat
#include <string.h> // strcmp
#include <pwd.h> // getpwnam, getpwuid
#include <limits.h>
#include <fnmatch.h> // fnmatch
#include <stdbool.h> // type bool, true, false
#include <unistd.h> // readlink
#include <stdlib.h> // calloc, free
#include <time.h> // localtime

/*
* --------------------------------------------------------------- defines --
*/
#define SUCCESS 0
#define ERROR 1
/*
* -------------------------------------------------------------- typedefs --
*/

/*
* ------------------------------------------------------------- functions --
*/
static void do_file(const char * file_name, const char * const * parms, const int offset);
static void do_dir(const char * dir_name, const char * const * parms, const int offset);

static int do_print(const char * file_name, const char * const * arg);
static int do_check_parms(const char * const * parms, const int offset);
static int do_user(const char * file_name, const char * const * parms, const int offset);

int main(int argc, const char const *argv[])
{
	if (argc > 1) //check if there are arguments on commandline
	{
		//check for correct parameter, if incorret exit, correct start process
		if ((do_check_parms(argv, 2)) == ERROR)
		{
			exit(EXIT_FAILURE);
		}

		do_file(argv[1], argv, 2);
	}
	else //no file or directory specified
	{
		fprintf(stderr, "Usage: %s\t<file or directory> [ <test-aktion> ] ...\n"
			"-user <name/uid>\n"
			"-name <pattern>\n"
			"-type [bcdpfls]\n"
			"-print\n"
			"-ls\n"
			"-nouser\n"
			"-path <pattern>\n",
			argv[0]);

		exit(EXIT_FAILURE);
	}

	if (fflush(stdout) == EOF) //flush stdout buffer
	{
		fprintf(stderr, "%s Unable to flush stdout: \n", strerror(errno));
	}
	return EXIT_SUCCESS;

}

static void do_file(const char *file_name, const char * const * parms, const int offset)
{
	struct stat buf;
	int new_offset = offset;
	int check_action = SUCCESS;
	int print_done = ERROR;


	if (lstat(file_name, &buf) == -1)
	{
		fprintf(stderr, "%s: `%s' %s\n", *parms, file_name, strerror(errno));
		return;
	}

	/*
	*runs as long as there are actions (parms != NULL) starting at the
	*first ((argv[0] + (new_offset=2)) and depending on the action if
	*there is an argument (new_offset = new_offset + 1 or + 2)
	*/
	while (*(parms + new_offset) != NULL || check_action == ERROR)
	{
		if (strcmp(*(parms + new_offset), "-user") == 0)
		{
			check_action = do_user(file_name, parms, (new_offset + 1));
						
			new_offset = new_offset + 2;
			print_done = check_action;
			
			continue;
		}

		if (strcmp(*(parms + new_offset), "-name") == 0)
		{
			//check_action = do_name(file_name, parms, new_offset + 1);
			check_action = ERROR;
			new_offset = new_offset + 2;
			continue;
		}
		if (strcmp(*(parms + new_offset), "-type") == 0)
		{
			//check_action = do_type(buf, parms, new_offset + 1);
			check_action = ERROR;
			new_offset = new_offset + 2;
			continue;
		}
		if (strcmp(*(parms + new_offset), "-print") == 0)
		{
			check_action = do_print(file_name, parms);
			new_offset = new_offset + 1;
			print_done = SUCCESS;
			continue;
		}
		if (strcmp(*(parms + new_offset), "-ls") == 0)
		{
			//check_action = do_ls(buf, file_name, parms);
			new_offset = new_offset + 1;
			print_done = check_action;
			continue;
		}
		if (strcmp(*(parms + new_offset), "-nouser") == 0)
		{
			//check_action = do_no_user(buf, file_name, parms);
			new_offset = new_offset + 1;
			continue;
		}
		if (strcmp(*(parms + new_offset), "-path") == 0)
		{
			//check_action = do_path(file_name, parms, new_offset + 1);
			new_offset = new_offset + 2;
			continue;
		}
	}

	if (print_done == ERROR && check_action == SUCCESS)
	{
		do_print(file_name, parms);
	}
	//fprintf(stdout, "%s\n", file_name);

	if (S_ISDIR(buf.st_mode))
	{
		do_dir(file_name, parms, new_offset);
	}

	return;
}


static void do_dir(const char * dir_name, const char * const * parms, const int offset)
{
	DIR *dirp;
	const struct dirent *dp;
	const char *sub_file_name;

	dirp = opendir(dir_name);

	if (dirp == NULL)
	{
		fprintf(stderr, "%s: error opening directory %s\n", strerror(errno), dir_name);
		return;
	}

	errno = 0; /*reset errno*/
	dp = readdir(dirp);

	if (dp == NULL)
	{
		if (closedir(dirp) == -1)
		{
			fprintf(stderr, "%s: error closing directory %s\n", strerror(errno), dir_name);
		}

		return;
	}

	while (dp != NULL)
	{
		sub_file_name = dp->d_name;

		/*excluding . and ..*/
		if (strcmp(sub_file_name, ".") != 0 && strcmp(sub_file_name, "..") != 0)
		{
			char new_path[sizeof(char) * (strlen(dir_name) + strlen(sub_file_name) + 2)];

			/*generate new dir or file path*/
			if (dir_name[strlen(dir_name) - 1] == '/')
			{
				sprintf(new_path, "%s%s", dir_name, sub_file_name);
			}
			else
			{
				sprintf(new_path, "%s/%s", dir_name, sub_file_name);
			}

			/*fprintf(stdout, "%s\n", new_path);*/

			do_file(new_path, parms, offset);
		}

		if (errno != 0)
		{
			fprintf(stderr, "%s: error reading information of dir: %s\n", strerror(errno), dir_name);
		}

		errno = 0; /*reset errno*/

		dp = readdir(dirp);

	}

	if (errno != 0)
	{
		fprintf(stderr, "%s: error reading information of dir: %s\n", strerror(errno), dir_name);
	}

	if (closedir(dirp) == -1)
	{
		fprintf(stderr, "%s: error closing directory %s\n", strerror(errno), dir_name);
	}

	return;
}


static int do_print(const char * file_name, const char * const * parms)
{
	if (fprintf(stdout, "%s\n", file_name) < 0)
	{
		fprintf(stderr, "%s: %s - %s\n", *parms, file_name, strerror(errno));
		return ERROR;
	}

	return SUCCESS;
}

static int do_check_parms(const char * const * parms, const int offset)
{
	int new_offset = offset;

	/*if(strcmp(*(parms + 1), ".") == 0  ||
	   strcmp(*(parms + 1), "..") == 0  ||
	   strcmp(*(parms + 1), "/") == 0)
	{*/
		while (*(parms + new_offset) != NULL)
		{
			if (strcmp(*(parms + new_offset), "-user") == 0 ||
				strcmp(*(parms + new_offset), "-name") == 0 ||
				strcmp(*(parms + new_offset), "-type") == 0 ||
				strcmp(*(parms + new_offset), "-path") == 0)
			{
				if (*(parms + new_offset + 1) == NULL ||
					strcmp(*(parms + new_offset + 1), "-user") == 0 ||
					strcmp(*(parms + new_offset + 1), "-name") == 0 ||
					strcmp(*(parms + new_offset + 1), "-type") == 0 ||
					strcmp(*(parms + new_offset + 1), "-path") == 0 ||
					strcmp(*(parms + new_offset + 1), "-nouser") == 0 ||
					strcmp(*(parms + new_offset + 1), "-print") == 0 ||
					strcmp(*(parms + new_offset + 1), "-ls") == 0)
				{
					fprintf(stderr, "%s: missing additional parameters `%s'\n", *parms, *(parms + new_offset));
					return ERROR;
				}

				new_offset = new_offset + 2;
			}
			else if (strcmp(*(parms + new_offset), "-nouser") == 0 ||
				strcmp(*(parms + new_offset), "-print") == 0 ||
				strcmp(*(parms + new_offset), "-ls") == 0)
			{
				new_offset = new_offset + 1;
			}
			else
			{
				return ERROR;
			}
		}
	/*}
	else
	{
		fprintf(stderr, "%s: `%s' No such file or directory\n", *parms, *(parms + 1));
		return ERROR;
	}*/
	
	return SUCCESS;
}


static int do_user(const char * file_name, const char * const * parms, const int offset)
{
	errno = 0; //reset errno

	const struct passwd *pwd_entry = getpwnam(*(parms + offset));

	if (pwd_entry == NULL)
	{
		//fprintf(stderr, "%s: User not found %s\n", strerror(errno), *(parms));
		fprintf(stderr, "%s: `%s' is not the name of a known user\n", *parms, *(parms + offset));
		
		return ERROR;

	}

	do_print(file_name, parms);
	
	return SUCCESS;

}
