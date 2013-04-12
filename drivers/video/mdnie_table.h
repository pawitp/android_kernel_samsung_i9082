#ifndef __MDNIE_TABLE_H__                                                          
#define __MDNIE_TABLE_H__                                                          
                                                                                   
#include "mdnie.h"  

#if defined(CONFIG_MACH_CAPRI_SS_CRATER)
/* + CABC on Sequence */
static const unsigned char cabc_on00[] = { /*0xB9 */
0xFF,
0x83,
0x89
};

static const unsigned char cabc_on01[] = { /*0xC9 */
0x0F,
0x02,
};
#else //Baffin
/* + CABC on Sequence */
static const unsigned char cabc_on00[] = { /*0xB9 */
0xFF,
0x83,
0x69
};

static const unsigned char cabc_on01[] = { /*0xC9 */
0x0F,
0x00,
};
#endif

static const unsigned char cabc_on02[] = { /* 0x51 */
0xFF,
0x3C,
};

static const unsigned char cabc_on03[] = { /* 0x53 */
0x24,
0x08,
};
/* - CABC on Sequence */

/* + CABC Mode Sequence */
static const unsigned char cabc_ui[] = { /* 0x55 */
0x01,
0x1D,
};

static const unsigned char cabc_still[] = { /* 0x55 */
0x02,
0x1E,
};
/* - CABC Mode Sequence */

/* + CABC off Sequence */
static const unsigned char cabc_off[] = { /* 0x55 */
0x00,
0x2C,
};
/* - CABC off Sequence */

static const unsigned char negative_tuning[] = {  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x30, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0xff, //scr Kb
0x00, //scr Wb
0xff, //scr Kg
0x00, //scr Wg
0xff, //scr Kr
0x00, //scr Wr
0x00, //scr Bb
0xff, //scr Yb
0xff, //scr Bg
0x00, //scr Yg
0xff, //scr Br
0x00, //scr Yr
0xff, //scr Gb
0x00, //scr Mb
0x00, //scr Gg
0xff, //scr Mg
0xff, //scr Gr
0x00, //scr Mr
0xff, //scr Rb
0x00, //scr Cb
0xff, //scr Rg
0x00, //scr Cg
0x00, //scr Rr
0xff, //scr Cr
0x00, //sharpen_set cc_en gamma_en 00 0 0
0x20, //curve24 a
0x00, //curve24 b
0x20, //curve23 a
0x00, //curve23 b
0x20, //curve22 a
0x00, //curve22 b
0x20, //curve21 a
0x00, //curve21 b
0x20, //curve20 a
0x00, //curve20 b
0x20, //curve19 a
0x00, //curve19 b
0x20, //curve18 a
0x00, //curve18 b
0x20, //curve17 a
0x00, //curve17 b
0x20, //curve16 a
0x00, //curve16 b
0x20, //curve15 a
0x00, //curve15 b
0x20, //curve14 a
0x00, //curve14 b
0x20, //curve13 a
0x00, //curve13 b
0x20, //curve12 a
0x00, //curve12 b
0x20, //curve11 a
0x00, //curve11 b
0x20, //curve10 a
0x00, //curve10 b
0x20, //curve 9 a
0x00, //curve 9 b
0x20, //curve 8 a
0x00, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x04, //cc b3
0x00,
0x00, //cc b2
0x00,
0x00, //cc b1
0x00,
0x00, //cc g3
0x00,
0x04, //cc g2
0x00,
0x00, //cc g1
0x00,
0x00, //cc r3
0x00,
0x00, //cc r2
0x00,
0x04, //cc r1
0x00,
};

