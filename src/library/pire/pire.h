#pragma once

#ifndef PIRE_NO_CONFIG
#define PIRE_NO_CONFIG
#endif

#include <contrib/libs/pire/pire/pire.h>
#include <contrib/libs/pire/pire/extra.h>

namespace NPire {
    typedef Pire::Char TChar;

    // Scanner classes
    typedef Pire::Scanner TScanner;
    typedef Pire::NonrelocScanner TNonrelocScanner;
    typedef Pire::ScannerNoMask TScannerNoMask;
    typedef Pire::NonrelocScannerNoMask TNonrelocScannerNoMask;
    typedef Pire::SimpleScanner TSimpleScanner;
    typedef Pire::SlowScanner TSlowScanner;
    typedef Pire::CapturingScanner TCapturingScanner;
    typedef Pire::CountingScanner TCountingScanner;

    // Helper classes
    typedef Pire::Fsm TFsm;
    typedef Pire::Lexer TLexer;
    typedef Pire::Term TTerm;
    typedef Pire::Encoding TEncoding;
    typedef Pire::Feature TFeature;

    // Helper functions
    using Pire::MmappedScanner;
    using Pire::Run;
    using Pire::Runner;
    using Pire::LongestPrefix;
    using Pire::LongestSuffix;
    using Pire::ShortestPrefix;
    using Pire::ShortestSuffix;
    using Pire::Matches;
    using Pire::Step;

    using namespace Pire::SpecialChar;
    using namespace Pire::Consts;

    namespace NFeatures {
        using Pire::Features::CaseInsensitive;
        using Pire::Features::GlueSimilarGlyphs;
        using Pire::Features::AndNotSupport;
        using Pire::Features::Capture;
        using Pire::Features::EnableUnicodeSequences;
    }

    namespace NEncodings {
        using Pire::Encodings::Latin1;
        using Pire::Encodings::Utf8;

    const NPire::TEncoding& Koi8r();
    const NPire::TEncoding& Cp1251();
        const NPire::TEncoding& Get(ECharset encoding);
    }

    namespace NTokenTypes {
        using namespace Pire::TokenTypes;
    }
}
