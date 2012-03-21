//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/DetectTextureAccess.h"

DetectTextureAccess::DetectTextureAccess()
    : hasTextureAccess(false)
{
}

bool DetectTextureAccess::visitAggregate(Visit visit, TIntermAggregate* node)
{
    if (visit != PreVisit)
        return true;
    
    switch (node->getOp())
    {
        case EOpDeclaration: {
            TIntermSequence& sequence = node->getSequence();
            TIntermTyped* variable = sequence.front()->getAsTyped();
            
            const TType& type = variable->getType();
            switch (type.getBasicType()) {
                case EbtSampler2D:
                case EbtSampler2DRect:
                case EbtSamplerCube:
                case EbtSamplerExternalOES: {
                    printf("found sampler\n");
                    for (size_t i = 0; i < sequence.size(); ++i) {
                        TIntermSymbol* symbol = sequence[i]->getAsSymbolNode();
                        if (symbol)
                            printf("symbol name: %s\n", symbol->getSymbol().c_str());
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    
    return true;
}