static const unsigned char video_tuning[] = {                                                                  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xFF, //scr Wb
0x00, //scr Kg
0xFF, //scr Wg
0x00, //scr Kr
0xFF, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x06, //sharpen_set cc_en gamma_en 00 0 0
0x20, //curve24 a
0x00, //curve24 b
0x20, //curve23 a
0x00, //curve23 b
0x20, //curve22 a
0x00, //curve22 b
0x20, //curve21 a
0x00, //curve21 b
0x20, //curve20 a
0x00, //curve20 b
0x20, //curve19 a
0x00, //curve19 b
0x20, //curve18 a
0x00, //curve18 b
0x20, //curve17 a
0x00, //curve17 b
0x20, //curve16 a
0x00, //curve16 b
0x20, //curve15 a
0x00, //curve15 b
0x20, //curve14 a
0x00, //curve14 b
0x20, //curve13 a
0x00, //curve13 b
0x20, //curve12 a
0x00, //curve12 b
0x20, //curve11 a
0x00, //curve11 b
0x20, //curve10 a
0x00, //curve10 b
0x20, //curve 9 a
0x00, //curve 9 b
0x20, //curve 8 a
0x00, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x05, //cc b3 0.5
0xc6,
0x1e, //cc b2
0xd3,
0x1f, //cc b1
0x67,
0x1f, //cc g3
0xc6,
0x04, //cc g2
0xd3,
0x1f, //cc g1
0x67,
0x1f, //cc r3
0xc6,
0x1e, //cc r2
0xd3,
0x05, //cc r1
0x67,                                                             
 };

static const unsigned char video_outdoor_tuning[] = {                                                                  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xFF, //scr Wb
0x00, //scr Kg
0xFF, //scr Wg
0x00, //scr Kr
0xFF, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x07, //sharpen_set cc_en gamma_en 00 0 0
0xff, //curve24 a
0x00, //curve24 b
0x0a, //curve23 a
0xaf, //curve23 b
0x0d, //curve22 a
0x99, //curve22 b
0x14, //curve21 a
0x6d, //curve21 b
0x1b, //curve20 a
0x48, //curve20 b
0x2c, //curve19 a
0x05, //curve19 b
0xb4, //curve18 a
0x0f, //curve18 b
0xbe, //curve17 a
0x26, //curve17 b
0xbe, //curve16 a
0x26, //curve16 b
0xbe, //curve15 a
0x26, //curve15 b
0xbe, //curve14 a
0x26, //curve14 b
0xae, //curve13 a
0x0c, //curve13 b
0xae, //curve12 a
0x0c, //curve12 b
0xae, //curve11 a
0x0c, //curve11 b
0xae, //curve10 a
0x0c, //curve10 b
0xae, //curve 9 a
0x0c, //curve 9 b
0xae, //curve 8 a
0x0c, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x06, //cc b3
0x7b,
0x1e, //cc b2
0x5b,
0x1f, //cc b1
0x2a,
0x1f, //cc g3
0xae,
0x05, //cc g2
0x28,
0x1f, //cc g1
0x2a,
0x1f, //cc r3
0xae,
0x1e, //cc r2
0x5b,
0x05, //cc r1
0xf7,                                                                 
 };

static const unsigned char video_warm_outdoor_tuning[] = {  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x33, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xe0, //scr Wb
0x00, //scr Kg
0xf1, //scr Wg
0x00, //scr Kr
0xff, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x07, //sharpen_set cc_en gamma_en 00 0 0
0xff, //curve24 a
0x00, //curve24 b
0x0a, //curve23 a
0xaf, //curve23 b
0x0d, //curve22 a
0x99, //curve22 b
0x14, //curve21 a
0x6d, //curve21 b
0x1b, //curve20 a
0x48, //curve20 b
0x2c, //curve19 a
0x05, //curve19 b
0xb4, //curve18 a
0x0f, //curve18 b
0xbe, //curve17 a
0x26, //curve17 b
0xbe, //curve16 a
0x26, //curve16 b
0xbe, //curve15 a
0x26, //curve15 b
0xbe, //curve14 a
0x26, //curve14 b
0xae, //curve13 a
0x0c, //curve13 b
0xae, //curve12 a
0x0c, //curve12 b
0xae, //curve11 a
0x0c, //curve11 b
0xae, //curve10 a
0x0c, //curve10 b
0xae, //curve 9 a
0x0c, //curve 9 b
0xae, //curve 8 a
0x0c, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x06, //cc b3
0x7b,
0x1e, //cc b2
0x5b,
0x1f, //cc b1
0x2a,
0x1f, //cc g3
0xae,
0x05, //cc g2
0x28,
0x1f, //cc g1
0x2a,
0x1f, //cc r3
0xae,
0x1e, //cc r2
0x5b,
0x05, //cc r1
0xf7,
};

