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
 |  Copyright (c) 2003 - 2010, Adobe Systems Incorporated.                    |
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


#include "StdAfx.h"
#include "FmsAuthAdaptor.h"
#include "FmsAuthActions.h"
#include "FmsMedia.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "hash.h"
#include <sstream>

#if defined (_WIN32)
#pragma warning(disable : 4996)

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	return TRUE;
}
#endif

// Flag to process SWF Verification in this auth sample.  A real SWF file
// must be targeted in the SWFVerification code below for the example to work.
static const bool kAuthorizeSwfVerification = false;

class FmsAuthAdaptor : public IFmsAuthAdaptor
{	
	public:

		FmsAuthAdaptor(IFmsAuthServerContext2* pFmsAuthServerContext) 
			: m_pFmsAuthServerContext(pFmsAuthServerContext) {}
		
		virtual ~FmsAuthAdaptor() {}
		
		void authorize(IFmsAuthEvent* pAev);
		void notify(IFmsAuthEvent* pAev);
		void getEvents(I32 aevBitAuth[], I32 aevBitNotf[], unsigned int count);

	private:

		bool getStats(I64 clientStatsHandle, FmsClientStats& baseStats);
		void processStats(IFmsAuthEvent* pAev);

		IFmsAuthServerContext2* m_pFmsAuthServerContext;

};

//
// Utils
//

// Note: Do not delete the return value.  The return value is a buffer 
// allocated in FMS memory space, and FMS will manage the memory.
static char* getStringField(const IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop) 
{
	FmsVariant field;
	if (pEv->getField(prop, field) == IFmsAuthEvent::S_SUCCESS && field.type == field.kString)
	{
		return reinterpret_cast<char*>(field.str);
	}
	return 0;
}

// Note: Do not delete the return value.  The return value is a buffer 
// allocated in FMS memory space, and FMS will manage the memory.
static U8* getBufferField(const IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop) 
{
	FmsVariant field;
	if (pEv->getField(prop, field) == IFmsAuthEvent::S_SUCCESS && field.type == field.kBuffer)
	{
		return field.buf;
	}
	return 0;
}

static bool getI8Field(const IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, I8& iValue) 
{
	FmsVariant field;
	if (pEv->getField(prop, field) == IFmsAuthEvent::S_SUCCESS && field.type == field.kI8)
	{
		iValue = field.i8;
		return true;
	}
	return false;
}

static bool getI32Field(const IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, I32& iValue) 
{
	FmsVariant field;
	if (pEv->getField(prop, field) == IFmsAuthEvent::S_SUCCESS && field.type == field.kI32)
	{
		iValue = field.i32;
		return true;
	}
	return false;
}

static bool getI64Field(const IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, I64& iValue) 
{
	FmsVariant field;
	if (pEv->getField(prop, field) == IFmsAuthEvent::S_SUCCESS && field.type == field.kI64)
	{
		iValue = field.i64;
		return true;
	}
	return false;
}

static bool getFloatField(const IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, float& fValue) 
{
	FmsVariant field;
	if (pEv->getField(prop, field) == IFmsAuthEvent::S_SUCCESS && field.type == field.kFloat)
	{
		fValue = field.f;
		return true;
	}
	return false;
}

static bool getU16Field(const IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, U16& iValue) 
{
	FmsVariant field;
	if (pEv->getField(prop, field) == IFmsAuthEvent::S_SUCCESS && field.type == field.kU16)
	{
		iValue = field.u16;
		return true;
	}
	return false;
}

static bool getU32Field(const IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, U32& iValue) 
{
	FmsVariant field;
	if (pEv->getField(prop, field) == IFmsAuthEvent::S_SUCCESS && field.type == field.kU32)
	{
		iValue = field.u32;
		return true;
	}
	return false;
}

static bool setStringField(IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, char* pValue) 
{
	FmsVariant field;
	field.setString(reinterpret_cast<I8*>(pValue));
	return pEv->setField(prop, field) == IFmsAuthEvent::S_SUCCESS; 
}

static bool setI8Field(IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, I8 iValue) 
{
	FmsVariant field;
	field.setI8(iValue);	
	return pEv->setField(prop, field) == IFmsAuthEvent::S_SUCCESS; 
}

static bool setU8Field(IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, U8 iValue)
{
	FmsVariant field;
	field.setU8(iValue);	
	return pEv->setField(prop, field) == IFmsAuthEvent::S_SUCCESS; 
}

static bool setI32Field(IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, I32 iValue) 
{
	FmsVariant field;
	field.setI32(iValue);	
	return pEv->setField(prop, field) == IFmsAuthEvent::S_SUCCESS; 
}

static bool setU32Field(IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, U32 uValue) 
{
	FmsVariant field;
	field.setU32(uValue);	
	return pEv->setField(prop, field) == IFmsAuthEvent::S_SUCCESS; 
}

static bool setI64Field(IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, I64 iValue) 
{
	FmsVariant field;
	field.setI64(iValue);	
	return pEv->setField(prop, field) == IFmsAuthEvent::S_SUCCESS; 
}

