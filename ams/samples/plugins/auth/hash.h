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
|  Copyright (c) 2009 - 2010, Adobe Systems Incorporated.                    |
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
#if !defined( Hash_H )
#define  Hash_H
////////////
// To actually use the hash calculation functionality 
// 1. define OPEN_SSL in you project
// 2. include the openssl/hmac.h and related dependent header files 
// 3. link you project with openssl libs 
// For more information on the documents releated to swf hash calcualtions and library
// pls contact support http://www.adobe.com/support/ - 
// Note: This was initially built and tested against OpenSSL 0.9.8j
///////////

#ifdef OPEN_SSL 
#include "openssl/hmac.h"
#endif

////////////
// To actually use the hash calculation functionality 
// 1. define ZLIB in you project
// 2. include the zlib.h and related dependent header files 
// 3. link you project with zlib libs 
// For more information on the documents releated to swf hash calcualtions and library
// pls contact support http://www.adobe.com/support/ 
// Note: This was initially built and tested agsint ZLib 1.0.4
///////////

#ifdef ZLIB
#include "zlib.h"
#endif

static const int kSHA256DigestLen = 32;
static const unsigned char kSalt[31] = "Genuine Adobe Flash Player 001";
static const int kSaltLength = 30;
///
/// this function calculates HMACSHA256
/// \param swf uncompressed data
/// \param length of data to be read
/// \param salt
/// \param salt length
/// \param this is return hash
///
inline void calcHMACSHA256(const unsigned char* data, unsigned long datalen, const unsigned char* key, unsigned long keylen, unsigned char* digest)
{
	// these are openssl functions and require openSSL inclusion
#ifdef OPEN_SSL 
	HMAC_CTX ctx;
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, key, keylen, EVP_sha256(), 0/*no engine*/);
	HMAC_Update(&ctx, data, datalen);
	unsigned int olen = kSHA256DigestLen;
	HMAC_Final(&ctx, digest, &olen);
	HMAC_CTX_cleanup(&ctx);
#else
	memset(digest,0, kSHA256DigestLen); // set to 0
#endif
}

///
/// this function calculates the hash for swf data for some depth
/// \param filename to get the SWF data
///		(commented out in sample to avoid a compiler warning)
/// \param depth for which hash is to be computed
/// \param this is return hash
///
void hashSwfFileAtDepth(const char* fileName, I64 depth, unsigned char* digest)
{
	// open our file, using reall old posix APIs
	FILE* inFile = 0;
	inFile = fopen(fileName, "rb");
	if( !inFile ) { return; } // bail-out!

	// obtain file size and get back to the beginning
	fseek(inFile, 0, SEEK_END);
	size_t lSize = ftell(inFile);
	fseek(inFile, 0, SEEK_SET);

	// read em in - I recommend more exception safe code
	// like boost's scoped_array - but we can't use it for sample
	// code for compiler restrictions
	unsigned char* rawData = new unsigned char[lSize];
	size_t readCount = fread(rawData, sizeof(const char), lSize, inFile);
	if( readCount != lSize )
	{	// oops, we didn't manage to read the file in.  In your implementation
		// you may retry to allow for partial reads or retry.  For this sample
		// we're giving up.
		return;
	}
	
	// a pointer to hold my decompressed data
	unsigned char* data = 0;

	// I check the first three characters, if they're FWS (SWF backwards) then
	// we're not compressed, if it's CWS then we need to decompress - to save
	// time here I'm not going to check all three - but you may want to
	// to be sure it's a real SWF.
	if( rawData[0] == 'C' )
	{	// Ok, we're decompressing - find out how long it is
		U32 fullLen = (U32)rawData[4] |
						((U32)rawData[5]<<8) | 
						((U32)rawData[6]<<16) |
						((U32)rawData[7]<<24);
		// we need a new buffer for our full length - it's worth
		// noting that I'm allocating heap memory for the whole
		// SWF file here - FMS won't do this for large files and neither
		// should you.  One should load chunks of file, decompress them,
		// and hash them progressively.  This is a more sophisticated
		// operation than this sample provides currently.
		unsigned char* newData = new unsigned char[fullLen];
		// now I'll copy my necessary pieces into it - I'm basically making
		// this buffer as if it were a decompressed SWF
		static const size_t kHeaderLength = 8;
		memcpy(newData, rawData, kHeaderLength);
		// don't forget to fix the compression byte
		newData[0] = 'F';

		// and now we use zlib to fix the rest - assuming you included
		// and linked it
#ifdef ZLIB
		z_stream decompressor;
		decompressor.next_in		= rawData+kHeaderLength;
		decompressor.avail_in		= lSize-kHeaderLength;
		decompressor.next_out		= newData+kHeaderLength;
		decompressor.avail_out		= fullLen-kHeaderLength;

		decompressor.zalloc = (alloc_func) NULL;
		decompressor.zfree = (free_func) NULL;
		decompressor.opaque = (voidpf) NULL;

		// initialize the compressor
		if( inflateInit_(&decompressor, ZLIB_VERSION, sizeof(decompressor)) == Z_OK )
		{
			if( inflate(&decompressor, Z_FINISH) == Z_STREAM_END )	// expand the SWF
			{	// we should complete all decompression here, anything else is an error
			}
			if( inflateEnd(&decompressor) != Z_OK )	// deallocate the decompressor
			{	// also an error if we fail to deallocate the decompressor
			}
		}
#endif
		// there - if the decompression worked - we're done
		data = newData;
		// cleanup
		delete[] rawData;
		rawData = 0;
	}
	else
	{	// easy here, we already have the data we need
		data = rawData;
	}

	// Now we run HMAC on it - we didn't do some good things like check that
	// we have that much data - or that we didn't get null - but this is a simple
	// example - one should add full error checking and logging in release
	calcHMACSHA256(data, static_cast<unsigned long>(depth), kSalt,
		kSaltLength, digest);

	// cleanup time
	delete[] data;
	data = 0;
}

#endif
