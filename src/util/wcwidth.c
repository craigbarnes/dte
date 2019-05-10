static const CodepointRange special_whitespace[] = {
    {0x00a0, 0x00a0},
    {0x00ad, 0x00ad},
    {0x2000, 0x200a},
    {0x2028, 0x2029},
    {0x202f, 0x202f},
    {0x205f, 0x205f},
};

static const CodepointRange default_ignorable[] = {
    {0x034f, 0x034f},
    {0x061c, 0x061c},
    {0x115f, 0x1160},
    {0x17b4, 0x17b5},
    {0x180b, 0x180e},
    {0x200b, 0x200f},
    {0x202a, 0x202e},
    {0x2060, 0x206f},
    {0x3164, 0x3164},
    {0xfe00, 0xfe0f},
    {0xfeff, 0xfeff},
    {0xffa0, 0xffa0},
    {0xfff0, 0xfff8},
    {0x1bca0, 0x1bca3},
    {0x1d173, 0x1d17a},
    {0xe0000, 0xe0fff},
};

static const CodepointRange nonspacing_mark[] = {
    {0x0300, 0x036f},
    {0x0483, 0x0489},
    {0x0591, 0x05bd},
    {0x05bf, 0x05bf},
    {0x05c1, 0x05c2},
    {0x05c4, 0x05c5},
    {0x05c7, 0x05c7},
    {0x0610, 0x061a},
    {0x064b, 0x065f},
    {0x0670, 0x0670},
    {0x06d6, 0x06dc},
    {0x06df, 0x06e4},
    {0x06e7, 0x06e8},
    {0x06ea, 0x06ed},
    {0x0711, 0x0711},
    {0x0730, 0x074a},
    {0x07a6, 0x07b0},
    {0x07eb, 0x07f3},
    {0x07fd, 0x07fd},
    {0x0816, 0x0819},
    {0x081b, 0x0823},
    {0x0825, 0x0827},
    {0x0829, 0x082d},
    {0x0859, 0x085b},
    {0x08d3, 0x08e1},
    {0x08e3, 0x0902},
    {0x093a, 0x093a},
    {0x093c, 0x093c},
    {0x0941, 0x0948},
    {0x094d, 0x094d},
    {0x0951, 0x0957},
    {0x0962, 0x0963},
    {0x0981, 0x0981},
    {0x09bc, 0x09bc},
    {0x09c1, 0x09c4},
    {0x09cd, 0x09cd},
    {0x09e2, 0x09e3},
    {0x09fe, 0x09fe},
    {0x0a01, 0x0a02},
    {0x0a3c, 0x0a3c},
    {0x0a41, 0x0a42},
    {0x0a47, 0x0a48},
    {0x0a4b, 0x0a4d},
    {0x0a51, 0x0a51},
    {0x0a70, 0x0a71},
    {0x0a75, 0x0a75},
    {0x0a81, 0x0a82},
    {0x0abc, 0x0abc},
    {0x0ac1, 0x0ac5},
    {0x0ac7, 0x0ac8},
    {0x0acd, 0x0acd},
    {0x0ae2, 0x0ae3},
    {0x0afa, 0x0aff},
    {0x0b01, 0x0b01},
    {0x0b3c, 0x0b3c},
    {0x0b3f, 0x0b3f},
    {0x0b41, 0x0b44},
    {0x0b4d, 0x0b4d},
    {0x0b56, 0x0b56},
    {0x0b62, 0x0b63},
    {0x0b82, 0x0b82},
    {0x0bc0, 0x0bc0},
    {0x0bcd, 0x0bcd},
    {0x0c00, 0x0c00},
    {0x0c04, 0x0c04},
    {0x0c3e, 0x0c40},
    {0x0c46, 0x0c48},
    {0x0c4a, 0x0c4d},
    {0x0c55, 0x0c56},
    {0x0c62, 0x0c63},
    {0x0c81, 0x0c81},
    {0x0cbc, 0x0cbc},
    {0x0cbf, 0x0cbf},
    {0x0cc6, 0x0cc6},
    {0x0ccc, 0x0ccd},
    {0x0ce2, 0x0ce3},
    {0x0d00, 0x0d01},
    {0x0d3b, 0x0d3c},
    {0x0d41, 0x0d44},
    {0x0d4d, 0x0d4d},
    {0x0d62, 0x0d63},
    {0x0dca, 0x0dca},
    {0x0dd2, 0x0dd4},
    {0x0dd6, 0x0dd6},
    {0x0e31, 0x0e31},
    {0x0e34, 0x0e3a},
    {0x0e47, 0x0e4e},
    {0x0eb1, 0x0eb1},
    {0x0eb4, 0x0ebc},
    {0x0ec8, 0x0ecd},
    {0x0f18, 0x0f19},
    {0x0f35, 0x0f35},
    {0x0f37, 0x0f37},
    {0x0f39, 0x0f39},
    {0x0f71, 0x0f7e},
    {0x0f80, 0x0f84},
    {0x0f86, 0x0f87},
    {0x0f8d, 0x0f97},
    {0x0f99, 0x0fbc},
    {0x0fc6, 0x0fc6},
    {0x102d, 0x1030},
    {0x1032, 0x1037},
    {0x1039, 0x103a},
    {0x103d, 0x103e},
    {0x1058, 0x1059},
    {0x105e, 0x1060},
    {0x1071, 0x1074},
    {0x1082, 0x1082},
    {0x1085, 0x1086},
    {0x108d, 0x108d},
    {0x109d, 0x109d},
    {0x135d, 0x135f},
    {0x1712, 0x1714},
    {0x1732, 0x1734},
    {0x1752, 0x1753},
    {0x1772, 0x1773},
    {0x17b4, 0x17b5},
    {0x17b7, 0x17bd},
    {0x17c6, 0x17c6},
    {0x17c9, 0x17d3},
    {0x17dd, 0x17dd},
    {0x180b, 0x180d},
    {0x1885, 0x1886},
    {0x18a9, 0x18a9},
    {0x1920, 0x1922},
    {0x1927, 0x1928},
    {0x1932, 0x1932},
    {0x1939, 0x193b},
    {0x1a17, 0x1a18},
    {0x1a1b, 0x1a1b},
    {0x1a56, 0x1a56},
    {0x1a58, 0x1a5e},
    {0x1a60, 0x1a60},
    {0x1a62, 0x1a62},
    {0x1a65, 0x1a6c},
    {0x1a73, 0x1a7c},
    {0x1a7f, 0x1a7f},
    {0x1ab0, 0x1abe},
    {0x1b00, 0x1b03},
    {0x1b34, 0x1b34},
    {0x1b36, 0x1b3a},
    {0x1b3c, 0x1b3c},
    {0x1b42, 0x1b42},
    {0x1b6b, 0x1b73},
    {0x1b80, 0x1b81},
    {0x1ba2, 0x1ba5},
    {0x1ba8, 0x1ba9},
    {0x1bab, 0x1bad},
    {0x1be6, 0x1be6},
    {0x1be8, 0x1be9},
    {0x1bed, 0x1bed},
    {0x1bef, 0x1bf1},
    {0x1c2c, 0x1c33},
    {0x1c36, 0x1c37},
    {0x1cd0, 0x1cd2},
    {0x1cd4, 0x1ce0},
    {0x1ce2, 0x1ce8},
    {0x1ced, 0x1ced},
    {0x1cf4, 0x1cf4},
    {0x1cf8, 0x1cf9},
    {0x1dc0, 0x1df9},
    {0x1dfb, 0x1dff},
    {0x20d0, 0x20f0},
    {0x2cef, 0x2cf1},
    {0x2d7f, 0x2d7f},
    {0x2de0, 0x2dff},
    {0x302a, 0x302d},
    {0x3099, 0x309a},
    {0xa66f, 0xa672},
    {0xa674, 0xa67d},
    {0xa69e, 0xa69f},
    {0xa6f0, 0xa6f1},
    {0xa802, 0xa802},
    {0xa806, 0xa806},
    {0xa80b, 0xa80b},
    {0xa825, 0xa826},
    {0xa8c4, 0xa8c5},
    {0xa8e0, 0xa8f1},
    {0xa8ff, 0xa8ff},
    {0xa926, 0xa92d},
    {0xa947, 0xa951},
    {0xa980, 0xa982},
    {0xa9b3, 0xa9b3},
    {0xa9b6, 0xa9b9},
    {0xa9bc, 0xa9bd},
    {0xa9e5, 0xa9e5},
    {0xaa29, 0xaa2e},
    {0xaa31, 0xaa32},
    {0xaa35, 0xaa36},
    {0xaa43, 0xaa43},
    {0xaa4c, 0xaa4c},
    {0xaa7c, 0xaa7c},
    {0xaab0, 0xaab0},
    {0xaab2, 0xaab4},
    {0xaab7, 0xaab8},
    {0xaabe, 0xaabf},
    {0xaac1, 0xaac1},
    {0xaaec, 0xaaed},
    {0xaaf6, 0xaaf6},
    {0xabe5, 0xabe5},
    {0xabe8, 0xabe8},
    {0xabed, 0xabed},
    {0xfb1e, 0xfb1e},
    {0xfe00, 0xfe0f},
    {0xfe20, 0xfe2f},
    {0x101fd, 0x101fd},
    {0x102e0, 0x102e0},
    {0x10376, 0x1037a},
    {0x10a01, 0x10a03},
    {0x10a05, 0x10a06},
    {0x10a0c, 0x10a0f},
    {0x10a38, 0x10a3a},
    {0x10a3f, 0x10a3f},
    {0x10ae5, 0x10ae6},
    {0x10d24, 0x10d27},
    {0x10f46, 0x10f50},
    {0x11001, 0x11001},
    {0x11038, 0x11046},
    {0x1107f, 0x11081},
    {0x110b3, 0x110b6},
    {0x110b9, 0x110ba},
    {0x11100, 0x11102},
    {0x11127, 0x1112b},
    {0x1112d, 0x11134},
    {0x11173, 0x11173},
    {0x11180, 0x11181},
    {0x111b6, 0x111be},
    {0x111c9, 0x111cc},
    {0x1122f, 0x11231},
    {0x11234, 0x11234},
    {0x11236, 0x11237},
    {0x1123e, 0x1123e},
    {0x112df, 0x112df},
    {0x112e3, 0x112ea},
    {0x11300, 0x11301},
    {0x1133b, 0x1133c},
    {0x11340, 0x11340},
    {0x11366, 0x1136c},
    {0x11370, 0x11374},
    {0x11438, 0x1143f},
    {0x11442, 0x11444},
    {0x11446, 0x11446},
    {0x1145e, 0x1145e},
    {0x114b3, 0x114b8},
    {0x114ba, 0x114ba},
    {0x114bf, 0x114c0},
    {0x114c2, 0x114c3},
    {0x115b2, 0x115b5},
    {0x115bc, 0x115bd},
    {0x115bf, 0x115c0},
    {0x115dc, 0x115dd},
    {0x11633, 0x1163a},
    {0x1163d, 0x1163d},
    {0x1163f, 0x11640},
    {0x116ab, 0x116ab},
    {0x116ad, 0x116ad},
    {0x116b0, 0x116b5},
    {0x116b7, 0x116b7},
    {0x1171d, 0x1171f},
    {0x11722, 0x11725},
    {0x11727, 0x1172b},
    {0x1182f, 0x11837},
    {0x11839, 0x1183a},
    {0x119d4, 0x119d7},
    {0x119da, 0x119db},
    {0x119e0, 0x119e0},
    {0x11a01, 0x11a0a},
    {0x11a33, 0x11a38},
    {0x11a3b, 0x11a3e},
    {0x11a47, 0x11a47},
    {0x11a51, 0x11a56},
    {0x11a59, 0x11a5b},
    {0x11a8a, 0x11a96},
    {0x11a98, 0x11a99},
    {0x11c30, 0x11c36},
    {0x11c38, 0x11c3d},
    {0x11c3f, 0x11c3f},
    {0x11c92, 0x11ca7},
    {0x11caa, 0x11cb0},
    {0x11cb2, 0x11cb3},
    {0x11cb5, 0x11cb6},
    {0x11d31, 0x11d36},
    {0x11d3a, 0x11d3a},
    {0x11d3c, 0x11d3d},
    {0x11d3f, 0x11d45},
    {0x11d47, 0x11d47},
    {0x11d90, 0x11d91},
    {0x11d95, 0x11d95},
    {0x11d97, 0x11d97},
    {0x11ef3, 0x11ef4},
    {0x16af0, 0x16af4},
    {0x16b30, 0x16b36},
    {0x16f4f, 0x16f4f},
    {0x16f8f, 0x16f92},
    {0x1bc9d, 0x1bc9e},
    {0x1d167, 0x1d169},
    {0x1d17b, 0x1d182},
    {0x1d185, 0x1d18b},
    {0x1d1aa, 0x1d1ad},
    {0x1d242, 0x1d244},
    {0x1da00, 0x1da36},
    {0x1da3b, 0x1da6c},
    {0x1da75, 0x1da75},
    {0x1da84, 0x1da84},
    {0x1da9b, 0x1da9f},
    {0x1daa1, 0x1daaf},
    {0x1e000, 0x1e006},
    {0x1e008, 0x1e018},
    {0x1e01b, 0x1e021},
    {0x1e023, 0x1e024},
    {0x1e026, 0x1e02a},
    {0x1e130, 0x1e136},
    {0x1e2ec, 0x1e2ef},
    {0x1e8d0, 0x1e8d6},
    {0x1e944, 0x1e94a},
    {0xe0100, 0xe01ef},
};