static bool setFloatField(IFmsAuthEvent* pEv, IFmsAuthEvent::Field prop, float fValue) 
{
	FmsVariant field;
	field.setFloat(fValue);	
	return pEv->setField(prop, field) == IFmsAuthEvent::S_SUCCESS; 
}

static bool isADPCMSupported(int iAudioCodecs)
{	
	return (iAudioCodecs & SUPPORT_SND_ADPCM) != 0;
}

static bool isVP6Supported(int iVideoCodecs)
{
	int iAllVP6 = ( SUPPORT_VID_VP6ALPHA | SUPPORT_VID_VP6 );
	return (iVideoCodecs & iAllVP6) != 0;
}

static bool isService(int iType)
{
	return (iType & TYPE_SERVICE) != 0;
}

static bool isAMF3(unsigned char uEncod)
{
	return (uEncod == ENCODE_AMF3);
}

//
// This class will process all authorization events
//
class MyFmsAuthorizeEvent 
{
public:
	
	MyFmsAuthorizeEvent(IFmsAuthEvent* pAev, IFmsAuthServerContext2* pFmsAuthServerContext)
		: m_pAev(pAev), m_pFmsAuthServerContext(pFmsAuthServerContext) {}
	
	virtual ~MyFmsAuthorizeEvent() {}
	
	void authorize();

private:
	
	IFmsAuthEvent*			m_pAev;
	IFmsAuthServerContext2*	m_pFmsAuthServerContext;
};

