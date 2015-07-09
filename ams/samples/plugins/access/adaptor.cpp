#include "adaptor.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

#if defined _MSC_VER
#pragma warning (disable:4996) // disable deprecated warning  
#endif

/*---------------------------------------------------------------------------
	Implement the factory
---------------------------------------------------------------------------*/

int FCExport IFCCreateAccessAdaptor( IFCAccessContext* pCtx, IFCAccessAdaptor*& pAdaptor )
{
	pAdaptor = new SampleAdaptor(pCtx);

	return pAdaptor ? 1 : 0;
}

SampleAdaptor::SampleAdaptor(IFCAccessContext* pCtx)  
{
	m_pCtx = pCtx;
}


SampleAdaptor::~SampleAdaptor()  
{
}

const char* SampleAdaptor::getDescription() const
{
	return "Sample Connection Adaptor";
}

void SampleAdaptor::getVersion( int& iMajor, int& iMinor, int& iMicro ) const
{
	iMajor = 1;
	iMinor = 1;
	iMicro = 0;
}

inline const char* safestr( const char* str )
{
	return str ? str : "";
}

void SampleAdaptor::onAccess( IFCAccess* pAccess )
{
	//open log file
	FILE* logFile = fopen("SampleAdaptor.log", "a");
	if (!logFile)
	{
		logFile = stderr;
	}
	
	switch( pAccess->getType() )
	{
		case IFCAccess::CONNECT:
		{
			const char* uri = pAccess->getValue("s-uri");
			if (uri && !strcmp(uri, "rtmp://localhost/streamtest"))
			{
				//redirect connection to new uri
				pAccess->redirect("rtmp://localhost:1935/streamtest", "please specify port");
				fprintf(logFile, "SampleAdaptor [Connect] redirected\n");
			
				if (logFile != stderr)
				{
					fclose(logFile);
				}

				return;
			}

			fprintf(logFile, "SampleAdaptor [Connect] referrer: %s\n", safestr(pAccess->getValue("c-referrer")) );
			fprintf(logFile, "SampleAdaptor [Connect] uri: %s\n", safestr(pAccess->getValue("s-uri")) );
			fprintf(logFile, "SampleAdaptor [Connect] user agent: %s\n", safestr(pAccess->getValue("c-user-agent")) );
			fprintf(logFile, "SampleAdaptor [Connect] client ip: %s\n", safestr(pAccess->getValue("c-ip")) );
			fprintf(logFile, "SampleAdaptor [Connect] pageurl: %s\n", safestr(pAccess->getValue("x-page-url")) );
			
			char strValue[STRING_VALUE_BUFFER_LEN];
			memset((void *)strValue, 0, STRING_VALUE_BUFFER_LEN);

			int iLength = STRING_VALUE_BUFFER_LEN;
			m_pCtx->getStats(IFCAccessContext::eTOTAL_CONNECTED, strValue, iLength);
			fprintf(logFile,   "SampleAdaptor [Connect] getStats, connected %s\n", safestr(strValue) );
			m_pCtx->getStats(IFCAccessContext::eBYTES_IN, strValue, iLength);
			fprintf(logFile,   "SampleAdaptor [Connect] getStats, bytes-in %s\n", safestr(strValue) );
			m_pCtx->getStats(IFCAccessContext::eBYTES_OUT, strValue, iLength);
			fprintf(logFile, "SampleAdaptor [Connect] getStats, bytes-out %s\n", safestr(strValue) );

			// Reset the uri string
			pAccess->setValue("s-uri", "rtmp://localhost/emptytest/boo"); 
			fprintf(logFile, "Adjusted [Connect] uri: %s\n", safestr(pAccess->getValue("s-uri")) );

			// Set the page url string
			pAccess->setValue("x-page-url", "http://localhost/empty"); 
			fprintf(logFile, "Adjusted [Connect] pageurl: %s\n", safestr(pAccess->getValue("x-page-url")) );
			
			// set read and write access fields
			pAccess->setValue("readAccess", "cric");
			pAccess->setValue("writeAccess", "bar");
			pAccess->setValue("readAccessLock", "false");
			pAccess->setValue("writeAccessLock", "true");

			// set audio and video sample access
			pAccess->setValue("audioSampleAccess", "foo");
			pAccess->setValue("videoSampleAccess", "baz");
			pAccess->setValue("audioSampleAccessLock", "false");
			pAccess->setValue("videoSampleAccessLock", "true");

			pAccess->accept();
			break;

		}

		default:
			// We really shouldn't get here!
			fprintf(logFile, "SampleAdaptor: Unknown access event!\n" );
	}
	
	//pAccess->reject("why not");

	if (logFile != stderr)
	{
		fclose(logFile);
	}
}

