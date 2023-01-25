/*
 * file: file.c
 * description: handle all file based efuns
 */

#include "std.h"
#include "lpc_incl.h"
#include "file_incl.h"
#include "comm.h"
#include "strstr.h"
#include "file.h"
#include "lex.h"
#include "md.h"

/* Removed due to hideousness: if you want to add it back, not that
 * we don't want redefinitions, and that some systems define major() in
 * one place, some in both, etc ...
 */
#if 0
#ifdef INCL_SYS_SYSMACROS_H
/* Why yes, this *is* a kludge! */
#  ifdef major
#    undef major
#  endif
#  ifdef minor
#    undef minor
#  endif
#  ifdef makedev
#    undef makedev
#  endif
#  include <sys/sysmacros.h>
#endif
#endif

/* see binaries.c.  We don't want no $@$(*)@# system dependent mess of
   includes */
#ifdef WIN32
#include <direct.h>
#include <io.h>
#endif

extern int errno;
extern int sys_nerr;

int legal_path PROT((char *));

static int match_string PROT((char *, char *));
static int isdir PROT((char *path));
static int copy PROT((char *from, char *to));
static int do_move PROT((char *from, char *to, int flag));
static int pstrcmp PROT((svalue_t *, svalue_t *));
static int parrcmp PROT((svalue_t *, svalue_t *));
static void encode_stat PROT((svalue_t *, int, char *, struct stat *));

#define MAX_LINES 50

/*
 * These are used by qsort in get_dir().
 */
static int pstrcmp P2(svalue_t *, p1, svalue_t *, p2)
{
    return strcmp(p1->u.string, p2->u.string);
}

static int parrcmp P2(svalue_t *, p1, svalue_t *, p2)
{
    return strcmp(p1->u.arr->item[0].u.string, p2->u.arr->item[0].u.string);
}

static void encode_stat P4(svalue_t *, vp, int, flags, char *, str, struct stat *, st)
{
    if (flags == -1) {
	array_t *v = allocate_empty_array(3);

	v->item[0].type = T_STRING;
	v->item[0].subtype = STRING_MALLOC;
	v->item[0].u.string = string_copy(str, "encode_stat");
	v->item[1].type = T_NUMBER;
	v->item[1].u.number =
	    ((st->st_mode & S_IFDIR) ? -2 : st->st_size);
	v->item[2].type = T_NUMBER;
	v->item[2].u.number = st->st_mtime;
	vp->type = T_ARRAY;
	vp->u.arr = v;
    } else {
	vp->type = T_STRING;
	vp->subtype = STRING_MALLOC;
	vp->u.string = string_copy(str, "encode_stat");
    }
}

/*
 * List files in directory. This function do same as standard list_files did,
 * but instead writing files right away to user this returns an array
 * containing those files. Actually most of code is copied from list_files()
 * function.
 * Differences with list_files:
 *
 *   - file_list("/w"); returns ({ "w" })
 *
 *   - file_list("/w/"); and file_list("/w/."); return contents of directory
 *     "/w"
 *
 *   - file_list("/");, file_list("."); and file_list("/."); return contents
 *     of directory "/"
 *
 * With second argument equal to non-zero, instead of returning an array
 * of strings, the function will return an array of arrays about files.
 * The information in each array is supplied in the order:
 *    name of file,
 *    size of file,
 *    last update of file.
 */
