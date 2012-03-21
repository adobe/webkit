//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_DETECT_TEXTURE_ACCESS_H_
#define COMPILER_DETECT_TEXTURE_ACCESS_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"

// Traverses intermediate tree to detect if unifrom textures are used.
class DetectTextureAccess : public TIntermTraverser {
public:
    DetectTextureAccess();
    
    virtual bool visitAggregate(Visit, TIntermAggregate*);
    
    bool detectTextureAccess() { return hasTextureAccess; }

private:
    bool hasTextureAccess;
};

#endif  // COMPILER_DETECT_RECURSION_H_