static const unsigned char video_warm_tuning[] = {  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x33, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xe0, //scr Wb
0x00, //scr Kg
0xf1, //scr Wg
0x00, //scr Kr
0xff, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x06, //sharpen_set cc_en gamma_en 00 0 0
0x20, //curve24 a
0x00, //curve24 b
0x20, //curve23 a
0x00, //curve23 b
0x20, //curve22 a
0x00, //curve22 b
0x20, //curve21 a
0x00, //curve21 b
0x20, //curve20 a
0x00, //curve20 b
0x20, //curve19 a
0x00, //curve19 b
0x20, //curve18 a
0x00, //curve18 b
0x20, //curve17 a
0x00, //curve17 b
0x20, //curve16 a
0x00, //curve16 b
0x20, //curve15 a
0x00, //curve15 b
0x20, //curve14 a
0x00, //curve14 b
0x20, //curve13 a
0x00, //curve13 b
0x20, //curve12 a
0x00, //curve12 b
0x20, //curve11 a
0x00, //curve11 b
0x20, //curve10 a
0x00, //curve10 b
0x20, //curve 9 a
0x00, //curve 9 b
0x20, //curve 8 a
0x00, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x05, //cc b3 0.5
0xc6,
0x1e, //cc b2
0xd3,
0x1f, //cc b1
0x67,
0x1f, //cc g3
0xc6,
0x04, //cc g2
0xd3,
0x1f, //cc g1
0x67,
0x1f, //cc r3
0xc6,
0x1e, //cc r2
0xd3,
0x05, //cc r1
0x67,
};

static const unsigned char video_cold_outdoor_tuning[] = {  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x33, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xff, //scr Wb
0x00, //scr Kg
0xe9, //scr Wg
0x00, //scr Kr
0xe2, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x07, //sharpen_set cc_en gamma_en 00 0 0
0xff, //curve24 a
0x00, //curve24 b
0x0a, //curve23 a
0xaf, //curve23 b
0x0d, //curve22 a
0x99, //curve22 b
0x14, //curve21 a
0x6d, //curve21 b
0x1b, //curve20 a
0x48, //curve20 b
0x2c, //curve19 a
0x05, //curve19 b
0xb4, //curve18 a
0x0f, //curve18 b
0xbe, //curve17 a
0x26, //curve17 b
0xbe, //curve16 a
0x26, //curve16 b
0xbe, //curve15 a
0x26, //curve15 b
0xbe, //curve14 a
0x26, //curve14 b
0xae, //curve13 a
0x0c, //curve13 b
0xae, //curve12 a
0x0c, //curve12 b
0xae, //curve11 a
0x0c, //curve11 b
0xae, //curve10 a
0x0c, //curve10 b
0xae, //curve 9 a
0x0c, //curve 9 b
0xae, //curve 8 a
0x0c, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x06, //cc b3
0x7b,
0x1e, //cc b2
0x5b,
0x1f, //cc b1
0x2a,
0x1f, //cc g3
0xae,
0x05, //cc g2
0x28,
0x1f, //cc g1
0x2a,
0x1f, //cc r3
0xae,
0x1e, //cc r2
0x5b,
0x05, //cc r1
0xf7,   
};

