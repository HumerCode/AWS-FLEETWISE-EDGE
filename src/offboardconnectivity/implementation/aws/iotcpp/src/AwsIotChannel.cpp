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

#include "AwsIotChannel.h"
#include "AwsIotConnectivityModule.h"
#include "TraceModule.h"
#include <sstream>

using namespace Aws::IoTFleetWise::OffboardConnectivityAwsIot;
using namespace Aws::IoTFleetWise::OffboardConnectivity;
using namespace Aws::Crt;

AwsIotChannel::AwsIotChannel( IConnectivityModule *connectivityModule,
                              std::shared_ptr<PayloadManager> payloadManager,
                              std::size_t maximumIotSDKHeapMemoryBytes )
    : mMaximumIotSDKHeapMemoryBytes( maximumIotSDKHeapMemoryBytes )
    , mConnectivityModule( connectivityModule )
    , mPayloadManager( std::move( payloadManager ) )
    , mSubscribed( false )
    , mSubscribeAsynchronously( false )
{
}

bool
AwsIotChannel::isAlive()
{
    std::lock_guard<std::mutex> connectivityLock( mConnectivityMutex );
    return isAliveNotThreadSafe();
}

bool
AwsIotChannel::isAliveNotThreadSafe()
{
    if ( mConnectivityModule == nullptr )
    {
        return false;
    }
    return mConnectivityModule->isAlive();
}

void
AwsIotChannel::setTopic( const std::string &topicNameRef, bool subscribeAsynchronously )
{
    if ( topicNameRef.empty() )
    {
        mLogger.error( "AwsIotChannel::setTopic", "Empty ingestion topic name provided" );
    }
    mSubscribeAsynchronously = subscribeAsynchronously;
    mTopicName = topicNameRef;
}

ConnectivityError
AwsIotChannel::subscribe()
{
    std::lock_guard<std::mutex> connectivityLock( mConnectivityMutex );
    if ( !isTopicValid() )
    {
        mLogger.error( "AwsIotChannel::subscribe", "Empty ingestion topic name provided" );
        return ConnectivityError::NotConfigured;
    }
    if ( !isAliveNotThreadSafe() )
    {
        mLogger.error( "AwsIotChannel::subscribe", "MQTT Connection not established, failed to subscribe" );
        return ConnectivityError::NoConnection;
    }
    auto connection = mConnectivityModule->getConnection();
    /*
     * This is invoked upon the reception of a message on a subscribed topic.
     */
    auto onMessage = [&]( Mqtt::MqttConnection &mqttConnection,
                          const String &topic,
                          const ByteBuf &byteBuf,
                          bool dup /*dup*/,
                          Mqtt::QOS /*qos*/,
                          bool retain /*retain*/ ) {
        std::ostringstream os;
        (void)mqttConnection;
        (void)dup;
        (void)retain;
        os << "Message received on topic  " << topic << " payload length: " << byteBuf.len;
        mLogger.trace( "AwsIotChannel::subscribeTopic", os.str() );
        notifyListeners<const std::uint8_t *, size_t>(
            &IReceiverCallback::onDataReceived, byteBuf.buffer, byteBuf.len );
    };

    /*
     * Subscribe for incoming publish messages on topic.
     */
    std::promise<void> subscribeFinishedPromise;
    auto onSubAck = [&]( Mqtt::MqttConnection &mqttConnection,
                         uint16_t packetId,
                         const String &topic,
                         Mqtt::QOS QoS,
                         int errorCode ) {
        (void)mqttConnection;
        mSubscribed = false;
        if ( errorCode != 0 )
        {
            TraceModule::get().incrementAtomicVariable( TraceAtomicVariable::SUBSCRIBE_ERROR );
            mLogger.error( "AwsIotChannel::subscribeTopic", "Subscribe failed with error" );
            mLogger.error( "AwsIotChannel::subscribeTopic", aws_error_debug_str( errorCode ) );
        }
        else
        {
            if ( packetId == 0u || QoS == Mqtt::QOS::AWS_MQTT_QOS_FAILURE )
            {
                TraceModule::get().incrementAtomicVariable( TraceAtomicVariable::SUBSCRIBE_REJECT );
                mLogger.error( "AwsIotChannel::subscribeTopic", "Subscribe rejected by the Remote broker." );
            }
            else
            {
                std::ostringstream os;
                os << "Subscribe on topic  " << topic << " on packetId " << packetId << " succeeded" << std::endl;
                mLogger.trace( "AwsIotChannel::subscribeTopic", os.str() );
                mSubscribed = true;
            }
            subscribeFinishedPromise.set_value();
        }
    };

    mLogger.trace( "AwsIotChannel::subscribeTopic", "Subscribing.." );
    connection->Subscribe( mTopicName.c_str(), Mqtt::QOS::AWS_MQTT_QOS_AT_LEAST_ONCE, onMessage, onSubAck );

    // Blocked call until subscribe finished this call should quickly either fail or succeed but
    // depends on the network quality the Bootstrap needs to retry subscribing if failed.
    subscribeFinishedPromise.get_future().wait();

    if ( !mSubscribed )
    {
        return ConnectivityError::NoConnection;
    }

    return ConnectivityError::Success;
}