/* WIN32 should be fixed to do this correctly (i.e. no ifdefs for it) */
#define MAX_FNAME_SIZE 255
#define MAX_PATH_LEN   1024
array_t *get_dir P2(char *, path, int, flags)
{
    array_t *v;
    int i, count = 0;
#ifndef WIN32
    DIR *dirp;
#endif
    int namelen, do_match = 0;

#ifndef WIN32
#ifdef USE_STRUCT_DIRENT
    struct dirent *de;
#else
    struct direct *de;
#endif
#endif
    struct stat st;
    char *endtemp;
    char temppath[MAX_FNAME_SIZE + MAX_PATH_LEN + 2];
    char regexppath[MAX_FNAME_SIZE + MAX_PATH_LEN + 2];
    char *p;

#ifdef WIN32
    struct _finddata_t FindBuffer;
    long FileHandle, FileCount;
#endif

    if (!path)
	return 0;

    path = check_valid_path(path, current_object, "stat", 0);

    if (path == 0)
	return 0;

    if (strlen(path) < 2) {
#ifndef LATTICE
	temppath[0] = path[0] ? path[0] : '.';
#else
	temppath[0] = path[0];
#endif
	temppath[1] = '\000';
	p = temppath;
    } else {
	strncpy(temppath, path, MAX_FNAME_SIZE + MAX_PATH_LEN + 1);
	temppath[MAX_FNAME_SIZE + MAX_PATH_LEN + 1] = '\0';

	/*
	 * If path ends with '/' or "/." remove it
	 */
#ifdef WIN32
	if ((p = strrchr(temppath, '\\')) == 0)
	    p = temppath;
	if (p[0] == '\\' && ((p[1] == '.' && p[2] == '\0') || p[1] == '\0'))
#else
	if ((p = strrchr(temppath, '/')) == 0)
	    p = temppath;
	if (p[0] == '/' && ((p[1] == '.' && p[2] == '\0') || p[1] == '\0'))
#endif
	    *p = '\0';
    }

    if (stat(temppath, &st) < 0) {
	if (*p == '\0')
	    return 0;
	if (p != temppath) {
	    strcpy(regexppath, p + 1);
	    *p = '\0';
	} else {
	    strcpy(regexppath, p);
#ifndef LATTICE
	    strcpy(temppath, ".");
#else
	    strcpy(temppath, "");
#endif
	}
	do_match = 1;
#ifdef WIN32
    } else if (*p != '\0' && strcmp(temppath, ".")) {
	if (*p == '\\' && *(p + 1) != '\0')
#else
    } else if (*p != '\0' && strcmp(temppath, ".")) {
	if (*p == '/' && *(p + 1) != '\0')
#endif
	    p++;
	v = allocate_empty_array(1);
	encode_stat(&v->item[0], flags, p, &st);
	return v;
    }
/*#ifdef LATTICE
	if (temppath[0]=='.') temppath[0]=0;
#endif*/
#ifdef WIN32
    FileHandle = -1;
    FileCount = 1;
    strcat(temppath, "\\*");
    if ((FileHandle = _findfirst(temppath, &FindBuffer)) == -1) return 0;
#else
    if ((dirp = opendir(temppath)) == 0)
	return 0;
#endif

    /*
     * Count files
     */
#ifdef WIN32
    do {
	if (!do_match && (!strcmp(FindBuffer.name, ".") ||
			  !strcmp(FindBuffer.name, ".."))) {
	    continue;
	}
	if (do_match && !match_string(regexppath, FindBuffer.name)) {
	    continue;
	}
	count++;
	if (count >= max_array_size) {
	    break;
	}
    } while (!_findnext(FileHandle, &FindBuffer));
    _findclose(FileHandle);
#else
    for (de = readdir(dirp); de; de = readdir(dirp)) {
#ifdef USE_STRUCT_DIRENT
	namelen = strlen(de->d_name);
#else
	namelen = de->d_namlen;
#endif
	if (!do_match && (strcmp(de->d_name, ".") == 0 ||
			  strcmp(de->d_name, "..") == 0))
	    continue;
	if (do_match && !match_string(regexppath, de->d_name))
	    continue;
	count++;
	if (count >= max_array_size)
	    break;
    }
#endif

    /*
     * Make array and put files on it.
     */
    v = allocate_empty_array(count);
    if (count == 0) {
	/* This is the easy case :-) */
#ifndef WIN32
	closedir(dirp);
#endif
	return v;
    }
#ifdef WIN32
    FileHandle = -1;
    if ((FileHandle = _findfirst(temppath, &FindBuffer)) == -1) return 0;
    endtemp = temppath + strlen(tmppath) - 2;
    *endtemp = 0;
    strcat(endtemp++, "\\");
    i = 0;
    do {
	if (!do_match && (!strcmp(FindBuffer.name, ".") ||
			  !strcmp(FindBuffer.name, ".."))) continue;
	if (do_match && !match_string(regexppath, FindBuffer.name)) continue;
	if (flags == -1) {
	    strcpy(endtemp, FindBuffer.name);
	    stat(temppath, &st);
	}
	encode_stat(&v->item[i], flags, FindBuffer.name, &st);
	i++;
    } while (!_findnext(FileHandle, &FindBuffer));
    _findclose(FileHandle);
#else				/* WIN32 */
    rewinddir(dirp);
    endtemp = temppath + strlen(temppath);

#ifdef LATTICE
    if (endtemp != temppath)
#endif
	strcat(endtemp++, "/");

    for (i = 0, de = readdir(dirp); i < count; de = readdir(dirp)) {
#ifdef USE_STRUCT_DIRENT
	namelen = strlen(de->d_name);
#else
	namelen = de->d_namlen;
#endif
	if (!do_match && (strcmp(de->d_name, ".") == 0 ||
			  strcmp(de->d_name, "..") == 0))
	    continue;
	if (do_match && !match_string(regexppath, de->d_name))
	    continue;
	de->d_name[namelen] = '\0';
	if (flags == -1) {
	    /*
	     * We'll have to .... sigh.... stat() the file to get some add'tl
	     * info.
	     */
	    strcpy(endtemp, de->d_name);
	    stat(temppath, &st);/* We assume it works. */
	}
	encode_stat(&v->item[i], flags, de->d_name, &st);
	i++;
    }
    closedir(dirp);
#endif				/* OS2 */

    /* Sort the names. */
#if 1
    qsort((void *) v->item, count, sizeof v->item[0],
	  (int (*) ()) ((flags == -1) ? parrcmp : pstrcmp));
#else
    qsort((char *) v->item, count, sizeof v->item[0],
	  (flags == -1) ? parrcmp : pstrcmp);
#endif
    return v;
}

