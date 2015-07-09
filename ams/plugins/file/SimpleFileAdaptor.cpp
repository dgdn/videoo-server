/*----------------------------------------------------------------------------+
 |       ___     _       _                                                    |
 |      /   |   | |     | |                                                   |
 |     / /| | __| | ___ | |__   ___                                           |
 |    / /_| |/ _  |/ _ \|  _ \ / _ \                                          |
 |   / ___  | (_| | (_) | |_) |  __/                                          |
 |  /_/   |_|\__,_|\___/|____/ \___|                                          |
 |                                                                            |
 |                                                                            |
 |  ADOBE CONFIDENTIAL                                                        |
 |  __________________                                                        |
 |                                                                            |
 |  Copyright (c) 2003 - 2007, Adobe Systems Incorporated.                    |
 |  All rights reserved.                                                      |
 |                                                                            |
 |  NOTICE:  All information contained herein is, and remains the property    |
 |  of Adobe Systems Incorporated and its suppliers, if any. The intellectual |
 |  and technical concepts contained herein are proprietary to Adobe Systems  |
 |  Incorporated and its suppliers and may be covered by U.S. and Foreign     |
 |  Patents, patents in process, and are protected by trade seeateet or         |
 |  copyright law. Dissemination of this information or reproduction of this  |
 |  material is strictly forbidden unless prior written permission is         |
 |  obtained from Adobe Systems Incorporated.                                 |
 |                                                                            |
 |          Adobe Systems Incorporated       415.832.2000                     |
 |          601 Townsend Street              415.832.2020 fax                 |
 |          San Francisco, CA 94103                                           |
 |                                                                            |
 +----------------------------------------------------------------------------*/
#include "StdAfx.h"
#include "SimpleFileAdaptor.h"
#include "FileModule.h"

#ifdef WIN32

inline static int open( const char* file, int flags, int mode )	{ return _open( file, flags, mode ); }
inline static int close( int fd )									{ return _close( fd ); }
inline static int read( int fd, void* buf, unsigned len )			{ return _read( fd, buf, len ); }
inline static int write( int fd, void* buf, unsigned len )			{ return _write( fd, buf, len ); }
inline static int fstat( int fd, stbuf* st )						{ return _fstati64( fd, st ); }
inline static int stat( const char* file, stbuf* st )				{ return _stati64( file, st ); }
inline static int unlink( const char* file )						{ return _unlink(file); }
inline static int rmdir( const char* file )							{ return _rmdir(file); }
inline static int mkdir( const char* file, int mask)				{ return _mkdir(file); }
inline static __int64 lseek(int fd, __int64 offset, int origin)     { return _lseeki64(fd, offset, origin); }

static const char dirSEP = '\\';

static int getTempHandle()
{
	int nHandle = -1;

	FILE* hFile = NULL;
	if (!tmpfile_s(&hFile))
	{
		nHandle = fileno(hFile);
		if (nHandle >= 0) 
			nHandle = ::dup(nHandle);
		if (nHandle >= 0)
			lseek(nHandle, 0L, SEEK_SET);
		fclose(hFile);
	}
	return nHandle;
}

inline static bool isExistsErr() { 	return GetLastError() == ERROR_ALREADY_EXISTS; }

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // Handle to DLL module
    DWORD fdwReason,     // Reason for calling function
    LPVOID lpReserved )  // Reserved
{
	return TRUE;
}

#else

static const char dirSEP = '/';

static int getTempHandle()
{
	int nHandle = -1;
	char sfn[] = "/tmp/fileXXXXXX";
	
	nHandle = mkstemp(sfn);
	unlink(sfn);
	return nHandle;
}

inline static bool isExistsErr() { return  errno == EEXIST; };

#endif

///////////////////////////////////////////////////////////////////

extern "C" void FCExport FmsCreateFileAdaptor(IFmsServerContext* pServerCtx, IFmsFileResponse* pResponse, IFmsFileAdaptor*& pFileAdaptor )
{
	pFileAdaptor = new SimpleFileAdaptor( pServerCtx, pResponse );
}

extern "C" void FCExport FmsDestroyFileAdaptor(IFmsFileAdaptor* pFileAdaptor)
{
	delete pFileAdaptor;
}

