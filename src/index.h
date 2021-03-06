#include <Arduino.h>
// https://www.aconvert.com/image/png-to-svg/
// https://realfavicongenerator.net/
// https://github.com/me-no-dev/ESPAsyncWebServer#send-binary-content-from-progmem

const uint8_t favicon_ico_gz[] PROGMEM = {0x1F,0x8B,0x08,0x08,0xE3,0x42,0x6B,0x5F,0x00,0x03,0x4D,0x79,0x73,0x6C,0x6F,0x67,0x6F,0x2E,0x70,0x6E,0x67,0x00,0xEB,0x0C,0xF0,0x73,0xE7,0xE5,0x92,0xE2,0x62,0x60,0x60,0xE0,0xF5,0xF4,0x70,0x09,0x02,0xD2,0x02,0x20,0xCC,0xC1,0x06,0x24,0xE5,0x3F,0xFF,0x4F,0x04,0x52,0x8C,0xC5,0x41,0xEE,0x4E,0x0C,0xEB,0xCE,0xC9,0xBC,0x04,0x72,0x5C,0x52,0x23,0x3C,0xD3,0x7C,0x7D,0x19,0xB4,0x80,0x6C,0x0E,0x06,0xC6,0xF6,0x4C,0x06,0x16,0x90,0x12,0x20,0x96,0x62,0x00,0x01,0xE6,0x05,0x8C,0x0C,0xCC,0x60,0x11,0xA0,0xD8,0x02,0x26,0xB8,0xAC,0xC0,0x02,0x66,0x04,0x1B,0xA4,0xD0,0x24,0xB4,0xF0,0x13,0x90,0x92,0xC9,0x74,0xF1,0x0F,0x01,0xD2,0x4C,0x0C,0x10,0xC0,0x01,0xC4,0x1A,0x50,0x1A,0x88,0x19,0x33,0xAB,0x0F,0xFC,0x7C,0x0F,0xA4,0x4D,0x3D,0x5D,0x1C,0x43,0x2C,0x04,0x93,0x12,0x1C,0xD8,0x27,0x6F,0xFB,0x68,0xA8,0x63,0x33,0xE5,0xD0,0x86,0x17,0x17,0xB4,0xDA,0x5C,0xD9,0xFD,0x9D,0x0C,0x26,0xF0,0x0A,0x31,0x9F,0x7C,0xC8,0xDA,0xCD,0xAF,0x62,0xA4,0x63,0xF7,0x43,0x6A,0xC5,0x4F,0xE1,0x80,0xC7,0x52,0x40,0xA9,0x5B,0x0A,0x3F,0x1E,0x44,0x6D,0x90,0xF0,0xE0,0x5B,0xC2,0xA6,0x1D,0x51,0xB1,0x81,0x79,0xC5,0x0B,0xFD,0x88,0xC3,0x77,0x7A,0xF7,0x48,0x64,0x39,0x24,0x25,0x30,0x37,0x66,0x39,0x64,0x05,0x32,0x9C,0x08,0x99,0xF0,0xAC,0xF5,0xB7,0x73,0xC0,0x91,0xC6,0x39,0xC6,0xC6,0x92,0x11,0xF6,0x67,0x52,0xC2,0xFF,0x99,0xB8,0x5F,0xF8,0xF5,0x98,0xC7,0x2E,0x74,0xA5,0x6B,0xF6,0xE2,0x03,0x59,0x0F,0xF5,0x18,0x7F,0x72,0xBF,0xC9,0x59,0x50,0x7C,0xF1,0xC0,0x55,0x36,0xB6,0x6D,0xFA,0x9F,0x5B,0xC4,0x6A,0xE6,0x6A,0x7F,0x79,0xFD,0xDA,0xE6,0xE1,0x7F,0x87,0x5F,0xEF,0xAF,0x57,0x3A,0x59,0x4F,0x11,0x5B,0xF3,0x82,0xFD,0x35,0x63,0x3B,0xA7,0xCA,0x01,0x3B,0xC3,0x84,0xC2,0x6F,0xCF,0x23,0x6D,0x6E,0x39,0xF8,0xEE,0xDF,0x7F,0x4E,0xB4,0xEE,0x6B,0xEA,0xBA,0x07,0x87,0xF4,0x8B,0x5B,0xD7,0x31,0xF2,0xFE,0xA9,0xA8,0xDC,0xB7,0x4C,0xDB,0xC2,0xC7,0xA1,0xAA,0x91,0x27,0x20,0x63,0x3D,0x63,0xE4,0xBF,0xB4,0x5D,0x1B,0xBE,0xB4,0x57,0xFE,0xF1,0x3A,0x9F,0x66,0x54,0xB0,0x77,0xBE,0xDE,0x82,0x0F,0xA9,0x62,0xF3,0xAD,0x2F,0x1C,0x9B,0xEF,0xC0,0xBC,0x8F,0x89,0x37,0x8A,0xD9,0x64,0xC9,0xA3,0xCF,0x31,0x8C,0xEE,0x0B,0x82,0xDC,0x16,0xC8,0x05,0x1C,0x58,0xC7,0x24,0xC5,0xD0,0xB5,0x8A,0x39,0x37,0xFE,0x19,0xB7,0x1B,0xA3,0xE8,0x7D,0x76,0xCB,0xEF,0x9F,0x63,0x16,0x26,0x3B,0xF8,0xC6,0x37,0x98,0xED,0x63,0xFF,0xE4,0xCF,0xFC,0xF2,0x7A,0x82,0x3E,0xC3,0x5E,0xC6,0x94,0xF8,0x1A,0xF3,0xE0,0x00,0x3E,0x29,0x6B,0x75,0x50,0x58,0xFF,0xFF,0xAF,0x99,0xA3,0x2D,0x01,0x0C,0x40,0x6B,0x50,0x00,0xD6,0x06,0x6E,0xF4,0x62,0x76,0x14,0x90,0xDD,0xF2,0x75,0xD2,0xED,0xD8,0xFA,0x03,0xB7,0x6E,0xF9,0xE5,0xAC,0x65,0x52,0xFA,0xC5,0xB8,0x48,0x3E,0x21,0x77,0xAA,0x57,0x72,0xA2,0xF2,0xA4,0x47,0x9B,0x76,0xAB,0x4C,0xD7,0x9B,0xE1,0xC4,0xAD,0x1B,0xA4,0x52,0xDE,0xEB,0xB1,0xEF,0x80,0xD3,0x27,0xC6,0x76,0x01,0xF3,0xED,0xEA,0x2C,0x4D,0xE7,0xCE,0x84,0x7A,0x1E,0x68,0xB4,0x2C,0x7B,0x76,0xB6,0xDA,0xFA,0xBB,0x98,0x82,0xDF,0x3D,0x45,0xB9,0xD8,0xE7,0x61,0x3C,0xC5,0xCA,0xF6,0x45,0xDA,0xB7,0xEF,0xAD,0x8A,0xCF,0x7F,0xB2,0xFA,0xFA,0xA7,0xB3,0xD1,0x86,0xA5,0x51,0x57,0xE6,0x4C,0x7C,0x7E,0xE3,0x77,0x8A,0xCF,0x69,0xEE,0x47,0x06,0xE2,0x8F,0xB6,0xB4,0xB7,0xBB,0xB3,0x1C,0xD7,0xF8,0xF4,0x47,0x36,0xF4,0x5B,0x86,0x86,0xF9,0x43,0xD5,0x67,0x0D,0xE7,0x5A,0xA7,0x0A,0x8B,0xBC,0xAE,0x56,0x36,0x9A,0x13,0xFD,0xE0,0x8C,0x62,0xD1,0xAF,0x33,0xAD,0x7E,0x5D,0x59,0x2B,0x8B,0xD3,0xAA,0x7B,0xA4,0x79,0xD2,0xAC,0x6C,0x3E,0x84,0x9D,0x9B,0x2B,0x18,0xEE,0xD7,0x5E,0xEE,0x31,0x43,0x3C,0xEC,0x67,0x91,0xB6,0xE9,0x61,0xAB,0x5B,0xF9,0xF7,0x38,0x80,0x86,0xBD,0x8B,0xD7,0xBA,0xE0,0xBB,0x29,0x6A,0x77,0xE2,0xFE,0x45,0x6B,0xEF,0xDE,0xD3,0xCC,0x11,0xD3,0x2D,0x9D,0x23,0xC9,0xA1,0x11,0xF8,0xE4,0x84,0x2D,0xCB,0xAB,0xE5,0xA1,0x77,0x67,0x9A,0xAD,0x5E,0x14,0xD7,0xCE,0x1B,0x27,0x6F,0x79,0x86,0xB5,0xEB,0xDD,0x0B,0x1E,0x51,0xF1,0x0B,0x92,0xF3,0x0A,0xFE,0xDE,0x6F,0x4F,0xCB,0x98,0x66,0xE1,0x2D,0xB8,0x7C,0x3B,0xCF,0x04,0xCB,0x67,0x9A,0x11,0x87,0x56,0x2C,0x90,0x93,0x76,0x17,0x57,0xED,0x35,0x9D,0x23,0xBD,0xC4,0x6B,0xF5,0xC5,0xFF,0x0B,0x37,0xAA,0x36,0xDE,0x55,0x3B,0x2C,0x93,0x9C,0x6D,0xBE,0xDE,0x6B,0xA2,0xFA,0xD2,0x8D,0x5F,0xAC,0x7E,0x1C,0x99,0xF2,0x23,0xE8,0x8A,0x72,0x8E,0xEA,0x3F,0x06,0x53,0x96,0xFA,0xD6,0x65,0xD2,0x3F,0x37,0x82,0x12,0xA8,0xA7,0xAB,0x9F,0xCB,0x3A,0xA7,0x84,0x26,0x00,0x44,0x27,0x71,0xFB,0x3A,0x03,0x00,0x00};
const uint8_t mask_icon_svg_gz[] PROGMEM = {0x1F,0x8B,0x08,0x08,0x2E,0x4D,0x6B,0x5F,0x00,0x03,0x73,0x61,0x66,0x61,0x72,0x69,0x2D,0x70,0x69,0x6E,0x6E,0x65,0x64,0x2D,0x74,0x61,0x62,0x2E,0x73,0x76,0x67,0x00,0x6D,0x55,0x5D,0x6F,0xDB,0x46,0x10,0x7C,0xE7,0xAF,0xD8,0xB2,0x2F,0x0D,0x10,0x8A,0xB7,0xF7,0x41,0x1E,0x0B,0xCB,0x41,0x63,0x1B,0x41,0x81,0xA6,0x35,0x1C,0xB7,0x45,0x1E,0x55,0x89,0x91,0x85,0xEA,0x0B,0x12,0x61,0xB9,0xFE,0xF5,0x9D,0x59,0xCA,0x75,0x53,0xD4,0x80,0xC5,0x13,0x6F,0x6F,0x77,0x76,0x66,0xF6,0x74,0xF1,0xEE,0x69,0xB3,0x96,0xC7,0xFE,0x70,0x5C,0xED,0xB6,0xD3,0x52,0x27,0xAE,0x94,0xE3,0x30,0xDB,0x2E,0x66,0xEB,0xDD,0xB6,0x9F,0x96,0xDB,0x5D,0xF9,0xEE,0xB2,0xB8,0xF8,0xE6,0xFA,0x97,0xAB,0xFB,0xCF,0xB7,0x37,0x72,0x7C,0x5C,0xCA,0xED,0xAF,0xEF,0x7F,0xFA,0xF1,0x4A,0xCA,0xAA,0xAE,0x7F,0x0F,0x57,0x75,0x7D,0x7D,0x7F,0x2D,0x9F,0x7E,0xFB,0x20,0xDE,0x39,0x75,0x9D,0x8B,0x75,0x7D,0xF3,0x73,0x59,0x48,0xF9,0x30,0x0C,0xFB,0xEF,0xEB,0xFA,0x74,0x3A,0x4D,0x4E,0x61,0xB2,0x3B,0x2C,0xEB,0xFB,0xBB,0x9A,0x41,0xF5,0xDD,0xCD,0x55,0x85,0x13,0xD5,0x3F,0x27,0x90,0xA2,0x46,0x6A,0x75,0x93,0xC5,0xB0,0x28,0x51,0x90,0x75,0xBE,0x46,0x05,0x9C,0xDB,0xE3,0xF4,0x7F,0x92,0x22,0x89,0xE3,0x61,0x94,0x3C,0xAD,0x16,0xC3,0xC3,0xB4,0xF4,0xA9,0x99,0x38,0xFB,0xDB,0x0F,0xA5,0x3C,0xF4,0xAB,0xE5,0xC3,0xF0,0xDF,0xB7,0x8F,0xAB,0xFE,0xF4,0x7E,0xF7,0x34,0x2D,0x9D,0x38,0x79,0xDD,0xFA,0xD7,0x12,0xF9,0xF6,0x87,0xFE,0xD8,0x1F,0x1E,0xFB,0x1F,0x8E,0xFB,0x7E,0x3E,0xDC,0xCD,0x86,0xD5,0x6E,0x5A,0x3E,0x7D,0x5C,0x2D,0x3E,0xE3,0x5F,0x36,0x7D,0x3F,0x10,0xEB,0xA6,0x1F,0x66,0x8B,0xD9,0x30,0xBB,0x2C,0xAE,0x0E,0xFD,0x6C,0xE8,0x17,0xF2,0xC7,0x5F,0xB2,0xDF,0x0D,0x87,0xD9,0xBC,0x17,0x9D,0xA8,0xBE,0x95,0xD3,0x61,0x35,0x0C,0xFD,0x96,0x1B,0xB7,0xFD,0xD0,0x1F,0xE4,0x53,0xBF,0x5E,0x6D,0x97,0x58,0x90,0x02,0xF0,0xA0,0xA1,0xB8,0xA8,0x5F,0x13,0x5D,0x2C,0x05,0xC7,0xB7,0xC7,0x2F,0xBB,0xC3,0x66,0x5A,0xDA,0x72,0x8D,0xCC,0xDF,0xB9,0x33,0xB6,0xB7,0xAF,0x30,0xDF,0xC8,0x71,0x3E,0x5B,0x73,0x4B,0xC7,0xAD,0xEA,0x65,0xF5,0xA6,0x2C,0xBE,0xAC,0xD6,0xEB,0x69,0xF9,0xED,0xB9,0x21,0x28,0x7B,0xD8,0xFD,0x69,0xAA,0x6E,0x7B,0x22,0xDF,0xCF,0x86,0x07,0x59,0x4C,0xCB,0x8F,0xAA,0x99,0xAD,0xFB,0x2C,0xF3,0xCA,0x77,0x59,0x2A,0xDF,0x48,0x95,0x9A,0x20,0x95,0x46,0xAC,0xDA,0x56,0xA5,0x0A,0xB1,0x93,0xAA,0xC3,0xAB,0x0E,0x5F,0x34,0xD8,0x1E,0x57,0x5D,0x53,0x54,0x3E,0x46,0xBC,0x77,0xE7,0x70,0x4D,0x1D,0xC3,0xB9,0xD9,0x22,0x57,0x02,0xAD,0x15,0x9E,0x4D,0x8B,0x18,0xA4,0x6E,0xF1,0x64,0x3E,0x17,0x84,0xD9,0x04,0x5F,0x43,0x16,0xC5,0xAB,0x26,0x16,0xCA,0xCD,0xD0,0x09,0x7C,0x81,0xD7,0x4D,0x04,0x41,0x2D,0x53,0x24,0x69,0x92,0x95,0xD7,0xCC,0x1C,0xDE,0x8B,0xCF,0x38,0xED,0x01,0x46,0x23,0xCF,0xB8,0x2C,0x86,0x50,0xBB,0x24,0x29,0x38,0x60,0xF2,0xAD,0xE4,0x44,0x34,0x12,0xD4,0xF1,0x8C,0x04,0xE6,0xC0,0x5A,0x83,0x78,0x89,0x0E,0x95,0x1B,0x7C,0x71,0x12,0x00,0x29,0xB5,0xD2,0x86,0x28,0xC1,0xAB,0x74,0x78,0xD5,0xB4,0x41,0xA2,0x97,0x1C,0xA5,0x4B,0x85,0x0F,0xA8,0x02,0x08,0xBE,0x43,0x0E,0xD1,0x4E,0x32,0xF7,0x10,0x95,0x9C,0x74,0xC8,0x0A,0x50,0xA2,0x09,0x3B,0xAD,0x78,0xB6,0x1D,0x2C,0x0D,0xFA,0x68,0xA4,0x41,0x4E,0xF4,0x91,0x25,0xA3,0x39,0xF0,0x81,0xD8,0x16,0x20,0x02,0xC8,0x62,0xB6,0xAA,0xF1,0x28,0x0F,0x28,0x55,0x0B,0x24,0x69,0xEC,0x43,0xD9,0x21,0x18,0x85,0x1C,0x68,0x07,0x50,0xAB,0x36,0x18,0x65,0x8A,0xE0,0x2A,0x3D,0x17,0x1B,0x4F,0x7D,0xBC,0x7A,0x99,0x07,0x12,0x8E,0xF3,0x4D,0x32,0xD5,0xA2,0x64,0x45,0xC1,0x94,0x51,0x87,0xF2,0x85,0x90,0xD0,0x2D,0x33,0x30,0x19,0x19,0x53,0x17,0x51,0x06,0x60,0xAB,0x36,0x03,0x50,0xB6,0x44,0xCA,0xB3,0xFE,0xFC,0xA1,0x26,0x66,0x4B,0xA5,0x5E,0x14,0x8D,0x29,0xBE,0x88,0xE9,0x47,0x11,0x89,0xD1,0xC4,0x54,0x61,0x90,0xE6,0xC2,0x5B,0x30,0x6D,0x84,0xD7,0x20,0xCB,0x54,0xC9,0xA0,0xD8,0x08,0x00,0x1E,0xFA,0x26,0x66,0x30,0x08,0xEE,0x2A,0x48,0xC5,0x5A,0xC8,0xCB,0x22,0x1D,0xB6,0xF9,0x16,0x48,0x53,0x2A,0x2A,0x9C,0xD2,0x4C,0xBC,0xE0,0x53,0x59,0x80,0x00,0x41,0x3F,0x41,0xE0,0x3C,0xBE,0xB4,0x62,0x66,0x52,0x32,0xC8,0xCF,0x88,0x20,0xF0,0x8F,0x80,0x71,0x1F,0xDD,0x9A,0x2F,0xBB,0x51,0x7B,0x6A,0x89,0x68,0x6D,0x60,0x8D,0x08,0xD1,0xE1,0x0C,0xD6,0x50,0xA0,0xC8,0xBE,0x79,0x2E,0xEB,0xAF,0x86,0x21,0x51,0xCA,0x0E,0xB8,0xE7,0x28,0x47,0x93,0xB1,0x23,0x32,0xAE,0x8E,0xDF,0x48,0x83,0x37,0x47,0x35,0x23,0x1F,0x48,0x4D,0xF9,0xEC,0x89,0x94,0x05,0xD5,0x54,0x21,0x58,0x5E,0x28,0x66,0x84,0x2C,0x6B,0xCF,0xAE,0x5B,0xF3,0x21,0x85,0xE3,0xB8,0x44,0x66,0x82,0xF1,0xA8,0x7F,0xA0,0x2C,0x6A,0xA9,0x99,0x85,0x75,0x20,0x0F,0xEA,0xC1,0xC3,0xCA,0xDA,0x48,0x99,0x49,0x25,0x2D,0x44,0x49,0xCC,0xCB,0xD4,0x85,0x30,0xA2,0x69,0x41,0xE1,0x39,0x58,0x58,0xC3,0x7A,0x15,0xD5,0xA6,0x73,0x7C,0x61,0x42,0x8C,0xC9,0xB3,0x21,0x02,0x0B,0xAD,0xB9,0x0F,0x24,0xA8,0x3D,0x31,0xD0,0x0E,0x26,0x15,0xF2,0xE4,0xC0,0x13,0x1D,0x2D,0x9E,0xF3,0x26,0x89,0x53,0x08,0x46,0x10,0x54,0x80,0x1C,0x21,0x20,0x04,0xD2,0xF6,0x2D,0xDD,0x0F,0x8F,0x71,0x11,0xD1,0x17,0xD9,0x30,0x9D,0xF0,0x6F,0xB4,0x65,0x7B,0x28,0x64,0x77,0xE3,0x85,0x41,0x83,0xA1,0x9C,0xC7,0x84,0xD2,0x11,0x1C,0x8F,0x84,0x4C,0x54,0x12,0x53,0xA8,0xBC,0x24,0x22,0xC7,0x86,0x89,0x31,0x36,0x90,0x02,0xD4,0x7A,0x4E,0x26,0x00,0x61,0xEE,0x10,0x02,0x5C,0xA3,0x41,0x80,0x19,0x1B,0x9D,0x2F,0xB0,0xB0,0xDA,0x3A,0xCE,0x4E,0x18,0x7B,0x84,0x7B,0xFC,0x58,0xDB,0xAE,0xA2,0x24,0xDC,0x73,0x96,0x10,0x31,0x38,0x3E,0xC6,0x72,0xE4,0xB0,0x2D,0x44,0xA7,0x9E,0x3E,0x26,0x49,0xD6,0x39,0xCE,0x70,0x2C,0x9D,0x11,0x1C,0x18,0x07,0xA9,0x22,0xBB,0xE0,0x1C,0x83,0x2F,0x54,0x4A,0xF8,0x06,0xC4,0x74,0x44,0xE4,0x0C,0x70,0xE2,0x92,0x69,0xC8,0x8B,0x83,0x47,0x09,0x93,0x7E,0x51,0xA7,0xE3,0x4C,0xC3,0xE6,0x36,0x1D,0x78,0x66,0xFE,0xFE,0xE0,0x08,0xDC,0x0F,0x9A,0x79,0xEB,0xA1,0xDA,0x78,0x87,0xF0,0xD2,0x55,0x8E,0x2F,0x24,0x4C,0x5A,0x74,0x64,0x8A,0xB9,0xF1,0x24,0xFD,0x64,0x2C,0x9C,0xC5,0x52,0x23,0x50,0x33,0x9F,0x20,0x07,0x11,0x58,0xB6,0x46,0x7C,0x1C,0x15,0x0E,0x3E,0x9B,0xFF,0x2B,0xC6,0x16,0xE3,0x4D,0x12,0xCE,0x06,0x52,0x7D,0x96,0x0D,0x5D,0xDD,0x42,0xFC,0xB9,0xD2,0x4F,0x24,0xB4,0xC9,0x76,0x4F,0xD8,0x7D,0x45,0x0E,0xC7,0xFA,0x9C,0xD1,0x8A,0xC3,0xA9,0xF4,0x5B,0xC6,0x1D,0x62,0x24,0xA1,0x39,0x6F,0x63,0xCA,0x70,0x9A,0x0E,0xEC,0xB3,0x79,0xE4,0xE7,0x88,0x24,0xE3,0xB2,0x32,0x52,0xE9,0x6E,0xF3,0x6B,0xA4,0xC0,0x6B,0x9A,0x1E,0x04,0x34,0xB0,0x3B,0x7F,0x7F,0x42,0x33,0x76,0x6D,0xF7,0x3F,0xDA,0x6B,0x98,0x24,0x8C,0x23,0x5A,0x2F,0xF9,0x81,0xDF,0xFA,0xCB,0xE2,0x6F,0xA3,0xE2,0x9A,0xA0,0xB9,0x08,0x00,0x00};

