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

#include "CollectionInspectionAPITypes.h"
#include "ICollectionScheme.h"
#include "LoggingModule.h"
#include <unordered_map>

namespace Aws
{
namespace IoTFleetWise
{
namespace DataManagement
{

using namespace Aws::IoTFleetWise::Platform::Linux;
using namespace Aws::IoTFleetWise::Schemas;

/**
 * @brief DecoderManifestIngestion (PI = Schema) is the implementation of ICollectionScheme used by
 * Schema.
 */
class CollectionSchemeIngestion : public ICollectionScheme
{
public:
    CollectionSchemeIngestion() = default;
    ~CollectionSchemeIngestion() override;

    CollectionSchemeIngestion( const CollectionSchemeIngestion & ) = delete;
    CollectionSchemeIngestion &operator=( const CollectionSchemeIngestion & ) = delete;
    CollectionSchemeIngestion( CollectionSchemeIngestion && ) = delete;
    CollectionSchemeIngestion &operator=( CollectionSchemeIngestion && ) = delete;

    bool isReady() const override;

    bool build() override;

    bool copyData( std::shared_ptr<CollectionSchemesMsg::CollectionScheme> protoCollectionSchemeMessagePtr );

    const std::string &getCollectionSchemeID() const override;

    const std::string &getDecoderManifestID() const override;

    uint64_t getStartTime() const override;

    uint64_t getExpiryTime() const override;

    uint32_t getAfterDurationMs() const override;

    bool isActiveDTCsIncluded() const override;

    bool isTriggerOnlyOnRisingEdge() const override;

    double getProbabilityToSend() const override;

    const Signals_t &getCollectSignals() const override;

    const RawCanFrames_t &getCollectRawCanFrames() const override;

    const ImagesDataType &getImageCaptureData() const override;

    bool isPersistNeeded() const override;

    bool isCompressionNeeded() const override;

    uint32_t getPriority() const override;

    const struct ExpressionNode *getCondition() const override;

    uint32_t getMinimumPublishIntervalMs() const override;

    const ExpressionNode_t &getAllExpressionNodes() const override;

private:
    /**
     * @brief The CollectionScheme message that will hold the deserialized proto.
     */
    std::shared_ptr<CollectionSchemesMsg::CollectionScheme> mProtoCollectionSchemeMessagePtr;

    /**
     * @brief Flag which is true if proto binary data is processed into readable data structures.
     */
    bool mReady{ false };

    /**
     * @brief Vector of all the signals that need to be collected/monitored
     */
    Signals_t mCollectedSignals;

    /**
     * @brief Vector of all the CAN Messages that need to be collected/monitored
     */
    RawCanFrames_t mCollectedRawCAN;

    /**
     * @brief Vector of Image Capture metadata.
     */
    ImagesDataType mImagesCaptureData;

    /**
     * @brief Expression Node Pointer to the Tree Root
     */
    ExpressionNode *mExpressionNode{ nullptr };

    /**
     * @brief Vector representing all of the ExpressionNode(s)
     */
    ExpressionNode_t mExpressionNodes;

    /**
     * @brief Function used to Flatten the Abstract Syntax Tree (AST)
     */
    ExpressionNode *serializeNode( const CollectionSchemesMsg::ConditionNode &node,
                                   uint32_t &nextIndex,
                                   const int depth );

    /**
     * @brief Helper function that returns all nodes in the AST by doing a recursive traversal
     *
     * @param node Root node of the AST
     * @param depth Used recursively to find the depth. Start with maximum depth.
     *
     * @return Returns the number of nodes in the AST
     */
    uint32_t getNumberOfNodes( const CollectionSchemesMsg::ConditionNode &node, const int depth );

    /**
     * @brief  Private Local Function used by the serializeNode Function to return the used Function Type
     */
    WindowFunction convertFunctionType(
        CollectionSchemesMsg::ConditionNode_NodeFunction_WindowFunction_WindowType function );

    /**
     * @brief Private Local Function used by the serializeNode Function to return the used Operator Type
     */
    ExpressionNodeType convertOperatorType( CollectionSchemesMsg::ConditionNode_NodeOperator_Operator op );

    /**
     * @brief Logging module used to output to logs
     */
    LoggingModule mLogger;
};
} // namespace DataManagement
} // namespace IoTFleetWise
} // namespace Aws
