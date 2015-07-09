/*----------------------------------------------------------------------------+
 |       ___     _       _                                                    |
 |      /   |   | |     | |                                                   |
 |     / /| | __| | ___ | |__   ___                                           |
 |    / /_| |/ _  |/ _ \|  _ \ / _ \                                          |
 |   / ___  | (_| | (_) | |_) |  __/                                          |
 |  /_/   |_|\__,_|\___/|____/ \___|                                          |
 |                                                                            |
 |  Copyright (c) 2006, Adobe Systems Incorporated.                           |
 |  All rights reserved.                                                      |
 |                                                                            |
 |  Unpublished -- Rights reserved under the copyright laws of the United     |
 |  States.  Use of a copyright notice is precautionary only and does not     |
 |  imply publication or disclosure.                                          |
 |                                                                            |
 |  This software contains confidential information and trade secrets of      |
 |  Adobe Systems Incorporated.  Use, disclosure, or reproduction is          |
 |  prohibited without the prior express written permission of Adobe Systems  |
 |  Incorporated.                                                             |
 |                                                                            |
 |  RESTRICTED RIGHTS LEGEND                                                  |
 |                                                                            |
 |  Use, duplication, or disclosure by the Government is subject to           |
 |  restrictions as set forth in subparagraph (c)(1)(ii) of the Rights in     |
 |  Technical Data and Computer Software clause at DFARS 252.227-7013.        |
 |                                                                            |
 |      Adobe Systems Incorporated                      415.832.2000          |
 |      601 Townsend Street								415.832.2020 fax      |
 |      San Francisco, CA 94103                                               |
 |                                                                            |
 +----------------------------------------------------------------------------*/

#ifndef FILEMODULE_H
#define FILEMODULE_H

#pragma once

extern "C" void FCExport FmsCreateFileAdaptor(IFmsServerContext* pServerCtx, IFmsFileResponse* pResponse, IFmsFileAdaptor*& pFileAdaptor );
extern "C" void FCExport FmsDestroyFileAdaptor(IFmsFileAdaptor* pFileAdaptor);

#endif
