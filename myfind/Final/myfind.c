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
#include <grp.h> //getgrgid
/*
* --------------------------------------------------------------- defines --
*/
#define SUCCESS 0
#define ERROR 1
/*
* -------------------------------------------------------------- typedefs --
*/
enum Bool { FALSE, TRUE };
/*
* ------------------------------------------------------------- functions --
*/
static void do_file(const char * file_name, const char * const * parms, const int offset);
static void do_dir(const char * dir_name, const char * const * parms, const int offset);
static int do_print(const char * file_name, const char * const * parms);
static int do_check_parms(const char * const * parms);
static int do_user(const struct stat buffer, const char * const * parms, const int offset);
static int do_name(const char * file_name, const char * const * parms, const int offset);
static int do_ls(const struct stat buffer, const char * file_name, const char * const * parms);
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
	int temp_offset = offset;
	int check_action = SUCCESS;
	int print_done = FALSE;


	if (lstat(file_name, &buf) == -1)
	{
		fprintf(stderr, "%s: `%s' %s\n", *parms, file_name, strerror(errno));
		return;
	}

	/*
	*runs as long as there are commandline arguments starting at the
	*first ((argv[0] + (temp_offset=2)) and depending on the action if
	*there are another arguments (temp_offset = temp_offset + 1 or + 2)
	*/
	while (*(parms + temp_offset) != NULL && check_action == SUCCESS)
	{
		if (strcmp(*(parms + temp_offset), "-user") == 0)
		{
			check_action = do_user(buf, parms, (temp_offset + 1));
			temp_offset = temp_offset + 2;
			continue;
		}

		if (strcmp(*(parms + temp_offset), "-name") == 0)
		{
			check_action = do_name(file_name, parms, (temp_offset + 1));
			temp_offset = temp_offset + 2;
			continue;

		}
		if (strcmp(*(parms + temp_offset), "-type") == 0)
		{
			check_action = do_type(buf, parms, (temp_offset + 1));
			temp_offset = temp_offset + 2;
			continue;

		}
		if (strcmp(*(parms + temp_offset), "-print") == 0)
		{
			check_action = do_print(file_name, parms);
			temp_offset = temp_offset + 1;
			print_done = TRUE;
			continue;
		}
		if (strcmp(*(parms + temp_offset), "-ls") == 0)
		{
			check_action = do_ls(buf, file_name, parms);
			temp_offset = temp_offset + 1;
			print_done = TRUE;
			continue;

		}
		if (strcmp(*(parms + temp_offset), "-nouser") == 0)
		{
			check_action = do_nouser(buf, parms);
			temp_offset = temp_offset + 1;
			continue;
		}
		if (strcmp(*(parms + temp_offset), "-path") == 0)
		{
			check_action = do_path(file_name, parms, (temp_offset + 1));
			temp_offset = temp_offset + 2;
			continue;

		}
	}

	if (print_done == FALSE && check_action == SUCCESS)
	{
		do_print(file_name, parms);
	}

	if (S_ISDIR(buf.st_mode))
	{
		do_dir(file_name, parms, offset);
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
	char ** cur_Arg = (char **)(parms + offset);

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

				struct stat buf;

				if (lstat(*(parms + 1), &buf) == -1)
				{
					fprintf(stderr, "%s: `%s' %s\n", *parms, *(parms + 1), strerror(errno));
					return ERROR;
				}

				const struct passwd *pwd_entry = getpwnam(*(cur_Arg + 1));

				if (errno != 0)
				{
					fprintf(stderr, "%s: error while processing -user %s\n", *parms, *(cur_Arg + 1));
					return ERROR;
				}

				if (pwd_entry != NULL)
				{
					//check if user found
					if (pwd_entry->pw_uid == buf.st_uid)
					{
						return SUCCESS;
					}
				}
				else //user not found, check uid
				{
					//conversion into long int
					uid = strtol(*(cur_Arg + 1), &p_end, 10);

					if (uid == LONG_MAX || uid == LONG_MIN)
					{
						fprintf(stderr, "%s: error overflow when trying to parse -user %s\n", *parms, *(cur_Arg + 1));
						exit(EXIT_FAILURE);
					}
					if (*p_end == '\0')
					{
						//check if uid found
						if (buf.st_uid == (unsigned)uid)
						{
							return SUCCESS;
						}
					}
				}
			}

			if (strcmp(*cur_Arg, "-type") == 0)
			{
				if (strcmp(*(cur_Arg + 1), "b") && strcmp(*(cur_Arg + 1), "c") &&
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
/*{
//int ret = 0; //return value

unsigned int uid;
char *p_end;

const struct passwd *pwd = NULL;


pwd = getpwnam(*(parms + offset));
if (pwd != NULL) user with corresponding name found
{
if (buffer.st_uid == pwd->pw_uid)
return SUCCESS;//ret = 1;
}
else //user not found, check uid..
{

uid = strtol(*(parms + offset), &p_end, 10);
if (*p_end == '\0') to int conversion OK
{
if (buffer.st_uid == uid) check if uid matches
{
return SUCCESS; //ret = 1;
}

}
//error handling for user not existing in check_parms() to avoid exiting in the middle of the run
}

return ERROR;
//return ret;
}*/

{
	errno = 0; //reset errno

	signed long uid = 0;
	char * p_end;

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
			if (buffer.st_uid == (unsigned)uid)
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

	return ERROR;
}


/*
*Checks and compares if the filename is the prompted name or not
*/
static int do_name(const char * file_name, const char * const * parms, const int offset)
{
	//int ret = 1; /*return value*/
	int fnmatch_ret;
	/*
	* ### FB_TMG: Jaja, die Signatur on basename(3) is gaga ... - Sie
	* könnten aber auch strrchr() verwenden dann sparen Sie das kopieren
	*/
	char temp_file_name[strlen(file_name) + 1]; /*VLA to avoid violating const of file_name*/
	char *base_name;

	memcpy(temp_file_name, file_name, strlen(file_name) + 1);
	base_name = basename(temp_file_name);

	/*check if basename matches pattern in name:*/
	fnmatch_ret = fnmatch(*(parms + offset), base_name, FNM_NOESCAPE);
	if (fnmatch_ret == 0)
	{
		return SUCCESS;
	}
	/*if fnmatch_ret not 0 and not FNM_NOMATCH -> error*/
	else if (fnmatch_ret != FNM_NOMATCH)
	{
		fprintf(stderr, "%s: error matching name to %s\n", *parms, *(parms + offset));
	}

	return ERROR;
}
/*{
int fnmatch_result;

//VLA
char temp_file_name[strlen(file_name) + 1];
char * base_name = NULL;

strcpy(temp_file_name, file_name);
base_name = basename(temp_file_name);

//check if basename matches pattern in name
fnmatch_result = fnmatch(*(parms + offset), base_name, FNM_NOESCAPE);

if (fnmatch_result == 0)
{
return SUCCESS;
}

return ERROR;
}*/



/*
*Checks if the file has no user
*SUCCESS if there is a file with no user
*EXIT if ther is no file with no user like as find
*/
static int do_nouser(const struct stat buffer, const char * const * parms)
{
	errno = 0; //reset errno

	const struct passwd *pwd_entry = getpwuid(buffer.st_uid);

	//check if file is with nouser, else user or errno exit like find
	if ((pwd_entry == NULL) && (errno == 0))
	{
		return SUCCESS;
	}
	else if (errno != 0)
	{
		fprintf(stderr, "%s: no file without user \n", *parms);
		exit(EXIT_FAILURE);
	}

	return ERROR;
}


/**
*
* \name do_ls
*
* \brief makes an ls output
*
* do_ls combines all usefull data and prints it
*
* \param buffer			stat file of the checkable file
* \param file_name		path to the file
* \param parameters		parameter list

* \return SUCCESS if no error occured, otherwise ERROR
* \retval 0	SUCCESS
* \retval 1	ERROR
*/
/*
* ### FB_TMG: Argumente mit komplexem Datentyp (i.e., structs)
* sollten besser als const * übergeben werden =>
* const struct stat *buffer
*/
static int do_ls(const struct stat buffer, const char * file_name, const char * const * parameters)
{
	char   mode[] = { "----------" };
	struct passwd* user;
	struct group* group;
	struct tm* time;
	/*
	* ### FB_TMG: Und was, wenn die Stringrepräsentation des abgekürzten
	* Monats länger als 4 Zeichen ist? [-1]
	*/
	char month[4];
	char* user_name = "";
	char* group_name = "";
	/*
	* ### FB_TMG: Und was, wenn die Stringrepräsentation des Datums
	* länger als 13 Zeichen ist? - Kann aber nur passieren, wenn die
	* Stringrepräsentation des abgekürzten Monats entsprechend lang ist
	* ...
	*/
	char time_disp[13] = { 0 }; //we want 0 termination
	char link_string[buffer.st_size + 1];
	char file_string[strlen(file_name)];
	unsigned long blocks = 0l;
	/*
	* ### FB_TMG: Und was, wenn die Stringrepräsentation der UID länger
	* als 13 Zeichen ist? [-1]
	*/
	char uid[13];
	/*
	* ### FB_TMG: Und was, wenn die Stringrepräsentation der GID länger
	* als 13 Zeichen ist? [-1]
	*/
	char gid[13];

	switch (buffer.st_mode & S_IFMT)
	{
	case S_IFREG:	/* regular file*/
		mode[0] = '-';
		break;
	case S_IFDIR:	/* directory */
		mode[0] = 'd';
		break;
	case S_IFCHR: 	/* character device */
		mode[0] = 'c';
		break;
	case S_IFBLK:	/* block device */
		mode[0] = 'b';
		break;
	case S_IFIFO:	/* FIFO */
		mode[0] = 'f';
		break;
	case S_IFLNK:	/* symbolic link */
		mode[0] = 'l';
		break;
	case S_IFSOCK:	/* socket */
		mode[0] = 's';
		break;
	default:		/* unknown */
		mode[0] = '?';
		break;
	}

	if (S_IRUSR & buffer.st_mode) /*read*/
		mode[1] = 'r';
	if (S_IWUSR & buffer.st_mode) /*write*/
		mode[2] = 'w';
	if ((S_IXUSR & buffer.st_mode) && !(S_ISUID & buffer.st_mode)) /*execute without sticky*/
		mode[3] = 'x';
	else if (S_IXUSR & buffer.st_mode) /*execute with sticky*/
		mode[3] = 's';
	else if (S_ISUID & buffer.st_mode) /*not exec with sticky*/
		mode[3] = 'S';

	/*rows the same as above*/
	if (S_IRGRP & buffer.st_mode)
		mode[4] = 'r';
	if (S_IWGRP & buffer.st_mode)
		mode[5] = 'w';
	if ((S_IXGRP & buffer.st_mode) && !(S_ISGID & buffer.st_mode))
		mode[6] = 'x';
	else if (S_IXGRP & buffer.st_mode)
		mode[6] = 's';
	else if (S_ISGID & buffer.st_mode)
		mode[6] = 'S';

	/*rows same as above*/
	if (S_IROTH & buffer.st_mode)
		mode[7] = 'r';
	if (S_IWOTH & buffer.st_mode)
		mode[8] = 'w';
	if ((S_IXOTH & buffer.st_mode) && !(S_ISVTX & buffer.st_mode))
		mode[9] = 'x';
	else if (S_IXOTH & buffer.st_mode)
		mode[9] = 't';
	else if (S_ISVTX & buffer.st_mode)
		mode[9] = 'T';

	//how to blocks -- credit to c-tutorials
	/*
	* ### FB_TMG: Blocksize bei symlinks wird korrekt behandelt [+2]
	*/
	if (mode[0] != 'l')
	{
		blocks = (unsigned long)buffer.st_blocks;
		/*
		* ### FB_TMG: POSIXLY_CORRECT wird korrekt behandelt [+2]
		*/
		if (getenv("POSIXLY_CORRECT") == NULL)
			blocks = ((unsigned long)buffer.st_blocks / 2 + buffer.st_blocks % 2);
	}

	if ((user = getpwuid(buffer.st_uid)) == NULL || user->pw_name == NULL)
	{
		if (errno != 0)
		{
			fprintf(stderr, "%s: %s - %s\n", *parameters, file_name, strerror(errno));
		}
		else
		{
			snprintf(uid, sizeof(uid), "%d", buffer.st_uid);
			user_name = uid;
		}

	}
	else
	{
		user_name = user->pw_name;
	}

	errno = 0;
	//similar to user 
	if ((group = getgrgid(buffer.st_gid)) == NULL || (group->gr_name == NULL))
	{
		if (errno != 0)
		{
			fprintf(stderr, "%s: %s - %s\n", *parameters, file_name, strerror(errno));
		}
		else
		{
			snprintf(gid, sizeof(gid), "%d", buffer.st_gid);
			group_name = gid;
		}
	}
	else
	{
		group_name = group->gr_name;
	}
	errno = 0;

	time = localtime(&(buffer.st_mtime));
	strftime(month, sizeof(month), "%b", time);

	if (sprintf(time_disp, "%s %2d %02d:%02d", month, time->tm_mday, time->tm_hour, time->tm_min) < 0)
	{
		fprintf(stderr, "%s: unable to create string", *parameters);
	}

	if (mode[0] != 'l')
	{
		strcpy(file_string, file_name);
	}
	else
	{
		int temp = readlink(file_name, link_string, buffer.st_size);
		if (temp>1)
		{
			/*
			* ### FB_TMG: readlink() fügt kein terminierendes '\0' an den String
			* => potentieller Absturz [-2]
			*/
			snprintf(file_string, temp + strlen(file_name) + 6, "%s -> %s", file_name, link_string);
			file_string[strlen(file_string) - 1] = '\0';
		}
		else
		{
			fprintf(stderr, "%s: couldn't read link: %s", *parameters, strerror(errno));
		}
	}

	/*
	* ### FB_TMG: Die Referenzimplementierung gibt keine size bei
	* character und block special files aus
	*/

	if (fprintf(stdout, "%6ld %4ld %s %3d %s %s %8ld %s %s\n", buffer.st_ino, blocks, mode, buffer.st_nlink, user_name, group_name, buffer.st_size, time_disp, file_string) < 0)
	{
		fprintf(stderr, "%s: ls error %s", *(parameters), file_name);
		return ERROR;
	}

	return SUCCESS;
}



/*
*SUCCESS if the given file has the searched pattern
*EXIT if the file has not the searched pattern like as find
*/
static int do_type(const struct stat buffer, const char * const* parms, const int offset)
{
	char type_char = '\0';

	//check type char of file_name
	type_char = get_type(buffer.st_mode);

	if (type_char == **(parms + offset))
	{
		return SUCCESS;

	}
	/*else if (type_char != **(parms + offset))
	{
	//wrong parameter type
	fprintf(stderr, "%s: wrong parameter type \n", *parms);
	exit(EXIT_FAILURE);
	}*/

	return ERROR;
}

/*
* returns the type of the file (bfcdpls) or default '\n'
*/
static char get_type(const mode_t mode)
{
	char type = '\0';

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