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

// Includes
#include "CollectionInspectionAPITypes.h"
#include "CollectionSchemeManagementListener.h"
#include "ICacheAndPersist.h"
#include "ICollectionScheme.h"
namespace Aws
{
namespace IoTFleetWise
{
namespace DataManagement
{
using namespace Aws::IoTFleetWise::Platform::Linux::PersistencyManagement;
using namespace Aws::IoTFleetWise::DataInspection;
/* when TimePointInMsec is used, it should refer to milliseconds since Epoch */
using TimePointInMsec = uint64_t;

class ICollectionSchemeManager
{
public:
    virtual ~ICollectionSchemeManager() = default;

    /**
     * @brief callback for CollectionScheme Ingestion to send update of ICollectionSchemeList
     * @param collectionSchemeList a constant shared pointer to ICollectionSchemeList from CollectionScheme Ingestion
     */
    virtual void onCollectionSchemeUpdate( const ICollectionSchemeListPtr &collectionSchemeList ) = 0;

    /**
     * @brief callback for CollectionScheme Ingestion to send update of IDecoderManifest
     * @param decoderManifest a constant shared pointer to IDecoderManifest from CollectionScheme Ingestion
     */
    virtual void onDecoderManifestUpdate( const IDecoderManifestPtr &decoderManifest ) = 0;

protected:
    /**
     * @brief Rebuilds enabled collectionScheme map, idle collectionScheme map, and time line;
     * This function runs after change of decoderManifest is detected.
     *
     * @param currTime time in seconds when main thread wakes up;
     * @return true when enabled collectionScheme map is updated.
     */
    virtual bool rebuildMapsandTimeLine( const TimePointInMsec &currTime ) = 0;

    /**
     * @brief Updates existing enabled collectionScheme map, idle collectionScheme map, and time line;
     * This function runs when receiving an update from CollectionScheme Ingestion with no decoderManifest change.
     * It detects new collectionScheme on the list and updates collectionScheme maps and time line, as well as missing
     * collectionSchemes, removes them from maps and calls callback functions when needed.
     *
     * @param currTime time in seconds when main thread wakes up;
     * @return true when enabled collectionScheme map is updated.
     */
    virtual bool updateMapsandTimeLine( const TimePointInMsec &currTime ) = 0;

    /**
     * @brief works on TimeData popped from mTimeLine to decide whether to disable/enable a collectionScheme
     *
     * @param currTime time in seconds when main thread wakes up;
     * @return true when enabled collectionScheme map is updated.
     */
    virtual bool checkTimeLine( const TimePointInMsec &currTime ) = 0;

    /**
     * @brief Extract from enabled collectionSchemes and aggregate into inspectMatrix
     *
     * @param inspectionMatrix the inspectionMatrix object to be filled
     *
     */
    virtual void inspectionMatrixExtractor( const std::shared_ptr<InspectionMatrix> &inspectionMatrix ) = 0;

    /**
     * @brief This function invoke all the listener for inspectMatrix update
     *
     * @param inspectionMatrix the inspectionMatrix object to be filled
     *
     */
    virtual void inspectionMatrixUpdater( const std::shared_ptr<const InspectionMatrix> &inspectionMatrix ) = 0;

    /**
     * @brief Retrieves Protobuf-ed collectionSchemeList or decoderManifest from
     * persistent memory
     *
     * @param retrieveType ENUM DataType defined in CacheAndPersist.h
     */
    virtual bool retrieve( DataType retrieveType ) = 0;

    /**
     * @brief Stores protobuf-ed collectionSchemeList or DecoderManifest to
     * persistent memory
     *
     * @param storeType ENUM DataType defined in CacheAndPersist.h
     *
     */
    virtual void store( DataType storeType ) = 0;

    /**
     * @brief processes proto-bufed decoder manifest
     *
     */
    virtual bool processDecoderManifest() = 0;

    /**
     * @brief processes proto-bufed collectionScheme
     *
     */
    virtual bool processCollectionScheme() = 0;

    /**
     * @brief pack checkin message and send out
     * @return True if the Connectivity Module packed and send the data out of the process space.
     * It does not guarantee that the data reached the Checkin topic ( best effort QoS )
     */
    virtual bool sendCheckin() = 0;

    /**
     * @brief checks if mCollectionSchemeAvailable and mDecoderManifestAvailable is set and
     * copies pointers out of critical section
     *
     */
    virtual void updateAvailable() = 0;
};

} // namespace DataManagement
} // namespace IoTFleetWise
} // namespace Aws
