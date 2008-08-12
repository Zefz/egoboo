#define _WIN32_WINNT 0x0700

#include <iostream>
#include <fstream>

using namespace std;

#include <windows.h>
#include <stdio.h>

ofstream outfile;


#define NOT_A_FILE (FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_DIRECTORY)

DWORD SplitPathName(
                    LPCTSTR lpFileName,
                    DWORD nBufferLength,
                    LPTSTR lpBuffer,
                    LPTSTR* lpFilePart
                    )
{
  DWORD retval;
  DWORD dwAttrs;
  LPSTR pszTmp;

  if(NULL==lpFileName) return 0;

  dwAttrs  = GetFileAttributes(lpFileName);
  if( dwAttrs & (FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_TEMPORARY) )
  {
    retval = 0;
  }
  else if(dwAttrs & NOT_A_FILE)
  {
    strncpy(lpBuffer, lpFileName, nBufferLength);
    retval = strlen(lpBuffer);
  }
  else
  {
    //get the file name
    GetFullPathName( lpFileName, nBufferLength, lpBuffer, &pszTmp );

    *lpFilePart = pszTmp;

    //clip the buffer to take out the "\\" before the file name
    pszTmp = strstr(lpBuffer, pszTmp);
    pszTmp--;
    *pszTmp = 0x00;

    retval = strlen(lpBuffer);
  }

  return retval;
};

unsigned __int16 icrc1(unsigned __int16 crc, unsigned __int8 one_ch)
{
  int i;
  unsigned __int16 ans = (crc ^ one_ch << 8);

  for(i=0;i<8;i++)
  {
    if(ans & 0x8000)
      ans = (ans <<= 1) ^ 0x1021;
    else
      ans <<=1;
  };

  return ans;
};

#define IM1 0x7FFFFFABL
#define IQ1 0x0000D1A4L
#define IR1 0x00002FB3L
#define IA1 0x00009C4EL

#define IM2 0x7FFFFF07L
#define IQ2 0x00009EF4L
#define IR2 0x00000ECFL
#define IA2 0x00009EF4L

#define IMM1 (IM1-1)

unsigned __int32 icrc1(__int32 crc, unsigned __int32 one_ch)
{
  static __int32 idum1;
  __int32 idum2=one_ch, k;
  unsigned __int32 result;

  if(crc<0) idum1 = crc;

  k = idum1 / IQ1;
  idum1 = IA1 * (idum1-k*IQ1)-k*IR1;
  if(idum1<0) idum1 += IM1;

  k = idum2 / IQ1;
  idum2 = IA2 * (crc-k*IQ2)-k*IR2;
  if(idum2<0) idum2 += IM2;

  result = idum1 - idum2;
  if(result<0) result += IMM1;

  return result;
};

unsigned __int16 Calculate_CRC_16(LPCTSTR szFilePath)
{
  typedef unsigned __int8 bin_type;

  unsigned int   cnt = 0;
  unsigned __int16 ans = 0;
  bin_type one_ch;
  ifstream bstream;

  bstream.open(szFilePath, ios_base::in | ios_base::binary);
  if(!bstream.good()) return 0;

  while( !bstream.eof() )
  {
    bstream >> one_ch;
    ans = icrc1(ans, one_ch);
    cnt++;
  };

  bstream.close();

  return ans;
};

unsigned __int32 Calculate_CRC_32(LPCTSTR szFilePath)
{
  typedef unsigned __int32 bin_type;

  int   cnt = 0;
  __int32 ans = -1234;
  __int32 one_ch;
  basic_ifstream<__int32> bstream;

  bstream.open(szFilePath, ios_base::in | ios_base::binary);
  if(!bstream.good()) return 0;

  while( !bstream.eof() )
  {
    bstream.get(one_ch);
    ans = icrc1(ans, one_ch);
    cnt++;
  };

  bstream.close();

  return ans;
};

