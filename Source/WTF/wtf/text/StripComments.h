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

#ifndef StripComments_h
#define StripComments_h

#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WTF {

// Strips comments from shader text. This allows non-ASCII characters
// to be used in comments without potentially breaking OpenGL
// implementations not expecting characters outside the GLSL ES set.
class StripComments {
public:
    StripComments(const String& str)
        : m_parseState(BeginningOfLine)
        , m_sourceString(str)
        , m_length(str.length())
        , m_position(0)
    {
        parse();
    }

    String result()
    {
        return m_builder.toString();
    }

private:
    bool hasMoreCharacters()
    {
        return (m_position < m_length);
    }

    void parse()
    {
        while (hasMoreCharacters()) {
            process(current());
            // process() might advance the position.
            if (hasMoreCharacters())
                advance();
        }
    }

    void process(UChar);

    bool peek(UChar& character)
    {
        if (m_position + 1 >= m_length)
            return false;
        character = m_sourceString[m_position + 1];
        return true;
    }

    UChar current()
    {
        ASSERT(m_position < m_length);
        return m_sourceString[m_position];
    }

    void advance()
    {
        ++m_position;
    }

    bool isNewline(UChar character)
    {
        // Don't attempt to canonicalize newline related characters.
        return (character == '\n' || character == '\r');
    }

    void emit(UChar character)
    {
        m_builder.append(character);
    }

    enum ParseState {
        // Have not seen an ASCII non-whitespace character yet on
        // this line. Possible that we might see a preprocessor
        // directive.
        BeginningOfLine,

        // Have seen at least one ASCII non-whitespace character
        // on this line.
        MiddleOfLine,

        // Handling a preprocessor directive. Passes through all
        // characters up to the end of the line. Disables comment
        // processing.
        InPreprocessorDirective,

        // Handling a single-line comment. The comment text is
        // replaced with a single space.
        InSingleLineComment,

        // Handling a multi-line comment. Newlines are passed
        // through to preserve line numbers.
        InMultiLineComment
    };

    ParseState m_parseState;
    String m_sourceString;
    unsigned m_length;
    unsigned m_position;
    StringBuilder m_builder;
};

} // namespace WTF

using WTF::StripComments;

#endif // StripComments_h