void MyFmsAuthorizeEvent::authorize()
{
	bool bAuthorized = true;  // default authorization state
	
	switch(m_pAev->getType())
	{
		case IFmsAuthEvent::E_APPSTART:
		{
			// only E_APPSTART allows changes to the following fields: 
			// F_APP_PRTMP
			// F_APP_PRTMP_COMMON_KEY_FILE
			// F_APP_PRTMP_VIDEO_ENC_LEVEL
			// F_APP_PRTMP_UPDATE_INTERVAL
			// F_APP_PRTMP_SWF_VERIFICATION
			// F_APP_PRTMP_SWF_WHITELIST_FOLDER
			bool bSet;
			I8 sValue;
			if (getI8Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP, sValue))
			{
				bSet = setI8Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP, sValue);
			}

			char* pCommonKeyPath = getStringField(m_pAev, IFmsAuthEvent::F_APP_PRTMP_COMMON_KEY_FILE); 
			if (pCommonKeyPath)
			{				
				bSet = setStringField(m_pAev, IFmsAuthEvent::F_APP_PRTMP_COMMON_KEY_FILE, 
					pCommonKeyPath);					
			}
			
			U32 uValue;
			if (getU32Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP_VIDEO_ENC_LEVEL, uValue))
			{
				bSet = setU32Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP_VIDEO_ENC_LEVEL, uValue);
			}

			I32 iValue;
			if (getI32Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP_UPDATE_INTERVAL, iValue))
			{
				bSet = setI32Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP_UPDATE_INTERVAL, iValue);
			}

			if (getI8Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP_SWF_VERIFICATION, sValue))
			{
				bSet = setI8Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP_SWF_VERIFICATION, sValue);
			}

			char* pWhiteListPath = getStringField(m_pAev, IFmsAuthEvent::F_APP_PRTMP_SWF_WHITELIST_FOLDER); 
			if (pWhiteListPath)
			{				
				bSet = setStringField(m_pAev, IFmsAuthEvent::F_APP_PRTMP_SWF_WHITELIST_FOLDER, 
					pWhiteListPath);					
			}

			bAuthorized = true;	
			
		}
		break;	
		case IFmsAuthEvent::E_CONNECT:
		{
			// only E_CONNECT allows changes to the following fields: 
			// F_CLIENT_AUDIO_SAMPLE_ACCESS
			// F_CLIENT_AUDIO_SAMPLE_ACCESS_LOCK
			// F_CLIENT_READ_ACCESS
			// F_CLIENT_READ_ACCESS_LOCK
			// F_CLIENT_VIDEO_SAMPLE_ACCESS
			// F_CLIENT_VIDEO_SAMPLE_ACCESS_LOCK
			// F_CLIENT_WRITE_ACCESS_LOCK
			// F_CLIENT_WRITE_ACCESS
			I8 iValue;
			if (getI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_WRITE_ACCESS, iValue))
			{
				setI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_WRITE_ACCESS, iValue);
			}

			// redirect connection example
			char* pUri = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_URI); 
			if (pUri && !strcmp(pUri, "rtmp://localhost/streamtest"))
			{				
				setStringField(m_pAev, IFmsAuthEvent::F_CLIENT_REDIRECT_URI, 
					"rtmp://localhost:1935/streamtest");
				bAuthorized = false;	
			}

			// set DiffServ fields based on a client IP 
			// char* pIp = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_IP); 
			// if (pIp && !strcmp(pIp, "192.168.1.1"))
			{				
				// set the DSCP bits and mask
				U8 m_diffServBits = 170;
				U8 m_diffServMask = 252;
				setU8Field(m_pAev, IFmsAuthEvent::F_CLIENT_DIFFSERV_BITS, m_diffServBits);
				setU8Field(m_pAev, IFmsAuthEvent::F_CLIENT_DIFFSERV_MASK, m_diffServMask);

				bAuthorized = true;	
			}
		}
		break;	
		case IFmsAuthEvent::E_PLAY:
		{			
			char* pStreamName = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_NAME); 
			if (pStreamName)
			{
				setStringField(m_pAev, IFmsAuthEvent::F_STREAM_NAME, pStreamName);
			}			
			char* pStreamType = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_TYPE);
			if (pStreamType)
			{
				setStringField(m_pAev, IFmsAuthEvent::F_STREAM_TYPE, pStreamType);
			}			
			char* pStreamQuery = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_QUERY);
			if (pStreamQuery)
			{
				setStringField(m_pAev, IFmsAuthEvent::F_STREAM_QUERY, pStreamQuery);
			}			
			m_pFmsAuthServerContext->log(pStreamName, IFmsServerContext::kInformation, false);
			m_pFmsAuthServerContext->log(pStreamType, IFmsServerContext::kInformation, false);
			m_pFmsAuthServerContext->log(pStreamQuery, IFmsServerContext::kInformation, false);

			I8 iValue;
			if (getI8Field(m_pAev, IFmsAuthEvent::F_STREAM_RESET, iValue))
			{
				// If iValue is 1 (true) the playlist will be reset and the 
				// stream will be the only stream in the playlist; otherwise 
				// 0 (false) means the stream will be added to the existing
				// playlist.
				setI8Field(m_pAev, IFmsAuthEvent::F_STREAM_RESET, iValue);	
			}
			if (getI8Field(m_pAev, IFmsAuthEvent::F_STREAM_IGNORE, iValue))
			{
				// If iValue is 1 (true) the stream timestamps will be ignored;
				// otherwise 0 (false) means the timestamps will be handled.
				setI8Field(m_pAev, IFmsAuthEvent::F_STREAM_IGNORE, iValue);	
			}
			
			char* pStreamTransition = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_TRANSITION); 
			if (pStreamTransition && strlen(pStreamTransition))
			{				
				// MBR transition example
				if (!strcmp(pStreamTransition, "switch") ||
					!strcmp(pStreamTransition, "swap"))
				{					
					// get the old stream's properties 
					char* pOldStreamName = getStringField(m_pAev, IFmsAuthEvent::F_OLD_STREAM_NAME);
					char* pOldStreamType = getStringField(m_pAev, IFmsAuthEvent::F_OLD_STREAM_TYPE);
					char* pOldStreamQuery = getStringField(m_pAev, IFmsAuthEvent::F_OLD_STREAM_QUERY);

					// if pOldStream is empty (optional for switch) current stream is in play
					// do we really want stream transition? 	
					{
						// no we do not allow transition
						// bAuthorized = false;
						// now transition will be turned off and old stream continue playing
						// break;					
					}
					// doing nothing will execute transition mode as is
					// or you could modify transition by changing transition properties

					// set it to 1 to indicate they will be hooking up the stream, 
					// but that it does not currently exist
					setI32Field(m_pAev, IFmsAuthEvent::F_STREAM_LIVE_PUBLISH_PENDING, 1);	
				}	

				// get the offset value if transition is set to offset mode for reconnect
				if (!strcmp(pStreamTransition, "resume"))
				{
					float fValue;
					if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_OFFSET, fValue))
					{
						float offset = fValue; //offset value in seconds
					}			
				}
			}
			else
			{
				// This is a regular play waiting for approval, which may be converted
				// into a play2 command by changing transition properties
			}	
		}
		break;
		case IFmsAuthEvent::E_PUBLISH:
		{	
			char* pStreamName = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_NAME); 
			if (pStreamName)
			{
				setStringField(m_pAev, IFmsAuthEvent::F_STREAM_NAME, pStreamName);
			}	
			char* pStreamType = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_TYPE);
			if (pStreamType)
			{
				setStringField(m_pAev, IFmsAuthEvent::F_STREAM_TYPE, pStreamType);
			}
			I32 iValue;
			if (getI32Field(m_pAev, IFmsAuthEvent::F_STREAM_PUBLISH_TYPE, iValue))
			{
				// publish types:
				// 0 : record
				// 1 : append
				// 2 : appendWithGap
				// -1 : live
				setI32Field(m_pAev, IFmsAuthEvent::F_STREAM_PUBLISH_TYPE, iValue);	
			}
		} 		
		break;
		case IFmsAuthEvent::E_FILENAME_TRANSFORM:
			{
				I64 iValue;
				if (getI64Field(m_pAev, IFmsAuthEvent::F_CLIENT_ID, iValue))
				{
					// some fields are not eligible to be modified. The return
					// value will be false when trying to modify the F_CLIENT_ID.
					bool bSet = setI64Field(m_pAev, IFmsAuthEvent::F_CLIENT_ID, iValue); 
				}
				char* pStreamName = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_NAME); 
				if (pStreamName)
				{
					// some fields are not eligible to be modified. The return
					// value will be false when trying to modify the F_STREAM_NAME.
					bool bSet = setStringField(m_pAev, IFmsAuthEvent::F_STREAM_NAME, pStreamName); 
				}
				char* pStreamPath = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_PATH); 
				if (pStreamPath)
				{
					setStringField(m_pAev, IFmsAuthEvent::F_STREAM_PATH, pStreamPath);
				}				
				char* pStreamType = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_TYPE);
				if (pStreamType)
				{
					setStringField(m_pAev, IFmsAuthEvent::F_STREAM_TYPE, pStreamType);		
				}						
			}
		break;
		case IFmsAuthEvent::E_PAUSE:
		{
			bAuthorized = false; // block all E_PAUSE events.
						
			float fValue;
			if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_PAUSE_TIME, fValue))
			{
				float fPauseTime = fValue;	// in seconds
			}		
			
			I8 iValue;
			if (getI8Field(m_pAev, IFmsAuthEvent::F_STREAM_PAUSE, iValue))
			{
				// 1 (true) means PAUSE
				// 0 (false) means UNPAUSE
				bool boolPause = iValue != 0;	
			}			
			if (getI8Field(m_pAev, IFmsAuthEvent::F_STREAM_PAUSE_TOGGLE, iValue))
			{
				// 1 (true) means PAUSE_TOGGLE
				// 0 (false) means no PAUSE_TOGGLE was set
				bool boolPauseToggle = iValue != 0;	
			}	
		
			FmsVariant field;
			
			// Notify Action example: An IFmsNofifyAction is created to notify
			// server side action script (SSAS) of the E_PAUSE event by calling
			// the function name "method" in the script.  In this example two
			// variables will be passed to "method" by calling addParam(field)
			// on the action.
			if (m_pAev->getField(IFmsAuthEvent::F_CLIENT_ID, field) == IFmsAuthEvent::S_SUCCESS)
			{
				I64 clientId = field.i64;
				IFmsNotifyAction* pAction = m_pAev->addNotifyAction("Notified by adaptor");
				pAction->setClientId(field);
				const char mtd[] = "method";
				field.setString(reinterpret_cast<I8*>(const_cast<char*>(mtd)));
				pAction->setMethodName(field);

				// create and insert a U16 "12345" as the first parameter 
				field.setU16(12345);
				pAction->addParam(field);

				// create and insert clientId as a double as the second parameter 
				field.setDouble((double)clientId);
				pAction->addParam(field);
				
				// Note: SSAS does not work with I64 or Buffer variants
				// field.setI64(clientId); 
				// pAction->addParam(field); // incorrect
			}
		}
		break;
		case IFmsAuthEvent::E_SEEK:
		{
			bAuthorized = false; // block all E_SEEK events
			
			float fValue;
			if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_SEEK_POSITION, fValue))
			{
				// Modification of the seek position example:
				// fValue + 3; will add 3 seconds to the initial seek posistion
				float fSeekTime = fValue; // value in seconds
				setFloatField(m_pAev, IFmsAuthEvent::F_STREAM_SEEK_POSITION, fSeekTime);
			}			
		}
		break;
		case IFmsAuthEvent::E_LOADSEGMENT:
		{
			// bAuthorized = false;	// block all E_LOADSEGMENT events
			// E_LOADSEGMENT is a read only event that substitutes E_PLAY on 
			// FMS Origin servers for recorded streams.
			
			I64 iValue;	
			if (getI64Field(m_pAev, IFmsAuthEvent::F_SEGMENT_START, iValue))
			{
				I64 iStart = iValue; // in bytes
			}			
			if (getI64Field(m_pAev, IFmsAuthEvent::F_SEGMENT_END, iValue))
			{
				I64 iEnd = iValue;	// in bytes
			}			
		}
		break;
		case IFmsAuthEvent::E_RECORD:
		{
			// bAuthorized = false;	// block all E_RECORD events
			float fValue;	
			if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_RECORD_MAXSIZE, fValue))
			{
				float recMaxSize = fValue; // in kilobytes
				setFloatField(m_pAev, IFmsAuthEvent::F_STREAM_RECORD_MAXSIZE, recMaxSize);
			}			
			if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_RECORD_MAXDURATION, fValue))
			{
				float recMaxDuration = fValue;	// in seconds
				setFloatField(m_pAev, IFmsAuthEvent::F_STREAM_RECORD_MAXDURATION, recMaxDuration);
			}	
		}
		break;
		case IFmsAuthEvent::E_SWF_VERIFY:
		{
			// SWF Verification example:
			// kAuthorizeSwfVerification is assigned false by default.  The
			// target SWF file must be updated for this to work.
			if(kAuthorizeSwfVerification) 
			{
				I8 swfvVersion = 0;
				if(getI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_SWFV_VERSION, swfvVersion))
				{
					std::stringstream stream;
					stream << "Swf verification version is " << static_cast<int>(swfvVersion);
					m_pFmsAuthServerContext->log(stream.str().c_str(), IFmsServerContext::kInformation, false);
				}
				I64 swfvDepth;
				if(getI64Field(m_pAev, IFmsAuthEvent::F_CLIENT_SWFV_DEPTH, swfvDepth))
				{				
					I32 swfvTTL;
					if(getI32Field(m_pAev, IFmsAuthEvent::F_CLIENT_SWFV_TTL, swfvTTL))
					{
						swfvTTL /= 2;
						setI32Field(m_pAev, IFmsAuthEvent::F_CLIENT_SWFV_TTL, swfvTTL);
					}

					U8 digest[kSHA256DigestLen];

					// Target a real SWF file instead of sample.swf 
					hashSwfFileAtDepth("C:\\sample.swf", swfvDepth, digest);

					FmsVariant field;
					field.setBuffer(digest, kSHA256DigestLen);	
					m_pAev->setField(IFmsAuthEvent::F_CLIENT_SWFV_DIGEST, field);
				}
			}			
		}
		break;
		
		case IFmsAuthEvent::E_APPSTOP:
		case IFmsAuthEvent::E_DISCONNECT:
		case IFmsAuthEvent::E_STOP:
		case IFmsAuthEvent::E_UNPUBLISH:
		case IFmsAuthEvent::E_ACTION:
		case IFmsAuthEvent::E_CODEC_CHANGE:
		case IFmsAuthEvent::E_RECORD_STOP:
		case IFmsAuthEvent::E_CLIENT_PAUSE:
		case IFmsAuthEvent::E_SWF_VERIFY_COMPLETE:
		case IFmsAuthEvent::E_CLIENT_SEEK:
		case IFmsAuthEvent::E_START_TRANSMIT:
		case IFmsAuthEvent::E_STOP_TRANSMIT:
		case IFmsAuthEvent::E_MAXEVENT:
		break;
	}
	
	IFmsAuthServerContext2::AuthFailureDesc* desc = NULL;
	
	if(!bAuthorized)
	{
		desc = new IFmsAuthServerContext2::AuthFailureDesc("Blocked by auth adaptor", 
			IFmsAuthServerContext2::kDefaultStatus, -1);
	}		
	char buf[1024];
	const char* const action = bAuthorized ? "approved" : "rejected";
	sprintf(buf, "Received dddd authorization type=%d id=%p %s\n", m_pAev->getType(), 
		m_pAev, action);

	char tlog[1024];
	sprintf(tlog,"bb\n");
	m_pFmsAuthServerContext->log(tlog, IFmsServerContext::kInformation, false) 
	
	// log to the configured FMS log directory. If the third parameter is true,
	// also send the log to the system event log.
	m_pFmsAuthServerContext->log(buf, IFmsServerContext::kInformation, false); 
	
	m_pFmsAuthServerContext->onAuthorize(m_pAev, bAuthorized, desc);
	delete desc;
}