int tail P1(char *, path)
{
    char buff[1000];
    FILE *f;
    struct stat st;
    int offset;

    path = check_valid_path(path, current_object, "tail", 0);

    if (path == 0)
	return 0;
#ifdef LATTICE
    if (stat(path, &st) == -1)
	return 0;
#endif
    f = fopen(path, "r");
    if (f == 0)
	return 0;
#ifndef LATTICE
    if (fstat(fileno(f), &st) == -1)
	fatal("Could not stat an open file.\n");
#endif
    offset = st.st_size - 54 * 20;
    if (offset < 0)
	offset = 0;
    if (fseek(f, offset, 0) == -1)
	fatal("Could not seek.\n");
    /* Throw away the first incomplete line. */
    if (offset > 0)
	(void) fgets(buff, sizeof buff, f);
    while (fgets(buff, sizeof buff, f)) {
	tell_object(command_giver, buff);
    }
    fclose(f);
    return 1;
}

int remove_file P1(char *, path)
{
    path = check_valid_path(path, current_object, "remove_file", 1);

    if (path == 0)
	return 0;
    if (unlink(path) == -1)
	return 0;
    return 1;
}

/*
 * Check that it is an legal path. No '..' are allowed.
 */
int legal_path P1(char *, path)
{
    char *p;

    if (path == NULL)
	return 0;
    if (path[0] == '/')
	return 0;
    /*
     * disallowing # seems the easiest way to solve a bug involving loading
     * files containing that character
     */
    if (strchr(path, '#')) {
	return 0;
    }
    p = path;
    while (p) {			/* Zak, 930530 - do better checking */
	if (p[0] == '.') {
	    if (p[1] == '\0')	/* trailing `.' ok */
		break;
	    if (p[1] == '.')	/* check for `..' or `../' */
		p++;
	    if (p[1] == '/' || p[1] == '\0')
		return 0;	/* check for `./', `..', or `../' */
	}
	p = (char *)_strstr(p, "/.");	/* search next component */
	if (p)
	    p++;		/* step over `/' */
    }
#if defined(AMIGA) || defined(LATTICE) || defined(WIN32)
    /*
     * I don't know what the proper define should be, just leaving an
     * appropriate place for the right stuff to happen here - Wayfarer
     */
    /*
     * fail if there's a ':' since on AmigaDOS this means it's a logical
     * device!
     */
    /* Could be a drive thingy for os2. */
    if (strchr(path, ':'))
	return 0;
#endif
    return 1;
}				/* legal_path() */

/*
 * There is an error in a specific file. Ask the MudOS driver to log the
 * message somewhere.
 */