extern "C" void FCExport FmsCreateFileAdaptor2(IFmsServerContext* pServerCtx, void* pResponse, void*& pFileAdaptor, U32& iVersion )
{
	pFileAdaptor = new SimpleFileAdaptor( pServerCtx, (IFmsFileResponse*)pResponse );
	iVersion = MKLONG(IFMSFILEADAPTOR_MINOR_VERSION, IFMSFILEADAPTOR_MAJOR_VERSION); 
}

extern "C" void FCExport FmsDestroyFileAdaptor2(void* pFileAdaptor)
{
	delete (IFmsFileAdaptor*)pFileAdaptor;
}

extern "C" void FCExport FmsCreateFileAdaptor3(IFmsServerContext* pServerCtx, void* pResponse, IFmsFileAdaptor2*& pFileAdaptor, U32& iVersion )
{
	pFileAdaptor = new SimpleFileAdaptor2( pServerCtx, (IFmsFileResponse2*)pResponse );
	iVersion = MKLONG(IFMSFILEADAPTOR_MINOR_VERSION, IFMSFILEADAPTOR_MAJOR_VERSION); 
}

extern "C" void FCExport FmsDestroyFileAdaptor3(IFmsFileAdaptor2* pFileAdaptor)
{
	delete pFileAdaptor;
}


///////////////////////////////////////////////////////////////////

SimpleFileAdaptor::SimpleFileAdaptor( IFmsServerContext* pServer, IFmsFileResponse* pResponse ) :
	m_pServer(pServer), m_pResponse(pResponse)
{

#ifdef WIN32
/*
This section introduces a thread pool implementation used by the plug-in. Any calls from
Flash Media Interactive Server should be returned by the plug-in as quickly as possible.
If a call requires processing time, the plug-in must store received parameters
and pass them through the implemented thread pool for further processing.
This sample only provides a Windows thread pool implementation
and should be revisited on Linux.
*/
	static const int THREADPOOL_SIZE = 5;

	HRESULT hr = m_pool.Initialize((void*)this, THREADPOOL_SIZE);
	if( FAILED( hr ) ) 
	{
		PrintLog("Failed to init thread pool!");
	}
#endif
}

SimpleFileAdaptor::~SimpleFileAdaptor()
{
#ifdef WIN32
/*
This section introduces a thread pool implementation used by the plug-in. Any calls from
Flash Media Interactive Server should be returned by the plug-in as quickly as possible.
If a call requires processing time, the plug-in must store received parameters
and pass them through the implemented thread pool for further processing.
This sample only provides a Windows thread pool implementation
and should be revisited on Linux.
*/
	// Shutdown the thread pool.
	m_pool.Shutdown();
#endif
}

void SimpleFileAdaptor::PrintLog(const char* messsage)
{
	// A message is printed in the server log; if true it is also printed in the event log.
	m_pServer->log(messsage, IFmsServerContext::kInformation, false);
}

int SimpleFileAdaptor::close( int iHandle, void* pCtx )
{		
	if (iHandle < 0)
		return -1;

	U32 uStatus = !::close(iHandle) ?  0 : IFmsFileResponse::kSystemErrors ;

	if (pCtx)
		m_pResponse->onClose(uStatus, pCtx );

	return 0;
}

int SimpleFileAdaptor::remove( const char* sFileName,  unsigned long uFlags, void* pCtx )
{
	if (!sFileName || !strlen(sFileName))
	{
		return -1;
	}

	int nVal = 0;

	// FMS currently only passes 3 flag combos:
	// kFile means delete a file
	// kDirs means delete an empty directory
	// kDirs|kSubDirs means recursively delete a directory
	if (uFlags == kFile)
	{
		nVal = unlink(sFileName) ;
	}
	else if (uFlags == kDirs)
	{
		nVal = ::rmdir(sFileName);
	}
	else if (uFlags == (kDirs | kSubDirs))
	{
		nVal = removeDirRecursive(sFileName);
	}
	else
	{
		nVal = -1;
	}
	U32 uStatus = !nVal  ? 0 : IFmsFileResponse::kSystemErrors ;
	if (pCtx)
		m_pResponse->onRemove(uStatus, pCtx);
	return 0;
}