const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de">
<head>
	<title>MySensors Wifi Gateway</title>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
	<link rel="shortcut icon" type="image/x-icon" href="favicon.ico">
	<link rel="mask-icon" type="image/svg" href="mask-icon.svg" color="#48A1BE">
	<meta name="theme-color" content="#ffffff">	
	<style type="text/css">
    </style>
	<link rel="stylesheet" type="text/css" href="style.css"/>
	<script type="text/javascript"> 
	// https://stackoverflow.com/a/2931108/10590793
	var CircularQueueItem = function (value, next, back) {
		this.next = next;
		this.value = value;
		this.back = back;
		return this;
	};
	var CircularQueue = function (queueLength) {
		/// <summary>Creates a circular queue of specified length</summary>
		/// <param name="queueLength" type="int">Length of the circular queue</type>
		this._current = new CircularQueueItem(".", undefined, undefined);
		var item = this._current;
		for (var i = 0; i < queueLength - 1; i++) {
			item.next = new CircularQueueItem("", "", item);
			item = item.next;
		}
		item.next = this._current;
		this._current.back = item;

		this.push = function (value) {
			/// <summary>Pushes a value/object into the circular queue</summary>
			/// <param name="value">Any value/object that should be stored into the queue</param>
			this._current.value = value;
			this._current = this._current.next;
		};
		this.pop = function () {
			/// <summary>Gets the last pushed value/object from the circular queue</summary>
			/// <returns>Returns the last pushed value/object from the circular queue</returns>
			this._current = this._current.back;
			return this._current.value;
		};
		return this;
	}	
	var queuelen = 20;
	var queue = new CircularQueue(queuelen); 

	function doTab(evt, section) {
		var i, tabcontent, tablinks;
		tabcontent = document.getElementsByClassName("tabcontent");
		for (i = 0; i < tabcontent.length; i++) {
			tabcontent[i].style.display = "none";
		}
		tablinks = document.getElementsByClassName("tablinks");
			for (i = 0; i < tablinks.length; i++) {
		tablinks[i].className = tablinks[i].className.replace(" active", "");
		}
		document.getElementById(section).style.display = "block";
		evt.currentTarget.className += " active";
	}
	
	function openDrawerMenu(){
		var x = document.getElementById("mainNavBar");
		if (x.className === "navBar"){
			x.className += " responsive";
		} else {
			x.className = "navBar";
		}
	}

	function startEvents(){
		var es = new EventSource('/events');
		es.onopen = function(e) {
			var element = document.getElementById('debug');
			element.innerHTML = "Events Opened";
		};
		es.onerror = function(e) {
			if (e.target.readyState != EventSource.OPEN) {
				var element = document.getElementById('debug');
				element.innerHTML = "Events Closed";
			}
		};
		es.addEventListener('debug', function(e) {
			var element = document.getElementById('debug');
			element.innerHTML = e.data;
		}, false);
		es.addEventListener('info', function(e) {
			var element = document.getElementById('info');
			element.innerHTML = e.data;
		}, false);
		es.addEventListener('indicator', function(e) {
			var element = document.getElementById('indicator');
			element.innerHTML = e.data;
		}, false);
		es.addEventListener('led', function(e) {
			var element = document.getElementById('led');
			element.innerHTML = e.data;
		}, false);
		es.addEventListener('messagesjson', function(e) {
			const obj = JSON.parse(e.data);
			console.log(obj);
			var row = "<tr>" +
				"<td>" + obj.time + "</td>" + 
				"<td>" + obj.cmd + "</td>" + 
				"<td>" + obj.sender + "</td>" + 
				"<td>" + obj.sensor + "</td>" + 
				"<td>" + obj.ack + "</td>" + 
				"<td>" + obj.msgtype + "</td>" + 
				"<td>" + obj.payload + "</td></tr>";
			queue.push(row);
			var i;
			var text = "<table><thead><tr><th>time</th><th>cmd</th><th>node</th><th>sensor</th><th>ack</th><th>type</th><th>message</th></tr></thead><tbody>";
			for (i = 0; i < queuelen; i++) {
				text += queue.pop();
			}
			text += "</tbody></table>";
			var msg = document.getElementById('messages');			
			msg.innerHTML = text ;
		}, false);
	}
	function onBodyLoad(){
		document.getElementById("defaultOpen").click();
		startEvents();
	}
	</script>
