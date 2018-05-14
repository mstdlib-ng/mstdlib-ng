/* The MIT License (MIT)
 * 
 * Copyright (c) 2018 Main Street Softworks, Inc.
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

#include "m_config.h"

#include <mstdlib/mstdlib.h>
#include "textcodec/m_textcodec_int.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Mapping table from The Unicode Consortium.
 * https://www.unicode.org/Public/MAPPINGS/ */
static M_textcodec_cp_map_t iso8859_10_map[] = {
	{ 0x00, 0x0000, "Null"                                   },
	{ 0x01, 0x0001, "Start Of Heading"                       },
	{ 0x02, 0x0002, "Start Of Text"                          },
	{ 0x03, 0x0003, "End Of Text"                            },
	{ 0x04, 0x0004, "End Of Transmission"                    },
	{ 0x05, 0x0005, "Enquiry"                                },
	{ 0x06, 0x0006, "Acknowledge"                            },
	{ 0x07, 0x0007, "Bell"                                   },
	{ 0x08, 0x0008, "Backspace"                              },
	{ 0x09, 0x0009, "Horizontal Tabulation"                  },
	{ 0x0A, 0x000A, "Line Feed"                              },
	{ 0x0B, 0x000B, "Vertical Tabulation"                    },
	{ 0x0C, 0x000C, "Form Feed"                              },
	{ 0x0D, 0x000D, "Carriage Return"                        },
	{ 0x0E, 0x000E, "Shift Out"                              },
	{ 0x0F, 0x000F, "Shift In"                               },
	{ 0x10, 0x0010, "Data Link Escape"                       },
	{ 0x11, 0x0011, "Device Control One"                     },
	{ 0x12, 0x0012, "Device Control Two"                     },
	{ 0x13, 0x0013, "Device Control Three"                   },
	{ 0x14, 0x0014, "Device Control Four"                    },
	{ 0x15, 0x0015, "Negative Acknowledge"                   },
	{ 0x16, 0x0016, "Synchronous Idle"                       },
	{ 0x17, 0x0017, "End Of Transmission Block"              },
	{ 0x18, 0x0018, "Cancel"                                 },
	{ 0x19, 0x0019, "End Of Medium"                          },
	{ 0x1A, 0x001A, "Substitute"                             },
	{ 0x1B, 0x001B, "Escape"                                 },
	{ 0x1C, 0x001C, "File Separator"                         },
	{ 0x1D, 0x001D, "Group Separator"                        },
	{ 0x1E, 0x001E, "Record Separator"                       },
	{ 0x1F, 0x001F, "Unit Separator"                         },
	{ 0x20, 0x0020, "Space"                                  },
	{ 0x21, 0x0021, "Exclamation Mark"                       },
	{ 0x22, 0x0022, "Quotation Mark"                         },
	{ 0x23, 0x0023, "Number Sign"                            },
	{ 0x24, 0x0024, "Dollar Sign"                            },
	{ 0x25, 0x0025, "Percent Sign"                           },
	{ 0x26, 0x0026, "Ampersand"                              },
	{ 0x27, 0x0027, "Apostrophe"                             },
	{ 0x28, 0x0028, "Left Parenthesis"                       },
	{ 0x29, 0x0029, "Right Parenthesis"                      },
	{ 0x2A, 0x002A, "Asterisk"                               },
	{ 0x2B, 0x002B, "Plus Sign"                              },
	{ 0x2C, 0x002C, "Comma"                                  },
	{ 0x2D, 0x002D, "Hyphen-Minus"                           },
	{ 0x2E, 0x002E, "Full Stop"                              },
	{ 0x2F, 0x002F, "Solidus"                                },
	{ 0x30, 0x0030, "Digit Zero"                             },
	{ 0x31, 0x0031, "Digit One"                              },
	{ 0x32, 0x0032, "Digit Two"                              },
	{ 0x33, 0x0033, "Digit Three"                            },
	{ 0x34, 0x0034, "Digit Four"                             },
	{ 0x35, 0x0035, "Digit Five"                             },
	{ 0x36, 0x0036, "Digit Six"                              },
	{ 0x37, 0x0037, "Digit Seven"                            },
	{ 0x38, 0x0038, "Digit Eight"                            },
	{ 0x39, 0x0039, "Digit Nine"                             },
	{ 0x3A, 0x003A, "Colon"                                  },
	{ 0x3B, 0x003B, "Semicolon"                              },
	{ 0x3C, 0x003C, "Less-Than Sign"                         },
	{ 0x3D, 0x003D, "Equals Sign"                            },
	{ 0x3E, 0x003E, "Greater-Than Sign"                      },
	{ 0x3F, 0x003F, "Question Mark"                          },
	{ 0x40, 0x0040, "Commercial At"                          },
	{ 0x41, 0x0041, "Latin Capital Letter A"                 },
	{ 0x42, 0x0042, "Latin Capital Letter B"                 },
	{ 0x43, 0x0043, "Latin Capital Letter C"                 },
	{ 0x44, 0x0044, "Latin Capital Letter D"                 },
	{ 0x45, 0x0045, "Latin Capital Letter E"                 },
	{ 0x46, 0x0046, "Latin Capital Letter F"                 },
	{ 0x47, 0x0047, "Latin Capital Letter G"                 },
	{ 0x48, 0x0048, "Latin Capital Letter H"                 },
	{ 0x49, 0x0049, "Latin Capital Letter I"                 },
	{ 0x4A, 0x004A, "Latin Capital Letter J"                 },
	{ 0x4B, 0x004B, "Latin Capital Letter K"                 },
	{ 0x4C, 0x004C, "Latin Capital Letter L"                 },
	{ 0x4D, 0x004D, "Latin Capital Letter M"                 },
	{ 0x4E, 0x004E, "Latin Capital Letter N"                 },
	{ 0x4F, 0x004F, "Latin Capital Letter O"                 },
	{ 0x50, 0x0050, "Latin Capital Letter P"                 },
	{ 0x51, 0x0051, "Latin Capital Letter Q"                 },
	{ 0x52, 0x0052, "Latin Capital Letter R"                 },
	{ 0x53, 0x0053, "Latin Capital Letter S"                 },
	{ 0x54, 0x0054, "Latin Capital Letter T"                 },
	{ 0x55, 0x0055, "Latin Capital Letter U"                 },
	{ 0x56, 0x0056, "Latin Capital Letter V"                 },
	{ 0x57, 0x0057, "Latin Capital Letter W"                 },
	{ 0x58, 0x0058, "Latin Capital Letter X"                 },
	{ 0x59, 0x0059, "Latin Capital Letter Y"                 },
	{ 0x5A, 0x005A, "Latin Capital Letter Z"                 },
	{ 0x5B, 0x005B, "Left Square Bracket"                    },
	{ 0x5C, 0x005C, "Reverse Solidus"                        },
	{ 0x5D, 0x005D, "Right Square Bracket"                   },
	{ 0x5E, 0x005E, "Circumflex Accent"                      },
	{ 0x5F, 0x005F, "Low Line"                               },
	{ 0x60, 0x0060, "Grave Accent"                           },
	{ 0x61, 0x0061, "Latin Small Letter A"                   },
	{ 0x62, 0x0062, "Latin Small Letter B"                   },
	{ 0x63, 0x0063, "Latin Small Letter C"                   },
	{ 0x64, 0x0064, "Latin Small Letter D"                   },
	{ 0x65, 0x0065, "Latin Small Letter E"                   },
	{ 0x66, 0x0066, "Latin Small Letter F"                   },
	{ 0x67, 0x0067, "Latin Small Letter G"                   },
	{ 0x68, 0x0068, "Latin Small Letter H"                   },
	{ 0x69, 0x0069, "Latin Small Letter I"                   },
	{ 0x6A, 0x006A, "Latin Small Letter J"                   },
	{ 0x6B, 0x006B, "Latin Small Letter K"                   },
	{ 0x6C, 0x006C, "Latin Small Letter L"                   },
	{ 0x6D, 0x006D, "Latin Small Letter M"                   },
	{ 0x6E, 0x006E, "Latin Small Letter N"                   },
	{ 0x6F, 0x006F, "Latin Small Letter O"                   },
	{ 0x70, 0x0070, "Latin Small Letter P"                   },
	{ 0x71, 0x0071, "Latin Small Letter Q"                   },
	{ 0x72, 0x0072, "Latin Small Letter R"                   },
	{ 0x73, 0x0073, "Latin Small Letter S"                   },
	{ 0x74, 0x0074, "Latin Small Letter T"                   },
	{ 0x75, 0x0075, "Latin Small Letter U"                   },
	{ 0x76, 0x0076, "Latin Small Letter V"                   },
	{ 0x77, 0x0077, "Latin Small Letter W"                   },
	{ 0x78, 0x0078, "Latin Small Letter X"                   },
	{ 0x79, 0x0079, "Latin Small Letter Y"                   },
	{ 0x7A, 0x007A, "Latin Small Letter Z"                   },
	{ 0x7B, 0x007B, "Left Curly Bracket"                     },
	{ 0x7C, 0x007C, "Vertical Line"                          },
	{ 0x7D, 0x007D, "Right Curly Bracket"                    },
	{ 0x7E, 0x007E, "Tilde"                                  },
	{ 0x7F, 0x007F, "Delete"                                 },
	{ 0x80, 0x0080, "<Control>"                              },
	{ 0x81, 0x0081, "<Control>"                              },
	{ 0x82, 0x0082, "<Control>"                              },
	{ 0x83, 0x0083, "<Control>"                              },
	{ 0x84, 0x0084, "<Control>"                              },
	{ 0x85, 0x0085, "<Control>"                              },
	{ 0x86, 0x0086, "<Control>"                              },
	{ 0x87, 0x0087, "<Control>"                              },
	{ 0x88, 0x0088, "<Control>"                              },
	{ 0x89, 0x0089, "<Control>"                              },
	{ 0x8A, 0x008A, "<Control>"                              },
	{ 0x8B, 0x008B, "<Control>"                              },
	{ 0x8C, 0x008C, "<Control>"                              },
	{ 0x8D, 0x008D, "<Control>"                              },
	{ 0x8E, 0x008E, "<Control>"                              },
	{ 0x8F, 0x008F, "<Control>"                              },
	{ 0x90, 0x0090, "<Control>"                              },
	{ 0x91, 0x0091, "<Control>"                              },
	{ 0x92, 0x0092, "<Control>"                              },
	{ 0x93, 0x0093, "<Control>"                              },
	{ 0x94, 0x0094, "<Control>"                              },
	{ 0x95, 0x0095, "<Control>"                              },
	{ 0x96, 0x0096, "<Control>"                              },
	{ 0x97, 0x0097, "<Control>"                              },
	{ 0x98, 0x0098, "<Control>"                              },
	{ 0x99, 0x0099, "<Control>"                              },
	{ 0x9A, 0x009A, "<Control>"                              },
	{ 0x9B, 0x009B, "<Control>"                              },
	{ 0x9C, 0x009C, "<Control>"                              },
	{ 0x9D, 0x009D, "<Control>"                              },
	{ 0x9E, 0x009E, "<Control>"                              },
	{ 0x9F, 0x009F, "<Control>"                              },
	{ 0xA0, 0x00A0, "No-Break Space"                         },
	{ 0xA1, 0x0104, "Latin Capital Letter A With Ogonek"     },
	{ 0xA2, 0x0112, "Latin Capital Letter E With Macron"     },
	{ 0xA3, 0x0122, "Latin Capital Letter G With Cedilla"    },
	{ 0xA4, 0x012A, "Latin Capital Letter I With Macron"     },
	{ 0xA5, 0x0128, "Latin Capital Letter I With Tilde"      },
	{ 0xA6, 0x0136, "Latin Capital Letter K With Cedilla"    },
	{ 0xA7, 0x00A7, "Section Sign"                           },
	{ 0xA8, 0x013B, "Latin Capital Letter L With Cedilla"    },
	{ 0xA9, 0x0110, "Latin Capital Letter D With Stroke"     },
	{ 0xAA, 0x0160, "Latin Capital Letter S With Caron"      },
	{ 0xAB, 0x0166, "Latin Capital Letter T With Stroke"     },
	{ 0xAC, 0x017D, "Latin Capital Letter Z With Caron"      },
	{ 0xAD, 0x00AD, "Soft Hyphen"                            },
	{ 0xAE, 0x016A, "Latin Capital Letter U With Macron"     },
	{ 0xAF, 0x014A, "Latin Capital Letter Eng"               },
	{ 0xB0, 0x00B0, "Degree Sign"                            },
	{ 0xB1, 0x0105, "Latin Small Letter A With Ogonek"       },
	{ 0xB2, 0x0113, "Latin Small Letter E With Macron"       },
	{ 0xB3, 0x0123, "Latin Small Letter G With Cedilla"      },
	{ 0xB4, 0x012B, "Latin Small Letter I With Macron"       },
	{ 0xB5, 0x0129, "Latin Small Letter I With Tilde"        },
	{ 0xB6, 0x0137, "Latin Small Letter K With Cedilla"      },
	{ 0xB7, 0x00B7, "Middle Dot"                             },
	{ 0xB8, 0x013C, "Latin Small Letter L With Cedilla"      },
	{ 0xB9, 0x0111, "Latin Small Letter D With Stroke"       },
	{ 0xBA, 0x0161, "Latin Small Letter S With Caron"        },
	{ 0xBB, 0x0167, "Latin Small Letter T With Stroke"       },
	{ 0xBC, 0x017E, "Latin Small Letter Z With Caron"        },
	{ 0xBD, 0x2015, "Horizontal Bar"                         },
	{ 0xBE, 0x016B, "Latin Small Letter U With Macron"       },
	{ 0xBF, 0x014B, "Latin Small Letter Eng"                 },
	{ 0xC0, 0x0100, "Latin Capital Letter A With Macron"     },
	{ 0xC1, 0x00C1, "Latin Capital Letter A With Acute"      },
	{ 0xC2, 0x00C2, "Latin Capital Letter A With Circumflex" },
	{ 0xC3, 0x00C3, "Latin Capital Letter A With Tilde"      },
	{ 0xC4, 0x00C4, "Latin Capital Letter A With Diaeresis"  },
	{ 0xC5, 0x00C5, "Latin Capital Letter A With Ring Above" },
	{ 0xC6, 0x00C6, "Latin Capital Letter Ae"                },
	{ 0xC7, 0x012E, "Latin Capital Letter I With Ogonek"     },
	{ 0xC8, 0x010C, "Latin Capital Letter C With Caron"      },
	{ 0xC9, 0x00C9, "Latin Capital Letter E With Acute"      },
	{ 0xCA, 0x0118, "Latin Capital Letter E With Ogonek"     },
	{ 0xCB, 0x00CB, "Latin Capital Letter E With Diaeresis"  },
	{ 0xCC, 0x0116, "Latin Capital Letter E With Dot Above"  },
	{ 0xCD, 0x00CD, "Latin Capital Letter I With Acute"      },
	{ 0xCE, 0x00CE, "Latin Capital Letter I With Circumflex" },
	{ 0xCF, 0x00CF, "Latin Capital Letter I With Diaeresis"  },
	{ 0xD0, 0x00D0, "Latin Capital Letter Eth (Icelandic)"   },
	{ 0xD1, 0x0145, "Latin Capital Letter N With Cedilla"    },
	{ 0xD2, 0x014C, "Latin Capital Letter O With Macron"     },
	{ 0xD3, 0x00D3, "Latin Capital Letter O With Acute"      },
	{ 0xD4, 0x00D4, "Latin Capital Letter O With Circumflex" },
	{ 0xD5, 0x00D5, "Latin Capital Letter O With Tilde"      },
	{ 0xD6, 0x00D6, "Latin Capital Letter O With Diaeresis"  },
	{ 0xD7, 0x0168, "Latin Capital Letter U With Tilde"      },
	{ 0xD8, 0x00D8, "Latin Capital Letter O With Stroke"     },
	{ 0xD9, 0x0172, "Latin Capital Letter U With Ogonek"     },
	{ 0xDA, 0x00DA, "Latin Capital Letter U With Acute"      },
	{ 0xDB, 0x00DB, "Latin Capital Letter U With Circumflex" },
	{ 0xDC, 0x00DC, "Latin Capital Letter U With Diaeresis"  },
	{ 0xDD, 0x00DD, "Latin Capital Letter Y With Acute"      },
	{ 0xDE, 0x00DE, "Latin Capital Letter Thorn (Icelandic)" },
	{ 0xDF, 0x00DF, "Latin Small Letter Sharp S (German)"    },
	{ 0xE0, 0x0101, "Latin Small Letter A With Macron"       },
	{ 0xE1, 0x00E1, "Latin Small Letter A With Acute"        },
	{ 0xE2, 0x00E2, "Latin Small Letter A With Circumflex"   },
	{ 0xE3, 0x00E3, "Latin Small Letter A With Tilde"        },
	{ 0xE4, 0x00E4, "Latin Small Letter A With Diaeresis"    },
	{ 0xE5, 0x00E5, "Latin Small Letter A With Ring Above"   },
	{ 0xE6, 0x00E6, "Latin Small Letter Ae"                  },
	{ 0xE7, 0x012F, "Latin Small Letter I With Ogonek"       },
	{ 0xE8, 0x010D, "Latin Small Letter C With Caron"        },
	{ 0xE9, 0x00E9, "Latin Small Letter E With Acute"        },
	{ 0xEA, 0x0119, "Latin Small Letter E With Ogonek"       },
	{ 0xEB, 0x00EB, "Latin Small Letter E With Diaeresis"    },
	{ 0xEC, 0x0117, "Latin Small Letter E With Dot Above"    },
	{ 0xED, 0x00ED, "Latin Small Letter I With Acute"        },
	{ 0xEE, 0x00EE, "Latin Small Letter I With Circumflex"   },
	{ 0xEF, 0x00EF, "Latin Small Letter I With Diaeresis"    },
	{ 0xF0, 0x00F0, "Latin Small Letter Eth (Icelandic)"     },
	{ 0xF1, 0x0146, "Latin Small Letter N With Cedilla"      },
	{ 0xF2, 0x014D, "Latin Small Letter O With Macron"       },
	{ 0xF3, 0x00F3, "Latin Small Letter O With Acute"        },
	{ 0xF4, 0x00F4, "Latin Small Letter O With Circumflex"   },
	{ 0xF5, 0x00F5, "Latin Small Letter O With Tilde"        },
	{ 0xF6, 0x00F6, "Latin Small Letter O With Diaeresis"    },
	{ 0xF7, 0x0169, "Latin Small Letter U With Tilde"        },
	{ 0xF8, 0x00F8, "Latin Small Letter O With Stroke"       },
	{ 0xF9, 0x0173, "Latin Small Letter U With Ogonek"       },
	{ 0xFA, 0x00FA, "Latin Small Letter U With Acute"        },
	{ 0xFB, 0x00FB, "Latin Small Letter U With Circumflex"   },
	{ 0xFC, 0x00FC, "Latin Small Letter U With Diaeresis"    },
	{ 0xFD, 0x00FD, "Latin Small Letter Y With Acute"        },
	{ 0xFE, 0x00FE, "Latin Small Letter Thorn (Icelandic)"   },
	{ 0xFF, 0x0138, "Latin Small Letter Kra"                 },
	{ 0, 0, NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

M_textcodec_error_t M_textcodec_encode_iso8859_10(M_textcodec_buffer_t *buf, const char *in, M_textcodec_ehandler_t ehandler)
{
	return M_textcodec_encode_cp_map(buf, in, ehandler, iso8859_10_map);
}

M_textcodec_error_t M_textcodec_decode_iso8859_10(M_textcodec_buffer_t *buf, const char *in, M_textcodec_ehandler_t ehandler)
{
	return M_textcodec_decode_cp_map(buf, in, ehandler, iso8859_10_map);
}
