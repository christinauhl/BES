/**
* @file myfind.c
*
* Beispiel 1
*
* @author Ralf Ziefuhs <ic17b065@technikum-wien.at>
* @author Christina <@technikum-wien.at>
* @author Clemens <@technikum-wien.at>
*
* @date 2018/03/08
*
* @version 1
*
*/

/*
* -------------------------------------------------------------- includes --
*/
#include <stdbool.h> // type bool, true, false
#include <string.h> // strcmp
#include <unistd.h> // readlink
#include <stdlib.h> // calloc, free
#include <pwd.h> // getpwnam, getpwuid
#include <grp.h> // getgrgid
#include <time.h> // localtime
#include <sys/stat.h> // lstat, stat
#include <dirent.h> // readdir, opendir...
#include <stdio.h> // printf
#include <fnmatch.h> // fnmatch
#include <errno.h> // errno

/*
* --------------------------------------------------------------- defines --
*/
/*
* -------------------------------------------------------------- typedefs --
*/


/* ### FB_TMG: Argumente mit komplexem Datentyp (i.e., structs)
sollten besser als const * �bergeben werden =>
const struct stat *file_stat*/

/*
* ------------------------------------------------------------- functions --
*/
static void do_file(const char * file_name, const char * const * parms);
static void do_dir(const char * dir_name, const char * const * parms);
/*static void do_show_usage(const char* const * parms);
static void do_check_parms(const char * const * parms);*/

int main(int argc, const char const *argv[])
{
	if (argc > 1) /*first parameter is mandatory, dont use default '.' because it could lead to problems when file_name starts with '-'*/
	{
		/*check_parms(&(argv[2]));*/
		do_file(argv[1], &(argv[2]));
	}
	else /*no file or directory specified*/
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

	//flush stdout buffer
	if (fflush(stdout) == EOF)
	{
		fprintf(stderr, "%s Unable to flush stdout: \n", strerror(errno));
	}
	return EXIT_SUCCESS;

}

static void do_file(const char *file_name, const char * const * parms)
{
	struct stat buf;

	if (lstat(file_name, &buf) == -1)
	{
		fprintf(stderr, "%s: error reading information of file: %s\n", strerror(errno), file_name);
		return;
	}

	/*print_if_match(file_name, parms);*/

	fprintf(stdout, "%s %s\n", file_name, *parms);

	if (S_ISDIR(buf.st_mode))
	{
		do_dir(file_name, parms);
	}

	/* else if(S_ISLINK(st.st_mode))
	{
	// This entry is a symbolic link
	}
	else if(S_ISREG(st.st_mode))
	{
	// This entry is a regular file
	}*/

	/*printf("File type:                ");

	switch (sb.st_mode & S_IFMT) {
	case S_IFBLK:  printf("block device\n");            break;
	case S_IFCHR:  printf("character device\n");        break;
	case S_IFDIR:  printf("directory\n");               break;
	case S_IFIFO:  printf("FIFO/pipe\n");               break;
	case S_IFLNK:  printf("symlink\n");                 break;
	case S_IFREG:  printf("regular file\n");            break;
	case S_IFSOCK: printf("socket\n");                  break;
	default:       printf("unknown?\n");                break;
	}*/
	return;
}


static void do_dir(const char * dir_name, const char * const * parms)
{
	DIR *dirp;
	const struct dirent *dp;
	const char *subdir;

	dirp = opendir(dir_name);

	if (dirp == NULL)
	{
		fprintf(stderr, "%s: error opening directory %s\n", strerror(errno), dir_name);
		exit(1);
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
		subdir = dp->d_name;

		/*excluding . and ..*/
		if (strcmp(subdir, ".") != 0 && strcmp(subdir, "..") != 0)
		{
			char new_path[sizeof(char) * (strlen(dir_name) + strlen(subdir) + 2)];

			/*generate new dir or file path*/
			if (dir_name[strlen(dir_name) - 1] == '/')
			{
				sprintf(new_path, "%s%s", dir_name, subdir);
			}
			else
			{
				sprintf(new_path, "%s/%s", dir_name, subdir);
			}

			/*fprintf(stdout, "%s\n", new_path);*/

			do_file(new_path, parms);
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



