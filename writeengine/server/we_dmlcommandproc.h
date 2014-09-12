/* Copyright (C) 2014 InfiniDB, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of
   the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA. */

/*******************************************************************************
* $Id: we_ddlcommandproc.h 3043 2011-08-29 22:03:03Z chao $
*
*******************************************************************************/
#ifndef WE_DMLCOMMANDPROC_H__
#define WE_DMLCOMMANDPROC_H__

#include <unistd.h>
#include <boost/scoped_ptr.hpp>
#include "bytestream.h"
#include "we_messages.h"
#include "dbrm.h"
#include "we_message_handlers.h"
#include "calpontdmlpackage.h"
#include "updatedmlpackage.h"
#include "calpontsystemcatalog.h"
#include "insertdmlpackage.h"
#include "liboamcpp.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include "dataconvert.h"
#include "writeengine.h"
#include "we_convertor.h"
#include "we_dbrootextenttracker.h"
#include "we_rbmetawriter.h"
#include "rowgroup.h"
#include "we_log.h"
#if defined(_MSC_VER) && defined(DDLPKGPROC_DLLEXPORT)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
namespace WriteEngine
{

class WE_DMLCommandProc
{
	public:
	typedef std::vector<std::string> ColValues;
	
		EXPORT WE_DMLCommandProc();
		EXPORT WE_DMLCommandProc(const WE_DMLCommandProc& rhs);
		EXPORT ~WE_DMLCommandProc();
		inline void isFirstBatchPm (bool firstBatch)
		{
			fIsFirstBatchPm = firstBatch;
		}
		
		inline bool isFirstBatchPm ()
		{
			return fIsFirstBatchPm;
		}
		
		inline void convertRidToColumn (u_int64_t& rid, uint32_t& partition, uint16_t& segment)
		{
			partition = rid / ( filesPerColumnPartition * extentsPerSegmentFile * extentRows );
		
			segment =( ( ( rid % ( filesPerColumnPartition * extentsPerSegmentFile * extentRows )) / extentRows ) ) % filesPerColumnPartition;
		
			//Calculate the relative rid for this segment file
			u_int64_t relRidInPartition = rid - ((u_int64_t)partition * (u_int64_t)filesPerColumnPartition * (u_int64_t)extentsPerSegmentFile * (u_int64_t)extentRows);
			assert ( relRidInPartition <= (u_int64_t)filesPerColumnPartition * (u_int64_t)extentsPerSegmentFile * (u_int64_t)extentRows );
			uint32_t numExtentsInThisPart = relRidInPartition / extentRows;
			unsigned numExtentsInThisSegPart = numExtentsInThisPart / filesPerColumnPartition;
			u_int64_t relRidInThisExtent = relRidInPartition - numExtentsInThisPart * extentRows;
			rid = relRidInThisExtent +  numExtentsInThisSegPart * extentRows;
		}
	
		EXPORT uint8_t processSingleInsert(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t commitVersion(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t rollbackBlocks(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t rollbackVersion(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t processBatchInsert(messageqcpp::ByteStream& bs, std::string & err, ByteStream::quadbyte & PMId);
		EXPORT uint8_t commitBatchAutoOn(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t commitBatchAutoOff(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t rollbackBatchAutoOn(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t rollbackBatchAutoOff(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t processBatchInsertHwm(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t processUpdate(messageqcpp::ByteStream& bs, std::string & err, ByteStream::quadbyte & PMId, uint64_t& blocksChanged);
		EXPORT uint8_t processUpdate1(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t processFlushFiles(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t processDelete(messageqcpp::ByteStream& bs, std::string & err, ByteStream::quadbyte & PMId, uint64_t& blocksChanged);
		EXPORT uint8_t processRemoveMeta(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t processBulkRollback(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t processBulkRollbackCleanup(messageqcpp::ByteStream& bs, std::string & err);
		EXPORT uint8_t updateSyscolumnNextval(ByteStream& bs, std::string & err);
		int validateColumnHWMs(
				execplan::CalpontSystemCatalog::RIDList& ridList,
				boost::shared_ptr<execplan::CalpontSystemCatalog> systemCatalogPtr,
				const std::vector<DBRootExtentInfo>& segFileInfo,
				const char* stage );
	private:	
		WriteEngineWrapper fWEWrapper;
		boost::scoped_ptr<RBMetaWriter> fRBMetaWriter;
		std::vector<DBRootExtentTracker*>   dbRootExtTrackerVec;
		inline bool isDictCol ( execplan::CalpontSystemCatalog::ColType colType )
		{
			if (((colType.colDataType == execplan::CalpontSystemCatalog::CHAR) && (colType.colWidth > 8)) 
				|| ((colType.colDataType == execplan::CalpontSystemCatalog::VARCHAR) && (colType.colWidth > 7)) 
				|| ((colType.colDataType == execplan::CalpontSystemCatalog::DECIMAL) && (colType.precision > 18))
				|| (colType.colDataType == execplan::CalpontSystemCatalog::VARBINARY)) 
			{
				return true;
			}
			else
				return false;
		}

		bool fIsFirstBatchPm;
		std::map<uint32_t,rowgroup::RowGroup *> rowGroups;
		std::map<uint32_t, dmlpackage::UpdateDMLPackage> cpackages;
		BRM::DBRM fDbrm;
		unsigned  extentsPerSegmentFile, extentRows, filesPerColumnPartition, dbrootCnt;
		Log fLog;
	
};
}
#undef EXPORT
#endif