void smart_log P4(char *, error_file, int, line, char *, what, int, flag)
{
    char *buff;
    svalue_t *mret;
    extern int pragmas;

    buff = (char *)
	DMALLOC(strlen(error_file) + strlen(what) + 
		((pragmas & PRAGMA_ERROR_CONTEXT) ? 100 : 40), TAG_TEMPORARY, "smart_log: 1");

    if (flag)
	sprintf(buff, "%s line %d: Warning: %s", error_file, line, what);
    else
	sprintf(buff, "%s line %d: %s", error_file, line, what);

    if (pragmas & PRAGMA_ERROR_CONTEXT){
        char *ls = strrchr(buff, '\n');
	char *tmp;
	if (ls) {
	    tmp = ls + 1;
	    while (*tmp && isspace(*tmp)) tmp++;
	    if (!*tmp) *ls = 0;
	}
	strcat(buff, show_error_context());
    } else strcat(buff, "\n");

    if (flag) 
	sprintf(buff, "%s line %d: Warning: %s%s", error_file, line, what,
		(pragmas & PRAGMA_ERROR_CONTEXT) ? show_error_context() : "\n");
    else
	sprintf(buff, "%s line %d: %s%s", error_file, line, what,
		(pragmas & PRAGMA_ERROR_CONTEXT) ? show_error_context() : "\n");
    
    push_constant_string(error_file);
    push_constant_string(buff);
    mret = safe_apply_master_ob(APPLY_LOG_ERROR, 2);
    if (!mret || mret == (svalue_t *)-1) {
	debug_message("%s", buff);
    }
    FREE(buff);
}				/* smart_log() */

/*
 * Append string to file. Return 0 for failure, otherwise 1.
 */
int write_file P3(char *, file, char *, str, int, flags)
{
    FILE *f;

#ifdef WIN32
    char fmode[3];
#endif

    file = check_valid_path(file, current_object, "write_file", 1);
    if (!file)
	return 0;
#ifdef WIN32
    fmode[0] = (flags & 1) ? 'w' : 'a';
    fmode[1] = 'b';
    fmode[2] = '\0';
    f = fopen(file, fmode);
#else    
    f = fopen(file, (flags & 1) ? "w" : "a");
#endif
    if (f == 0) {
	error("Wrong permissions for opening file /%s for %s.\n\"%s\"\n",
	      file, (flags & 1) ? "overwrite" : "append", strerror(errno));
    }
    fwrite(str, strlen(str), 1, f);
    fclose(f);
    return 1;
}				/* write_file() */

char *read_file P3(char *, file, int, start, int, len)
{
    struct stat st;
    FILE *f;
    char *str, *end, *tmp, esc[2];
    register char *p, *p2;
    int size;

    // Gestur
    esc[0]=27;
    esc[1]=0;
    //
    if (len < 0)
	return 0;
    file = check_valid_path(file, current_object, "read_file", 0);

    if (!file)
	return 0;

    /*
     * file doesn't exist, or is really a directory
     */
    if (stat(file, &st) == -1 || (st.st_mode & S_IFDIR))
	return 0;

    f = fopen(file, "r");
    if (f == 0)
	return 0;

#ifndef LATTICE
    if (fstat(fileno(f), &st) == -1)
	fatal("Could not stat an open file.\n");
#endif

    size = st.st_size;
    if (size > READ_FILE_MAX_SIZE) {
	if (start || len)
	    size = READ_FILE_MAX_SIZE;
	else {
	    fclose(f);
	    return 0;
	}
    }
    if (start < 1)
	start = 1;
    if (!len)
	len = READ_FILE_MAX_SIZE;
    str = new_string(size, "read_file: str");
    str[size] = '\0';
    do {
	if ((fread(str, size, 1, f) != 1) || !size) {
	    fclose(f);
	    FREE_MSTR(str);
	    return 0;
	}

	if (size > st.st_size) {
	    size = st.st_size;
	}		
        // Gestur lo toquetea todo como siempre.//
        while (tmp=strstr(str, "@@"))
        {
           tmp[1]='%';
           tmp[0]='#';
        }
/*        while (tmp=strstr(str, esc))
        {
           tmp[0]='!';
        }
*/                                                                              
	st.st_size -= size;
	end = str + size;
	for (p = str; --start && (p2 = (char *) memchr(p, '\n', end - p));) {
	    p = p2 + 1;
	}
    } while (start > 1);
    
    if (len != READ_FILE_MAX_SIZE || st.st_size){
        for (p2 = str; p != end;) {
	    if ((*p2++ = *p++) == '\n')
	        if (!--len)
		    break;
	}
	if (len && st.st_size) {
	    size -= (p2 - str);
	    
	    if (size > st.st_size)
		size = st.st_size;

	    if ((fread(p2, size, 1, f) != 1) || !size) {
		fclose(f);
		FREE_MSTR(str);
		return 0;
	    }
	    st.st_size -= size;
	    end = p2 + size;
	    for (; p2 != end;) {
		if (*p2++ == '\n')
		    if (!--len)
			break;
	    }

	    if (st.st_size && len) {
		/* tried to read more than READ_MAX_FILE_SIZE */
		fclose(f);
		FREE_MSTR(str);
		return 0;
	    }
	}
	*p2 = '\0';
	str = extend_string(str, p2 - str);
    } 
	
    fclose(f);
    return str;
}				/* read_file() */


