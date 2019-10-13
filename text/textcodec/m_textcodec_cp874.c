/* The MIT License (MIT)
 * 
 * Copyright (c) 2018 Monetra Technologies, LLC.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "textcodec/m_textcodec_int.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Mapping table from The Unicode Consortium.
 * https://www.unicode.org/Public/MAPPINGS/ */
static M_textcodec_cp_map_t cp_map[] = {
	{ 0x00, 0x0000, "Null"                            },
	{ 0x01, 0x0001, "Start Of Heading"                },
	{ 0x02, 0x0002, "Start Of Text"                   },
	{ 0x03, 0x0003, "End Of Text"                     },
	{ 0x04, 0x0004, "End Of Transmission"             },
	{ 0x05, 0x0005, "Enquiry"                         },
	{ 0x06, 0x0006, "Acknowledge"                     },
	{ 0x07, 0x0007, "Bell"                            },
	{ 0x08, 0x0008, "Backspace"                       },
	{ 0x09, 0x0009, "Horizontal Tabulation"           },
	{ 0x0A, 0x000A, "Line Feed"                       },
	{ 0x0B, 0x000B, "Vertical Tabulation"             },
	{ 0x0C, 0x000C, "Form Feed"                       },
	{ 0x0D, 0x000D, "Carriage Return"                 },
	{ 0x0E, 0x000E, "Shift Out"                       },
	{ 0x0F, 0x000F, "Shift In"                        },
	{ 0x10, 0x0010, "Data Link Escape"                },
	{ 0x11, 0x0011, "Device Control One"              },
	{ 0x12, 0x0012, "Device Control Two"              },
	{ 0x13, 0x0013, "Device Control Three"            },
	{ 0x14, 0x0014, "Device Control Four"             },
	{ 0x15, 0x0015, "Negative Acknowledge"            },
	{ 0x16, 0x0016, "Synchronous Idle"                },
	{ 0x17, 0x0017, "End Of Transmission Block"       },
	{ 0x18, 0x0018, "Cancel"                          },
	{ 0x19, 0x0019, "End Of Medium"                   },
	{ 0x1A, 0x001A, "Substitute"                      },
	{ 0x1B, 0x001B, "Escape"                          },
	{ 0x1C, 0x001C, "File Separator"                  },
	{ 0x1D, 0x001D, "Group Separator"                 },
	{ 0x1E, 0x001E, "Record Separator"                },
	{ 0x1F, 0x001F, "Unit Separator"                  },
	{ 0x20, 0x0020, "Space"                           },
	{ 0x21, 0x0021, "Exclamation Mark"                },
	{ 0x22, 0x0022, "Quotation Mark"                  },
	{ 0x23, 0x0023, "Number Sign"                     },
	{ 0x24, 0x0024, "Dollar Sign"                     },
	{ 0x25, 0x0025, "Percent Sign"                    },
	{ 0x26, 0x0026, "Ampersand"                       },
	{ 0x27, 0x0027, "Apostrophe"                      },
	{ 0x28, 0x0028, "Left Parenthesis"                },
	{ 0x29, 0x0029, "Right Parenthesis"               },
	{ 0x2A, 0x002A, "Asterisk"                        },
	{ 0x2B, 0x002B, "Plus Sign"                       },
	{ 0x2C, 0x002C, "Comma"                           },
	{ 0x2D, 0x002D, "Hyphen-Minus"                    },
	{ 0x2E, 0x002E, "Full Stop"                       },
	{ 0x2F, 0x002F, "Solidus"                         },
	{ 0x30, 0x0030, "Digit Zero"                      },
	{ 0x31, 0x0031, "Digit One"                       },
	{ 0x32, 0x0032, "Digit Two"                       },
	{ 0x33, 0x0033, "Digit Three"                     },
	{ 0x34, 0x0034, "Digit Four"                      },
	{ 0x35, 0x0035, "Digit Five"                      },
	{ 0x36, 0x0036, "Digit Six"                       },
	{ 0x37, 0x0037, "Digit Seven"                     },
	{ 0x38, 0x0038, "Digit Eight"                     },
	{ 0x39, 0x0039, "Digit Nine"                      },
	{ 0x3A, 0x003A, "Colon"                           },
	{ 0x3B, 0x003B, "Semicolon"                       },
	{ 0x3C, 0x003C, "Less-Than Sign"                  },
	{ 0x3D, 0x003D, "Equals Sign"                     },
	{ 0x3E, 0x003E, "Greater-Than Sign"               },
	{ 0x3F, 0x003F, "Question Mark"                   },
	{ 0x40, 0x0040, "Commercial At"                   },
	{ 0x41, 0x0041, "Latin Capital Letter A"          },
	{ 0x42, 0x0042, "Latin Capital Letter B"          },
	{ 0x43, 0x0043, "Latin Capital Letter C"          },
	{ 0x44, 0x0044, "Latin Capital Letter D"          },
	{ 0x45, 0x0045, "Latin Capital Letter E"          },
	{ 0x46, 0x0046, "Latin Capital Letter F"          },
	{ 0x47, 0x0047, "Latin Capital Letter G"          },
	{ 0x48, 0x0048, "Latin Capital Letter H"          },
	{ 0x49, 0x0049, "Latin Capital Letter I"          },
	{ 0x4A, 0x004A, "Latin Capital Letter J"          },
	{ 0x4B, 0x004B, "Latin Capital Letter K"          },
	{ 0x4C, 0x004C, "Latin Capital Letter L"          },
	{ 0x4D, 0x004D, "Latin Capital Letter M"          },
	{ 0x4E, 0x004E, "Latin Capital Letter N"          },
	{ 0x4F, 0x004F, "Latin Capital Letter O"          },
	{ 0x50, 0x0050, "Latin Capital Letter P"          },
	{ 0x51, 0x0051, "Latin Capital Letter Q"          },
	{ 0x52, 0x0052, "Latin Capital Letter R"          },
	{ 0x53, 0x0053, "Latin Capital Letter S"          },
	{ 0x54, 0x0054, "Latin Capital Letter T"          },
	{ 0x55, 0x0055, "Latin Capital Letter U"          },
	{ 0x56, 0x0056, "Latin Capital Letter V"          },
	{ 0x57, 0x0057, "Latin Capital Letter W"          },
	{ 0x58, 0x0058, "Latin Capital Letter X"          },
	{ 0x59, 0x0059, "Latin Capital Letter Y"          },
	{ 0x5A, 0x005A, "Latin Capital Letter Z"          },
	{ 0x5B, 0x005B, "Left Square Bracket"             },
	{ 0x5C, 0x005C, "Reverse Solidus"                 },
	{ 0x5D, 0x005D, "Right Square Bracket"            },
	{ 0x5E, 0x005E, "Circumflex Accent"               },
	{ 0x5F, 0x005F, "Low Line"                        },
	{ 0x60, 0x0060, "Grave Accent"                    },
	{ 0x61, 0x0061, "Latin Small Letter A"            },
	{ 0x62, 0x0062, "Latin Small Letter B"            },
	{ 0x63, 0x0063, "Latin Small Letter C"            },
	{ 0x64, 0x0064, "Latin Small Letter D"            },
	{ 0x65, 0x0065, "Latin Small Letter E"            },
	{ 0x66, 0x0066, "Latin Small Letter F"            },
	{ 0x67, 0x0067, "Latin Small Letter G"            },
	{ 0x68, 0x0068, "Latin Small Letter H"            },
	{ 0x69, 0x0069, "Latin Small Letter I"            },
	{ 0x6A, 0x006A, "Latin Small Letter J"            },
	{ 0x6B, 0x006B, "Latin Small Letter K"            },
	{ 0x6C, 0x006C, "Latin Small Letter L"            },
	{ 0x6D, 0x006D, "Latin Small Letter M"            },
	{ 0x6E, 0x006E, "Latin Small Letter N"            },
	{ 0x6F, 0x006F, "Latin Small Letter O"            },
	{ 0x70, 0x0070, "Latin Small Letter P"            },
	{ 0x71, 0x0071, "Latin Small Letter Q"            },
	{ 0x72, 0x0072, "Latin Small Letter R"            },
	{ 0x73, 0x0073, "Latin Small Letter S"            },
	{ 0x74, 0x0074, "Latin Small Letter T"            },
	{ 0x75, 0x0075, "Latin Small Letter U"            },
	{ 0x76, 0x0076, "Latin Small Letter V"            },
	{ 0x77, 0x0077, "Latin Small Letter W"            },
	{ 0x78, 0x0078, "Latin Small Letter X"            },
	{ 0x79, 0x0079, "Latin Small Letter Y"            },
	{ 0x7A, 0x007A, "Latin Small Letter Z"            },
	{ 0x7B, 0x007B, "Left Curly Bracket"              },
	{ 0x7C, 0x007C, "Vertical Line"                   },
	{ 0x7D, 0x007D, "Right Curly Bracket"             },
	{ 0x7E, 0x007E, "Tilde"                           },
	{ 0x7F, 0x007F, "Delete"                          },
	{ 0x80, 0x20AC, "Euro Sign"                       },
	{ 0x85, 0x2026, "Horizontal Ellipsis"             },
	{ 0x91, 0x2018, "Left Single Quotation Mark"      },
	{ 0x92, 0x2019, "Right Single Quotation Mark"     },
	{ 0x93, 0x201C, "Left Double Quotation Mark"      },
	{ 0x94, 0x201D, "Right Double Quotation Mark"     },
	{ 0x95, 0x2022, "Bullet"                          },
	{ 0x96, 0x2013, "En Dash"                         },
	{ 0x97, 0x2014, "Em Dash"                         },
	{ 0xA0, 0x00A0, "No-Break Space"                  },
	{ 0xA1, 0x0E01, "Thai Character Ko Kai"           },
	{ 0xA2, 0x0E02, "Thai Character Kho Khai"         },
	{ 0xA3, 0x0E03, "Thai Character Kho Khuat"        },
	{ 0xA4, 0x0E04, "Thai Character Kho Khwai"        },
	{ 0xA5, 0x0E05, "Thai Character Kho Khon"         },
	{ 0xA6, 0x0E06, "Thai Character Kho Rakhang"      },
	{ 0xA7, 0x0E07, "Thai Character Ngo Ngu"          },
	{ 0xA8, 0x0E08, "Thai Character Cho Chan"         },
	{ 0xA9, 0x0E09, "Thai Character Cho Ching"        },
	{ 0xAA, 0x0E0A, "Thai Character Cho Chang"        },
	{ 0xAB, 0x0E0B, "Thai Character So So"            },
	{ 0xAC, 0x0E0C, "Thai Character Cho Choe"         },
	{ 0xAD, 0x0E0D, "Thai Character Yo Ying"          },
	{ 0xAE, 0x0E0E, "Thai Character Do Chada"         },
	{ 0xAF, 0x0E0F, "Thai Character To Patak"         },
	{ 0xB0, 0x0E10, "Thai Character Tho Than"         },
	{ 0xB1, 0x0E11, "Thai Character Tho Nangmontho"   },
	{ 0xB2, 0x0E12, "Thai Character Tho Phuthao"      },
	{ 0xB3, 0x0E13, "Thai Character No Nen"           },
	{ 0xB4, 0x0E14, "Thai Character Do Dek"           },
	{ 0xB5, 0x0E15, "Thai Character To Tao"           },
	{ 0xB6, 0x0E16, "Thai Character Tho Thung"        },
	{ 0xB7, 0x0E17, "Thai Character Tho Thahan"       },
	{ 0xB8, 0x0E18, "Thai Character Tho Thong"        },
	{ 0xB9, 0x0E19, "Thai Character No Nu"            },
	{ 0xBA, 0x0E1A, "Thai Character Bo Baimai"        },
	{ 0xBB, 0x0E1B, "Thai Character Po Pla"           },
	{ 0xBC, 0x0E1C, "Thai Character Pho Phung"        },
	{ 0xBD, 0x0E1D, "Thai Character Fo Fa"            },
	{ 0xBE, 0x0E1E, "Thai Character Pho Phan"         },
	{ 0xBF, 0x0E1F, "Thai Character Fo Fan"           },
	{ 0xC0, 0x0E20, "Thai Character Pho Samphao"      },
	{ 0xC1, 0x0E21, "Thai Character Mo Ma"            },
	{ 0xC2, 0x0E22, "Thai Character Yo Yak"           },
	{ 0xC3, 0x0E23, "Thai Character Ro Rua"           },
	{ 0xC4, 0x0E24, "Thai Character Ru"               },
	{ 0xC5, 0x0E25, "Thai Character Lo Ling"          },
	{ 0xC6, 0x0E26, "Thai Character Lu"               },
	{ 0xC7, 0x0E27, "Thai Character Wo Waen"          },
	{ 0xC8, 0x0E28, "Thai Character So Sala"          },
	{ 0xC9, 0x0E29, "Thai Character So Rusi"          },
	{ 0xCA, 0x0E2A, "Thai Character So Sua"           },
	{ 0xCB, 0x0E2B, "Thai Character Ho Hip"           },
	{ 0xCC, 0x0E2C, "Thai Character Lo Chula"         },
	{ 0xCD, 0x0E2D, "Thai Character O Ang"            },
	{ 0xCE, 0x0E2E, "Thai Character Ho Nokhuk"        },
	{ 0xCF, 0x0E2F, "Thai Character Paiyannoi"        },
	{ 0xD0, 0x0E30, "Thai Character Sara A"           },
	{ 0xD1, 0x0E31, "Thai Character Mai Han-Akat"     },
	{ 0xD2, 0x0E32, "Thai Character Sara Aa"          },
	{ 0xD3, 0x0E33, "Thai Character Sara Am"          },
	{ 0xD4, 0x0E34, "Thai Character Sara I"           },
	{ 0xD5, 0x0E35, "Thai Character Sara Ii"          },
	{ 0xD6, 0x0E36, "Thai Character Sara Ue"          },
	{ 0xD7, 0x0E37, "Thai Character Sara Uee"         },
	{ 0xD8, 0x0E38, "Thai Character Sara U"           },
	{ 0xD9, 0x0E39, "Thai Character Sara Uu"          },
	{ 0xDA, 0x0E3A, "Thai Character Phinthu"          },
	{ 0xDF, 0x0E3F, "Thai Currency Symbol Baht"       },
	{ 0xE0, 0x0E40, "Thai Character Sara E"           },
	{ 0xE1, 0x0E41, "Thai Character Sara Ae"          },
	{ 0xE2, 0x0E42, "Thai Character Sara O"           },
	{ 0xE3, 0x0E43, "Thai Character Sara Ai Maimuan"  },
	{ 0xE4, 0x0E44, "Thai Character Sara Ai Maimalai" },
	{ 0xE5, 0x0E45, "Thai Character Lakkhangyao"      },
	{ 0xE6, 0x0E46, "Thai Character Maiyamok"         },
	{ 0xE7, 0x0E47, "Thai Character Maitaikhu"        },
	{ 0xE8, 0x0E48, "Thai Character Mai Ek"           },
	{ 0xE9, 0x0E49, "Thai Character Mai Tho"          },
	{ 0xEA, 0x0E4A, "Thai Character Mai Tri"          },
	{ 0xEB, 0x0E4B, "Thai Character Mai Chattawa"     },
	{ 0xEC, 0x0E4C, "Thai Character Thanthakhat"      },
	{ 0xED, 0x0E4D, "Thai Character Nikhahit"         },
	{ 0xEE, 0x0E4E, "Thai Character Yamakkan"         },
	{ 0xEF, 0x0E4F, "Thai Character Fongman"          },
	{ 0xF0, 0x0E50, "Thai Digit Zero"                 },
	{ 0xF1, 0x0E51, "Thai Digit One"                  },
	{ 0xF2, 0x0E52, "Thai Digit Two"                  },
	{ 0xF3, 0x0E53, "Thai Digit Three"                },
	{ 0xF4, 0x0E54, "Thai Digit Four"                 },
	{ 0xF5, 0x0E55, "Thai Digit Five"                 },
	{ 0xF6, 0x0E56, "Thai Digit Six"                  },
	{ 0xF7, 0x0E57, "Thai Digit Seven"                },
	{ 0xF8, 0x0E58, "Thai Digit Eight"                },
	{ 0xF9, 0x0E59, "Thai Digit Nine"                 },
	{ 0xFA, 0x0E5A, "Thai Character Angkhankhu"       },
	{ 0xFB, 0x0E5B, "Thai Character Khomut"           },
	{ 0, 0, NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

M_textcodec_error_t M_textcodec_encode_cp874(M_textcodec_buffer_t *buf, const char *in, M_textcodec_ehandler_t ehandler)
{
	return M_textcodec_encode_cp_map(buf, in, ehandler, cp_map);
}

M_textcodec_error_t M_textcodec_decode_cp874(M_textcodec_buffer_t *buf, const char *in, M_textcodec_ehandler_t ehandler)
{
	return M_textcodec_decode_cp_map(buf, in, ehandler, cp_map);
}