size_t
AwsIotChannel::getMaxSendSize() const
{
    return AWS_IOT_MAX_MESSAGE_SIZE;
}

ConnectivityError
AwsIotChannel::send( const std::uint8_t *buf, size_t size, struct CollectionSchemeParams collectionSchemeParams )
{
    std::lock_guard<std::mutex> connectivityLock( mConnectivityMutex );
    if ( !isTopicValid() )
    {
        mLogger.warn( "AwsIotChannel::send", "Invalid topic provided" );
        return ConnectivityError::NotConfigured;
    }

    if ( buf == nullptr || size == 0 )
    {
        mLogger.warn( "AwsIotChannel::send", "No valid data provided" );
        return ConnectivityError::WrongInputData;
    }

    if ( size > getMaxSendSize() )
    {
        mLogger.warn( "AwsIotChannel::send", "Payload provided is too long" );
        return ConnectivityError::WrongInputData;
    }

    if ( !isAliveNotThreadSafe() )
    {
        mLogger.warn( "AwsIotChannel::send", "There is no active MQTT Connection." );
        if ( mPayloadManager != nullptr )
        {
            bool isDataPersisted = mPayloadManager->storeData( buf, size, collectionSchemeParams );

            if ( isDataPersisted )
            {
                mLogger.trace( "AwsIotChannel::send", "Payload has persisted successfully on disk" );
            }
            else
            {
                mLogger.warn( "AwsIotChannel::send", "Payload has not been persisted" );
            }
        }
        return ConnectivityError::NoConnection;
    }

    uint64_t currentMemoryUsage = mConnectivityModule->reserveMemoryUsage( size );
    if ( mMaximumIotSDKHeapMemoryBytes != 0 && currentMemoryUsage > mMaximumIotSDKHeapMemoryBytes )
    {
        mConnectivityModule->releaseMemoryUsage( size );
        mLogger.error( "AwsIotChannel::send",
                       "Not sending out the message  with size " + std::to_string( size ) +
                           " because IoT device SDK allocated the maximum defined memory. Currently allocated " +
                           std::to_string( currentMemoryUsage ) );
        if ( mPayloadManager != nullptr )
        {
            bool isDataPersisted = mPayloadManager->storeData( buf, size, collectionSchemeParams );

            if ( isDataPersisted )
            {
                mLogger.trace( "AwsIotChannel::send", "Data was persisted successfully" );
            }
            else
            {
                mLogger.warn( "AwsIotChannel::send", "Data was not persisted and is lost" );
            }
        }
        return ConnectivityError::QuotaReached;
    }

    auto connection = mConnectivityModule->getConnection();

    auto payload = ByteBufNewCopy( DefaultAllocator(), (const uint8_t *)buf, size );

    auto onPublishComplete =
        [payload, size, this]( Mqtt::MqttConnection &mqttConnection, uint16_t packetId, int errorCode ) {
            /* This call means that the data was handed over to some lower level in the stack but not
                that the data is actually sent on the bus or removed from RAM*/
            (void)mqttConnection;
            aws_byte_buf_clean_up( (Aws::Crt::ByteBuf *)&payload ); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
            {
                std::lock_guard<std::mutex> connectivityLambdaLock( mConnectivityLambdaMutex );
                if ( mConnectivityModule != nullptr )
                {
                    mConnectivityModule->releaseMemoryUsage( size );
                }
            }
            if ( packetId != 0U && errorCode == 0 )
            {
                mLogger.trace( "AwsIotChannel::send",
                               "Operation on packetId  " + std::to_string( packetId ) + " Succeeded" );
            }
            else
            {
                mLogger.error( "AwsIotChannel::send",
                               std::string( "Operation failed with error" ) + aws_error_debug_str( errorCode ) );
            }
        };
    connection->Publish( mTopicName.c_str(), Mqtt::QOS::AWS_MQTT_QOS_AT_MOST_ONCE, false, payload, onPublishComplete );
    return ConnectivityError::Success;
}

bool
AwsIotChannel::unsubscribe()
{
    std::lock_guard<std::mutex> connectivityLock( mConnectivityMutex );
    if ( mSubscribed && isAliveNotThreadSafe() )
    {
        auto connection = mConnectivityModule->getConnection();

        std::promise<void> unsubscribeFinishedPromise;
        mLogger.trace( "AwsIotChannel::unsubscribe", "Unsubscribing ..." );
        connection->Unsubscribe( mTopicName.c_str(),
                                 [&]( Mqtt::MqttConnection &mqttConnection, uint16_t packetId, int errorCode ) {
                                     (void)mqttConnection;
                                     (void)packetId;
                                     (void)errorCode;
                                     mLogger.trace( "AwsIotChannel::unsubscribe", "Unsubscribed" );
                                     unsubscribeFinishedPromise.set_value();
                                 } );
        // Blocked call until subscribe finished this call should quickly either fail or succeed but
        // depends on the network quality the Bootstrap needs to retry subscribing if failed.
        unsubscribeFinishedPromise.get_future().wait();
        return true;
    }
    return false;
}

AwsIotChannel::~AwsIotChannel()
{
    unsubscribe();
}
