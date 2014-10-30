#include <util/charset/codepage.h>

extern const Encoder asciiEncoder;
extern const CodePage asciiCodePage;

////////////////////////////////////////////////////////////////////////////////
//
// AsciiEncoder
//

static char ZZ[256];
static char ASC[] = {
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
    "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
    "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
    "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f"
    "\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"
    "\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
    "\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f"
    "\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f"
    "\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f"
    "\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f"
    "\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f"
    "\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f"
    "\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f"
    "\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f"
    "\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f"
};

static const char asciiDefChars[] =
    "?????????????????????????????????????????????????????????";
static_assert(ARRAY_SIZE(asciiDefChars) == DEFCHAR_BUF, "Invalid size of 'asciiDefChars'");

const Encoder asciiEncoder = {
    {
        ASC,ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ,
        ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ, ZZ
    },
    asciiDefChars,
    0
};

////////////////////////////////////////////////////////////////////////////////
//
// ASCII CodePage
//

#define FFFD 0xfffd

const CodePage asciiCodePage = {
    CODES_UNKNOWN,
    { "ascii", 0 },
    {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD,
    FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD,
    FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD,
    FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD,
    FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD,
    FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD,
    FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD,
    FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD, FFFD
    },
    asciiDefChars
};