char *read_bytes P4(char *, file, int, start, int, len, int *, rlen)
{
    struct stat st;
    FILE *fp;
    char *str;
    int size;

    if (len < 0)
	return 0;
    file = check_valid_path(file, current_object,
			    "read_bytes", 0);
    if (!file)
	return 0;
#ifdef LATTICE
    if (stat(file, &st) == -1)
	return 0;
#endif
    fp = fopen(file, "rb");
    if (fp == NULL)
	return 0;
#ifndef LATTICE
    if (fstat(fileno(fp), &st) == -1)
	fatal("Could not stat an open file.\n");
#endif
    size = st.st_size;
    if (start < 0)
	start = size + start;

    if (len == 0)
	len = size;
    if (len > MAX_BYTE_TRANSFER) {
	error("Transfer exceeded maximum allowed number of bytes.\n");
	return 0;
    }
    if (start >= size) {
	fclose(fp);
	return 0;
    }
    if ((start + len) > size)
	len = (size - start);

    if ((size = fseek(fp, start, 0)) < 0)
	return 0;

    str = new_string(len, "read_bytes: str");

    size = fread(str, 1, len, fp);

    fclose(fp);

    if (size <= 0) {
	FREE_MSTR(str);
	return 0;
    }
    /*
     * The string has to end to '\0'!!!
     */
    str[size] = '\0';

    *rlen = size;
    return str;
}

int write_bytes P4(char *, file, int, start, char *, str, int, theLength)
{
    struct stat st;
    int size;
    FILE *fp;

    file = check_valid_path(file, current_object, "write_bytes", 1);

    if (!file)
	return 0;
    if (theLength > MAX_BYTE_TRANSFER)
	return 0;
    /* Under system V, it isn't possible change existing data in a file
     * opened for append, so it can't be opened for append.
     * opening for r+ won't create the file if it doesn't exist.
     * opening for w or w+ will truncate it if it does exist.  So we
     * have to check if it exists first.
     */
    if (stat(file, &st) == -1) {
      fp = fopen(file, "wb");
    } else {
      fp = fopen(file, "r+b");
    }
    if (fp == NULL) {
	return 0;
    }
#ifndef LATTICE
    if (fstat(fileno(fp), &st) == -1)
	fatal("Could not stat an open file.\n");
#endif
    size = st.st_size;
    if (start < 0)
	start = size + start;
    if (start < 0 || start > size) {
	fclose(fp);
	return 0;
    }
    if ((size = fseek(fp, start, 0)) < 0) {
	fclose(fp);
	return 0;
    }
    size = fwrite(str, 1, theLength, fp);

    fclose(fp);

    if (size <= 0) {
	return 0;
    }
    return 1;
}

int file_size P1(char *, file)
{
    struct stat st;

    file = check_valid_path(file, current_object, "file_size", 0);
    if (!file)
	return -1;
    if (stat(file, &st) == -1)
	return -1;
    if (S_IFDIR & st.st_mode)
	return -2;
    return st.st_size;
}


