#ifndef __SD_H__
#define __SD_H__

#include "dirent.h"
#include "ff.h"
/* the structure to store the time of the file */
typedef struct {
    short date;
    short time;
} sdtime_t;

/* the structure to store the attribute of the file */
typedef struct {
    long st_size;
    char st_mode;
    sdtime_t st_mtime;
} vfs_stat_t;

typedef FIL vfs_file_t;
typedef FIL vfs_t;
/**
  * @brief      open the directory in the sdcard
  *
  * @param      the name of directory which in the sdcard
  *
  * @return     the directory structure
  */
DIR * opennextdir(char *SDdirname);
/**
  * @brief      Get File Status 
  *
  * @param      filename: the name of file
  *             st: the structure to store the attribute of the file
  *
  * @return     1: error;
  *             0: OK;
  */
int vfs_stat(const char* filename, vfs_stat_t* st);
/**
  * @brief      open a file to read
  *
  * @param      SDfilename: the name of the file
  *
  * @return     file structure
  */
FILE* openfile_read(char *SDfilename);
/**
  * @brief      open a file to write
  *
  * @param      SDfilename: the name of the file
  *
  * @return     file structure
  */
FILE* openfile_write(char *q);
/**
  * @brief      initilise the SDCard with SD mode
  *
  * @param      null
  *
  * @return     null
  */
void initilalise_SD(void);

vfs_file_t* vfs_open(const char* filename) ;
int vfs_read (char* buffer, int len, FIL* file) ;
void vfs_close(vfs_t* vfs);

#endif