static const CodepointRange double_width[] = {
    {0x1100, 0x115f},
    {0x231a, 0x231b},
    {0x2329, 0x232a},
    {0x23e9, 0x23ec},
    {0x23f0, 0x23f0},
    {0x23f3, 0x23f3},
    {0x25fd, 0x25fe},
    {0x2614, 0x2615},
    {0x2648, 0x2653},
    {0x267f, 0x267f},
    {0x2693, 0x2693},
    {0x26a1, 0x26a1},
    {0x26aa, 0x26ab},
    {0x26bd, 0x26be},
    {0x26c4, 0x26c5},
    {0x26ce, 0x26ce},
    {0x26d4, 0x26d4},
    {0x26ea, 0x26ea},
    {0x26f2, 0x26f3},
    {0x26f5, 0x26f5},
    {0x26fa, 0x26fa},
    {0x26fd, 0x26fd},
    {0x2705, 0x2705},
    {0x270a, 0x270b},
    {0x2728, 0x2728},
    {0x274c, 0x274c},
    {0x274e, 0x274e},
    {0x2753, 0x2755},
    {0x2757, 0x2757},
    {0x2795, 0x2797},
    {0x27b0, 0x27b0},
    {0x27bf, 0x27bf},
    {0x2b1b, 0x2b1c},
    {0x2b50, 0x2b50},
    {0x2b55, 0x2b55},
    {0x2e80, 0x2e99},
    {0x2e9b, 0x2ef3},
    {0x2f00, 0x2fd5},
    {0x2ff0, 0x2ffb},
    {0x3000, 0x303e},
    {0x3041, 0x3096},
    {0x3099, 0x30ff},
    {0x3105, 0x312f},
    {0x3131, 0x318e},
    {0x3190, 0x31ba},
    {0x31c0, 0x31e3},
    {0x31f0, 0x321e},
    {0x3220, 0x3247},
    {0x3250, 0x4dbf},
    {0x4e00, 0xa48c},
    {0xa490, 0xa4c6},
    {0xa960, 0xa97c},
    {0xac00, 0xd7a3},
    {0xf900, 0xfaff},
    {0xfe10, 0xfe19},
    {0xfe30, 0xfe52},
    {0xfe54, 0xfe66},
    {0xfe68, 0xfe6b},
    {0xff01, 0xff60},
    {0xffe0, 0xffe6},
    {0x16fe0, 0x16fe3},
    {0x17000, 0x187f7},
    {0x18800, 0x18af2},
    {0x1b000, 0x1b11e},
    {0x1b150, 0x1b152},
    {0x1b164, 0x1b167},
    {0x1b170, 0x1b2fb},
    {0x1f004, 0x1f004},
    {0x1f0cf, 0x1f0cf},
    {0x1f18e, 0x1f18e},
    {0x1f191, 0x1f19a},
    {0x1f200, 0x1f202},
    {0x1f210, 0x1f23b},
    {0x1f240, 0x1f248},
    {0x1f250, 0x1f251},
    {0x1f260, 0x1f265},
    {0x1f300, 0x1f320},
    {0x1f32d, 0x1f335},
    {0x1f337, 0x1f37c},
    {0x1f37e, 0x1f393},
    {0x1f3a0, 0x1f3ca},
    {0x1f3cf, 0x1f3d3},
    {0x1f3e0, 0x1f3f0},
    {0x1f3f4, 0x1f3f4},
    {0x1f3f8, 0x1f43e},
    {0x1f440, 0x1f440},
    {0x1f442, 0x1f4fc},
    {0x1f4ff, 0x1f53d},
    {0x1f54b, 0x1f54e},
    {0x1f550, 0x1f567},
    {0x1f57a, 0x1f57a},
    {0x1f595, 0x1f596},
    {0x1f5a4, 0x1f5a4},
    {0x1f5fb, 0x1f64f},
    {0x1f680, 0x1f6c5},
    {0x1f6cc, 0x1f6cc},
    {0x1f6d0, 0x1f6d2},
    {0x1f6d5, 0x1f6d5},
    {0x1f6eb, 0x1f6ec},
    {0x1f6f4, 0x1f6fa},
    {0x1f7e0, 0x1f7eb},
    {0x1f90d, 0x1f971},
    {0x1f973, 0x1f976},
    {0x1f97a, 0x1f9a2},
    {0x1f9a5, 0x1f9aa},
    {0x1f9ae, 0x1f9ca},
    {0x1f9cd, 0x1f9ff},
    {0x1fa70, 0x1fa73},
    {0x1fa78, 0x1fa7a},
    {0x1fa80, 0x1fa82},
    {0x1fa90, 0x1fa95},
    {0x20000, 0x2fffd},
    {0x30000, 0x3fffd},
};