</head>
<body onload=onBodyLoad()>
<header>
	<nav>
			<div class="navBar" id="mainNavBar">
			<img class="logo" src='data:image/svg+xml;utf8,<svg style="fill-rule:evenodd;clip-rule:evenodd;stroke-linecap:round;stroke-linejoin:round;" xmlns:vectornator="http://vectornator.io" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns="http://www.w3.org/2000/svg" xml:space="preserve" version="1.1" viewBox="0 0 256 256"><path stroke="#00a3c2" stroke-width="20" d="M128+242.89C191.452+242.89+242.89+191.452+242.89+128C242.89+64.5481+191.452+13.11+128+13.11C64.5481+13.11+13.11+64.5481+13.11+128C13.11+191.452+64.5481+242.89+128+242.89Z" fill="none" stroke-linecap="butt" opacity="1" stroke-linejoin="miter"/><path stroke="#f59e13" stroke-width="4" d="M143.59+142.98L143.59+167.172C143.59+170.074+142.545+172.591+140.454+174.724C138.363+176.858+135.782+177.924+132.71+177.924C129.809+177.924+127.27+176.858+125.094+174.724C122.918+172.591+121.83+170.074+121.83+167.172L121.83+141.828C121.83+138.671+120.827+136.175+118.822+134.34C116.817+132.506+114.278+131.588+111.206+131.588C108.305+131.588+105.83+132.527+103.782+134.404C101.734+136.282+100.71+138.756+100.71+141.828L100.71+167.172C100.71+170.074+99.6432+172.591+97.5099+174.724C95.3765+176.858+92.8165+177.924+89.8299+177.924C86.8432+177.924+84.2619+176.858+82.0859+174.724C79.9099+172.591+78.8219+170.074+78.8219+167.172L78.8219+141.828C78.8219+138.756+77.8192+136.282+75.8139+134.404C73.8085+132.527+71.3552+131.588+68.4539+131.588C65.4672+131.588+62.9499+132.527+60.9019+134.404C58.8539+136.282+57.8299+138.756+57.8299+141.828L57.8299+167.172C57.8299+170.074+56.7419+172.591+54.5659+174.724C52.3899+176.858+49.8512+177.924+46.9499+177.924C43.9632+177.924+41.3819+176.858+39.2059+174.724C37.0299+172.591+35.9419+170.074+35.9419+167.172L35.9419+142.98C35.9419+133.764+39.1419+126.191+45.5419+120.26C51.9419+114.33+59.5792+111.364+68.4539+111.364C76.6459+111.364+83.7712+113.924+89.8299+119.044C95.7179+113.924+102.843+111.364+111.206+111.364C120.081+111.364+127.697+114.33+134.054+120.26C140.411+126.191+143.59+133.764+143.59+142.98Z" fill="#f59e13" stroke-linecap="butt" opacity="1" stroke-linejoin="miter"/><path stroke="#f59e13" stroke-width="4" d="M168.394+151.865C172.548+151.865+176.047+150.963+178.89+149.159C181.732+147.355+183.154+144.922+183.154+141.861C183.154+139.346+181.732+137.242+178.89+135.547C176.047+133.852+172.521+132.568+168.312+131.693C164.102+130.818+159.538+129.561+154.618+127.921C149.698+126.281+145.133+124.368+140.924+122.181C136.714+119.994+133.188+116.578+130.346+111.931C127.503+107.284+126.082+101.736+126.082+95.2849C126.082+85.4449+129.881+77.1356+137.48+70.3569C145.078+63.5783+155.055+60.1889+167.41+60.1889C179.655+60.1889+189.659+62.6763+197.422+67.6509C205.184+72.6256+209.066+78.2836+209.066+84.6249C209.066+87.7956+207.781+90.6656+205.212+93.2349C202.642+95.8043+199.335+97.0889+195.29+97.0889C191.572+97.0889+187.035+95.2576+181.678+91.5949C176.32+87.9323+171.182+86.1009+166.262+86.1009C162.763+86.1009+159.975+86.8116+157.898+88.2329C155.82+89.6543+154.782+91.5676+154.782+93.9729C154.782+96.1596+156.203+98.0456+159.046+99.6309C161.888+101.216+165.469+102.501+169.788+103.485C174.106+104.469+178.753+105.89+183.728+107.749C188.702+109.608+193.349+111.712+197.668+114.063C201.986+116.414+205.567+119.912+208.41+124.559C211.252+129.206+212.674+134.645+212.674+140.877C212.674+151.92+208.41+160.912+199.882+167.855C191.354+174.798+180.748+178.269+168.066+178.269C156.804+178.269+146.691+175.563+137.726+170.151C128.76+164.739+124.278+158.917+124.278+152.685C124.278+148.64+125.562+145.278+128.132+142.599C130.701+139.92+134.008+138.581+138.054+138.581C140.24+138.581+142.181+139.046+143.876+139.975C145.57+140.904+147.101+141.998+148.468+143.255C149.834+144.512+151.31+145.797+152.896+147.109C154.481+148.421+156.586+149.542+159.21+150.471C161.834+151.4+164.895+151.865+168.394+151.865Z" fill="#f59e13" stroke-linecap="butt" opacity="1" stroke-linejoin="miter"/></svg>' width="42" height="42">
			<a class="tablinks" id="defaultOpen" onclick="doTab(event, 'tab_messages')" href="#"> messages </a>
			<a class="tablinks" onclick="doTab(event, 'tab_info')" href="#"> info </a>
			<span class="title">MySensors Wifi Gateway</span>
			<a href="javascript:void(0);" class="drawer" onclick="openDrawerMenu()">&#9776;</a>
		</div>
	</nav>
</header>

<section>    
	<div class="row">
		<a class="button" href="/reboot">reboot</a>
		<a class="button" href="/reconnect">reconnect</a>
	</div>
</section>

<section id="tab_info" class="tabcontent">            
	<div class="row">
		<div id="info">&nbsp;</div>
	</div>
</section>    

<section id="tab_messages" class="tabcontent">
	<div class="row">
		<div class="tooltip">
			<div id="messages">&nbsp;</div>
			<span class="tooltiptext" id="indicator"></span>
		</div>
	</div>
</section>    

<section>    
	<div class="row">
		<div id="led"></div>
		<div class="debug" id="debug"></div>
	</div>
</section>
</body>
</html>
)=====";
