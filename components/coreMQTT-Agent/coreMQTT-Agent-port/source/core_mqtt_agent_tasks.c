/*
 * Lab-Project-coreMQTT-Agent 201215
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 */

/**
 * @file core_mqtt_agent_tasks.c
 * @brief Functions to interact with coreMQTT agent.
 */


/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "list.h"

/* MQTT library includes. */
#include "core_mqtt.h"

/* MQTT agent include. */
#include "core_mqtt_agent.h"
#include "core_mqtt_agent_tasks.h"

/* Subscription manager header include. */
#include "subscription_manager.h"

/**
 * @brief This demo uses task notifications to signal tasks from MQTT callback
 * functions.  mqttexampleMS_TO_WAIT_FOR_NOTIFICATION defines the time, in ticks,
 * to wait for such a callback.
 */
#define mqttexampleMS_TO_WAIT_FOR_NOTIFICATION            ( 10000 )

/**
 * @brief Size of statically allocated buffers for holding topic names and
 * payloads.
 */
#define mqttexampleSTRING_BUFFER_LENGTH                   ( 100 )

#define mqttexampleSTRING_TOPIC_BUFFER_LENGTH             ( 100 )
#define mqttexampleSTRING_PAYLOAD_BUFFER_LENGTH           ( 2048 )

/**
 * @brief Number of publishes done by each task in this demo.
 */
#define mqttexamplePUBLISH_COUNT                          ( 0xffffffffUL )

/**
 * @brief The maximum amount of time in milliseconds to wait for the commands
 * to be posted to the MQTT agent should the MQTT agent's command queue be full.
 * Tasks wait in the Blocked state, so don't use any CPU time.
 */
#define mqttexampleMAX_COMMAND_SEND_BLOCK_TIME_MS         ( 500 )

/*-----------------------------------------------------------*/

/**
 * @brief Defines the structure to use as the command callback context in this
 * demo.
 */
struct MQTTAgentCommandContext
{
    MQTTStatus_t xReturnStatus;
    TaskHandle_t xTaskToNotify;
    uint32_t ulNotificationValue;
    void * pArgs;
};

/*-----------------------------------------------------------*/

/**
 * @brief Passed into MQTTAgent_Subscribe() as the callback to execute when the
 * broker ACKs the SUBSCRIBE message.  Its implementation sends a notification
 * to the task that called MQTTAgent_Subscribe() to let the task know the
 * SUBSCRIBE operation completed.  It also sets the xReturnStatus of the
 * structure passed in as the command's context to the value of the
 * xReturnStatus parameter - which enables the task to check the status of the
 * operation.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pxCommandContext Context of the initial command.
 * @param[in].xReturnStatus The result of the command.
 */
static void prvSubscribeCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,
                                         MQTTAgentReturnInfo_t * pxReturnInfo );

/**
 * @brief Passed into MQTTAgent_Publish() as the callback to execute when the
 * broker ACKs the PUBLISH message.  Its implementation sends a notification
 * to the task that called MQTTAgent_Publish() to let the task know the
 * PUBLISH operation completed.  It also sets the xReturnStatus of the
 * structure passed in as the command's context to the value of the
 * xReturnStatus parameter - which enables the task to check the status of the
 * operation.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pxCommandContext Context of the initial command.
 * @param[in].xReturnStatus The result of the command.
 */
static void prvPublishCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,
                                       MQTTAgentReturnInfo_t * pxReturnInfo );

/**
 * @brief Called by the task to wait for a notification from a callback function
 * after the task first executes either MQTTAgent_Publish()* or
 * MQTTAgent_Subscribe().
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pxCommandContext Context of the initial command.
 * @param[out] pulNotifiedValue The task's notification value after it receives
 * a notification from the callback.
 *
 * @return pdTRUE if the task received a notification, otherwise pdFALSE.
 */
static BaseType_t prvWaitForCommandAcknowledgment( uint32_t * pulNotifiedValue );

/*-----------------------------------------------------------*/

/**
 * @brief The MQTT agent manages the MQTT contexts.  This set the handle to the
 * context used by this demo.
 */
extern MQTTAgentContext_t xGlobalMqttAgentContext;

/*-----------------------------------------------------------*/

/**
 * @brief The buffer to hold the topic filter. The topic is generated at runtime
 * by adding the task names.
 *
 * @note The topic strings must persist until unsubscribed.
 */
static char payloadBuf[ mqttexampleSTRING_PAYLOAD_BUFFER_LENGTH ];

static char topicBuf[mqttexampleSTRING_TOPIC_BUFFER_LENGTH];

static void * vIncomingPublishCallback = NULL;
static void * vIncomingPublishCallbackContext = NULL;

extern MQTTAgentContext_t xGlobalMqttAgentContext;

