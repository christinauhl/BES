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
#include <dirent.h> // readdir, opendir
#include <stdio.h> // fprintf
#include <errno.h> // errno
#include <sys/stat.h> // lstat, stat
#include <string.h> // strcmp
#include <pwd.h> // getpwnam, getpwuid
#include <limits.h> //
#include <libgen.h> //basename
#include <fnmatch.h> // fnmatch
#include <stdbool.h> // type bool, true, false
#include <unistd.h> // readlink
#include <stdlib.h> // calloc, free
#include <time.h> // localtime
#include <fcntl.h>
#include <sys/types.h>

/*
* --------------------------------------------------------------- defines --
*/
#define SUCCESS 0
#define ERROR 1
/*
* -------------------------------------------------------------- typedefs --
*/
enum Bool{ FALSE, TRUE };
/*
* ------------------------------------------------------------- functions --
*/
static void do_file(const char * file_name, const char * const * parms, const int offset);
static void do_dir(const char * dir_name, const char * const * parms, const int offset);

static int do_print(const char * file_name, const char * const * parms);
static int do_check_parms(const char * const * parms);
static int do_user(const struct stat buffer, const char * const * parms, const int offset);
static int do_name(const char * file_name, const char * const * parms, const int offset);
static int do_nouser(const struct stat buffer, const char * const * parms);
static int do_type(const struct stat buffer, const char * const * parms, const int offset);
static char get_type(const mode_t mode);
static int do_path(const char * file_name, const char * const * parms, const int offset);