/*
 * Check that a path to a file is valid for read or write.
 * This is done by functions in the master object.
 * The path is always treated as an absolute path, and is returned without
 * a leading '/'.
 * If the path was '/', then '.' is returned.
 * The returned string may or may not be residing inside the argument 'path',
 * so don't deallocate arg 'path' until the returned result is used no longer.
 * Otherwise, the returned path is temporarily allocated by apply(), which
 * means it will be deallocated at next apply().
 */
char *check_valid_path P4(char *, path, object_t *, call_object, char *, call_fun, int, writeflg)
{
    svalue_t *v;

    if (call_object == 0 || call_object->flags & O_DESTRUCTED)
	return 0;
    push_string(path, STRING_MALLOC);
    push_object(call_object);
    push_string(call_fun, STRING_CONSTANT);
    if (writeflg)
	v = apply_master_ob(APPLY_VALID_WRITE, 3);
    else
	v = apply_master_ob(APPLY_VALID_READ, 3);

    if (v && v != (svalue_t *)-1 && v->type == T_NUMBER && v->u.number == 0)
	return 0;
    if (path[0] == '/')
	path++;
#ifndef LATTICE
    if (path[0] == '\0')
	path = ".";
#endif
    if (legal_path(path)) {
#ifdef WIN32
	static char paths[10][100];
	char *fluff;
	static int bing = 0;
	int old_bing;

	/* Mangle the path for writing to the disk. */
	strcpy(paths[bing], path);
	fluff = paths[bing];
	while (fluff = strchr(fluff, '/')) {
	    *fluff = '\\';
	}			/* endwhile */
	old_bing = bing;
	bing = (bing + 1) % 10;
	return paths[old_bing];
#else
	return path;
#endif
    }
    return 0;
}

static int match_string P2(char *, match, char *, str)
{
    int i;

  again:
    if (*str == '\0' && *match == '\0')
	return 1;
    switch (*match) {
    case '?':
	if (*str == '\0')
	    return 0;
	str++;
	match++;
	goto again;
    case '*':
	match++;
	if (*match == '\0')
	    return 1;
	for (i = 0; str[i] != '\0'; i++)
	    if (match_string(match, str + i))
		return 1;
	return 0;
    case '\0':
	return 0;
    case '\\':
	match++;
	if (*match == '\0')
	    return 0;
	/* Fall through ! */
    default:
	if (*match == *str) {
	    match++;
	    str++;
	    goto again;
	}
	return 0;
    }
}

/*
 * Credits for some of the code below goes to Free Software Foundation
 * Copyright (C) 1990 Free Software Foundation, Inc.
 * See the GNU General Public License for more details.
 */
#ifndef S_ISDIR
#define	S_ISDIR(m)	(((m)&S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define	S_ISREG(m)	(((m)&S_IFMT) == S_IFREG)
#endif

#ifndef S_ISCHR
#define	S_ISCHR(m)	(((m)&S_IFMT) == S_IFCHR)
#endif

#ifndef S_ISBLK
#define	S_ISBLK(m)	(((m)&S_IFMT) == S_IFBLK)
#endif

static int isdir P1(char *, path)
{
    struct stat stats;

    return stat(path, &stats) == 0 && S_ISDIR(stats.st_mode);
}

static struct stat to_stats, from_stats;

static int copy P2(char *, from, char *, to)
{
    int ifd;
    int ofd;
    char buf[1024 * 8];
    int len;			/* Number of bytes read into `buf'. */

    if (!S_ISREG(from_stats.st_mode)) {
	return 1;
    }
    if (unlink(to) && errno != ENOENT) {
	return 1;
    }
    ifd = open(from, O_RDONLY);
    if (ifd < 0) {
	return errno;
    }
    ofd = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (ofd < 0) {
	close(ifd);
	return 1;
    }
#ifdef HAS_FCHMOD
    if (fchmod(ofd, from_stats.st_mode & 0777)) {
	close(ifd);
	close(ofd);
	unlink(to);
	return 1;
    }
#endif

    while ((len = read(ifd, buf, sizeof(buf))) > 0) {
	int wrote = 0;
	char *bp = buf;

	do {
	    wrote = write(ofd, bp, len);
	    if (wrote < 0) {
		close(ifd);
		close(ofd);
		unlink(to);
		return 1;
	    }
	    bp += wrote;
	    len -= wrote;
	} while (len > 0);
    }
    if (len < 0) {
	close(ifd);
	close(ofd);
	unlink(to);
	return 1;
    }
    if (close(ifd) < 0) {
	close(ofd);
	return 1;
    }
    if (close(ofd) < 0) {
	return 1;
    }
#ifdef FCHMOD_MISSING
    if (chmod(to, from_stats.st_mode & 0777)) {
	return 1;
    }
#endif

    return 0;
}

