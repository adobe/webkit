/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#include <wtf/text/StripComments.h>

namespace WTF {

void StripComments::process(UChar c)
{
    if (isNewline(c)) {
        // No matter what state we are in, pass through newlines
        // so we preserve line numbers.
        emit(c);

        if (m_parseState != InMultiLineComment)
            m_parseState = BeginningOfLine;

        return;
    }

    UChar temp = 0;
    switch (m_parseState) {
    case BeginningOfLine:
        if (WTF::isASCIISpace(c)) {
            emit(c);
            break;
        }

        if (c == '#') {
            m_parseState = InPreprocessorDirective;
            emit(c);
            break;
        }

        // Transition to normal state and re-handle character.
        m_parseState = MiddleOfLine;
        process(c);
        break;

    case MiddleOfLine:
        if (c == '/' && peek(temp)) {
            if (temp == '/') {
                m_parseState = InSingleLineComment;
                emit(' ');
                advance();
                break;
            }

            if (temp == '*') {
                m_parseState = InMultiLineComment;
                // Emit the comment start in case the user has
                // an unclosed comment and we want to later
                // signal an error.
                emit('/');
                emit('*');
                advance();
                break;
            }
        }

        emit(c);
        break;

    case InPreprocessorDirective:
        // No matter what the character is, just pass it
        // through. Do not parse comments in this state. This
        // might not be the right thing to do long term, but it
        // should handle the #error preprocessor directive.
        emit(c);
        break;

    case InSingleLineComment:
        // The newline code at the top of this function takes care
        // of resetting our state when we get out of the
        // single-line comment. Swallow all other characters.
        break;

    case InMultiLineComment:
        if (c == '*' && peek(temp) && temp == '/') {
            emit('*');
            emit('/');
            m_parseState = MiddleOfLine;
            advance();
            break;
        }

        // Swallow all other characters. Unclear whether we may
        // want or need to just emit a space per character to try
        // to preserve column numbers for debugging purposes.
        break;
    }
}

} // namespace WTF

