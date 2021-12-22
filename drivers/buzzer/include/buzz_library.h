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
 * @file buzz_library.h
 * @brief Functions to interact with OBD dongle buzzer.
 */

#ifndef BUZZ_LIBRARY_H
#define BUZZ_LIBRARY_H

/**
 * @brief Initialize buzz device.
 *
 * @param[in] buzzDevice buzz peripheral descriptor.
 */
void buzz_init( Peripheral_Descriptor_t buzzDevice );

/**
 * @brief Buzz device start beeping.
 *
 * @param[in] buzzDevice buzz peripheral descriptor.
 * @param[in] beepDurationMs beep cycle duration in milisecond.
 * @param[in] times beep cycle count.
 */
void buzz_beep( Peripheral_Descriptor_t buzzDevice, uint32_t beepDurationMs, uint32_t times );

/**
 * @brief Buzz device plays a tone.
 *
 * @param[in] buzzDevice buzz peripheral descriptor.
 * @param[in] freq tone frequency.
 * @param[in] durationMs tone playing duration in milisecond.
 */
void buzz_playtone( Peripheral_Descriptor_t buzzDevice, uint16_t freq, uint32_t durationMs );

#endif /* BUZZ_LIBRARY_H */
