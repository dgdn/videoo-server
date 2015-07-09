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
|  Copyright (c) 2000, Adobe Systems Incorporated.         					 |
|  All rights reserved.                                                      |
|                                                                            |
|  NOTICE:  All information contained herein is, and remains the property    |
|  of Adobe Systems Incorporated and its suppliers, if any. The intellectual |
|  and technical concepts contained herein are proprietary to Adobe Systems  |
|  Incorporated and are protected by trade secret or						 |
|  copyright law. Dissemination of this information or reproduction of this  |
|  material is strictly forbidden unless prior written permission is         |
|  obtained from Adobe Systems Incorporated.                                 |
|                                                                            |
|          Adobe Systems Incorporated       415.832.2000                     |
|          601 Townsend Street              415.832.2020 fax                 |
|          San Francisco, CA 94103                                           |
|                                                                            |
+----------------------------------------------------------------------------*/


#if !defined( Adaptor_h )
#define  Adaptor_h

/*---------------------------------------------------------------------------
	Include files
---------------------------------------------------------------------------*/

#include "IFCAccessAdaptor.h"


class	FCExport SampleAdaptor : public IFCAccessAdaptor 
{
	IFCAccessContext* m_pCtx;

	public:
		SampleAdaptor(IFCAccessContext* pCtx);

		virtual ~SampleAdaptor();

		virtual const char* getDescription() const;

		virtual void getVersion( int& iMajor, int& iMinor, int& iMicro ) const;

		virtual void onAccess( IFCAccess* pAccess );
};

#endif	// !defined( Adaptor_h )
