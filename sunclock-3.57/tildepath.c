/*
T* NAME
 *
 *    tildepath.c - Convert ~name to a path.
 *
T* SYNOPSIS
 *
 *    char *tildepath(path);
 *    char *path;		- ~path specification
 *
T* DESCRIPTION
 *
 *    This routine takes a path of the for ~[name]/... and returns the true
 * path for the users home directory.  Any additional path information is
 * tacked onto the end of the new path. 
 *
T* RETURNS
 *
 *    A path if one is valid, NULL otherwise.  Note that the memory allocated
 * for the new path should be freed by the calling program.
 *
T* AUTHOR
 *
 *    Stephen Martin, March 12, 1991.
 */

/*
 * Version
 */

/* static char SccsId[] = { "@(#) tildepath.c 1.1@(#)" }; */

/*
 * Include Files
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <malloc.h>
#include <string.h>

/*
 * Define Statements
 */

#ifndef NULL
#define NULL (char *) 0
#endif

/*
 * tildepath()
 */

char *
tildepath(path)
char *path;		/* Path starting with ~ */
{
    /*
     * Local Variables
     */

    char *username;	/* Points to the username */
    char *rest;		/* Rest of input path */
    char *result;	/* Used to construct the result */
    struct passwd *pwent; /* Password entry for user */
    int size;		/* Length of new path */

    /*
     * Functions
     */

    /*    char *strdup();	*/	/* Make a copy of a path */

    /*
     * If the path doesn't start with ~ quit right now
     */

    if (path[0] != '~')
	return(NULL);

    /*
     * Find the name delimited by / or the end of the string
     */

    if ((username = strdup(++path)) == NULL)
	return(NULL);
    if ((rest = strchr(username, '/'))) {
	*rest = '\0';
	rest++;
    }

    /*
     * Get the user's password entry
     */

    if (username[0])
	pwent = getpwnam(username);
    else
	pwent = getpwuid(getuid());

    /*
     * Check to see if a password entry was found
     */

    if (pwent == NULL) {
	free(username);	
	return(NULL);
    }

    /*
     * Determine the size of the new path and allocate space for it
     */

    size = strlen(pwent->pw_dir) + 1;
    if (rest && (rest[0] != '\0'))
	size += strlen(rest) + 1;
    if ((result = calloc(size, sizeof(char))) == NULL) {
	free(username);
	return(NULL);
    }

    /*
     * Copy over the new path
     */

    strcpy(result, pwent->pw_dir);
    if (rest && (rest[0] != '\0')) {
	strcat(result, "/");
	strcat(result, rest);
    }

    /*
     * Done
     */

    free(username);
    return(result);
}