static const unsigned char video_cold_tuning[] = {  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x33, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xff, //scr Wb
0x00, //scr Kg
0xe9, //scr Wg
0x00, //scr Kr
0xe2, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x06, //sharpen_set cc_en gamma_en 00 0 0
0x20, //curve24 a
0x00, //curve24 b
0x20, //curve23 a
0x00, //curve23 b
0x20, //curve22 a
0x00, //curve22 b
0x20, //curve21 a
0x00, //curve21 b
0x20, //curve20 a
0x00, //curve20 b
0x20, //curve19 a
0x00, //curve19 b
0x20, //curve18 a
0x00, //curve18 b
0x20, //curve17 a
0x00, //curve17 b
0x20, //curve16 a
0x00, //curve16 b
0x20, //curve15 a
0x00, //curve15 b
0x20, //curve14 a
0x00, //curve14 b
0x20, //curve13 a
0x00, //curve13 b
0x20, //curve12 a
0x00, //curve12 b
0x20, //curve11 a
0x00, //curve11 b
0x20, //curve10 a
0x00, //curve10 b
0x20, //curve 9 a
0x00, //curve 9 b
0x20, //curve 8 a
0x00, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x05, //cc b3 0.5
0xc6,
0x1e, //cc b2
0xd3,
0x1f, //cc b1
0x67,
0x1f, //cc g3
0xc6,
0x04, //cc g2
0xd3,
0x1f, //cc g1
0x67,
0x1f, //cc r3
0xc6,
0x1e, //cc r2
0xd3,
0x05, //cc r1
0x67,
};
    
static const unsigned char ui_tuning[] = {                                                                  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xFF, //scr Wb
0x00, //scr Kg
0xFF, //scr Wg
0x00, //scr Kr
0xFF, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x02, //sharpen_set cc_en gamma_en 00 0 0
0x20, //curve24 a
0x00, //curve24 b
0x20, //curve23 a
0x00, //curve23 b
0x20, //curve22 a
0x00, //curve22 b
0x20, //curve21 a
0x00, //curve21 b
0x20, //curve20 a
0x00, //curve20 b
0x20, //curve19 a
0x00, //curve19 b
0x20, //curve18 a
0x00, //curve18 b
0x20, //curve17 a
0x00, //curve17 b
0x20, //curve16 a
0x00, //curve16 b
0x20, //curve15 a
0x00, //curve15 b
0x20, //curve14 a
0x00, //curve14 b
0x20, //curve13 a
0x00, //curve13 b
0x20, //curve12 a
0x00, //curve12 b
0x20, //curve11 a
0x00, //curve11 b
0x20, //curve10 a
0x00, //curve10 b
0x20, //curve 9 a
0x00, //curve 9 b
0x20, //curve 8 a
0x00, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x06, //cc b3 0.6
0x21,
0x1e, //cc b2
0x97,
0x1f, //cc b1
0x48,
0x1f, //cc g3
0xba,
0x04, //cc g2
0xfe,
0x1f, //cc g1
0x48,
0x1f, //cc r3
0xba,
0x1e, //cc r2
0x97,
0x05, //cc r1
0xaf,                                             
 };

static const unsigned char gallery_tuning[] = {                                                                  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xFF, //scr Wb
0x00, //scr Kg
0xFF, //scr Wg
0x00, //scr Kr
0xFF, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x02, //sharpen_set cc_en gamma_en 00 0 0
0x20, //curve24 a
0x00, //curve24 b
0x20, //curve23 a
0x00, //curve23 b
0x20, //curve22 a
0x00, //curve22 b
0x20, //curve21 a
0x00, //curve21 b
0x20, //curve20 a
0x00, //curve20 b
0x20, //curve19 a
0x00, //curve19 b
0x20, //curve18 a
0x00, //curve18 b
0x20, //curve17 a
0x00, //curve17 b
0x20, //curve16 a
0x00, //curve16 b
0x20, //curve15 a
0x00, //curve15 b
0x20, //curve14 a
0x00, //curve14 b
0x20, //curve13 a
0x00, //curve13 b
0x20, //curve12 a
0x00, //curve12 b
0x20, //curve11 a
0x00, //curve11 b
0x20, //curve10 a
0x00, //curve10 b
0x20, //curve 9 a
0x00, //curve 9 b
0x20, //curve 8 a
0x00, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x05, //cc b3 0.5
0xc6,
0x1e, //cc b2
0xd3,
0x1f, //cc b1
0x67,
0x1f, //cc g3
0xc6,
0x04, //cc g2
0xd3,
0x1f, //cc g1
0x67,
0x1f, //cc r3
0xc6,
0x1e, //cc r2
0xd3,
0x05, //cc r1
0x67,                                     
 };