class MyFmsNotifyEvent 
{
	public:
		MyFmsNotifyEvent(IFmsAuthEvent* pAev, IFmsAuthServerContext2* pFmsAuthServerContext)
			: m_pAev(pAev), m_pFmsAuthServerContext(pFmsAuthServerContext) {}
		
		virtual ~MyFmsNotifyEvent() {}
		
		void notify() const;

	private:
		
		IFmsAuthEvent*	m_pAev;
		IFmsAuthServerContext2*	m_pFmsAuthServerContext;
		
};

void MyFmsNotifyEvent::notify() const
{	
	switch(m_pAev->getType())
	{
		case IFmsAuthEvent::E_PLAY:
		{
			char* pAppName = getStringField(m_pAev, IFmsAuthEvent::F_APP_NAME);
			char* pAppInst = getStringField(m_pAev, IFmsAuthEvent::F_APP_INST);
			char* pAppUri = getStringField(m_pAev, IFmsAuthEvent::F_APP_URI);
			
			///new prtmp fields
			I8 sValue;
			if (getI8Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP, sValue))
			{
				bool isProtected = sValue != 0;
			}
			char* pCommonKeyPath = getStringField(m_pAev, IFmsAuthEvent::F_APP_PRTMP_COMMON_KEY_FILE);
			U32 u32Value;
			if (getU32Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP_VIDEO_ENC_LEVEL, u32Value))
			{
				U32 videoEncyption = u32Value;
			}
			I32 iValue;
			if (getI32Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP_UPDATE_INTERVAL, iValue))
			{
				I32 updateInterval = iValue;
			}
			if (getI8Field(m_pAev, IFmsAuthEvent::F_APP_PRTMP_SWF_VERIFICATION, sValue))
			{
				bool isPrtmpSwf = sValue != 0;
			}
            char* pWhiteListPath = getStringField(m_pAev, IFmsAuthEvent::F_APP_PRTMP_SWF_WHITELIST_FOLDER);
			////////

			char* pClIp = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_IP);
			char* pClUri = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_URI);
			char* pClNewUri = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_REDIRECT_URI);
			char* pClVhost = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_VHOST);
			char* pClRef = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_REFERRER);
			char* pClPurl = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_PAGE_URL);
			char* pClAgent = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_USER_AGENT);
			char* pClRAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_READ_ACCESS);
			char* pClWAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_WRITE_ACCESS);
			char* pClAudioAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_AUDIO_SAMPLE_ACCESS);
			char* pClVideoAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_VIDEO_SAMPLE_ACCESS);
			char* pClProto = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_PROTO);
			char* pClUstem = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_URI_STEM);
			
			char* pStreamName = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_NAME);
			char* pStreamType = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_TYPE);
			char* pStreamQuery = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_QUERY);
			char* pStreamPath = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_PATH);
			
			if (getI32Field(m_pAev, IFmsAuthEvent::F_CLIENT_AUDIO_CODECS, iValue))
			{
				bool bADPCM = isADPCMSupported(iValue);
			}
			if (getI32Field(m_pAev, IFmsAuthEvent::F_CLIENT_VIDEO_CODECS, iValue))
			{
				bool bVP6 = isVP6Supported(iValue);
			}			
			if (getI32Field(m_pAev, IFmsAuthEvent::F_CLIENT_TYPE, iValue))
			{	
				bool bService = isService(iValue);
			}
			if (getI32Field(m_pAev, IFmsAuthEvent::F_STREAM_ID, iValue))
			{	
				I32 iStreamId = iValue;
			}
						
			float fValue;
			if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_LENGTH, fValue))
			{
				float fLength = fValue; // in seconds
			}
			if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_POSITION, fValue))
			{
				float iPosition = fValue; // in seconds
			}

			I64 lValue;
			if (getI64Field(m_pAev, IFmsAuthEvent::F_CLIENT_ID, lValue))
			{
				I64 iClientId = lValue;		
			}
						
			
			if (getI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_SECURE, sValue))
			{
				bool bSecure = sValue != 0;
			}
			if (getI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_AMF_ENCODING, sValue))
			{
				bool bAMF3 = isAMF3(sValue);
			}
			if (getI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_READ_ACCESS_LOCK, sValue))
			{			
				bool bRead = sValue != 0;
			}
			if (getI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_WRITE_ACCESS_LOCK, sValue))
			{
				bool bWrite = sValue != 0;
			}
			if (getI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_AUDIO_SAMPLE_ACCESS_LOCK, sValue))
			{
				bool bAudioRead = sValue != 0;
			}
			if (getI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_VIDEO_SAMPLE_ACCESS_LOCK, sValue))
			{
				bool bVideoRead = sValue != 0;
			}
			if (getI8Field(m_pAev, IFmsAuthEvent::F_STREAM_RESET, sValue))
			{
				bool bReset = sValue != 0;
			}
			if (getI8Field(m_pAev, IFmsAuthEvent::F_STREAM_IGNORE, sValue))
			{
				bool bIgnore = sValue != 0;
			}
		}			
		break;
		
		case IFmsAuthEvent::E_SEEK:
		{
			float fValue;
			if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_SEEK_POSITION, fValue))
			{
				float fSeekTime = fValue;
			}

			// Disconnect Action example: disconnect the client that was 
			// specified by the E_SEEK notify event 
			FmsVariant field;
			if (m_pAev->getField(IFmsAuthEvent::F_CLIENT_ID, field) == IFmsAuthEvent::S_SUCCESS)
			{
				IFmsDisconnectAction* pAction = 
					const_cast<IFmsAuthEvent*>(m_pAev)->
						addDisconnectAction("Seek is not allowed. Blocked by adaptor");
				pAction->setClientId(field);
			}
		}
		break;

		case IFmsAuthEvent::E_CODEC_CHANGE:
		{
			char* pAppName = getStringField(m_pAev, IFmsAuthEvent::F_APP_NAME);
			char* pAppInst = getStringField(m_pAev, IFmsAuthEvent::F_APP_INST);
			char* pAppUri = getStringField(m_pAev, IFmsAuthEvent::F_APP_URI);
			
			char* pClIp = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_IP);
			char* pClUri = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_URI);
			char* pClNewUri = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_REDIRECT_URI);
			char* pClVhost = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_VHOST);
			char* pClRef = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_REFERRER);
			char* pClPurl = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_PAGE_URL);
			char* pClAgent = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_USER_AGENT);
			char* pClRAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_READ_ACCESS);
			char* pClWAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_WRITE_ACCESS);
			char* pClAudioAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_AUDIO_SAMPLE_ACCESS);
			char* pClVideoAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_VIDEO_SAMPLE_ACCESS);
			char* pClProto = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_PROTO);
			char* pClUstem = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_URI_STEM);
			
			char* pStreamName = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_NAME);
			char* pStreamType = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_TYPE);
			char* pStreamQuery = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_QUERY);
			char* pStreamPath = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_PATH);

			U16 fType;
			if (getU16Field(m_pAev, IFmsAuthEvent::F_STREAM_CODEC_TYPE, fType))
			{
				U16 streamCodecType = fType;
				if (streamCodecType == kVIDEO_CODEC)
				{
					U16 fValue;
					if (getU16Field(m_pAev, IFmsAuthEvent::F_STREAM_CODEC, fValue))
					{				
						U16 streamCodecValue = fValue;
						if (streamCodecValue == VIDEO_CODEC_SORENSON)
						{
							// Disconnect Action example: Disallow clients trying 
							// to publish content with the sorenson video codec.
							FmsVariant field;
							if (m_pAev->getField(IFmsAuthEvent::F_CLIENT_ID, field) == IFmsAuthEvent::S_SUCCESS)
							{
								IFmsDisconnectAction* pAction = 
									const_cast<IFmsAuthEvent*>(m_pAev)->
									addDisconnectAction("Sorenson is not allowed. Blocked by adaptor");
								pAction->setClientId(field);
							}
						}
					}		
				}
			}				
		}
		break;
		case IFmsAuthEvent::E_RECORD_STOP:
		{
			char* pAppName = getStringField(m_pAev, IFmsAuthEvent::F_APP_NAME);
			char* pAppInst = getStringField(m_pAev, IFmsAuthEvent::F_APP_INST);
			char* pAppUri = getStringField(m_pAev, IFmsAuthEvent::F_APP_URI);
			
			char* pClIp = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_IP);
			char* pClUri = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_URI);
			char* pClNewUri = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_REDIRECT_URI);
			char* pClVhost = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_VHOST);
			char* pClRef = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_REFERRER);
			char* pClPurl = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_PAGE_URL);
			char* pClAgent = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_USER_AGENT);
			char* pClRAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_READ_ACCESS);
			char* pClWAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_WRITE_ACCESS);
			char* pClAudioAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_AUDIO_SAMPLE_ACCESS);
			char* pClVideoAccess = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_VIDEO_SAMPLE_ACCESS);
			char* pClProto = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_PROTO);
			char* pClUstem = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_URI_STEM);
			
			char* pStreamName = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_NAME);
			char* pStreamType = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_TYPE);
			char* pStreamQuery = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_QUERY);
			char* pStreamPath = getStringField(m_pAev, IFmsAuthEvent::F_STREAM_PATH);
			
			float fValue;	
			if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_RECORD_MAXSIZE, fValue))
			{
				float recMaxSize = fValue; // in kilobytes
			}			
			if (getFloatField(m_pAev, IFmsAuthEvent::F_STREAM_RECORD_MAXDURATION, fValue))
			{
				float recMaxDuration = fValue;	// in seconds
			}	
		}
		break;
		case IFmsAuthEvent::E_SWF_VERIFY_COMPLETE:
		{
			char* pClIp = getStringField(m_pAev, IFmsAuthEvent::F_CLIENT_IP);

			I8 version;	// version of SWF verification 
			getI8Field(m_pAev, IFmsAuthEvent::F_CLIENT_SWFV_VERSION, version);

			I64 depth; // depth in the SWF file hashed
			getI64Field(m_pAev, IFmsAuthEvent::F_CLIENT_SWFV_DEPTH, depth);

			I32 ttl; // time to live of the SWF hash provided
			getI32Field(m_pAev, IFmsAuthEvent::F_CLIENT_SWFV_TTL, ttl);

			// digest provided to match against
			U8* buffer = getBufferField(m_pAev, IFmsAuthEvent::F_CLIENT_SWFV_DIGEST);

			// result of the attempted match-- see FmsAuthEvents.h enum
			// eSWFMatch for the meaning of this field
			I32 match;	
			getI32Field(m_pAev, IFmsAuthEvent::F_CLIENT_SWFV_RESULT, match);

			std::stringstream stream;
			stream << "swf verification for client: " 
					<< std::string(pClIp) 
					<< " is complete, the result is: " << match;
			m_pFmsAuthServerContext->log(stream.str().c_str(), IFmsServerContext::kInformation, false);
		}
		break;

		case IFmsAuthEvent::E_APPSTART:
		case IFmsAuthEvent::E_APPSTOP:
		case IFmsAuthEvent::E_CONNECT:
		case IFmsAuthEvent::E_DISCONNECT:
		case IFmsAuthEvent::E_FILENAME_TRANSFORM:
		case IFmsAuthEvent::E_STOP:
		case IFmsAuthEvent::E_PAUSE:
		case IFmsAuthEvent::E_PUBLISH:
		case IFmsAuthEvent::E_UNPUBLISH:
		case IFmsAuthEvent::E_LOADSEGMENT:
		case IFmsAuthEvent::E_ACTION:
		case IFmsAuthEvent::E_RECORD:
		case IFmsAuthEvent::E_CLIENT_PAUSE:
		case IFmsAuthEvent::E_SWF_VERIFY:
		case IFmsAuthEvent::E_CLIENT_SEEK:
		case IFmsAuthEvent::E_START_TRANSMIT:
		case IFmsAuthEvent::E_STOP_TRANSMIT:
		case IFmsAuthEvent::E_MAXEVENT:
		break;

	}	
	char buf[1024];
	sprintf(buf, "Received notification type=%d id=%p\n", m_pAev->getType(), m_pAev);

	// log to the configured FMS log directory. If the third parameter is true,
	// also send the log to the system event log.
	m_pFmsAuthServerContext->log(buf, IFmsServerContext::kInformation, false); 
	m_pFmsAuthServerContext->onNotify(m_pAev);
}

