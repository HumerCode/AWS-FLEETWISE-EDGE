/**
 * Copyright 2020 Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-.amazon.com.-AmznSL-1.0
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 * http://aws.amazon.com/asl/
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#pragma once

#include "CacheAndPersist.h"
#include "ClockHandler.h"
#include "CollectionSchemeIngestion.h"
#include "CollectionSchemeIngestionList.h"
#include "CollectionSchemeManagementListener.h"
#include "CollectionSchemeManager.h"
#include "DecoderManifestIngestion.h"
#include "Listener.h"
#include "LoggingModule.h"
#include <algorithm>
#include <atomic>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace Aws::IoTFleetWise::Platform::Linux::PersistencyManagement;
using namespace Aws::IoTFleetWise::DataManagement;
using Aws::IoTFleetWise::Platform::Linux::ThreadListeners;

#define SECOND_TO_MILLISECOND( x ) ( 1000 ) * ( x )
using uint8Ptr = std::uint8_t *;
using vectorUint8 = std::vector<uint8_t>;
using vectorICollectionSchemePtr = std::vector<ICollectionSchemePtr>;

class mockCollectionSchemeManagerTest : public CollectionSchemeManager
{
public:
    mockCollectionSchemeManagerTest( std::string dm_id )
        : CollectionSchemeManager( dm_id )
    {
    }
    mockCollectionSchemeManagerTest( std::string dm_id,
                                     std::map<std::string, ICollectionSchemePtr> &mapEnabled,
                                     std::map<std::string, ICollectionSchemePtr> &mapIdle )
        : CollectionSchemeManager( dm_id, mapEnabled, mapIdle )
    {
    }
    // void inspectionMatrixExtractor( const std::shared_ptr<InspectionMatrix> &inspectionMatrix ) override;
    using sharedPtrInspectionMatrix = std::shared_ptr<InspectionMatrix>;
    MOCK_METHOD( void, inspectionMatrixExtractor, (const sharedPtrInspectionMatrix &));

    // void inspectionMatrixUpdater( const std::shared_ptr<const InspectionMatrix> &inspectionMatrix ) override;
    using sharedPtrConstInspectionMatrix = std::shared_ptr<const InspectionMatrix>;
    MOCK_METHOD( void, inspectionMatrixUpdater, (const sharedPtrConstInspectionMatrix &));

    void
    setCollectionSchemePersistency( const std::shared_ptr<ICacheAndPersist> &collectionSchemePersistency )
    {
        CollectionSchemeManager::mSchemaPersistency = collectionSchemePersistency;
    }
    void
    setDecoderManifest( const IDecoderManifestPtr &dm )
    {
        // CollectionSchemeManager::mDecoderManifest = dm;
        mDecoderManifest = dm;
    }
    void
    setCollectionSchemeList( const ICollectionSchemeListPtr &pl )
    {
        mCollectionSchemeList = pl;
    }

    void
    setTimeLine( const std::priority_queue<TimeData, std::vector<TimeData>, std::greater<TimeData>> &TimeLine )
    {
        mTimeLine = TimeLine;
    }

    std::priority_queue<TimeData, std::vector<TimeData>, std::greater<TimeData>>
    getTimeLine()
    {
        return mTimeLine;
    }

    bool
    getmProcessCollectionScheme()
    {
        return mProcessCollectionScheme;
    }

    bool
    getmProcessDecoderManifest()
    {
        return mProcessDecoderManifest;
    }

    bool
    rebuildMapsandTimeLine( const TimePointInMsec &currTime )
    {
        return CollectionSchemeManager::rebuildMapsandTimeLine( currTime );
    }

    bool
    updateMapsandTimeLine( const TimePointInMsec &currTime )
    {
        return CollectionSchemeManager::updateMapsandTimeLine( currTime );
    }

    bool
    checkTimeLine( const TimePointInMsec &currTime )
    {
        return CollectionSchemeManager::checkTimeLine( currTime );
    }

    bool
    sendCheckin()
    {
        return CollectionSchemeManager::sendCheckin();
    }

    void
    store( DataType storeType )
    {
        CollectionSchemeManager::store( storeType );
    }

    bool
    retrieve( DataType retrieveType )
    {
        return CollectionSchemeManager::retrieve( retrieveType );
    }
};

class mockCollectionScheme : public CollectionSchemeIngestion
{
public:
    // bool build() override;
    MOCK_METHOD( bool, build, () );
    // const std::string &getCollectionSchemeID()
    MOCK_METHOD( const std::string &, getCollectionSchemeID, (), ( const ) );

    // virtual const std::string &getDecoderManifestID() const = 0;
    MOCK_METHOD( const std::string &, getDecoderManifestID, (), ( const ) );
    // virtual uint64_t getStartTime() const = 0;
    MOCK_METHOD( uint64_t, getStartTime, (), ( const ) );
    // virtual uint64_t getExpiryTime() const = 0;
    MOCK_METHOD( uint64_t, getExpiryTime, (), ( const ) );
};

class mockDecoderManifest : public DecoderManifestIngestion
{
public:
    // virtual std::string getID() const = 0;
    MOCK_METHOD( std::string, getID, (), ( const ) );

    // bool build() override;
    MOCK_METHOD( bool, build, () );

    // bool copyData( const std::uint8_t *inputBuffer, const size_t size ) override;
    MOCK_METHOD( bool, copyData, ( const std::uint8_t *, const size_t ) );

    // inline const std::vector<uint8_t>getData() const override
    MOCK_METHOD( const std::vector<uint8_t> &, getData, (), ( const ) );
};

class mockCollectionSchemeList : public CollectionSchemeIngestionList
{
public:
    // bool build() override;
    MOCK_METHOD( bool, build, () );

    // const std::vector<ICollectionSchemePtr> &getCollectionSchemes() const override;
    MOCK_METHOD( const std::vector<ICollectionSchemePtr> &, getCollectionSchemes, (), ( const ) );

    MOCK_METHOD( bool, copyData, ( const std::uint8_t *, const size_t ) );
    MOCK_METHOD( const std::vector<uint8_t> &, getData, (), ( const ) );
};

class mockCacheAndPersist : public CacheAndPersist
{
public:
    mockCacheAndPersist()
        : CacheAndPersist( "", 0 )
    {
    }
    // ErrorCode write( const uint8_t *bufPtr, size_t size, DataType dataType );
    MOCK_METHOD( ErrorCode, write, ( const uint8_t *, size_t, DataType ) );

    // size_t getSize( DataType dataType );
    MOCK_METHOD( size_t, getSize, ( DataType ) );

    // ErrorCode read( uint8_t *const readBufPtr, size_t size, DataType dataType );
    MOCK_METHOD( ErrorCode, read, ( uint8_t *const, size_t, DataType ) );
};