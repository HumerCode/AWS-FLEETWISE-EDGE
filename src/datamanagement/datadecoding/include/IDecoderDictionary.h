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
#include "IDecoderManifest.h"
#include <unordered_map>
#include <unordered_set>

namespace Aws
{
namespace IoTFleetWise
{
namespace DataManagement
{

/**
 * @brief enum class for message collect type. It specifies whether the message is intended to
 * be collected as decoded or kept as raw format or have both formats
 * Below are three type of possible collection type:
 *
 * DECODE: Indicates this CAN Message shall be collected as decoded message
 * RAW: Indicates this CAN Message shall be collected as raw message
 * RAW_AND_DECODE: Indicates this CAN Message shall be collected as both raw and decoded format
 */
enum class CANMessageCollectType
{
    DECODE = 0,
    RAW,
    RAW_AND_DECODE
};

/**
 * @brief This struct define the necessary information required to decode CAN frame
 *
 * nodeID: VSS ID of the CAN node. This is not the physical Channel ID.
 * collectType: specify whether the message is intended to be decoded or kept as raw or both
 * format: CAN message format specifying the frame ID, number of bytes and whether it's Multiplexed.
 * Note the format only contains the signals intended to be collected.
 */
struct CANMessageDecoderMethod
{
    CANMessageCollectType collectType;
    CANMessageFormat format;
};

/**
 * @brief Base class of Decoder Dictionary. It contains a set of signal IDs to collect
 */
struct DecoderDictionary
{
    DecoderDictionary() = default;
    DecoderDictionary( std::unordered_set<SignalID> signalIDs )
        : signalIDsToCollect( std::move( signalIDs ) )
    {
    }
    virtual ~DecoderDictionary() = default;
    std::unordered_set<SignalID> signalIDsToCollect;
};

/**
 * @brief Metadata regarding a signal
 *
 */
struct SignalMetaData
{
    SignalMetaData( SignalID pSignalId, SignalDataType pDataType )
        : signalId( pSignalId )
        , dataType( pDataType )
    {
    }
    SignalID signalId;
    SignalDataType dataType;
};

inline bool
operator<( const SignalMetaData &lhs, const SignalMetaData &rhs )
{
    return lhs.signalId < rhs.signalId;
}

inline bool
operator==( const SignalMetaData &lhs, const SignalMetaData &rhs )
{
    return lhs.signalId == rhs.signalId && lhs.dataType == rhs.dataType;
}

/**
 * @brief Synthetic Data decoder dictionary to be used to decode messages based on signal data type.
 *
 * The key of dictionary is the Message Identifier. The value is a list of signal data primitive types.
 */
struct SyntheticDecoderDictionary : DecoderDictionary
{
    std::unordered_map<uint64_t, std::vector<SignalMetaData>> genericMessageDecoderMethod;
};

/**
 * @brief CAN decoder dictionary to be used to decode CAN Frame message to signals. This dictionary comes from
 * CollectionScheme Management
 *
 * canMessageDecoderMethod is a two dimension map. The top layer index is CANChannelNumericID; second layer index is
 * CAN Frame ID which is the CAN Arbitration ID
 * signalIDsToCollect is an unordered_set to specify which SignalID to be collected based on the CollectionScheme
 * pidMessageDecoderMethod is a map from OBD-II PID to its decoding method
 */
struct CANDecoderDictionary : DecoderDictionary
{
    using CANMsgDecoderMethodType =
        std::unordered_map<CANChannelNumericID, std::unordered_map<CANRawFrameID, CANMessageDecoderMethod>>;
    CANDecoderDictionary() = default;
    CANDecoderDictionary( CANMsgDecoderMethodType canMsgDecoderMethod, std::unordered_set<SignalID> signalIDsToCollect )
        : DecoderDictionary( std::move( signalIDsToCollect ) )
        , canMessageDecoderMethod( std::move( canMsgDecoderMethod ) )
    {
    }
    CANMsgDecoderMethodType canMessageDecoderMethod;
};

/**
 * @brief define shared pointer type for CAN Frame decoder dictionary
 */
using ConstDecoderDictionaryConstPtr = const std::shared_ptr<const DecoderDictionary>;

} // namespace DataManagement
} // namespace IoTFleetWise
} // namespace Aws