static const CodepointRange unprintable[] = {
    {0x0080, 0x009f},
    {0x0378, 0x0379},
    {0x0380, 0x0383},
    {0x038b, 0x038b},
    {0x038d, 0x038d},
    {0x03a2, 0x03a2},
    {0x0530, 0x0530},
    {0x0557, 0x0558},
    {0x058b, 0x058c},
    {0x0590, 0x0590},
    {0x05c8, 0x05cf},
    {0x05eb, 0x05ee},
    {0x05f5, 0x05ff},
    {0x061d, 0x061d},
    {0x070e, 0x070e},
    {0x074b, 0x074c},
    {0x07b2, 0x07bf},
    {0x07fb, 0x07fc},
    {0x082e, 0x082f},
    {0x083f, 0x083f},
    {0x085c, 0x085d},
    {0x085f, 0x085f},
    {0x086b, 0x089f},
    {0x08b5, 0x08b5},
    {0x08be, 0x08d2},
    {0x0984, 0x0984},
    {0x098d, 0x098e},
    {0x0991, 0x0992},
    {0x09a9, 0x09a9},
    {0x09b1, 0x09b1},
    {0x09b3, 0x09b5},
    {0x09ba, 0x09bb},
    {0x09c5, 0x09c6},
    {0x09c9, 0x09ca},
    {0x09cf, 0x09d6},
    {0x09d8, 0x09db},
    {0x09de, 0x09de},
    {0x09e4, 0x09e5},
    {0x09ff, 0x0a00},
    {0x0a04, 0x0a04},
    {0x0a0b, 0x0a0e},
    {0x0a11, 0x0a12},
    {0x0a29, 0x0a29},
    {0x0a31, 0x0a31},
    {0x0a34, 0x0a34},
    {0x0a37, 0x0a37},
    {0x0a3a, 0x0a3b},
    {0x0a3d, 0x0a3d},
    {0x0a43, 0x0a46},
    {0x0a49, 0x0a4a},
    {0x0a4e, 0x0a50},
    {0x0a52, 0x0a58},
    {0x0a5d, 0x0a5d},
    {0x0a5f, 0x0a65},
    {0x0a77, 0x0a80},
    {0x0a84, 0x0a84},
    {0x0a8e, 0x0a8e},
    {0x0a92, 0x0a92},
    {0x0aa9, 0x0aa9},
    {0x0ab1, 0x0ab1},
    {0x0ab4, 0x0ab4},
    {0x0aba, 0x0abb},
    {0x0ac6, 0x0ac6},
    {0x0aca, 0x0aca},
    {0x0ace, 0x0acf},
    {0x0ad1, 0x0adf},
    {0x0ae4, 0x0ae5},
    {0x0af2, 0x0af8},
    {0x0b00, 0x0b00},
    {0x0b04, 0x0b04},
    {0x0b0d, 0x0b0e},
    {0x0b11, 0x0b12},
    {0x0b29, 0x0b29},
    {0x0b31, 0x0b31},
    {0x0b34, 0x0b34},
    {0x0b3a, 0x0b3b},
    {0x0b45, 0x0b46},
    {0x0b49, 0x0b4a},
    {0x0b4e, 0x0b55},
    {0x0b58, 0x0b5b},
    {0x0b5e, 0x0b5e},
    {0x0b64, 0x0b65},
    {0x0b78, 0x0b81},
    {0x0b84, 0x0b84},
    {0x0b8b, 0x0b8d},
    {0x0b91, 0x0b91},
    {0x0b96, 0x0b98},
    {0x0b9b, 0x0b9b},
    {0x0b9d, 0x0b9d},
    {0x0ba0, 0x0ba2},
    {0x0ba5, 0x0ba7},
    {0x0bab, 0x0bad},
    {0x0bba, 0x0bbd},
    {0x0bc3, 0x0bc5},
    {0x0bc9, 0x0bc9},
    {0x0bce, 0x0bcf},
    {0x0bd1, 0x0bd6},
    {0x0bd8, 0x0be5},
    {0x0bfb, 0x0bff},
    {0x0c0d, 0x0c0d},
    {0x0c11, 0x0c11},
    {0x0c29, 0x0c29},
    {0x0c3a, 0x0c3c},
    {0x0c45, 0x0c45},
    {0x0c49, 0x0c49},
    {0x0c4e, 0x0c54},
    {0x0c57, 0x0c57},
    {0x0c5b, 0x0c5f},
    {0x0c64, 0x0c65},
    {0x0c70, 0x0c76},
    {0x0c8d, 0x0c8d},
    {0x0c91, 0x0c91},
    {0x0ca9, 0x0ca9},
    {0x0cb4, 0x0cb4},
    {0x0cba, 0x0cbb},
    {0x0cc5, 0x0cc5},
    {0x0cc9, 0x0cc9},
    {0x0cce, 0x0cd4},
    {0x0cd7, 0x0cdd},
    {0x0cdf, 0x0cdf},
    {0x0ce4, 0x0ce5},
    {0x0cf0, 0x0cf0},
    {0x0cf3, 0x0cff},
    {0x0d04, 0x0d04},
    {0x0d0d, 0x0d0d},
    {0x0d11, 0x0d11},
    {0x0d45, 0x0d45},
    {0x0d49, 0x0d49},
    {0x0d50, 0x0d53},
    {0x0d64, 0x0d65},
    {0x0d80, 0x0d81},
    {0x0d84, 0x0d84},
    {0x0d97, 0x0d99},
    {0x0db2, 0x0db2},
    {0x0dbc, 0x0dbc},
    {0x0dbe, 0x0dbf},
    {0x0dc7, 0x0dc9},
    {0x0dcb, 0x0dce},
    {0x0dd5, 0x0dd5},
    {0x0dd7, 0x0dd7},
    {0x0de0, 0x0de5},
    {0x0df0, 0x0df1},
    {0x0df5, 0x0e00},
    {0x0e3b, 0x0e3e},
    {0x0e5c, 0x0e80},
    {0x0e83, 0x0e83},
    {0x0e85, 0x0e85},
    {0x0e8b, 0x0e8b},
    {0x0ea4, 0x0ea4},
    {0x0ea6, 0x0ea6},
    {0x0ebe, 0x0ebf},
    {0x0ec5, 0x0ec5},
    {0x0ec7, 0x0ec7},
    {0x0ece, 0x0ecf},
    {0x0eda, 0x0edb},
    {0x0ee0, 0x0eff},
    {0x0f48, 0x0f48},
    {0x0f6d, 0x0f70},
    {0x0f98, 0x0f98},
    {0x0fbd, 0x0fbd},
    {0x0fcd, 0x0fcd},
    {0x0fdb, 0x0fff},
    {0x10c6, 0x10c6},
    {0x10c8, 0x10cc},
    {0x10ce, 0x10cf},
    {0x1249, 0x1249},
    {0x124e, 0x124f},
    {0x1257, 0x1257},
    {0x1259, 0x1259},
    {0x125e, 0x125f},
    {0x1289, 0x1289},
    {0x128e, 0x128f},
    {0x12b1, 0x12b1},
    {0x12b6, 0x12b7},
    {0x12bf, 0x12bf},
    {0x12c1, 0x12c1},
    {0x12c6, 0x12c7},
    {0x12d7, 0x12d7},
    {0x1311, 0x1311},
    {0x1316, 0x1317},
    {0x135b, 0x135c},
    {0x137d, 0x137f},
    {0x139a, 0x139f},
    {0x13f6, 0x13f7},
    {0x13fe, 0x13ff},
    {0x169d, 0x169f},
    {0x16f9, 0x16ff},
    {0x170d, 0x170d},
    {0x1715, 0x171f},
    {0x1737, 0x173f},
    {0x1754, 0x175f},
    {0x176d, 0x176d},
    {0x1771, 0x1771},
    {0x1774, 0x177f},
    {0x17de, 0x17df},
    {0x17ea, 0x17ef},
    {0x17fa, 0x17ff},
    {0x180f, 0x180f},
    {0x181a, 0x181f},
    {0x1879, 0x187f},
    {0x18ab, 0x18af},
    {0x18f6, 0x18ff},
    {0x191f, 0x191f},
    {0x192c, 0x192f},
    {0x193c, 0x193f},
    {0x1941, 0x1943},
    {0x196e, 0x196f},
    {0x1975, 0x197f},
    {0x19ac, 0x19af},
    {0x19ca, 0x19cf},
    {0x19db, 0x19dd},
    {0x1a1c, 0x1a1d},
    {0x1a5f, 0x1a5f},
    {0x1a7d, 0x1a7e},
    {0x1a8a, 0x1a8f},
    {0x1a9a, 0x1a9f},
    {0x1aae, 0x1aaf},
    {0x1abf, 0x1aff},
    {0x1b4c, 0x1b4f},
    {0x1b7d, 0x1b7f},
    {0x1bf4, 0x1bfb},
    {0x1c38, 0x1c3a},
    {0x1c4a, 0x1c4c},
    {0x1c89, 0x1c8f},
    {0x1cbb, 0x1cbc},
    {0x1cc8, 0x1ccf},
    {0x1cfb, 0x1cff},
    {0x1dfa, 0x1dfa},
    {0x1f16, 0x1f17},
    {0x1f1e, 0x1f1f},
    {0x1f46, 0x1f47},
    {0x1f4e, 0x1f4f},
    {0x1f58, 0x1f58},
    {0x1f5a, 0x1f5a},
    {0x1f5c, 0x1f5c},
    {0x1f5e, 0x1f5e},
    {0x1f7e, 0x1f7f},
    {0x1fb5, 0x1fb5},
    {0x1fc5, 0x1fc5},
    {0x1fd4, 0x1fd5},
    {0x1fdc, 0x1fdc},
    {0x1ff0, 0x1ff1},
    {0x1ff5, 0x1ff5},
    {0x1fff, 0x1fff},
    {0x2065, 0x2065},
    {0x2072, 0x2073},
    {0x208f, 0x208f},
    {0x209d, 0x209f},
    {0x20c0, 0x20cf},
    {0x20f1, 0x20ff},
    {0x218c, 0x218f},
    {0x2427, 0x243f},
    {0x244b, 0x245f},
    {0x2b74, 0x2b75},
    {0x2b96, 0x2b97},
    {0x2c2f, 0x2c2f},
    {0x2c5f, 0x2c5f},
    {0x2cf4, 0x2cf8},
    {0x2d26, 0x2d26},
    {0x2d28, 0x2d2c},
    {0x2d2e, 0x2d2f},
    {0x2d68, 0x2d6e},
    {0x2d71, 0x2d7e},
    {0x2d97, 0x2d9f},
    {0x2da7, 0x2da7},
    {0x2daf, 0x2daf},
    {0x2db7, 0x2db7},
    {0x2dbf, 0x2dbf},
    {0x2dc7, 0x2dc7},
    {0x2dcf, 0x2dcf},
    {0x2dd7, 0x2dd7},
    {0x2ddf, 0x2ddf},
    {0x2e50, 0x2e7f},
    {0x2e9a, 0x2e9a},
    {0x2ef4, 0x2eff},
    {0x2fd6, 0x2fef},
    {0x2ffc, 0x2fff},
    {0x3040, 0x3040},
    {0x3097, 0x3098},
    {0x3100, 0x3104},
    {0x3130, 0x3130},
    {0x318f, 0x318f},
    {0x31bb, 0x31bf},
    {0x31e4, 0x31ef},
    {0x321f, 0x321f},
    {0x4db6, 0x4dbf},
    {0x9ff0, 0x9fff},
    {0xa48d, 0xa48f},
    {0xa4c7, 0xa4cf},
    {0xa62c, 0xa63f},
    {0xa6f8, 0xa6ff},
    {0xa7c0, 0xa7c1},
    {0xa7c7, 0xa7f6},
    {0xa82c, 0xa82f},
    {0xa83a, 0xa83f},
    {0xa878, 0xa87f},
    {0xa8c6, 0xa8cd},
    {0xa8da, 0xa8df},
    {0xa954, 0xa95e},
    {0xa97d, 0xa97f},
    {0xa9ce, 0xa9ce},
    {0xa9da, 0xa9dd},
    {0xa9ff, 0xa9ff},
    {0xaa37, 0xaa3f},
    {0xaa4e, 0xaa4f},
    {0xaa5a, 0xaa5b},
    {0xaac3, 0xaada},
    {0xaaf7, 0xab00},
    {0xab07, 0xab08},
    {0xab0f, 0xab10},
    {0xab17, 0xab1f},
    {0xab27, 0xab27},
    {0xab2f, 0xab2f},
    {0xab68, 0xab6f},
    {0xabee, 0xabef},
    {0xabfa, 0xabff},
    {0xd7a4, 0xd7af},
    {0xd7c7, 0xd7ca},
    {0xd7fc, 0xf8ff},
    {0xfa6e, 0xfa6f},
    {0xfada, 0xfaff},
    {0xfb07, 0xfb12},
    {0xfb18, 0xfb1c},
    {0xfb37, 0xfb37},
    {0xfb3d, 0xfb3d},
    {0xfb3f, 0xfb3f},
    {0xfb42, 0xfb42},
    {0xfb45, 0xfb45},
    {0xfbc2, 0xfbd2},
    {0xfd40, 0xfd4f},
    {0xfd90, 0xfd91},
    {0xfdc8, 0xfdef},
    {0xfdfe, 0xfdff},
    {0xfe1a, 0xfe1f},
    {0xfe53, 0xfe53},
    {0xfe67, 0xfe67},
    {0xfe6c, 0xfe6f},
    {0xfe75, 0xfe75},
    {0xfefd, 0xfefe},
    {0xff00, 0xff00},
    {0xffbf, 0xffc1},
    {0xffc8, 0xffc9},
    {0xffd0, 0xffd1},
    {0xffd8, 0xffd9},
    {0xffdd, 0xffdf},
    {0xffe7, 0xffe7},
    {0xffef, 0xfff8},
    {0xfffe, 0xffff},
    {0x1000c, 0x1000c},
    {0x10027, 0x10027},
    {0x1003b, 0x1003b},
    {0x1003e, 0x1003e},
    {0x1004e, 0x1004f},
    {0x1005e, 0x1007f},
    {0x100fb, 0x100ff},
    {0x10103, 0x10106},
    {0x10134, 0x10136},
    {0x1018f, 0x1018f},
    {0x1019c, 0x1019f},
    {0x101a1, 0x101cf},
    {0x101fe, 0x1027f},
    {0x1029d, 0x1029f},
    {0x102d1, 0x102df},
    {0x102fc, 0x102ff},
    {0x10324, 0x1032c},
    {0x1034b, 0x1034f},
    {0x1037b, 0x1037f},
    {0x1039e, 0x1039e},
    {0x103c4, 0x103c7},
    {0x103d6, 0x103ff},
    {0x1049e, 0x1049f},
    {0x104aa, 0x104af},
    {0x104d4, 0x104d7},
    {0x104fc, 0x104ff},
    {0x10528, 0x1052f},
    {0x10564, 0x1056e},
    {0x10570, 0x105ff},
    {0x10737, 0x1073f},
    {0x10756, 0x1075f},
    {0x10768, 0x107ff},
    {0x10806, 0x10807},
    {0x10809, 0x10809},
    {0x10836, 0x10836},
    {0x10839, 0x1083b},
    {0x1083d, 0x1083e},
    {0x10856, 0x10856},
    {0x1089f, 0x108a6},
    {0x108b0, 0x108df},
    {0x108f3, 0x108f3},
    {0x108f6, 0x108fa},
    {0x1091c, 0x1091e},
    {0x1093a, 0x1093e},
    {0x10940, 0x1097f},
    {0x109b8, 0x109bb},
    {0x109d0, 0x109d1},
    {0x10a04, 0x10a04},
    {0x10a07, 0x10a0b},
    {0x10a14, 0x10a14},
    {0x10a18, 0x10a18},
    {0x10a36, 0x10a37},
    {0x10a3b, 0x10a3e},
    {0x10a49, 0x10a4f},
    {0x10a59, 0x10a5f},
    {0x10aa0, 0x10abf},
    {0x10ae7, 0x10aea},
    {0x10af7, 0x10aff},
    {0x10b36, 0x10b38},
    {0x10b56, 0x10b57},
    {0x10b73, 0x10b77},
    {0x10b92, 0x10b98},
    {0x10b9d, 0x10ba8},
    {0x10bb0, 0x10bff},
    {0x10c49, 0x10c7f},
    {0x10cb3, 0x10cbf},
    {0x10cf3, 0x10cf9},
    {0x10d28, 0x10d2f},
    {0x10d3a, 0x10e5f},
    {0x10e7f, 0x10eff},
    {0x10f28, 0x10f2f},
    {0x10f5a, 0x10fdf},
    {0x10ff7, 0x10fff},
    {0x1104e, 0x11051},
    {0x11070, 0x1107e},
    {0x110c2, 0x110cc},
    {0x110ce, 0x110cf},
    {0x110e9, 0x110ef},
    {0x110fa, 0x110ff},
    {0x11135, 0x11135},
    {0x11147, 0x1114f},
    {0x11177, 0x1117f},
    {0x111ce, 0x111cf},
    {0x111e0, 0x111e0},
    {0x111f5, 0x111ff},
    {0x11212, 0x11212},
    {0x1123f, 0x1127f},
    {0x11287, 0x11287},
    {0x11289, 0x11289},
    {0x1128e, 0x1128e},
    {0x1129e, 0x1129e},
    {0x112aa, 0x112af},
    {0x112eb, 0x112ef},
    {0x112fa, 0x112ff},
    {0x11304, 0x11304},
    {0x1130d, 0x1130e},
    {0x11311, 0x11312},
    {0x11329, 0x11329},
    {0x11331, 0x11331},
    {0x11334, 0x11334},
    {0x1133a, 0x1133a},
    {0x11345, 0x11346},
    {0x11349, 0x1134a},
    {0x1134e, 0x1134f},
    {0x11351, 0x11356},
    {0x11358, 0x1135c},
    {0x11364, 0x11365},
    {0x1136d, 0x1136f},
    {0x11375, 0x113ff},
    {0x1145a, 0x1145a},
    {0x1145c, 0x1145c},
    {0x11460, 0x1147f},
    {0x114c8, 0x114cf},
    {0x114da, 0x1157f},
    {0x115b6, 0x115b7},
    {0x115de, 0x115ff},
    {0x11645, 0x1164f},
    {0x1165a, 0x1165f},
    {0x1166d, 0x1167f},
    {0x116b9, 0x116bf},
    {0x116ca, 0x116ff},
    {0x1171b, 0x1171c},
    {0x1172c, 0x1172f},
    {0x11740, 0x117ff},
    {0x1183c, 0x1189f},
    {0x118f3, 0x118fe},
    {0x11900, 0x1199f},
    {0x119a8, 0x119a9},
    {0x119d8, 0x119d9},
    {0x119e5, 0x119ff},
    {0x11a48, 0x11a4f},
    {0x11aa3, 0x11abf},
    {0x11af9, 0x11bff},
    {0x11c09, 0x11c09},
    {0x11c37, 0x11c37},
    {0x11c46, 0x11c4f},
    {0x11c6d, 0x11c6f},
    {0x11c90, 0x11c91},
    {0x11ca8, 0x11ca8},
    {0x11cb7, 0x11cff},
    {0x11d07, 0x11d07},
    {0x11d0a, 0x11d0a},
    {0x11d37, 0x11d39},
    {0x11d3b, 0x11d3b},
    {0x11d3e, 0x11d3e},
    {0x11d48, 0x11d4f},
    {0x11d5a, 0x11d5f},
    {0x11d66, 0x11d66},
    {0x11d69, 0x11d69},
    {0x11d8f, 0x11d8f},
    {0x11d92, 0x11d92},
    {0x11d99, 0x11d9f},
    {0x11daa, 0x11edf},
    {0x11ef9, 0x11fbf},
    {0x11ff2, 0x11ffe},
    {0x1239a, 0x123ff},
    {0x1246f, 0x1246f},
    {0x12475, 0x1247f},
    {0x12544, 0x12fff},
    {0x1342f, 0x1342f},
    {0x13439, 0x143ff},
    {0x14647, 0x167ff},
    {0x16a39, 0x16a3f},
    {0x16a5f, 0x16a5f},
    {0x16a6a, 0x16a6d},
    {0x16a70, 0x16acf},
    {0x16aee, 0x16aef},
    {0x16af6, 0x16aff},
    {0x16b46, 0x16b4f},
    {0x16b5a, 0x16b5a},
    {0x16b62, 0x16b62},
    {0x16b78, 0x16b7c},
    {0x16b90, 0x16e3f},
    {0x16e9b, 0x16eff},
    {0x16f4b, 0x16f4e},
    {0x16f88, 0x16f8e},
    {0x16fa0, 0x16fdf},
    {0x16fe4, 0x16fff},
    {0x187f8, 0x187ff},
    {0x18af3, 0x1afff},
    {0x1b11f, 0x1b14f},
    {0x1b153, 0x1b163},
    {0x1b168, 0x1b16f},
    {0x1b2fc, 0x1bbff},
    {0x1bc6b, 0x1bc6f},
    {0x1bc7d, 0x1bc7f},
    {0x1bc89, 0x1bc8f},
    {0x1bc9a, 0x1bc9b},
    {0x1bca4, 0x1cfff},
    {0x1d0f6, 0x1d0ff},
    {0x1d127, 0x1d128},
    {0x1d1e9, 0x1d1ff},
    {0x1d246, 0x1d2df},
    {0x1d2f4, 0x1d2ff},
    {0x1d357, 0x1d35f},
    {0x1d379, 0x1d3ff},
    {0x1d455, 0x1d455},
    {0x1d49d, 0x1d49d},
    {0x1d4a0, 0x1d4a1},
    {0x1d4a3, 0x1d4a4},
    {0x1d4a7, 0x1d4a8},
    {0x1d4ad, 0x1d4ad},
    {0x1d4ba, 0x1d4ba},
    {0x1d4bc, 0x1d4bc},
    {0x1d4c4, 0x1d4c4},
    {0x1d506, 0x1d506},
    {0x1d50b, 0x1d50c},
    {0x1d515, 0x1d515},
    {0x1d51d, 0x1d51d},
    {0x1d53a, 0x1d53a},
    {0x1d53f, 0x1d53f},
    {0x1d545, 0x1d545},
    {0x1d547, 0x1d549},
    {0x1d551, 0x1d551},
    {0x1d6a6, 0x1d6a7},
    {0x1d7cc, 0x1d7cd},
    {0x1da8c, 0x1da9a},
    {0x1daa0, 0x1daa0},
    {0x1dab0, 0x1dfff},
    {0x1e007, 0x1e007},
    {0x1e019, 0x1e01a},
    {0x1e022, 0x1e022},
    {0x1e025, 0x1e025},
    {0x1e02b, 0x1e0ff},
    {0x1e12d, 0x1e12f},
    {0x1e13e, 0x1e13f},
    {0x1e14a, 0x1e14d},
    {0x1e150, 0x1e2bf},
    {0x1e2fa, 0x1e2fe},
    {0x1e300, 0x1e7ff},
    {0x1e8c5, 0x1e8c6},
    {0x1e8d7, 0x1e8ff},
    {0x1e94c, 0x1e94f},
    {0x1e95a, 0x1e95d},
    {0x1e960, 0x1ec70},
    {0x1ecb5, 0x1ed00},
    {0x1ed3e, 0x1edff},
    {0x1ee04, 0x1ee04},
    {0x1ee20, 0x1ee20},
    {0x1ee23, 0x1ee23},
    {0x1ee25, 0x1ee26},
    {0x1ee28, 0x1ee28},
    {0x1ee33, 0x1ee33},
    {0x1ee38, 0x1ee38},
    {0x1ee3a, 0x1ee3a},
    {0x1ee3c, 0x1ee41},
    {0x1ee43, 0x1ee46},
    {0x1ee48, 0x1ee48},
    {0x1ee4a, 0x1ee4a},
    {0x1ee4c, 0x1ee4c},
    {0x1ee50, 0x1ee50},
    {0x1ee53, 0x1ee53},
    {0x1ee55, 0x1ee56},
    {0x1ee58, 0x1ee58},
    {0x1ee5a, 0x1ee5a},
    {0x1ee5c, 0x1ee5c},
    {0x1ee5e, 0x1ee5e},
    {0x1ee60, 0x1ee60},
    {0x1ee63, 0x1ee63},
    {0x1ee65, 0x1ee66},
    {0x1ee6b, 0x1ee6b},
    {0x1ee73, 0x1ee73},
    {0x1ee78, 0x1ee78},
    {0x1ee7d, 0x1ee7d},
    {0x1ee7f, 0x1ee7f},
    {0x1ee8a, 0x1ee8a},
    {0x1ee9c, 0x1eea0},
    {0x1eea4, 0x1eea4},
    {0x1eeaa, 0x1eeaa},
    {0x1eebc, 0x1eeef},
    {0x1eef2, 0x1efff},
    {0x1f02c, 0x1f02f},
    {0x1f094, 0x1f09f},
    {0x1f0af, 0x1f0b0},
    {0x1f0c0, 0x1f0c0},
    {0x1f0d0, 0x1f0d0},
    {0x1f0f6, 0x1f0ff},
    {0x1f10d, 0x1f10f},
    {0x1f16d, 0x1f16f},
    {0x1f1ad, 0x1f1e5},
    {0x1f203, 0x1f20f},
    {0x1f23c, 0x1f23f},
    {0x1f249, 0x1f24f},
    {0x1f252, 0x1f25f},
    {0x1f266, 0x1f2ff},
    {0x1f6d6, 0x1f6df},
    {0x1f6ed, 0x1f6ef},
    {0x1f6fb, 0x1f6ff},
    {0x1f774, 0x1f77f},
    {0x1f7d9, 0x1f7df},
    {0x1f7ec, 0x1f7ff},
    {0x1f80c, 0x1f80f},
    {0x1f848, 0x1f84f},
    {0x1f85a, 0x1f85f},
    {0x1f888, 0x1f88f},
    {0x1f8ae, 0x1f8ff},
    {0x1f90c, 0x1f90c},
    {0x1f972, 0x1f972},
    {0x1f977, 0x1f979},
    {0x1f9a3, 0x1f9a4},
    {0x1f9ab, 0x1f9ad},
    {0x1f9cb, 0x1f9cc},
    {0x1fa54, 0x1fa5f},
    {0x1fa6e, 0x1fa6f},
    {0x1fa74, 0x1fa77},
    {0x1fa7b, 0x1fa7f},
    {0x1fa83, 0x1fa8f},
    {0x1fa96, 0x1ffff},
    {0x2a6d7, 0x2a6ff},
    {0x2b735, 0x2b73f},
    {0x2b81e, 0x2b81f},
    {0x2cea2, 0x2ceaf},
    {0x2ebe1, 0x2f7ff},
    {0x2fa1e, 0xe0000},
    {0xe0002, 0xe001f},
    {0xe0080, 0xe00ff},
    {0xe01f0, 0x10ffff},
};

