/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _BACKED_BLOCK_H_
#define _BACKED_BLOCK_H_

#include "ext4_utils.h"
#include "output_file.h"

typedef void (*data_block_callback_t)(void *priv, u64 off, u8 *data, int len);
typedef void (*data_block_fill_callback_t)(void *priv, u64 off, u32 fill_val, int len);
typedef void (*data_block_file_callback_t)(void *priv, u64 off,
					   const char *file, off64_t offset,
					   int len);

void queue_data_block(u8 *data, u32 len, u32 block);
void queue_fill_block(u32 fill_val, u32 len, u32 block);
void queue_data_file(const char *filename, off64_t offset, u32 len,
	u32 block);
void for_each_data_block(data_block_callback_t data_func,
	data_block_file_callback_t file_func,
	data_block_fill_callback_t fill_func, void *priv);
void free_data_blocks();

#endif
