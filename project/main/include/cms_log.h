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
 * @file mcs_log.h
 * @brief Wrapper of CMS application log printing.
 */

#ifndef CMS_LOG_H
#define CMS_LOG_H

#include "esp_log.h"

#define CMS_LOGE( tag, format, ... ) ESP_LOGE( tag, format, ##__VA_ARGS__ )
#define CMS_LOGW( tag, format, ... ) ESP_LOGW( tag, format, ##__VA_ARGS__ )
#define CMS_LOGI( tag, format, ... ) ESP_LOGI( tag, format, ##__VA_ARGS__ )
#define CMS_LOGD( tag, format, ... ) ESP_LOGD( tag, format, ##__VA_ARGS__ )
#define CMS_LOGV( tag, format, ... ) ESP_LOGV( tag, format, ##__VA_ARGS__ )

#endif /* CMS_LOG_H */