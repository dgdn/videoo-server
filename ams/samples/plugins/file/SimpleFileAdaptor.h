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
 |  Patents, patents in process, and are protected by trade secret or         |
 |  copyright law. Dissemination of this information or reproduction of this  |
 |  material is strictly forbidden unless prior written permission is         |
 |  obtained from Adobe Systems Incorporated.                                 |
 |                                                                            |
 |          Adobe Systems Incorporated       415.832.2000                     |
 |          601 Townsend Street              415.832.2020 fax                 |
 |          San Francisco, CA 94103                                           |
 |                                                                            |
 +----------------------------------------------------------------------------*/
#ifndef SIMPLEFILEADAPTOR_H
#define SIMPLEFILEADAPTOR_H

#include <stdio.h>	
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "FmsFileAdaptor.h"
#include <string>
#include <errno.h>
#include <stdlib.h>

#ifndef S_ISDIR
#ifndef S_IFDIR
#ifndef _S_IFDIR
#define S_IFDIR		(-1)
#else
#define S_IFDIR		_S_IFDIR
#endif
#endif
#define S_ISDIR(m)	(((m)&S_IFDIR)==S_IFDIR)
#endif



//////////////////////////////////////////////////////////////////////////////
/*
This part introduces thread pool implementation used by adaptor. Any calls from 
FMS should be returned back by adaptor as quickly as possible. 
If call requires some processing time the adaptor has to store received params 
and pass them through implemented thread pool for further processing.
This sample provides only Windows thread pool implementation 
and should be revisited on Linux OS.
*/

#ifdef _WIN32
#include <atlutil.h>

class SimpleFileAdaptor;

struct open_param
{
	std::string sFileName;
	int iFlags;
	int iMode;
	int* pAttrKeys;
	int nAttrs;
	void* pCtx;
	SimpleFileAdaptor* pAdaptor;
};

class CTaskBase
{
public:

	virtual void DoTask(void *pvParam, OVERLAPPED *pOverlapped)=0;

};

class CTaskOpen : public CTaskBase
{
public:

	CTaskOpen(open_param& par) : m_Param(par) {}
	virtual void DoTask(void *pvParam, OVERLAPPED *pOverlapped);
	
	open_param m_Param;
};


class CMyWorker
{
public:
    typedef DWORD_PTR RequestType;

	CMyWorker();
	
    virtual BOOL Initialize(void *pvParam);
    
    virtual void Terminate(void* /*pvParam*/);
    
	void Execute(RequestType dw, void *pvParam, OVERLAPPED* pOverlapped) throw();
    
    virtual BOOL GetWorkerData(DWORD /*dwParam*/, void ** /*ppvData*/);
    
protected:	
	
}; // CMyWorker
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
	#include <io.h>
	#include <direct.h>
	
	#pragma warning(disable : 4996)

	typedef struct _stati64 stbuf;
#else
	#include <dirent.h>
	#include <unistd.h>

	typedef struct stat			stbuf;
	#define DWORD				unsigned long	
#endif

class SimpleFileAdaptor : public virtual IFmsFileAdaptor
{
	public:
		SimpleFileAdaptor( IFmsServerContext* pServer, IFmsFileResponse* pResponse ); 

		virtual ~SimpleFileAdaptor();

		virtual int open( const char* sFileName, int iFlags, int iMode, int* pAttrKeys, int nAttrs, void* pCtx );

		virtual int getAttributes( const char* sFileName, int* pAttrKeys, int nAttrs, void* pCtx );
		virtual int getAttributes( int iHandle, int* pAttrKeys, int nAttrs, void* pCtx );

		virtual int read( int iHandle, I64 uOffset, char* pBuffer, int nBytes, void* pCtx );

		virtual int write( int iHandle, I64 uOffset, char* pBuffer, int nBytes, void* pCtx );

		virtual int close( int iHandle, void* pCtx );

		virtual int remove( const char* sFileName,  unsigned long uFlags, void* pCtx );

		virtual int createDir( const char* sFileName, void* pCtx );

		void PrintLog(const char* messsage);

		int openFile( const char* sFileName, int iFlags, int iMode, int* pAttrKeys, int nAttrs, void* pCtx );

	private:

		bool isAbsolutePath( const char* strPath );
		bool isAttributeAllExist(int* pAttrKeys, int nAttrs);

		int removeDirRecursive(std::string strDir);

		void setupFileAttribute(int* pAttrKeys, FmsFileAttribute* pAttribute, int nSize, 
			stbuf* pBuffer);

		bool writeDirContent(const char* file, stbuf* &st, int fd);

		bool createFileDir(const char* sFileName);

		bool makeDir(const std::string &sDir, int mode);

	protected:

		IFmsServerContext* m_pServer;
		IFmsFileResponse*  m_pResponse;

	#ifdef _WIN32
		CThreadPool<CMyWorker> m_pool;
	#endif

};

class SimpleFileAdaptor2 : public SimpleFileAdaptor, public IFmsFileAdaptor2
{
	public:
		SimpleFileAdaptor2( IFmsServerContext* pServer, IFmsFileResponse2* pResponse );

		virtual int rename( const char* sOldFileName, const char* sNewFileName, void* pCtx );

		virtual int truncate( int iHandle, U64 size, void* pCtx );
};

#endif

