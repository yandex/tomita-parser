#pragma once

#include <util/system/defaults.h>

////////////////////////////////////////////////////////////////////////////////
//
// Yandex encoding
//

const char yaCYR_GHE_WITH_UPTURN = '\x8e';

const char yaCYR_ghe_with_upturn = '\x9e';

const char yaCYR_IO  = '\xa8';
const char yaCYR_SHORT_U = '\xac';
const char yaCYR_UKR_I = '\xad';
const char yaCYR_UKR_IE = '\xae';
const char yaCYR_YI  = '\xaf';

const char yaCYR_io  = '\xb8';
const char yaCYR_short_u = '\xbc';
const char yaCYR_UKR_i = '\xbd';
const char yaCYR_UKR_ie = '\xbe';
const char yaCYR_yi  = '\xbf';

const char yaCYR_A   = '\xc0';
const char yaCYR_BE  = '\xc1';
const char yaCYR_VE  = '\xc2';
const char yaCYR_GHE = '\xc3';
const char yaCYR_DE  = '\xc4';
const char yaCYR_IE  = '\xc5';
const char yaCYR_ZHE = '\xc6';
const char yaCYR_ZE  = '\xc7';
const char yaCYR_I   = '\xc8';
const char yaCYR_SHORT_I = '\xc9';
const char yaCYR_KA  = '\xca';
const char yaCYR_EL  = '\xcb';
const char yaCYR_EM  = '\xcc';
const char yaCYR_EN  = '\xcd';
const char yaCYR_O   = '\xce';
const char yaCYR_PE  = '\xcf';
const char yaCYR_ER  = '\xd0';
const char yaCYR_ES  = '\xd1';
const char yaCYR_TE  = '\xd2';
const char yaCYR_U   = '\xd3';
const char yaCYR_EF  = '\xd4';
const char yaCYR_HA  = '\xd5';
const char yaCYR_TSE = '\xd6';
const char yaCYR_CHE = '\xd7';
const char yaCYR_SHA = '\xd8';
const char yaCYR_SHCHA = '\xd9';
const char yaCYR_HARD_SIGN = '\xda';
const char yaCYR_YERU = '\xdb';
const char yaCYR_SOFT_SIGN = '\xdc';
const char yaCYR_E   = '\xdd';
const char yaCYR_YU  = '\xde';
const char yaCYR_YA  = '\xdf';

const char yaCYR_a   = '\xe0';
const char yaCYR_be  = '\xe1';
const char yaCYR_ve  = '\xe2';
const char yaCYR_ghe = '\xe3';
const char yaCYR_de  = '\xe4';
const char yaCYR_ie  = '\xe5';
const char yaCYR_zhe = '\xe6';
const char yaCYR_ze  = '\xe7';
const char yaCYR_i   = '\xe8';
const char yaCYR_short_i = '\xe9';
const char yaCYR_ka  = '\xea';
const char yaCYR_el  = '\xeb';
const char yaCYR_em  = '\xec';
const char yaCYR_en  = '\xed';
const char yaCYR_o   = '\xee';
const char yaCYR_pe  = '\xef';
const char yaCYR_er  = '\xf0';
const char yaCYR_es  = '\xf1';
const char yaCYR_te  = '\xf2';
const char yaCYR_u   = '\xf3';
const char yaCYR_ef  = '\xf4';
const char yaCYR_ha  = '\xf5';
const char yaCYR_tse = '\xf6';
const char yaCYR_che = '\xf7';
const char yaCYR_sha = '\xf8';
const char yaCYR_shcha = '\xf9';
const char yaCYR_hard_sign = '\xfa';
const char yaCYR_yeru = '\xfb';
const char yaCYR_soft_sign = '\xfc';
const char yaCYR_e   = '\xfd';
const char yaCYR_yu  = '\xfe';
const char yaCYR_ya  = '\xff';

const char yaUNK_Up = '\xa6';
const char yaUNK_Lo = '\xb6';
const char yaNumero = '\xb7';
const char yaIDEOGR = '\x9f';
const char yaSHY    = '\x8f';
const char yaACUTE  = '\x80';
const char yaGradus = '\xb0';

////////////////////////////////////////////////////////////////////////////////
//
// Unicode
//

const wchar16 wCYR_IO = 0x0401;
const wchar16 wCYR_UKR_I = 0x0406;
const wchar16 wCYR_YI = 0x0407;
const wchar16 wCYR_A = 0x410;
const wchar16 wCYR_IE = 0x0415;
const wchar16 wCYR_SHORT_I = 0x0419;
const wchar16 wCYR_HARD_SIGN = 0x042a;
const wchar16 wCYR_SOFT_SIGN = 0x042c;
const wchar16 wCYR_YA = 0x042f;

