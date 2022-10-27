// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2011-2022 Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// ----------------------------------------------------------------------------

/**
 * @brief Functions and data declarations for the outer context.
 *
 * The outer context includes thread-pool management, which is slower to
 * compile due to increased use of C++ stdlib. The inner context used in the
 * majority of the codec library does not include this.
 */

#ifndef ASTCENC_INTERNAL_ENTRY_INCLUDED
#define ASTCENC_INTERNAL_ENTRY_INCLUDED

#include "astcenc_internal.h"

/**
 * @brief The astcenc compression context.
 */
struct astcenc_context
{
	/** @brief The context internal state. */
	astcenc_contexti context;
};

#endif