/* All authorization events flow through this wrapper function.
*
* Note: This sample auth adaptor has MyFmsAppAuthEvent allocated on the 
* stack, but time intensive implementations may warrant authorization to
* be allocated on the heap so work may be passed to a thread pool.  This
* prevents starvation of the calling FMS threads in custom code that may
* have processing delays (ie database calls, network filesystem access, etc..). 
*/
void FmsAuthAdaptor::authorize(IFmsAuthEvent* pAev)
{
	 MyFmsAuthorizeEvent(pAev, m_pFmsAuthServerContext).authorize();
}

/* All notification events flow through this wrapper function.
*
* Note: This sample auth adaptor has MyFmsNotifyEvent allocated on the 
* stack, but time intensive implementations may warrant notifications to
* be allocated on the heap so work may be passed to a thread pool.  This
* prevents starvation of the calling FMS threads in custom code that may
* have processing delays (ie database calls, network filesystem access, etc..). 
*/
void FmsAuthAdaptor::notify(IFmsAuthEvent* pAev)
{
	 processStats(pAev);
	 MyFmsNotifyEvent(pAev, m_pFmsAuthServerContext).notify();
}

/*
* Get client statistics.
*/
bool FmsAuthAdaptor::getStats(I64 clientStatsHandle, FmsClientStats& baseStats)
{
	bool bValue= m_pFmsAuthServerContext->getClientStats(clientStatsHandle, baseStats);

	return bValue;
}