int Scan_Directory(LPCTSTR szHomeDir)
{
  WIN32_FIND_DATA FileData; 
  LPSTR pszTmp = NULL;
  LPSTR pszExtension = NULL;
  char szNewPath[MAX_PATH] = {0x00}; 
  char szNewFile[MAX_PATH] = {0x00};

  ofstream inheritfile;

  HANDLE hSearch; 
  bool finished = false;

  lstrcpy(szNewPath, szHomeDir); 
  lstrcat(szNewPath, "\\*.*");

  hSearch = FindFirstFile(szNewPath, &FileData); 
  if (hSearch == INVALID_HANDLE_VALUE) 
  {
    MessageBox(NULL, "No files found.", "", MB_OK); 
    return (-1);
  } 

  // Copy each .TXT file to the new directory 
  // and change it to read only, if not already. 

  lstrcpy(szNewPath, szHomeDir);
  lstrcat(szNewPath, "\\inherit.txt");
  inheritfile.open(szNewPath, ios::out );

  do
  {
    // DO SOMETHING FOR EACH FILE IN THIS DIR

    lstrcpy(szNewFile, szHomeDir);
    lstrcat(szNewFile, "\\");
    lstrcat(szNewFile, FileData.cFileName);

    //if( 0 != strncmp(FileData.cFileName, ".", MAX_PATH*sizeof(char) ) && 
    //    0 != strncmp(FileData.cFileName, "..", MAX_PATH*sizeof(char) ) )
    //{
    //  if( (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    //  {
    //    Scan_Directory(szNewFile);
    //  }
    //  else
    //  {
    //    if(FileData.cFileName[0]=='_')
    //    {
    //      outfile << "ren " << szNewFile << " " << &FileData.cFileName[1] << endl;
    //      cout << "ren " << szNewFile << " " << &FileData.cFileName[1] << endl;
    //    };
    //  };
    //};



    if( 0 != strncmp(FileData.cFileName, ".", MAX_PATH*sizeof(char) ) && 
        0 != strncmp(FileData.cFileName, "..", MAX_PATH*sizeof(char) ) )
    {
      if( (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        Scan_Directory(szNewFile);
      }
      else
      {
        pszExtension = strstr(FileData.cFileName,".");

        unsigned __int32 crc = Calculate_CRC_32(szNewFile);
        if(NULL==pszExtension)
        {
          inheritfile << ": " << FileData.cFileName << " - " << crc << endl;
          outfile << "\t" << crc << "\t" << FileData.cFileName << "\t" << szHomeDir << endl;
          cout << "\t" << crc << "\t" << FileData.cFileName << "\t" << szHomeDir << endl;
        }
        else
        {
          inheritfile << ": " << FileData.cFileName << " - " << crc << " - " << pszExtension << endl;
          outfile << pszExtension << "\t" << crc << "\t" << FileData.cFileName << "\t" << szHomeDir << endl;
          cout << pszExtension << "\t" << crc << "\t" << FileData.cFileName << "\t" << szHomeDir << endl;
        };
      };
    };

    //FIND NEXT FILE
    if (!FindNextFile(hSearch, &FileData)) 
    {
      if (GetLastError() == ERROR_NO_MORE_FILES) 
      { 
        cout << "No more files." << endl;
        finished = true; 
      } 
      else 
      { 
        MessageBox(NULL, "Couldn't find next file.", "", MB_OK); 
        return -1;
      } 
    }
  }
  while(!finished);

  inheritfile.close();

  // Close the search handle. 
  FindClose(hSearch);

  return 0;
}



int main( int argc , __int8 *argv[ ], __int8 *envp[ ] )
{
  int retval = 0;
  LPSTR pszTmp = NULL;
  char szHomeDir[MAX_PATH] = {0x00};

  int cnt = 0;

  bool finished = false;

  outfile.open("C:\\Projects\\Ego\\CRC.txt", ios::out | ios::binary);
  cout << ios::hex;

  if(argc<=1)
  {
    //just the execution string
    SplitPathName(argv[0], MAX_PATH, szHomeDir, &pszTmp);
    cnt++;
    finished = true;
  }
  else
  {
    //scan through all the the parameters to find paths
    cnt = 1;
    while( 0 == SplitPathName(argv[cnt], MAX_PATH, szHomeDir, &pszTmp) && cnt < argc)
    {
      cnt++;
    }

    if(strlen(szHomeDir) == 0)
    {
      MessageBox(NULL, "Could not find any valid directories", "", MB_OK); 
      exit(-1);
    };
  };

  do
  {
    retval = Scan_Directory(szHomeDir);

    //FIND NEXT DIR
    if(cnt>=argc) break;

    cnt++;
    while( 0 == SplitPathName(argv[cnt], MAX_PATH, szHomeDir, &pszTmp) )
    {
      cnt++;
      if(cnt>=argc) return retval;
    }

    if(strlen(szHomeDir) == 0)
    {
      MessageBox(NULL, "Invalid directory", "", MB_OK); 
      return -1;
    };

  } while(!finished);

  outfile.close();

  return retval;
};