/* Move FROM onto TO.  Handles cross-filesystem moves.
   If TO is a directory, FROM must be also.
   Return 0 if successful, 1 if an error occurred.  */

#ifdef F_RENAME
static int do_move P3(char *, from, char *, to, int, flag)
{
    if (lstat(from, &from_stats) != 0) {
	error("/%s: lstat failed\n", from);
	return 1;
    }
    if (lstat(to, &to_stats) == 0) {
#ifdef WIN32
	if (!strcmp(from, to))
#else
	if (from_stats.st_dev == to_stats.st_dev
	    && from_stats.st_ino == to_stats.st_ino)
#endif
	{
	    error("`/%s' and `/%s' are the same file", from, to);
	    return 1;
	}
	if (S_ISDIR(to_stats.st_mode)) {
	    error("/%s: cannot overwrite directory", to);
	    return 1;
	}
#ifdef WIN32
	unlink(to);
#endif
    } else if (errno != ENOENT) {
	error("/%s: unknown error\n", to);
	return 1;
    }
#if defined(SYSV) && !defined(_SEQUENT_)
    if ((flag == F_RENAME) && isdir(from)) {
	char cmd_buf[100];

	sprintf(cmd_buf, "/usr/lib/mv_dir %s %s", from, to);
	return system(cmd_buf);
    } else
#endif				/* SYSV */
    if ((flag == F_RENAME) && (rename(from, to) == 0))
	return 0;
#ifdef F_LINK
    else if (flag == F_LINK) {
#ifdef WIN32
	error("link() not supported.\n");
#else
	if (link(from, to) == 0)
	    return 0;
#endif
    }
#endif

    if (errno != EXDEV) {
	if (flag == F_RENAME)
	    error("cannot move `/%s' to `/%s'\n", from, to);
	else
	    error("cannot link `/%s' to `/%s'\n", from, to);
	return 1;
    }
    /* rename failed on cross-filesystem link.  Copy the file instead. */

    if (flag == F_RENAME) {
	if (copy(from, to))
	    return 1;
	if (unlink(from)) {
	    error("cannot remove `/%s'", from);
	    return 1;
	}
    }
#ifdef F_LINK
    else if (flag == F_LINK) {
	if (symlink(from, to) == 0)	/* symbolic link */
	    return 0;
    }
#endif
    return 0;
}
#endif

void debug_perror P2(char *, what, char *, file) {
    if (file)
	debug_message("System Error: %s:%s:%s\n", what, file, strerror(errno));
    else
	debug_message("System Error: %s:%s\n", what, strerror(errno));
}

/*
 * do_rename is used by the efun rename. It is basically a combination
 * of the unix system call rename and the unix command mv.
 */

#ifdef F_RENAME
int do_rename P3(char *, fr, char *, t, int, flag)
{
    char *from, *to, tbuf[3];
    char newfrom[MAX_FNAME_SIZE + MAX_PATH_LEN + 2];
    int flen;
    
    /*
     * important that the same write access checks are done for link() as are
     * done for rename().  Otherwise all kinds of security problems would
     * arise (e.g. creating links to files in protected directories and then
     * modifying the protected file by modifying the linked file). The idea
     * is prevent linking to a file unless the person doing the linking has
     * permission to move the file.
     */
    from = check_valid_path(fr, current_object, "rename", 1);
    if (!from)
	return 1;
    to = check_valid_path(t, current_object, "rename", 1);
    if (!to)
	return 1;
    if (!strlen(to) && !strcmp(t, "/")) {
	to = tbuf;
	sprintf(to, "./");
    }

    /* Strip trailing slashes */
    flen = strlen(from);
    if (flen > 1 && from[flen - 1] == '/') {
	char *p = from + flen - 2;
	int n;
	
	while (*p == '/' && (p > from))
	    p--;
	n = p - from + 1;
	memcpy(newfrom, from, n);
	newfrom[n] = 0;
	from = newfrom;
    }

    if (isdir(to)) {
	/* Target is a directory; build full target filename. */
	char *cp;
	char newto[MAX_FNAME_SIZE + MAX_PATH_LEN + 2];

	cp = strrchr(from, '/');
	if (cp)
	    cp++;
	else
	    cp = from;

	sprintf(newto, "%s/%s", to, cp);
	return do_move(from, newto, flag);
    } else
	return do_move(from, to, flag);
}
#endif				/* F_RENAME */