/*
* Example obtainting client stats from an E_CONNECT or E_STOP event 
*/
void FmsAuthAdaptor::processStats(IFmsAuthEvent* pAev)
{
	 I64 statsHandle;
	 FmsClientStats baseStats;
	 if (!getI64Field(pAev, IFmsAuthEvent::F_CLIENT_STATS_HANDLE, statsHandle))
		 return;
	 char* pAppName = getStringField(pAev, IFmsAuthEvent::F_APP_NAME);
	 if (pAev->getType() == IFmsAuthEvent::E_CONNECT) 
	 {
		 getStats(statsHandle, baseStats);

		 // log data
		 char buf[1024];
		 char hashKey[9];
		 memset(hashKey, 0, 9);
		 memcpy(hashKey, &statsHandle, sizeof(statsHandle));
		 sprintf(buf, "client Stats Handle= %s, bytes_in= %f, bytes_out= %f\n", hashKey, 
			 static_cast<double>(baseStats.bytes_in), static_cast<double>(baseStats.bytes_out));
		 m_pFmsAuthServerContext->log(buf, IFmsServerContext::kInformation, false); 
	 }
	 else if (pAev->getType() == IFmsAuthEvent::E_STOP)
	 {
		 getStats(statsHandle, baseStats);
	 }
}

