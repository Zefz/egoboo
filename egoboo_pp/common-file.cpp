// common-file.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "egoboo.h"

#ifndef MAX_PATH
#define MAX_PATH 260 // Same value that Windows uses...
#endif

#ifdef WIN32
#define snprintf _snprintf
#endif

// FIXME: Doesn't handle deleting directories recursively yet.
void fs_removeDirectoryAndContents(const char *dirname)
{
  // ZZ> This function deletes all files in a directory,
  //     and the directory itself
  char filePath[MAX_PATH];
  const char *fileName;

  // List all the files in the directory
  fileName = fs_findFirstFile(dirname, NULL);
  while (fileName != NULL)
  {
    // Ignore files that start with a ., like .svn for example.
    if (fileName[0] != '.')
    {
      snprintf(filePath, MAX_PATH, "%s/%s", dirname, fileName);
      if (fs_fileIsDirectory(filePath))
      {
        //fs_removeDirectoryAndContents(filePath);
      }
      else
      {
        fs_deleteFile(filePath);
      }
    }

    fileName = fs_findNextFile();
  }
  fs_findClose();
  fs_removeDirectory(dirname);
}

void fs_copyDirectory(const char *sourceDir, const char *destDir)
{
  // ZZ> This function copies all files in a directory
  char srcPath[MAX_PATH], destPath[MAX_PATH];
  const char *fileName;

  // List all the files in the directory
  fileName = fs_findFirstFile(sourceDir, NULL);
  if (fileName != NULL)
  {
    // Make sure the destination directory exists
    fs_createDirectory(destDir);

    do
    {
      // Ignore files that begin with a .
      if (fileName[0] != '.')
      {
        snprintf(srcPath, MAX_PATH, "%s/%s", sourceDir, fileName);
        snprintf(destPath, MAX_PATH, "%s/%s", destDir, fileName);
        fs_copyFile(srcPath, destPath);
      }

      fileName = fs_findNextFile();
    }
    while (fileName != NULL);
  }
  fs_findClose();
}