/*
This is just a basic sample how to use mkdir to create all directories in specified path 
beginning with root. The problem is that mkdir creates only one level of subdirectory. 
In real project you should revisit this implemetation for instance by using extended 
OS API to do that. For instance, on Windows you could use SHCreateDirectoryEx or 
MakeSureDirectoryPathExists which both implement recursive directory creation.
*/
bool SimpleFileAdaptor::createFileDir(const char* sFileName)
{
	if (!sFileName)
	{
		return false;
	}

	std::string sDir(sFileName);
	int mode = 0;

#ifndef WIN32
	mode = S_IRWXU|S_IRWXG|S_IRWXO;
#endif
	

	bool bskipFirstSep = isAbsolutePath(sDir.c_str());
	
	for ( size_t i = 0; i < sDir.length(); ++i )
	{
		if ( sDir.at(i) == dirSEP )
		{		
			if (bskipFirstSep)
			{
				//first root dirSEP skipped
				bskipFirstSep = false;				
			}
			else
			{
				if (!makeDir(sDir.substr(0, i), mode))
				{			
					return false;
				}
			}
		}
	}	
	return makeDir( sDir, mode );	
}

int SimpleFileAdaptor::createDir( const char* sFileName, void* pCtx )
{	
	if (!sFileName || !strlen(sFileName))
		return -1;
	
	U32 uStatus = createFileDir(sFileName) ? 0 : IFmsFileResponse::kSystemErrors;
	m_pResponse->onCreateDir(uStatus, pCtx);
	
	return 0;
}

int SimpleFileAdaptor::getAttributes( const char* sFileName, int* pAttrKeys, int nAttrs, void* pCtx )
{
	if (!sFileName || !strlen(sFileName))
	{
		return -1;
	}

	U32 uStatus = 0;
	int nSize = nAttrs;
	FmsFileAttribute* pAttribute = NULL;

	stbuf buffer64;

	if (!stat(sFileName, &buffer64))
	{
		if (isAttributeAllExist(pAttrKeys, nAttrs))
		{
			nSize = FmsFileAttribute::kMaxAttr;
			pAttribute = new FmsFileAttribute[nSize];
		}
		else
			pAttribute = new FmsFileAttribute[nAttrs];
		setupFileAttribute(pAttrKeys, pAttribute, nSize, &buffer64);
		m_pResponse->onGetAttributes(uStatus,pAttribute,nSize,pCtx);

		if (pAttribute)
		{
			delete [] pAttribute;
			pAttribute = NULL;
		}
	}
	else
	{
		m_pResponse->onGetAttributes(IFmsFileResponse::kInvalidHandle,pAttribute,nSize,pCtx);
	}
	return 0;
}

int SimpleFileAdaptor::getAttributes( int iHandle, int* pAttrKeys, int nAttrs, void* pCtx )
{
	if (iHandle < 0)
		return -1;

	U32 uStatus = 0;
	int nSize = nAttrs;
	FmsFileAttribute* pAttribute = NULL;

	stbuf buffer64;
	fstat(iHandle, &buffer64);
	if (isAttributeAllExist(pAttrKeys, nAttrs))
	{
		nSize = FmsFileAttribute::kMaxAttr;
		pAttribute = new FmsFileAttribute[nSize];
	}
	else
		pAttribute = new FmsFileAttribute[nAttrs];
	setupFileAttribute(pAttrKeys, pAttribute, nSize, &buffer64);
	m_pResponse->onGetAttributes(uStatus,pAttribute,nSize,pCtx);
	if (pAttribute)
	{
		delete [] pAttribute;
		pAttribute = NULL;
	}
	return 0;
}

int SimpleFileAdaptor::open( const char* sFileName, int iFlags, int iMode, int* pAttrKeys, int nAttrs, void* pCtx )
{
	if (!sFileName || !strlen(sFileName))
	{
		return -1;
	}

#ifdef WIN32
/*
This section introduces a thread pool implementation used by the plug-in. Any calls from
Flash Media Interactive Server should be returned by the plug-in as quickly as possible.
If a call requires processing time, the plug-in must store received parameters
and pass them through the implemented thread pool for further processing.
This sample only provides a Windows thread pool implementation
and should be revisited on Linux.
*/

	struct open_param par;
	par.sFileName = sFileName;
	par.iFlags = iFlags;
	par.iMode = iMode;
	par.pAttrKeys = pAttrKeys;
	par.nAttrs = nAttrs;
	par.pCtx = pCtx;
	par.pAdaptor = this;
	CTaskBase* pTask = new CTaskOpen(par);
	if (!m_pool.QueueRequest((CMyWorker::RequestType) pTask ))
		return -1;
	return 0;
#else
	return openFile(sFileName, iFlags, iMode, pAttrKeys, nAttrs, pCtx);
#endif
}