/* By default, all authorization and notifications events will be sent.
* Call excludeEvents with the bit set to 1, to stop recieving events.
* Note: The events: 
* E_APPSTART, E_APPSTOP, E_DISCONNECT, E_STOP, E_UNPUBLISH, E_CODEC_CHANGE
* are excluded by default and are not authorizable.
*/
void FmsAuthAdaptor::getEvents(I32 aevBitAuth[], I32 aevBitNotf[], unsigned int count)
{	
	// exclude certain auth events
	IFmsAuthEvent::EventType authExcludeEvent[] = { IFmsAuthEvent::E_SEEK };
	// set E_SEEK to a non authorizable event
	m_pFmsAuthServerContext->excludeEvents(aevBitAuth, count, authExcludeEvent, 1);

	// Warning: if E_CODEC_CHANGE event is not excluded, all messages will be
	// scanned to detect codec change. Subscribe to this event only as needed.
	// Example that excludes certain notify events. (E_PAUSE, E_CODEC_CHANGE)
	IFmsAuthEvent::EventType notifyExcludeEvent[] =
		{ IFmsAuthEvent::E_PAUSE, IFmsAuthEvent::E_CODEC_CHANGE };
	m_pFmsAuthServerContext->excludeEvents(aevBitNotf, count, notifyExcludeEvent, 2);	
}

