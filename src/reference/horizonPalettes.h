#ifndef HORIZONPALETTES_H
#define HORIZONPALETTES_H

namespace horizons {

  DEFINE_GRADIENT_PALETTE(orange_gp){
  0,   0xD8, 0x30, 0x10,
  17,  0xDB, 0x3B, 0x16,
  34,  0xDD, 0x47, 0x1B,
  51,  0xE0, 0x52, 0x21,
  68,  0xE2, 0x5D, 0x27,
  85,  0xE5, 0x68, 0x2D,
  102,  0xE8, 0x74, 0x32,
  119, 0xEA, 0x7F, 0x38,
  136, 0xED, 0x8A, 0x3E,
  153, 0xEF, 0x95, 0x44,
  170, 0xF2, 0xA1, 0x49,
  187, 0xF5, 0xAC, 0x4F,
  204, 0xF7, 0xB7, 0x55,
  221, 0xFA, 0xC2, 0x5B,
  238, 0xFC, 0xCE, 0x60,
  255, 0xFF, 0xD9, 0x66
  };

  DEFINE_GRADIENT_PALETTE(blue_gp){
  0,   0x08, 0x15, 0x35,
  17,  0x12, 0x28, 0x55,
  34,  0x1C, 0x3C, 0x75,
  51,  0x28, 0x50, 0x95,
  68,  0x35, 0x62, 0xB5,
  85,  0x40, 0x70, 0xD0,
  102,  0x48, 0x7C, 0xE8,
  119, 0x42, 0x88, 0xF8,
  136, 0x38, 0x94, 0xFC,
  153, 0x30, 0xA0, 0xF8,
  170, 0x28, 0xAC, 0xF4,
  187, 0x20, 0xB8, 0xF0,
  204, 0x18, 0xC4, 0xEC,
  221, 0x10, 0xD0, 0xE8,
  238, 0x08, 0xDC, 0xE4,
  255, 0x04, 0xE6, 0xE0
  };

  DEFINE_GRADIENT_PALETTE(goldberry_gp){
  0, 109,16,26,
  17, 117,24,27,
  34, 125,31,28,
  51, 133,39,28,
  68, 141,47,29,
  85, 149,54,30,
  102, 157,62,31,
  119, 165,70,32,
  136, 173,77,32,
  153, 181,85,33,
  170, 189,93,34,
  187, 197,100,35,
  204, 205,108,36,
  221, 213,116,36,
  238, 221,123,37,
  255, 229,131,38
  };

  DEFINE_GRADIENT_PALETTE(ocean_breeze_gp){
  0, 3,4,94,
  17, 3,16,102,
  34, 3,27,110,
  51, 2,39,118,
  68, 2,51,127,
  85, 2,63,135,
  102, 2,74,143,
  119, 2,86,151,
  136, 1,98,159,
  153, 1,110,167,
  170, 1,121,175,
  187, 1,133,183,
  204, 1,145,192,
  221, 0,157,200,
  238, 0,168,208,
  255, 0,180,216
  };


  DEFINE_GRADIENT_PALETTE(barney_gp){
  0, 62,0,109,
  17, 71,0,115,
  34, 80,0,120,
  51, 89,0,126,
  68, 97,0,132,
  85, 106,0,138,
  102, 115,0,143,
  119, 124,0,149,
  136, 133,0,155,
  153, 142,0,161,
  170, 151,0,166,
  187, 160,0,172,
  204, 168,0,178,
  221, 177,0,184,
  238, 186,0,189,
  255, 195,0,195
  };

  DEFINE_GRADIENT_PALETTE(burgundy_gp){
  0, 77,0,0,
  17, 82,0,4,
  34, 86,0,9,
  51, 91,0,13,
  68, 96,0,18,
  85, 101,0,22,
  102, 105,0,27,
  119, 110,0,31,
  136, 115,0,36,
  153, 120,0,40,
  170, 124,0,45,
  187, 129,0,49,
  204, 134,0,54,
  221, 139,0,58,
  238, 143,0,63,
  255, 148,0,67
  };

DEFINE_GRADIENT_PALETTE(evergreen_gp){
0, 0,51,20,
17, 5,56,19,
34, 11,60,17,
51, 16,65,16,
68, 21,70,15,
85, 27,75,13,
102, 32,79,12,
119, 37,84,11,
136, 43,89,9,
153, 48,94,8,
170, 53,98,7,
187, 59,103,5,
204, 64,108,4,
221, 69,113,3,
238, 75,117,1,
255, 80,122,0
};

DEFINE_GRADIENT_PALETTE(mustard_gp){
0, 102,83,0,
17, 109,89,0,
34, 116,95,0,
51, 123,101,0,
68, 131,106,0,
85, 138,112,0,
102, 145,118,0,
119, 152,124,0,
136, 159,130,0,
153, 166,136,0,
170, 173,142,0,
187, 180,148,0,
204, 188,153,0,
221, 195,159,0,
238, 202,165,0,
255, 209,171,0
};


  const TProgmemRGBGradientPaletteRef hGradientPalettes[] = {
    orange_gp,
    blue_gp,
    goldberry_gp,
    ocean_breeze_gp,
    barney_gp,
    burgundy_gp,
    evergreen_gp,
    mustard_gp
  };

  const char* paletteNames[] = {
    "orange",
    "blue",
    "goldberry",
    "ocean_breeze",
    "barney",
    "burgundy",
    "evergreen",
    "mustard"
  };

  const uint8_t hGradientPaletteCount = 
    sizeof( hGradientPalettes) / sizeof( TProgmemRGBGradientPaletteRef );

  #endif

} // namespace horizons