int SimpleFileAdaptor::openFile( const char* sFileName, int iFlags, int iMode, int* pAttrKeys, int nAttrs, void* pCtx )
{	
	stbuf buffer64;

	//remove slash if directory
	std::string strFileName(sFileName);
		
	bool bCreate = iFlags & O_CREAT ? true : false;
	bool bTrunc = iFlags & O_TRUNC ? true : false;
			
	U32 uStatus = 0;
	int nSize = nAttrs;
	int nHandle = -1;
	FmsFileAttribute* pAttribute = NULL;
	int mode = 0;

#ifndef WIN32
	mode = S_IRWXU|S_IRWXG|S_IRWXO;
	if (iMode & S_IWRITE)
	{
		iMode = S_IREAD|S_IWRITE|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	}
	else if (iMode & S_IREAD)
	{
		iMode = S_IREAD|S_IRGRP|S_IROTH;
	}
#endif
	
	nHandle = bCreate ? ::open(sFileName, iFlags, iMode) : ::open(sFileName, iFlags);
	if (nHandle < 0)
	{		
		if (errno == ENOENT)
		{		
			bool bDirectory = false;
			if (strFileName.at(strFileName.length() - 1) == dirSEP)
			{
				//striping off dirSEP at the end if there
				strFileName = strFileName.substr(0, strFileName.length() - 1);
				bDirectory = true;
			}
			
			if ( bCreate || (!bCreate && !bDirectory))				
			{	
				/// create file should be succeded in open otherwise that is en error.
				/// create directory should not be called. Create directory called first.
				/// not create (open) file - file is not exist and could not be opened.  
				std::string mes(strFileName + " does not exist");
				PrintLog(mes.c_str());
				m_pResponse->onOpen(IFmsFileResponse::kSystemErrors,-1,NULL,nAttrs,pCtx);
			
				return 0;
			}
			//not create (open) directory proceeded striping off dirSEP
		}
		if (!stat(strFileName.c_str(), &buffer64) && S_ISDIR(buffer64.st_mode))
		{			
			//this is a directory
			std::string mes("open a directory " + strFileName);
			PrintLog(mes.c_str());
			//open temp file and write dir content
			if ((nHandle = ::getTempHandle()) >= 0 )
			{					
				stbuf* pbuff = &buffer64;
				if (!writeDirContent(sFileName, pbuff, nHandle))
				{
					::close(nHandle);
					nHandle = -1;			
				}					
			}		
		}			
	}
	
	if (nHandle < 0)
	{
		std::string mes("failed to open file or directory " + strFileName);
		PrintLog(mes.c_str());
		uStatus = IFmsFileResponse::kSystemErrors;
	}

	if (!pAttrKeys || !nAttrs || uStatus)
	{
		m_pResponse->onOpen(uStatus,nHandle,NULL,nAttrs,pCtx);
		
		return 0;
	}
	
	if (isAttributeAllExist(pAttrKeys, nAttrs))
	{
		nSize = FmsFileAttribute::kMaxAttr;
	}
	pAttribute = new FmsFileAttribute[nSize];

	setupFileAttribute(pAttrKeys, pAttribute, nSize, &buffer64);
	m_pResponse->onOpen(uStatus,nHandle,pAttribute,nSize,pCtx);
	if (pAttribute)
	{
		delete [] pAttribute;
		pAttribute = NULL;
	}
	
	return 0;
}

int SimpleFileAdaptor::read( int iHandle, I64 uOffset, char* pBuffer, int nBytes, void* pCtx )
{
	if ((iHandle < 0) || !pBuffer || (nBytes < 0))
	{
		return -1;
	}

	if (uOffset != -1) //no seek
	{
		if (lseek(iHandle, uOffset, SEEK_SET) == -1)
		{
			m_pResponse->onRead(IFmsFileResponse::kSystemErrors, pBuffer, 0, pCtx);
			return 0;
		}
	}

	int nCount = ::read(iHandle, pBuffer, nBytes);
	if ( nCount == -1 )
	{
		m_pResponse->onRead(IFmsFileResponse::kSystemErrors, pBuffer, 0, pCtx);
	}
	else
	{
		m_pResponse->onRead(0,pBuffer,nCount,pCtx);
	}
	return 0;
}