int copy_file P2(char *, from, char *, to)
{
    char buf[128];
    int from_fd, to_fd;
    int num_read, num_written;
    char *write_ptr;

    from = check_valid_path(from, current_object, "move_file", 0);
    to = check_valid_path(to, current_object, "move_file", 1);
    if (from == 0)
	return -1;
    if (to == 0)
	return -2;

    from_fd = open(from, O_RDONLY);
    if (from_fd < 0)
	return (-1);

    if (isdir(to)) {
	/* Target is a directory; build full target filename. */
	char *cp;
	char newto[MAX_FNAME_SIZE + MAX_PATH_LEN + 2];

	cp = strrchr(from, '/');
	if (cp)
	    cp++;
	else
	    cp = from;

	sprintf(newto, "%s/%s", to, cp);
	close(from_fd);
	return copy_file(from, newto);
    }
    to_fd = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (to_fd < 0) {
	close(from_fd);
	return (-2);
    }
    while ((num_read = read(from_fd, buf, 128)) != 0) {
	if (num_read < 0) {
	    debug_perror("copy_file: read", from);
	    close(from_fd);
	    close(to_fd);
	    return (-3);
	}
	write_ptr = buf;
	while (write_ptr != (buf + num_read)) {
	    num_written = write(to_fd, write_ptr, num_read);
	    if (num_written < 0) {
		debug_perror("copy_file: write", to);
		close(from_fd);
		close(to_fd);
		return (-3);
	    }
	    write_ptr += num_written;
	}
    }
    close(from_fd);
    close(to_fd);
    return 1;
}

void dump_file_descriptors P1(outbuffer_t *, out)
{
    int i;
    dev_t dev;
    struct stat stbuf;

    outbuf_add(out, "Fd  Device Number  Inode   Mode    Uid    Gid      Size\n");
    outbuf_add(out, "--  -------------  -----  ------  -----  -----  ----------\n");

    for (i = 0; i < FD_SETSIZE; i++) {
	/* bug in NeXT OS 2.1, st_mode == 0 for sockets */
	if (fstat(i, &stbuf) == -1)
	    continue;

#if !defined(LATTICE) && !defined(WIN32)
	if (S_ISCHR(stbuf.st_mode) || S_ISBLK(stbuf.st_mode))
	    dev = stbuf.st_rdev;
	else
#endif
	    dev = stbuf.st_dev;

	outbuf_addv(out, "%2d", i);
	outbuf_addv(out, "%13x", dev);
	outbuf_addv(out, "%9d", stbuf.st_ino);
	outbuf_add(out, "  ");

	switch (stbuf.st_mode & S_IFMT) {

	case S_IFDIR:
	    outbuf_add(out, "d");
	    break;
	case S_IFCHR:
	    outbuf_add(out, "c");
	    break;
#ifdef S_IFBLK
	case S_IFBLK:
	    outbuf_add(out, "b");
	    break;
#endif
	case S_IFREG:
	    outbuf_add(out, "f");
	    break;
#ifdef S_IFIFO
	case S_IFIFO:
	    outbuf_add(out, "p");
	    break;
#endif
#ifdef S_IFLNK
	case S_IFLNK:
	    outbuf_add(out, "l");
	    break;
#endif
#ifdef S_IFSOCK
	case S_IFSOCK:
	    outbuf_add(out, "s");
	    break;
#endif
	default:
	    outbuf_add(out, "?");
	    break;
	}

	outbuf_addv(out, "%5o", stbuf.st_mode & ~S_IFMT);
	outbuf_addv(out, "%7d", stbuf.st_uid);
	outbuf_addv(out, "%7d", stbuf.st_gid);
	outbuf_addv(out, "%12d", stbuf.st_size);
	outbuf_add(out, "\n");
    }
}