extern "C" void FCExport FmsCreateAuthAdaptor3(IFmsAuthServerContext2* pAuthServerCtx, 
											   IFmsAuthAdaptor*& pFmsAuthAdaptor, U32& iVersion)
{
	pFmsAuthAdaptor = new FmsAuthAdaptor(pAuthServerCtx);
	U32 version = pAuthServerCtx->getVersion();
	U16 w2 = LWORD(version);
	U16 w1 = HWORD(version);
	iVersion = MKLONG(INTERFACE_MINOR_VERSION, INTERFACE_MAJOR_VERSION);
	char buf[1024];
	char *ptr = buf;
	int valueLen = pAuthServerCtx->getConfig("UserKey1", &ptr, sizeof(buf));
	if (!valueLen)
	{
		valueLen = pAuthServerCtx->getConfig("UserKey2", &ptr, sizeof(buf));
		if (!valueLen)
		{
			return; 
		}
		if (valueLen < 0)
		{
			// failed to find this key
			return;
		}
	}
	if (valueLen < 0)
	{
		// failed to find this key
		return; 
	}
	// value length is bigger then the buffer size, and a real adaptor should 
    // allocate valueLen + 1 bytes and call again
}


extern "C" void FCExport FmsDestroyAuthAdaptor3(IFmsAuthAdaptor* pAuthAdaptor )
{
	delete pAuthAdaptor;	
}