int SimpleFileAdaptor::write( int iHandle, I64 uOffset, char* pBuffer, int nBytes, void* pCtx )
{
	if ((iHandle < 0) || !pBuffer || (nBytes < 0))
	{
		return -1;
	}

	if (uOffset != -1) //no seek
	{
		if (lseek(iHandle, uOffset, SEEK_SET) == -1)
		{
			m_pResponse->onWrite(IFmsFileResponse::kSystemErrors, 0, pCtx);
			return 0;
		}
	}

	int nCount = ::write(iHandle, pBuffer, nBytes);
	if (nCount == -1)
	{
		m_pResponse->onWrite(IFmsFileResponse::kSystemErrors, 0, pCtx);
	}
	else
	{
		m_pResponse->onWrite(0,nCount,pCtx);
	}
	return 0;
}

bool SimpleFileAdaptor::writeDirContent(const char* file, stbuf* &st, int fd)
{	
	///walking directory	
	std::string 	strFileName(file);
	if (strFileName.at(strFileName.length() - 1) == dirSEP 
#ifdef WIN32		
		|| strFileName.at(strFileName.length() - 1) == '/'
#endif
	)
	{
		strFileName = strFileName.substr(0, strFileName.length() - 1);
	}
	if (!stat(strFileName.c_str(), st) && S_ISDIR(st->st_mode))
	{					
#ifdef WIN32
		strFileName += "\\*";
				
		WIN32_FIND_DATA wfd;
		HANDLE h = FindFirstFile(strFileName.c_str(), &wfd);
		
		if (h != INVALID_HANDLE_VALUE)	
		{			
			do 
			{				
				if (strcmp(wfd.cFileName, ".") != 0 && strcmp(wfd.cFileName, "..") != 0)
				{					
					::write(fd, wfd.cFileName, strlen(wfd.cFileName));
					::write(fd, "\n", 1);					
				}
			}	 
			while (FindNextFile(h, &wfd));
			FindClose(h);
			return true;
		}
#else
		DIR					*dirStruct;      
		struct dirent		*direntp;        
		if ((dirStruct = opendir(file)) != NULL)
		{				
			while ((direntp = readdir(dirStruct)) != NULL)
			{				
				if (direntp->d_name && strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0)
				{				
					::write(fd, direntp->d_name, strlen(direntp->d_name));
					::write(fd, "\n", 1);					
				}
			}			         
			closedir(dirStruct);
			return true;
		}
#endif
	}	
	return false;
}

bool SimpleFileAdaptor::makeDir(const std::string &sDir, int mode) 
{ 	
	return !::mkdir( sDir.c_str(), mode ) ? true : ::isExistsErr();
}

int SimpleFileAdaptor::removeDirRecursive(std::string strDir)
{	
	if (!strDir.length())
		return -1;
	
	// Remove any trailing slash
	if (strDir.at(strDir.length() - 1) == dirSEP 
#ifdef WIN32		
		|| strDir.at(strDir.length() - 1) == '/'
#endif
	)
	{
		strDir = strDir.substr(0, strDir.length() - 1);
	}

#ifdef WIN32
	std::string strSearch = strDir + dirSEP + "*";

	WIN32_FIND_DATA wfd;

	HANDLE h = FindFirstFile(strSearch.c_str(), &wfd);
	if (h != INVALID_HANDLE_VALUE)
	{
		// check every file/directory
		do
		{
			std::string strEntry = strDir + dirSEP + wfd.cFileName;

			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// this is a directory, so recurse
				if (strcmp(wfd.cFileName, ".") && strcmp(wfd.cFileName, ".."))
				{
					removeDirRecursive(strEntry);	
				}
			}
			else
			{
				unlink(strEntry.c_str());
			}

		} while (FindNextFile(h, &wfd));

		FindClose(h);
	}
#else
	DIR					*dirStruct;      
	struct dirent		*direntp;        
	if ((dirStruct = opendir(strDir.c_str())) != NULL)
	{				
		while ((direntp = readdir(dirStruct)) != NULL)
		{	
			if (!direntp->d_name)
			{
				continue;
			}

			std::string strEntry = strDir + dirSEP + direntp->d_name;

			stbuf buf;
			if (!stat(strEntry.c_str(), &buf) && S_ISDIR(buf.st_mode))
			{
				// This is a directory, so recurse
				if (strcmp(direntp->d_name, ".") && strcmp(direntp->d_name, ".."))
				{				
					removeDirRecursive(strEntry);				
				}
			}
			else
			{
				unlink(strEntry.c_str());
			}
		}			         
		closedir(dirStruct);
	}
