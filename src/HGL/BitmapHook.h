/*
 * Copyright 2022-2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#pragma once

#include <stdint.h>

class BBitmap;

class BitmapHook {
public:
    virtual ~BitmapHook() {};
    virtual void GetSize(uint32_t &width, uint32_t &height) = 0;
    virtual BBitmap *SetBitmap(BBitmap *bmp) = 0;
};
