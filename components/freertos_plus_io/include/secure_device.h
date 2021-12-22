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
 * @file secure_device.h
 * @brief Define macros to access secure device io.
 */

#ifndef SECURE_DEVICE_H
#define SECURE_DEVICE_H

#define ioctlSECURE_ROOT_CA             ( 0 )
#define ioctlSECURE_CLIENT_CERT         ( 1 )
#define ioctlSECURE_CLIENT_KEY          ( 2 )
#define ioctlSECURE_CLIENT_ID           ( 3 )
#define ioctlSECURE_VIN                 ( 4 )
#define ioctlSECURE_BROKER_ENDPOINT     ( 5 )
#define ioctlSECURE_BROKER_PORT         ( 6 )

#endif /* SECURE_DEVICE_H */