#endif

	return ::rmdir( strDir.c_str() );
}

bool SimpleFileAdaptor::isAbsolutePath( const char* strPath )
{
	int len;
	if (!strPath || !(len = strlen(strPath)))
		return false;

#if defined(WIN32)
	if (strstr(strPath, "\\\\") != 0) 
	{
		return true; 
	}
	if (len > 1 && strstr(strPath, ":\\") != 0)
	{
		return true;
	}
#else
	if (*strPath == '/')
	{
		return true;
	}
#endif
	return false;
}

bool SimpleFileAdaptor::isAttributeAllExist(int* pAttrKeys, int nAttrs)
{	
	//check for kAll
	for (int i = 0; i < nAttrs; i++)
	{
		if (pAttrKeys[i] == FmsFileAttribute::kAll)
		{
			return true;
		}
	}
	return false;
}

void SimpleFileAdaptor::setupFileAttribute(int* pAttrKeys, FmsFileAttribute* pAttribute, 
	int nSize, stbuf* pBuffer)
{
	if (!pAttrKeys || !pAttribute | !pBuffer | !nSize)
		return;
	
	int nLen = nSize;
	int i;

	//check kAll
	if (isAttributeAllExist(pAttrKeys, nSize))
	{
		for (i = 0; i < nLen; i++)
		{
			switch(i)
			{
				case FmsFileAttribute::kAll:
					pAttribute[i].attr = FmsFileAttribute::kAll;
					pAttribute[i].value.u32 = 0;
				break;
				case FmsFileAttribute::kTimeCreated:
					pAttribute[i].attr = FmsFileAttribute::kTimeCreated;
					pAttribute[i].value.u32 = (U32)pBuffer->st_ctime;
				break;
				case FmsFileAttribute::kTimeModified:
					pAttribute[i].attr = FmsFileAttribute::kTimeModified;
					pAttribute[i].value.u32 = (U32)pBuffer->st_mtime;
				break;
				case FmsFileAttribute::kTimeAccessed:
					pAttribute[i].attr = FmsFileAttribute::kTimeAccessed;
					pAttribute[i].value.u32 = (U32)pBuffer->st_atime;
				break;
				case FmsFileAttribute::kSize:
					pAttribute[i].attr = FmsFileAttribute::kSize;
					pAttribute[i].value.i64 = pBuffer->st_size;
				break;
				case FmsFileAttribute::kMode:
					pAttribute[i].attr = FmsFileAttribute::kMode;
					if (S_ISDIR(pBuffer->st_mode))
						pAttribute[i].value.u8 = 1;
					else
						pAttribute[i].value.u8 = 0;
				break;
			}
		}
		return;
	}

	i=0;
	while (nLen > 0)
	{
		switch(pAttrKeys[i])
		{
		case FmsFileAttribute::kTimeCreated:
			pAttribute[i].attr = FmsFileAttribute::kTimeCreated;
			pAttribute[i].value.u32 = (U32)pBuffer->st_ctime;
			break;

		case FmsFileAttribute::kTimeModified:
			pAttribute[i].attr = FmsFileAttribute::kTimeModified;
			pAttribute[i].value.u32 = (U32)pBuffer->st_mtime;
			break;

		case FmsFileAttribute::kTimeAccessed:
			pAttribute[i].attr = FmsFileAttribute::kTimeAccessed;
			pAttribute[i].value.u32 = (U32)pBuffer->st_atime;
			break;

		case FmsFileAttribute::kSize:
			pAttribute[i].attr = FmsFileAttribute::kSize;
			pAttribute[i].value.i64 = pBuffer->st_size;
			break;
		
		case FmsFileAttribute::kMode:
			pAttribute[i].attr = FmsFileAttribute::kMode;
			if (S_ISDIR(pBuffer->st_mode))
				pAttribute[i].value.u8 = 1;
			else
				pAttribute[i].value.u8 = 0;
			break;
		}
		nLen--;
		i++;
	}
}

//////////////////////////////////////////////////////////////////////////////

SimpleFileAdaptor2::SimpleFileAdaptor2( IFmsServerContext* pServer, IFmsFileResponse2* pResponse ) :
					SimpleFileAdaptor( pServer, pResponse )
{
}