static const unsigned char camera_tuning[] = {                                                                  
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xFF, //scr Wb
0x00, //scr Kg
0xFF, //scr Wg
0x00, //scr Kr
0xFF, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x06, //sharpen_set cc_en gamma_en 00 0 0
0x20, //curve24 a
0x00, //curve24 b
0x20, //curve23 a
0x00, //curve23 b
0x20, //curve22 a
0x00, //curve22 b
0x20, //curve21 a
0x00, //curve21 b
0x20, //curve20 a
0x00, //curve20 b
0x20, //curve19 a
0x00, //curve19 b
0x20, //curve18 a
0x00, //curve18 b
0x20, //curve17 a
0x00, //curve17 b
0x20, //curve16 a
0x00, //curve16 b
0x20, //curve15 a
0x00, //curve15 b
0x20, //curve14 a
0x00, //curve14 b
0x20, //curve13 a
0x00, //curve13 b
0x20, //curve12 a
0x00, //curve12 b
0x20, //curve11 a
0x00, //curve11 b
0x20, //curve10 a
0x00, //curve10 b
0x20, //curve 9 a
0x00, //curve 9 b
0x20, //curve 8 a
0x00, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x05, //cc b3
0x6b,
0x1f, //cc b2
0x10,
0x1f, //cc b1
0x85,
0x1f, //cc g3
0xd1,
0x04, //cc g2
0xa9,
0x1f, //cc g1
0x86,
0x1f, //cc r3
0xd1,
0x1f, //cc r2
0x10,
0x05, //cc r1
0x1f,                                                                    
 };