const wchar16 wCYR_io = 0x0451;
const wchar16 wCYR_ukr_i = 0x0456;
const wchar16 wCYR_yi = 0x0457;
const wchar16 wCYR_a = 0x0430;
const wchar16 wCYR_ie = 0x0435;
const wchar16 wCYR_el = 0x043b;
const wchar16 wCYR_short_i = 0x0439;
const wchar16 wCYR_er = 0x0440;
const wchar16 wCYR_es = 0x0441;
const wchar16 wCYR_te = 0x0442;
const wchar16 wCYR_u = 0x0443;
const wchar16 wCYR_ef = 0x0444;
const wchar16 wCYR_hard_sign = 0x044a;
const wchar16 wCYR_soft_sign = 0x044c;
const wchar16 wCYR_ya = 0x044f;

const wchar16 wNULL = 0x0000;
const wchar16 wNBSP = 0x00a0;
const wchar16 wNOT_SIGN = 0x00ac;
const wchar16 wSHY = 0x00ad;
const wchar16 wMIDDLE_DOT = 0x00b7;
const wchar16 wELLIPSIS = 0x2026;
const wchar16 wNUMERO = 0x2116;
const wchar16 wIDEOGRAPHIC_FULL_STOP = 0x3002;
const wchar16 wBOM = 0xfeff;
const wchar16 wFULLWIDTH_EXCLAMATION_MARK = 0xff01;
const wchar16 wFULLWIDTH_QUESTION_MARK = 0xff1f;
const wchar16 wINVALID = 0xfffd;

// For Turkish.
const wchar16 wDotted_I = 0x0130;
const wchar16 wDotless_i = 0x0131;
const wchar16 wCircumflex_A = 0x00C2;
const wchar16 wCircumflex_a = 0x00E2;
const wchar16 wCircumflex_I = 0x00CE;
const wchar16 wCircumflex_i = 0x00EE;
const wchar16 wCircumflex_U = 0x00DB;
const wchar16 wCircumflex_u = 0x00FB;
const wchar16 wDiaeresis_U = 0x00DC;
const wchar16 wDiaeresis_u = 0x00FC;

// MSDN: Unicode Subset Bitfields
struct TUnicodeRange {
    static const wchar16 LATIN_EXTENDED_A = 0x0100;
    static const wchar16 LATIN_EXTENDED_A_END = 0x0180;
    static const wchar16 LATIN_EXTENDED_B = 0x0180;
    static const wchar16 LATIN_EXTENDED_B_END = 0x0250;
    static const wchar16 IPA_EXTENSIONS = 0x0250;
    static const wchar16 IPA_EXTENSIONS_END = 0x02b0;
    static const wchar16 GREEK = 0x0370;
    static const wchar16 GREEK_END = 0x0400;
    static const wchar16 CYRILLIC = 0x0400;
    static const wchar16 CYRILLIC_END = 0x0500;
    static const wchar16 CYRILLIC_SUPPLEMENTARY = 0x0500;
    static const wchar16 CYRILLIC_SUPPLEMENTARY_END = 0x0530;
    static const wchar16 ARMENIAN = 0x0530;
    static const wchar16 ARMENIAN_END = 0x0590;
    static const wchar16 HEBREW = 0x0590;
    static const wchar16 HEBREW_END = 0x0600;
    static const wchar16 ARABIC = 0x0600;
    static const wchar16 ARABIC_END = 0x0700;
    static const wchar16 ARABIC_SUPPLEMENT = 0x0750;
    static const wchar16 ARABIC_SUPPLEMENT_END = 0x0780;
    static const wchar16 ARABIC_EXTENDED_A = 0x08A0;
    static const wchar16 ARABIC_EXTENDED_A_END = 0x0900;
    static const wchar16 THAI = 0x0e00;
    static const wchar16 THAI_END = 0x0e80;
    static const wchar16 GEORGIAN = 0x10A0;
    static const wchar16 GEORGIAN_END = 0x1100;
    static const wchar16 PRIVATE_USE_AREA = 0xe000;
    static const wchar16 PRIVATE_USE_AREA_END = 0xf900;
};

////////////////////////////////////////////////////////////////////////////////
//
// UTF-8
//

/** Converts UTF8 character string to wchar16 code
    Example: U_CHR("я") == 0x044f */
#define U_CHR(ch) wchar16(((ch[0] & 0x1f) << 6) | (ch[1] & 0x3f))

#define U_SOFT_HYPHEN "\xc2\xad"
#define U_INVALID "\xef\xbf\xbd"

#define BOM_UTF8 "\xef\xbb\xbf"

////////////////////////////////////////////////////////////////////////////////
//
// Functions
//

inline bool IsSurrogate(wchar16 ch) {
    return (ch & 0xf800) == 0xd800;
}