/*-----------------------------------------------------------*/

void prvPublishCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,
                                       MQTTAgentReturnInfo_t * pxReturnInfo )
{
    /* Store the result in the application defined context so the task that
     * initiated the publish can check the operation's status. */
    pxCommandContext->xReturnStatus = pxReturnInfo->returnCode;

    if( pxCommandContext->xTaskToNotify != NULL )
    {
        /* Send the context's ulNotificationValue as the notification value so
         * the receiving task can check the value it set in the context matches
         * the value it receives in the notification. */
        xTaskNotify( pxCommandContext->xTaskToNotify,
                     pxCommandContext->ulNotificationValue,
                     eSetValueWithOverwrite );
    }
}

/*-----------------------------------------------------------*/

static void prvSubscribeCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,
                                         MQTTAgentReturnInfo_t * pxReturnInfo )
{
    bool xSubscriptionAdded = false;
    MQTTAgentSubscribeArgs_t * pxSubscribeArgs = ( MQTTAgentSubscribeArgs_t * ) pxCommandContext->pArgs;

    /* Store the result in the application defined context so the task that
     * initiated the subscribe can check the operation's status.  Also send the
     * status as the notification value.  These things are just done for
     * demonstration purposes. */
    pxCommandContext->xReturnStatus = pxReturnInfo->returnCode;

    /* Check if the subscribe operation is a success. Only one topic is
     * subscribed by this demo. */
    if( pxReturnInfo->returnCode == MQTTSuccess )
    {
        /* Add subscription so that incoming publishes are routed to the application
         * callback. */
        xSubscriptionAdded = addSubscription( ( SubscriptionElement_t * ) xGlobalMqttAgentContext.pIncomingCallbackContext,
                                              pxSubscribeArgs->pSubscribeInfo->pTopicFilter,
                                              pxSubscribeArgs->pSubscribeInfo->topicFilterLength,
                                              vIncomingPublishCallback,
                                              vIncomingPublishCallbackContext
                                            );

        if( xSubscriptionAdded == false )
        {
            LogError(( "Failed to register an incoming publish callback for topic %.*s.",
                      pxSubscribeArgs->pSubscribeInfo->topicFilterLength,
                      pxSubscribeArgs->pSubscribeInfo->pTopicFilter ));
        }
    }

    xTaskNotify( pxCommandContext->xTaskToNotify,
                 ( uint32_t ) ( pxReturnInfo->returnCode ),
                 eSetValueWithOverwrite );
}

/*-----------------------------------------------------------*/

static void prvUnsubscribeCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,
                                      MQTTAgentReturnInfo_t * pxReturnInfo )
{
    MQTTAgentSubscribeArgs_t * pxSubscribeArgs = ( MQTTAgentSubscribeArgs_t * ) pxCommandContext->pArgs;

    /* Store the result in the application defined context so the task that
     * initiated the Unsubscribe can check the operation's status.  Also send the
     * status as the notification value.  These things are just done for
     * demonstration purposes. */
    pxCommandContext->xReturnStatus = pxReturnInfo->returnCode;

    /* Check if the Unsubscribe operation is a success. Only one topic is
     * subscribed by this demo. */
    if( pxReturnInfo->returnCode == MQTTSuccess )
    {
        /* Add subscription so that incoming publishes are routed to the application
         * callback. */
        removeSubscription( ( SubscriptionElement_t * ) xGlobalMqttAgentContext.pIncomingCallbackContext,
                              pxSubscribeArgs->pSubscribeInfo->pTopicFilter,
                              pxSubscribeArgs->pSubscribeInfo->topicFilterLength
                            );
    }

    xTaskNotify( pxCommandContext->xTaskToNotify,
                 ( uint32_t ) ( pxReturnInfo->returnCode ),
                 eSetValueWithOverwrite );
}

/*-----------------------------------------------------------*/

static BaseType_t prvWaitForCommandAcknowledgment( uint32_t * pulNotifiedValue )
{
    BaseType_t xReturn;

    /* Wait for this task to get notified, passing out the value it gets
     * notified with. */
    xReturn = xTaskNotifyWait( 0,
                               0,
                               pulNotifiedValue,
                               pdMS_TO_TICKS( mqttexampleMS_TO_WAIT_FOR_NOTIFICATION ) );
    return xReturn;
}

/*-----------------------------------------------------------*/