static const unsigned char camera_outdoor_tuning[] = {    
0x5A, //password 5A
0x00, //mask 000
0x00, //data_width
0x03, //scr_roi 1 scr algo_roi 1 algo 00 1 0 00 1 0
0x00, //roi_ctrl
0x00, //roi1 y end
0x00, 
0x00, //roi1 y start
0x00,
0x00, //roi1 x end
0x00,
0x00, //roi1 x start
0x00,
0x00, //roi0 y end
0x00,
0x00, //roi0 y start
0x00,
0x00, //roi0 x end
0x00,
0x00, //roi0 x start
0x00,
0x00, //scr Kb
0xFF, //scr Wb
0x00, //scr Kg
0xFF, //scr Wg
0x00, //scr Kr
0xFF, //scr Wr
0xFF, //scr Bb
0x00, //scr Yb
0x00, //scr Bg
0xFF, //scr Yg
0x00, //scr Br
0xFF, //scr Yr
0x00, //scr Gb
0xFF, //scr Mb
0xFF, //scr Gg
0x00, //scr Mg
0x00, //scr Gr
0xFF, //scr Mr
0x00, //scr Rb
0xFF, //scr Cb
0x00, //scr Rg
0xFF, //scr Cg
0xFF, //scr Rr
0x00, //scr Cr
0x07, //sharpen_set cc_en gamma_en 00 0 0
0xff, //curve24 a
0x00, //curve24 b
0x0a, //curve23 a
0xaf, //curve23 b
0x0d, //curve22 a
0x99, //curve22 b
0x14, //curve21 a
0x6d, //curve21 b
0x1b, //curve20 a
0x48, //curve20 b
0x2c, //curve19 a
0x05, //curve19 b
0xb4, //curve18 a
0x0f, //curve18 b
0xbe, //curve17 a
0x26, //curve17 b
0xbe, //curve16 a
0x26, //curve16 b
0xbe, //curve15 a
0x26, //curve15 b
0xbe, //curve14 a
0x26, //curve14 b
0xae, //curve13 a
0x0c, //curve13 b
0xae, //curve12 a
0x0c, //curve12 b
0xae, //curve11 a
0x0c, //curve11 b
0xae, //curve10 a
0x0c, //curve10 b
0xae, //curve 9 a
0x0c, //curve 9 b
0xae, //curve 8 a
0x0c, //curve 8 b
0x20, //curve 7 a
0x00, //curve 7 b
0x20, //curve 6 a
0x00, //curve 6 b
0x20, //curve 5 a
0x00, //curve 5 b
0x20, //curve 4 a
0x00, //curve 4 b
0x20, //curve 3 a
0x00, //curve 3 b
0x20, //curve 2 a
0x00, //curve 2 b
0x20, //curve 1 a
0x00, //curve 1 b
0x06, //cc b3
0x7b,
0x1e, //cc b2
0x5b,
0x1f, //cc b1
0x2a,
0x1f, //cc g3
0xae,
0x05, //cc g2
0x28,
0x1f, //cc g1
0x2a,
0x1f, //cc r3
0xae,
0x1e, //cc r2
0x5b,
0x05, //cc r1
0xf7,
};
                                                                                   
                                                                                   
static const unsigned short tune_dynamic_gallery[] = {                             
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_dynamic_ui[] = {                                  
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_dynamic_video[] = {                               
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_dynamic_vt[] = {                                  
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_movie_gallery[] = {                               
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_movie_ui[] = {                                    
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_movie_video[] = {                                 
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_movie_vt[] = {                                    
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_standard_gallery[] = {                            
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_standard_ui[] = {                                 
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_standard_video[] = {                              
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_standard_vt[] = {                                 
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_natural_gallery[] = {                             
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_natural_ui[] = {                                  
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_natural_video[] = {                               
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_natural_vt[] = {                                  
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_camera[] = {                                      
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_camera_outdoor[] = {                              
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_cold[] = {                                        
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_cold_outdoor[] = {                                
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_normal_outdoor[] = {                              
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_warm[] = {                                        
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
static const unsigned short tune_warm_outdoor[] = {                                
	0x0000, 0x0000, /*BANK 0*/                                                       
	0x0008, 0x008c, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x00ff, 0x0000, /*Mask Release*/                                                 
	END_SEQ, 0x0000,                                                                 
};                                                                                 
                                                                                   
#if defined(CONFIG_FB_MDNIE_PWM)                                                   
struct mdnie_tunning_info etc_table[CABC_MAX][OUTDOOR_MAX][TONE_MAX] = {           
	{                                                                                
		{                                                                              
			{"NORMAL",		NULL},                                                         
			{"WARM",		tune_warm},                                                      
			{"COLD",		tune_cold},                                                      
		},                                                                             
		{                                                                              
			{"NORMAL_OUTDOOR",	tune_normal_outdoor},                                    
			{"WARM_OUTDOOR",	tune_warm_outdoor},                                        
			{"COLD_OUTDOOR",	tune_cold_outdoor},                                        
		},                                                                             
	},                                                                               
	{                                                                                
		{                                                                              
			{"NORMAL_CABC",		NULL},                                                     
			{"WARM_CABC",		tune_warm},                                                  
			{"COLD_CABC",		tune_cold},                                                  
		},                                                                             
		{                                                                              
			{"NORMAL_OUTDOOR_CABC",	tune_normal_outdoor},                                
			{"WARM_OUTDOOR_CABC",	tune_warm_outdoor},                                    
			{"COLD_OUTDOOR_CABC",	tune_cold_outdoor},                                    
		},                                                                             
	},                                                                               
};                                                                                 
                                                                                   
struct mdnie_tunning_info_cabc tunning_table[CABC_MAX][MODE_MAX][SCENARIO_MAX] = { 
	{                                                                                
		{                                                                              
			{"DYNAMIC_UI",			tune_dynamic_ui,		0},                                  
			{"DYNAMIC_VIDEO",		tune_dynamic_video,	LUT_VIDEO},                          
			{"DYNAMIC_VIDEO",		tune_dynamic_video,	LUT_VIDEO},                          
			{"DYNAMIC_VIDEO",		tune_dynamic_video,	LUT_VIDEO},                          
			{"CAMERA",			NULL/*tune_camera*/,		0},                                  
			{"DYNAMIC_UI",			tune_dynamic_ui,		0},                                  
			{"DYNAMIC_GALLERY",		tune_dynamic_gallery,	0},                              
			{"DYNAMIC_VT",			tune_dynamic_vt,		0},                                  
		}, {                                                                           
			{"STANDARD_UI",			tune_standard_ui,		0},                                  
			{"STANDARD_VIDEO",		tune_standard_video,	LUT_VIDEO},                      
			{"STANDARD_VIDEO",		tune_standard_video,	LUT_VIDEO},                      
			{"STANDARD_VIDEO",		tune_standard_video,	LUT_VIDEO},                      
			{"CAMERA",			NULL/*tune_camera*/,		0},                                  
			{"STANDARD_UI",			tune_standard_ui,		0},                                  
			{"STANDARD_GALLERY",		tune_standard_gallery,	0},                          
			{"STANDARD_VT",			tune_standard_vt,		0},                                  
		}, {                                                                           
			{"MOVIE_UI",			tune_movie_ui,			0},                                    
			{"MOVIE_VIDEO",			tune_movie_video,	LUT_VIDEO},                            
			{"MOVIE_VIDEO",			tune_movie_video,	LUT_VIDEO},                            
			{"MOVIE_VIDEO",			tune_movie_video,	LUT_VIDEO},                            
			{"CAMERA",			NULL/*tune_camera*/,		0},                                  
			{"MOVIE_UI",			tune_movie_ui,			0},                                    
			{"MOVIE_GALLERY",		tune_movie_gallery,		0},                                
			{"MOVIE_VT",			tune_movie_vt,		0},                                      
		},                                                                             
	},                                                                               
	{                                                                                
		{                                                                              
			{"DYNAMIC_UI_CABC",		tune_dynamic_ui,		0},                                
			{"DYNAMIC_VIDEO",		tune_dynamic_video,	LUT_VIDEO},                          
			{"DYNAMIC_VIDEO",		tune_dynamic_video,	LUT_VIDEO},                          
			{"DYNAMIC_VIDEO",		tune_dynamic_video,	LUT_VIDEO},                          
			{"CAMERA",			NULL/*tune_camera*/,		0},                                  
			{"DYNAMIC_UI_CABC",		tune_dynamic_ui,		0},                                
			{"DYNAMIC_GALLERY_CABC",	tune_dynamic_gallery,	0},                          
			{"DYNAMIC_VT_CABC",		tune_dynamic_vt,		0},                                
		}, {                                                                           
			{"STANDARD_UI_CABC",		tune_standard_ui,		0},                              
			{"STANDARD_VIDEO_CABC",		tune_standard_video,	LUT_VIDEO},                  
			{"STANDARD_VIDEO_CABC",		tune_standard_video,	LUT_VIDEO},                  
			{"STANDARD_VIDEO_CABC",		tune_standard_video,	LUT_VIDEO},                  
			{"CAMERA",			NULL/*tune_camera*/,		0},                                  
			{"STANDARD_UI_CABC",		tune_standard_ui,		0},                              
			{"STANDARD_GALLERY_CABC",	tune_standard_gallery,	0},                        
			{"STANDARD_VT_CABC",		tune_standard_vt,		0},                              
		}, {                                                                           
			{"MOVIE_UI_CABC",		tune_movie_ui,			0},                                  
			{"MOVIE_VIDEO_CABC",		tune_movie_video,	LUT_VIDEO},                        
			{"MOVIE_VIDEO_CABC",		tune_movie_video,	LUT_VIDEO},                        
			{"MOVIE_VIDEO_CABC",		tune_movie_video,	LUT_VIDEO},                        
			{"CAMERA",			NULL/*tune_camera*/,		0},                                  
			{"MOVIE_UI_CABC",		tune_movie_ui,			0},                                  
			{"MOVIE_GALLERY_CABC",		tune_movie_gallery,		0},                          
			{"MOVIE_VT_CABC",		tune_movie_vt,		0},                                    
		},                                                                             
	},                                                                               
};                                                                                 
#else                                                                              
struct mdnie_tunning_info etc_table[CABC_MAX][OUTDOOR_MAX][TONE_MAX] = {           
	{                                                                                
		{                                                                              
			{"NORMAL",		NULL},                                                         
			{"WARM",		tune_warm},                                                      
			{"COLD",		tune_cold},                                                      
		},                                                                             
		{                                                                              
			{"NORMAL_OUTDOOR",	tune_normal_outdoor},                                    
			{"WARM_OUTDOOR",	tune_warm_outdoor},                                        
			{"COLD_OUTDOOR",	tune_cold_outdoor},                                        
		},                                                                             
	}                                                                                
};                                                                                 
                                                                                   
struct mdnie_tunning_info tunning_table[CABC_MAX][MODE_MAX][SCENARIO_MAX] = {      
	{                                                                                
		{                                                                              
			{"DYNAMIC_UI",			tune_dynamic_ui},                                        
			{"DYNAMIC_VIDEO",		tune_dynamic_video},                                     
			{"DYNAMIC_VIDEO",		tune_dynamic_video},                                     
			{"DYNAMIC_VIDEO",		tune_dynamic_video},                                     
			{"CAMERA",			NULL/*tune_camera*/},                                        
			{"DYNAMIC_UI",			tune_dynamic_ui},                                        
			{"DYNAMIC_GALLERY",		tune_dynamic_gallery},                                 
			{"DYNAMIC_VT",			tune_dynamic_vt},                                        
		}, {                                                                           
			{"STANDARD_UI",			tune_standard_ui},                                       
			{"STANDARD_VIDEO",		tune_standard_video},                                  
			{"STANDARD_VIDEO",		tune_standard_video},                                  
			{"STANDARD_VIDEO",		tune_standard_video},                                  
			{"CAMERA",			NULL/*tune_camera*/},                                        
			{"STANDARD_UI",			tune_standard_ui},                                       
			{"STANDARD_GALLERY",		tune_standard_gallery},                              
			{"STANDARD_VT",			tune_standard_vt},                                       
		}, {                                                                           
			{"NATURAL_UI",			tune_natural_ui},                                        
			{"NATURAL_VIDEO",		tune_natural_video},                                     
			{"NATURAL_VIDEO_WARM",		tune_natural_video},                               
			{"NATURAL_VIDEO_COLD",		tune_natural_video},                               
			{"CAMERA",			NULL/*tune_camera*/},                                        
			{"NATURAL_UI",			tune_natural_ui},                                        
			{"NATURAL_GALLERY",		tune_natural_gallery},                                 
			{"NATURAL_VT",			tune_natural_vt},                                        
		}, {                                                                           
			{"MOVIE_UI",			tune_movie_ui},                                            
			{"MOVIE_VIDEO",			tune_movie_video},                                       
			{"MOVIE_VIDEO",			tune_movie_video},                                       
			{"MOVIE_VIDEO",			tune_movie_video},                                       
			{"CAMERA",			NULL/*tune_camera*/},                                        
			{"MOVIE_UI",			tune_movie_ui},                                            
			{"MOVIE_GALLERY",		tune_movie_gallery},                                     
			{"MOVIE_VT",			tune_movie_vt},                                            
		},                                                                             
	}                                                                                
};                                                                                 
#endif                                                                             
                                                                                   
struct mdnie_tunning_info camera_table[OUTDOOR_MAX] = {                            
	{"CAMERA",		tune_camera},                                                      
	{"CAMERA_OUTDOOR",	tune_camera_outdoor},                                        
};                                                                                 
                                                                                   
#endif /* __MDNIE_TABLE_H__ */                                                     
                                                                                   