int main(int argc, const char const *argv[])
{
	if (argc > 1) //check if there are arguments on commandline
	{
		//check for correct parameter, if incorret exit, correct start process
		if (do_check_parms(argv) == ERROR)
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
	int print_done = FALSE;


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
	while (*(parms + new_offset) != NULL && check_action == SUCCESS)
	{
		if (strcmp(*(parms + new_offset), "-user") == 0)
		{
			check_action = do_user(buf, parms, (new_offset + 1));
			new_offset = new_offset + 2;
			continue;
		}

		if (strcmp(*(parms + new_offset), "-name") == 0)
		{
			check_action = do_name(file_name, parms, (new_offset + 1));
			if (check_action == SUCCESS)
			{
				do_print(file_name, parms);
				return;
			}
			
		}
		if (strcmp(*(parms + new_offset), "-type") == 0)
		{
			check_action = do_type(buf, parms, (new_offset + 1));
			if (check_action == SUCCESS)
			{
				do_print(file_name, parms);
				return;
			}
			
		}
		if (strcmp(*(parms + new_offset), "-print") == 0)
		{
			check_action = do_print(file_name, parms);
			new_offset = new_offset + 1;
			print_done = TRUE;
			continue;
		}
		if (strcmp(*(parms + new_offset), "-ls") == 0)
		{
			//check_action = do_ls(buf, file_name, parms);
			check_action = ERROR;
			new_offset = new_offset + 1;
			print_done = TRUE;
			continue;
		}
		if (strcmp(*(parms + new_offset), "-nouser") == 0)
		{
			check_action = do_nouser(buf, parms);
			new_offset = new_offset + 1;
			continue;
		}
		if (strcmp(*(parms + new_offset), "-path") == 0)
		{
			check_action = do_path(file_name, parms, (new_offset + 1));
			if (check_action == SUCCESS)
			{
				do_print(file_name, parms);
				return;
			}
			
		}
	}

	if (print_done == FALSE && check_action == SUCCESS)
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

	errno = 0; //reset errno
	dp = readdir(dirp);
	
	while (dp != NULL)
	{
		sub_file_name = dp->d_name;

		//excluding . and ..
		if (strcmp(sub_file_name, ".") != 0 && strcmp(sub_file_name, "..") != 0)
		{
			char new_path[sizeof(char) * (strlen(dir_name) + strlen(sub_file_name) + 2)];

			//new dir or file path
			if (dir_name[strlen(dir_name) - 1] == '/')
			{
				sprintf(new_path, "%s%s", dir_name, sub_file_name);
			}
			else
			{
				sprintf(new_path, "%s/%s", dir_name, sub_file_name);
			}
			
			do_file(new_path, parms, offset);
		}
				
		errno = 0;
		dp = readdir(dirp);

	}
	
	if (errno != 0)
	{
		fprintf(stderr, "%s: error while processing directory %s\n", *parms, dir_name);
	}
	
	if (closedir(dirp) == -1)
	{
		fprintf(stderr, "%s: error closing directory %s\n", *parms, dir_name);
	}

	return;
}

/*
*function to print the located file
*/
static int do_print(const char * file_name, const char * const * parms)
{
	if (fprintf(stdout, "%s\n", file_name) < 0)
	{
		fprintf(stderr, "%s: %s - %s\n", *parms, file_name, strerror(errno));
		return ERROR;
	}

	return SUCCESS;
}

/*
*function to check the prompted action and parameters on correctness
*/
static int do_check_parms(const char * const * parms)
{
	int offset = 2;
	char ** cur_Arg = (char **) (parms + offset);
	
		while (*cur_Arg != NULL)
		{
			if (strcmp(*cur_Arg, "-user") == 0 ||
				strcmp(*cur_Arg, "-name") == 0 ||
				strcmp(*cur_Arg, "-type") == 0 ||
				strcmp(*cur_Arg, "-path") == 0)
			{
				if (*(cur_Arg + 1) == NULL ||
					strcmp(*(cur_Arg + 1), "-user") == 0 ||
					strcmp(*(cur_Arg + 1), "-name") == 0 ||
					strcmp(*(cur_Arg + 1), "-type") == 0 ||
					strcmp(*(cur_Arg + 1), "-path") == 0 ||
					strcmp(*(cur_Arg + 1), "-nouser") == 0 ||
					strcmp(*(cur_Arg + 1), "-print") == 0 ||
					strcmp(*(cur_Arg + 1), "-ls") == 0)
				{
					fprintf(stderr, "%s: missing additional parameters `%s'\n", *parms, *cur_Arg);
					return ERROR;
				}
				
				if (strcmp(*cur_Arg, "-user") == 0)
				{
					errno = 0; //reset errno
										
					signed long uid = 0;
					char * p_end;

					const struct passwd *pwd_entry = getpwnam(*(cur_Arg + 1));

					if (errno != 0)
					{
						fprintf(stderr, "%s: error while processing -user %s\n", *parms, *(cur_Arg + 1));
						return ERROR;
					}

					if (pwd_entry == NULL) //user was not found
					{
						//conversion into long int
						uid = strtol(*cur_Arg, &p_end, 10);

						if (uid == LONG_MAX || uid == LONG_MIN)
						{
							fprintf(stderr, "%s: error overflow when trying to parse -user %s\n", *parms, *(cur_Arg + 1));
							return ERROR;
						}
						if (*p_end != '\0')
						{							
							fprintf(stderr, "%s: %s is not the name of a known user\n", *parms, *(cur_Arg + 1));
							return ERROR;
						}
					}
				}
				
				if (strcmp(*cur_Arg, "-type") == 0)
				{
					if ( strcmp(*(cur_Arg + 1), "b") && strcmp(*(cur_Arg + 1), "c") &&
						strcmp(*(cur_Arg + 1), "d") && strcmp(*(cur_Arg + 1), "p") &&
						strcmp(*(cur_Arg + 1), "f") && strcmp(*(cur_Arg + 1), "l") &&
						strcmp(*(cur_Arg + 1), "s") != 0)
					{
						fprintf(stderr, "%s: only one type of [bfcdpls]\n", *parms);
						return ERROR;
					}
				}
				
				cur_Arg = cur_Arg + 2;
			}
			else if (strcmp(*cur_Arg, "-nouser") == 0 ||
				strcmp(*cur_Arg, "-print") == 0 ||
				strcmp(*cur_Arg, "-ls") == 0)
			{
				cur_Arg = cur_Arg + 1;
			}
			else
			{
				return ERROR;
			}
		}
			
	return SUCCESS;
	
}


/*
*Checks if the searched user has a file
*First the name is searched and if not found then it will
*check if the entry was an uid
*If there is no file with the name or uid is looked for,
*then Exit like as find
*/
static int do_user(const struct stat buffer, const char * const * parms, const int offset)
{
	errno = 0; //reset errno
	
	signed long uid = 0;
	char * p_end = '\0';
	
	const struct passwd *pwd_entry = getpwnam(*(parms + offset));

	if (errno != 0)
	{
		fprintf(stderr, "%s: error while processing -user %s\n", *parms, *(parms + offset));
		return ERROR;
	}

	if (pwd_entry != NULL)
	{
		//check if user found
		if (pwd_entry->pw_uid == buffer.st_uid)
		{
			return SUCCESS;
		}
	}
	else //user not found, check uid
	{
		//conversion into long int
		uid = strtol(*(parms + offset), &p_end, 10);

		if (uid == LONG_MAX || uid == LONG_MIN)
		{
			fprintf(stderr, "%s: error overflow when trying to parse -user %s\n", *parms, *(parms + offset));
			exit(EXIT_FAILURE);
		}
		if (*p_end == '\0')
		{
			//check if uid found
			if ((unsigned)uid == buffer.st_uid)
			{
				return SUCCESS;
			}
		}
		else
		{
			fprintf(stderr, "%s: %s is not the name of a known user\n", *parms, *(parms + offset));
			exit(EXIT_FAILURE);
		}
		
	}
		
	return SUCCESS;

}


/*
*Checks and compares if the filename is the prompted name or not
*/
static int do_name(const char * file_name, const char * const * parms, const int offset)
{
	int fnmatch_result;
	
	//VLA
	char temp_file_name[strlen(file_name) + 1];
	char *base_name;

	memcpy(temp_file_name, file_name, strlen(file_name) + 1);
	base_name = basename(temp_file_name);

	//check if basename matches pattern in name
	fnmatch_result = fnmatch(*(parms + offset), base_name, FNM_NOESCAPE);
	
	if (fnmatch_result == 0)
	{
		return SUCCESS;
	}
	
	return ERROR;
}



/*
*Checks if the file has no user
*TRUE if there is a file with no user
*EXIT if ther is no file with no user like as find
*/
static int do_nouser(const struct stat buffer, const char * const * parms)
{	
	errno = 0; //reset errno

	const struct passwd *pwd_entry = getpwuid(buffer.st_uid);
	
	//check if file is with nouser, else user or errno exit like find
	if ((pwd_entry == NULL) && (errno == 0))
	{
		return TRUE;
	}
	else if ((pwd_entry != NULL) || (errno != 0))
	{
		fprintf(stderr, "%s: no file without user \n", *parms);
		exit(EXIT_FAILURE);
	}
	
	return SUCCESS;
	
}

/*
*SUCCESS if the given file has the searched pattern
*EXIT if the file has not the searched pattern like as find
*/
static int do_type(const struct stat buffer, const char * const* parms, const int offset)
{
	char type_char = NULL;

	//check type char of file_name
	type_char = get_type(buffer.st_mode);
			
	if (type_char == **(parms + offset))
	{	
		return SUCCESS;
				
	}
	else if (type_char != **(parms + offset))
	{
		//wrong parameter type
		fprintf(stderr, "%s: wrong parameter type \n", *parms);
		exit(EXIT_FAILURE);
	}

	return ERROR;
}

/*
* returns the type of the file (bfcdpls) or default '\n'
*/
static char get_type(const mode_t mode)
{
	char type = '\n';
	
	if (S_ISBLK(mode))
	{
		type = 'b';
	}
	else if (S_ISREG(mode))
	{
		type = 'f';
	}
	else if (S_ISCHR(mode))
	{
		type = 'c';
	}
	else if (S_ISDIR(mode))
	{
		type = 'd';
	}
	else if (S_ISFIFO(mode))
	{
		type = 'p';
	}
	else if (S_ISLNK(mode))
	{
		type = 'l';
	}
	else if (S_ISSOCK(mode))
	{
		type = 's';
	}
	return type;
}

/*
*checks the path against the prompted filepath
*/
static int do_path(const char * file_name, const char * const * parms, const int offset)
{
	int fnmatch_result;

	fnmatch_result = fnmatch(*(parms + offset), file_name, FNM_NOESCAPE);

	if (fnmatch_result == 0)
	{
		return SUCCESS;
	}

	return ERROR;
}