BaseType_t mqttAgentSubscribe( MQTTQoS_t qos,
                         const char * pTopic,
                         const void * incomingPublishCallback,
                         const void * incomingPublishCallbackContext )
{
    MQTTStatus_t xCommandAdded;
    BaseType_t xCommandAcknowledged = pdFALSE;
    MQTTAgentSubscribeArgs_t xSubscribeArgs;
    MQTTSubscribeInfo_t xSubscribeInfo;
    static int32_t ulNextSubscribeMessageID = 0;
    MQTTAgentCommandContext_t xApplicationDefinedContext = { 0UL };
    MQTTAgentCommandInfo_t xCommandParams = { 0UL };

    vIncomingPublishCallback = incomingPublishCallback;
    vIncomingPublishCallbackContext = incomingPublishCallbackContext;

    /* Create a unique number of the subscribe that is about to be sent.  The number
     * is used as the command context and is sent back to this task as a notification
     * in the callback that executed upon receipt of the subscription acknowledgment.
     * That way this task can match an acknowledgment to a subscription. */
    xTaskNotifyStateClear( NULL );
    // taskENTER_CRITICAL(); 
    {
        ulNextSubscribeMessageID++;
    }
    // taskEXIT_CRITICAL();

    /* Complete the subscribe information.  The topic string must persist for
     * duration of subscription! */
    xSubscribeInfo.pTopicFilter = pTopic;
    xSubscribeInfo.topicFilterLength = ( uint16_t ) strlen( pTopic );
    xSubscribeInfo.qos = qos;
    xSubscribeArgs.pSubscribeInfo = &xSubscribeInfo;
    xSubscribeArgs.numSubscriptions = 1;

    /* Complete an application defined context associated with this subscribe message.
     * This gets updated in the callback function so the variable must persist until
     * the callback executes. */
    xApplicationDefinedContext.ulNotificationValue = ulNextSubscribeMessageID;
    xApplicationDefinedContext.xTaskToNotify = xTaskGetCurrentTaskHandle();
    xApplicationDefinedContext.pArgs = ( void * ) &xSubscribeArgs;

    xCommandParams.blockTimeMs = mqttexampleMAX_COMMAND_SEND_BLOCK_TIME_MS;
    xCommandParams.cmdCompleteCallback = prvSubscribeCommandCallback;
    xCommandParams.pCmdCompleteCallbackContext = ( void * ) &xApplicationDefinedContext;

    /* Loop in case the queue used to communicate with the MQTT agent is full and
     * attempts to post to it time out.  The queue will not become full if the
     * priority of the MQTT agent task is higher than the priority of the task
     * calling this function. */
    LogInfo(( "Subscribe to topic : %s.", pTopic ));

    do
    {
        /* TODO: prvIncomingPublish as publish callback. */
        xCommandAdded = MQTTAgent_Subscribe( &xGlobalMqttAgentContext,
                                             &xSubscribeArgs,
                                             &xCommandParams );
    } while( xCommandAdded != MQTTSuccess );

    /* Wait for acks to the subscribe message - this is optional but done here
     * so the code below can check the notification sent by the callback matches
     * the ulNextSubscribeMessageID value set in the context above. */
    xCommandAcknowledged = prvWaitForCommandAcknowledgment( NULL );

    /* Check both ways the status was passed back just for demonstration
     * purposes. */
    if( ( xCommandAcknowledged != pdTRUE ) ||
        ( xApplicationDefinedContext.xReturnStatus != MQTTSuccess ) )
    {
        LogError(( "Error or timed out waiting for ack to subscribe message topic %s.", pTopic ));
    }
    else
    {
        LogDebug(( "Received subscribe ack for topic %s containing ID %d.",
                  pTopic,
                  ( int ) xApplicationDefinedContext.ulNotificationValue ));
    }

    return xCommandAcknowledged;
}
/*-----------------------------------------------------------*/

