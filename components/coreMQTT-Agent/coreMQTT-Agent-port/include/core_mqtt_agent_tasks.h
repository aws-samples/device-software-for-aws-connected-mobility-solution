/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file core_mqtt_agent_tasks.h
 * @brief Functions to interact with coreMQTT agent.
 */

#ifndef CORE_MQTT_AGENT_TASKS_H
#define CORE_MQTT_AGENT_TASKS_H

#include "FreeRTOS.h"

/**
 * @brief structure of mqtt subscribe information.
 */
typedef struct mqttSuRequest
{
    MQTTQoS_t qos;
    char topic[130];
    size_t topicLength;
    void * incomingPublishCallback;
    void * incomingPublishCallbackContext;
} mqttSuRequest_t;

/**
 * @brief structure of mqtt publish information.
 */
typedef struct mqttPubRequest
{
    MQTTQoS_t qos;
    char topic[130];
    size_t topicLength;
    char payload[1024];
    size_t payloadLength;
} mqttPuRequest_t;

/**
 * @brief function pointer of mqtt incoming message callback.
 */
typedef void (*pApplicationHandler_t)( void * pvIncomingPublishCallbackContext, MQTTPublishInfo_t * pxPublishInfo );

/**
 * @brief Function to start coreMQTT agent by creating a task.
 */
extern void startMqttAgentTask( void );

/**
 * @brief Subscribe to a MQTT topic through coreMQTT agent.
 *
 * @param[in] qos MQTT qos.
 * @param[in] pTopic char pointer to mqtt topic name that subscribing to.
 * @param[in] incomingPublishCallback callback function for incoming mqtt message on subscribed topic.
 * @param[in] incomingPublishCallbackContext context that feeds to callback function.
 *
 * @return pdTRUE for successful subscribe, otherwise return pdFALSE.
 */
extern BaseType_t mqttAgentSubscribe( MQTTQoS_t qos,
                                      const char * pTopic,
                                      const void * incomingPublishCallback,
                                      const void * incomingPublishCallbackContext );

/**
 * @brief Publish message through coreMQTT agent.
 *
 * @param[in] qos MQTT qos.
 * @param[in] pTopic char pointer to mqtt topic name that publishing to.
 * @param[in] topicLength length of topic name.
 * @param[in] pMsg pointer to message to be published.
 * @param[in] msgLength length of message
 *
 * @return pdTRUE for successful publish, otherwise return pdFALSE.
 */
extern BaseType_t mqttAgentPublish( MQTTQoS_t qos,
                                    const char *pTopic,
                                    size_t topicLength,
                                    const char *pMsg,
                                    size_t msgLength );

/**
 * @brief Unsubscribe to a MQTT topic through coreMQTT agent.
 *
 * @param[in] qos MQTT qos.
 * @param[in] pTopic char pointer to mqtt topic name that unsubscribing to.
 * @param[in] topicLength length of topic name.
 *
 * @return pdTRUE for successful unsubscribe, otherwise return pdFALSE.
 */
extern BaseType_t mqttAgentUnsubscribe( MQTTQoS_t qos,
                                        const char * pTopic,
                                        uint16_t topicLength );

#endif /* CORE_MQTT_AGENT_TASKS_H */