int SimpleFileAdaptor2::rename( const char* sOldFileName, const char* sNewFileName, void* pCtx )
{
	if (!sOldFileName || !sOldFileName)
		return -1;

	int nVal = 0;
#ifdef _WIN32
	// WIN32 rename API doesn't overwrite existing file except itself.  We need to use MoveFileEx instead.
	nVal = MoveFileEx(sOldFileName, sNewFileName,
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)? 0 : -1;
#else
	nVal = ::rename(sOldFileName, sNewFileName);
#endif
	U32 uStatus = !nVal? 0 : IFmsFileResponse2::kSystemErrors;
	if (pCtx)
	{
		// We can safely downcast m_pResponse to IFmsFileResponse2*; 
		// FMS server would always pass IFmsFileResponse2* object in the constructor of SimpleFileAdaptor2 class
		IFmsFileResponse2* pResponse = (IFmsFileResponse2*)m_pResponse;
		pResponse->onRename(uStatus, pCtx);
		return 0;
	}
	return nVal;
}

int SimpleFileAdaptor2::truncate( int iHandle, U64 size, void* pCtx )
{
	if(iHandle < 0)
		return -1;

	int nVal = 0;
#ifdef _WIN32
	nVal = _chsize_s(iHandle, size);
#else
	nVal = ::ftruncate(iHandle, size);
#endif
	U32 uStatus = !nVal? 0 : IFmsFileResponse2::kSystemErrors;
	if(pCtx)
	{
		IFmsFileResponse2* pResponse = (IFmsFileResponse2*)m_pResponse;
		pResponse->onTruncate(uStatus, pCtx);
		return 0;
	}
	return nVal;
}

//////////////////////////////////////////////////////////////////////////////
/*
This section introduces a thread pool implementation used by the plug-in. Any calls from
Flash Media Interactive Server should be returned by the plug-in as quickly as possible.
If a call requires processing time, the plug-in must store received parameters
and pass them through the implemented thread pool for further processing.
This sample only provides a Windows thread pool implementation
and should be revisited on Linux.
*/

#ifdef WIN32

CMyWorker::CMyWorker()
{
}

BOOL CMyWorker::Initialize(void *pvParam)
{
	char buf[1024];
	SimpleFileAdaptor* pAdaptor = (SimpleFileAdaptor*)pvParam;
	sprintf(buf, "[%d]: CMyWorker.Initialize(%d)", (DWORD_PTR)::GetCurrentThreadId(), (DWORD_PTR)pvParam );
	pAdaptor->PrintLog(buf);
	return TRUE;
}

void CMyWorker::Terminate(void* pvParam)
{
	char buf[1024];
	SimpleFileAdaptor* pAdaptor = (SimpleFileAdaptor*)pvParam;
	sprintf(buf,  "CMyWorker.Terminate");
	pAdaptor->PrintLog(buf);
}

void CMyWorker::Execute(RequestType dw, void *pvParam, OVERLAPPED* pOverlapped) throw()
{
	ATLASSERT(pvParam != NULL);

	char buf[1024];
	SimpleFileAdaptor* pAdaptor = (SimpleFileAdaptor*)pvParam;

	sprintf(buf, "[%d] CMyWorker::Execute(dw=%d, pvParam=%d, pOverlapped=%d",
	::GetCurrentThreadId(), dw, (DWORD_PTR)pvParam, (DWORD_PTR)pOverlapped);
	//pAdaptor->PrintLog(buf);

	CTaskBase* pTask = (CTaskBase*)(DWORD_PTR)dw;
	pTask->DoTask(pvParam, pOverlapped);
	delete pTask;
}

BOOL CMyWorker::GetWorkerData(DWORD /*dwParam*/, void ** /*ppvData*/)
{
    return FALSE;
}

void CTaskOpen::DoTask(void *pvParam, OVERLAPPED *pOverlapped)
{
	char buf[1024];
	sprintf(buf, "[%d]: CTaskOpen::DoTask(pvParam=%d, pOverlapped=%d",
			::GetCurrentThreadId(), (DWORD_PTR)pvParam, (DWORD_PTR)pOverlapped);
	//m_Param.pAdaptor->PrintLog(buf);
	m_Param.pAdaptor->openFile( m_Param.sFileName.c_str(), m_Param.iFlags, m_Param.iMode, m_Param.pAttrKeys, m_Param.nAttrs, m_Param.pCtx );
}
#endif