BaseType_t mqttAgentPublish( MQTTQoS_t qos,
                             const char *pTopic,
                             size_t topicLength,
                             const char *pMsg,
                             size_t msgLength
                            )
{
    MQTTPublishInfo_t xPublishInfo = { 0UL };
    MQTTAgentCommandContext_t xCommandContext;
    uint32_t ulNotification = 0U;
    static uint32_t ulValueToNotify = 0UL;
    MQTTStatus_t xCommandAdded;
    MQTTAgentCommandInfo_t xCommandParams = { 0UL };

    /* Configure the publish operation. */
    memset( ( void * ) &xPublishInfo, 0x00, sizeof( xPublishInfo ) );
    memset( ( void * ) topicBuf, 0x00, mqttexampleSTRING_TOPIC_BUFFER_LENGTH);
    memset( ( void * ) payloadBuf, 0x00, mqttexampleSTRING_PAYLOAD_BUFFER_LENGTH);

    xPublishInfo.qos = qos;
    xPublishInfo.pTopicName = topicBuf;
    xPublishInfo.pPayload = (void *) payloadBuf;

    memcpy(xPublishInfo.pTopicName, pTopic, topicLength);
    xPublishInfo.topicNameLength = topicLength;
    memcpy(xPublishInfo.pPayload, pMsg, msgLength);
    xPublishInfo.payloadLength = msgLength;

    /* Store the handler to this task in the command context so the callback
     * that executes when the command is acknowledged can send a notification
     * back to this task. */
    memset( ( void * ) &xCommandContext, 0x00, sizeof( xCommandContext ) );
    xCommandContext.xTaskToNotify = xTaskGetCurrentTaskHandle();

    xCommandParams.blockTimeMs = mqttexampleMAX_COMMAND_SEND_BLOCK_TIME_MS;
    xCommandParams.cmdCompleteCallback = prvPublishCommandCallback;
    xCommandParams.pCmdCompleteCallbackContext = &xCommandContext;


    /* Also store the incrementing number in the command context so it can
     * be accessed by the callback that executes when the publish operation
     * is acknowledged. */
    xCommandContext.ulNotificationValue = ulValueToNotify;

    LogInfo(( "Publish message to topic: %.*s", xPublishInfo.topicNameLength, (char *)xPublishInfo.pTopicName ));

    /* To ensure ulNotification doesn't accidentally hold the expected value
     * as it is to be checked against the value sent from the callback.. */
    ulNotification = ~ulValueToNotify;

    xCommandAdded = MQTTAgent_Publish( &xGlobalMqttAgentContext,
                                       &xPublishInfo,
                                       &xCommandParams );
    configASSERT( xCommandAdded == MQTTSuccess );

    /* For QoS 1 and 2, wait for the publish acknowledgment.  For QoS0,
     * wait for the publish to be sent. */
    LogDebug(( "Task waiting for publish %d to complete.", ulValueToNotify ));
    prvWaitForCommandAcknowledgment( &ulNotification );

    /* The value received by the callback that executed when the publish was
     * acked came from the context passed into MQTTAgent_Publish() above, so
     * should match the value set in the context above. */
    if( ulNotification == ulValueToNotify++ )
    {
        LogDebug(( "Rx'ed ack for %d on topic: %s.", ulNotification, xPublishInfo.pTopicName ));
        return pdTRUE;
    }
    else
    {
        LogError(( "Timed out Rx'ing ack for %d on topic: %s.", ulNotification, xPublishInfo.pTopicName ));
        return pdFALSE;
    }
}

BaseType_t mqttAgentUnsubscribe( MQTTQoS_t qos,
                                 const char * pTopic,
                                 uint16_t topicLength
                                )
{
    MQTTStatus_t xCommandAdded;
    BaseType_t xCommandAcknowledged = pdFALSE;
    MQTTAgentSubscribeArgs_t xSubscribeArgs = { 0 };
    MQTTSubscribeInfo_t xSubscribeInfo = { 0 };
    MQTTAgentCommandInfo_t xCommandParams = { 0 };
    MQTTAgentCommandContext_t xApplicationDefinedContext = { 0 };

    configASSERT( pTopic != NULL );
    configASSERT( topicLength > 0 );

    xSubscribeInfo.pTopicFilter = pTopic;
    xSubscribeInfo.topicFilterLength = topicLength;
    xSubscribeInfo.qos = qos;
    xSubscribeArgs.pSubscribeInfo = &xSubscribeInfo;
    xSubscribeArgs.numSubscriptions = 1;


    xApplicationDefinedContext.xTaskToNotify = xTaskGetCurrentTaskHandle();
    xApplicationDefinedContext.pArgs = ( void * ) &xSubscribeArgs;

    xCommandParams.blockTimeMs = mqttexampleMAX_COMMAND_SEND_BLOCK_TIME_MS;
    xCommandParams.cmdCompleteCallback = prvUnsubscribeCommandCallback;
    xCommandParams.pCmdCompleteCallbackContext = ( void * ) &xApplicationDefinedContext;

    LogInfo(( " Unsubscribe to topic: %s", pTopic ));
 
    do
    {
        xCommandAdded = MQTTAgent_Unsubscribe( &xGlobalMqttAgentContext,
                                               &xSubscribeArgs,
                                               &xCommandParams );
    } while( xCommandAdded != MQTTSuccess );

    xCommandAcknowledged = prvWaitForCommandAcknowledgment( NULL );

    if( ( xCommandAcknowledged != pdTRUE ) ||
        ( xApplicationDefinedContext.xReturnStatus != MQTTSuccess ) )
    {
        LogError(( "Error or timed out waiting for ack to unsubscribe message topic %s.", pTopic ));
    }
    else
    {
        LogDebug(( "Received unsubscribe ack for topic %s containing ID %d.",
                  pTopic,
                  ( int ) xApplicationDefinedContext.ulNotificationValue ));
    }

    return (bool) xCommandAcknowledged;
}