/*
 * nvm.c
 *
 *  Created on: 24.1.2017
 *      Author: Mika
 */

#include <chip.h>
#include <string.h>
#include "atomic.h"

#define LONG_STR(...) #__VA_ARGS__

char nvm_buffer[8][1024] __attribute__((aligned(256), section(".text_Flash2"))) =
{

"10 peek 1000, l\n"
"20 let r = 0\n"
"30 let g = 0\n"
"40 let b = 0\n"
"50 clr\n"
"60 for i = 0 to 31\n"
"70 let s = (sin(4 * i) - 128) * 2\n"
"80 poke ((r + i) * 3) % (l*3), s\n"
"90 poke ((g + i) * 3) % (l*3) + 1, s\n"
"100 poke ((b + i) * 3) % (l*3) + 2, s\n"
"110 next i\n"
"120 let r = r + 1\n"
"130 let g = g + 2\n"
"140 let b = b + 3\n"
"150 update l\n"
"160 sleep 20\n"
"170 goto 50\n"
,

"10 peek 1000, l\n"
"20 poke 0, (rnd % 255)\n"
"30 poke 1, (rnd % 255)\n"
"40 poke 2, (rnd % 255)\n"
"45 for j = 0 to 20\n"
"50 gosub 200\n"
"100 update l\n"
"105 sleep 10\n"
"110 next j\n"
"120 goto 20\n"
"200 for k = 0 to l * 3\n"
"210 peek (l*3 - k), a\n"
"240 poke (l*3 - k + 3), a\n"
"250 next k\n"
"260 for i = 0 to 2\n"
"270 peek i + 3, a\n"
"280 let a = a - 16 + rnd % 15\n"
"285 poke i, a\n"
"290 next i\n"
"300 return\n"
,

"10 peek 1000, l\n"
"20 for a = 0 to l*3 \n"
"30 poke a, rnd % 255\n"
"40 next a\n"
"50 update l\n"
"70 goto 20\n"
,

"10 peek 1000, l\n"
"20 let a = 0\n"
"30 let b = 0\n"
"40 let c = 0\n"
"50 let d = a\n"
"60 let e = b\n"
"70 let f = c\n"
"80 let a = (rnd % 7 + 1) * 32\n"
"90 let b = (rnd % 7 + 1) * 32\n"
"100 let c = (rnd % 7 + 1) * 32\n"
"101 if a = b then c = 0\n"
"102 if a = c then b = 0\n"
"103 if b = c then a = 0\n"
"110 for i = 1 to 100\n"
"111 let r = (d * (100 - i) + a * i) / 100\n"
"112 let g = (e * (100 - i) + b * i) / 100\n"
"113 let b = (f * (100 - i) + c * i) / 100\n"
"120 for j = 0 to l\n"
"130 poke(j * 3 + 0), g\n"
"140 poke(j * 3 + 1), r\n"
"150 poke(j * 3 + 2), b\n"
"160 next j\n"
"170 update l\n"
"190 next i\n"
"200 goto 50\n"
,

"10 peek 1000, l\n"
"20 let c = 0\n"
"30 let c = c + 1\n"
"40 if c = 3 then let c = 0\n"
"50 let a = c\n"
"60 poke a, 128\n"
"70 let a = a + 3\n"
"80 if a > 270 then goto 110\n"
"90 update l\n"
"95 sleep 1\n"
"100 goto 60\n"
"110 let a = c\n"
"120 poke a, 0\n"
"130 let a = a + 3\n"
"140 if a > 270 then goto 30\n"
"150 update l\n"
"170 goto 120\n"
,

"10 peek 1000, l\n"
"20 let r = 1\n"
"30 let g = 0\n"
"40 let b = 2\n"
"50 clr\n"
"60 poke r, 128\n"
"70 poke g, 128\n"
"80 poke b, 128\n"
"90 let r = (r + 1 * 3) % (l*3)\n"
"100 let g = (g + 2 * 3) % (l*3)\n"
"110 let b = (b + 3 * 3) % (l*3)\n"
"120 for i = 0 to 2\n"
"130 update l\n"
"140 next i\n"
"150 goto 50\n"
,

"10 goto 10"
,

//Knight rider
/*
"10 let l = 90\n"
"20 let s = 10\n"
"30 for i = 0 to s\n"
"40 gosub 200\n"
"50 poke i * 3 + 1, 255\n"
"60 update l\n"
"70 sleep 50\n"
"80 next i\n"
"100 for j = 0 to s\n"
"110 gosub 200\n"
"120 poke (s - j)*3 + 1, 255\n"
"130 update l\n"
"131 sleep 50\n"
"135 next j\n"
"140 goto 20\n"
"200 for k = 0 to s * 3 + 1\n"
"210 peek k, a\n"
"220 let a = a - 64\n"
"230 if a < 0 then a = 0\n"
"240 poke k, a\n"
"250 next k\n"
"260 return\n",
*/

"LEDS : 60\n"
"PROG : 1\n"
"BRI : 1\n"
,

};

void save_file(const int sector, char* src)
{
	const int sector_offset = 24;
	atomic(){
	Chip_IAP_PreSectorForReadWrite(sector + sector_offset, sector + sector_offset);
	Chip_IAP_EraseSector(sector + sector_offset, sector + sector_offset);
	Chip_IAP_PreSectorForReadWrite(sector + sector_offset, sector + sector_offset);
	Chip_IAP_CopyRamToFlash((int)nvm_buffer[sector], (void*)src, 1024);
	}
	return;
}

void load_file(const int sector, char* dest)
{
	memcpy(dest, nvm_buffer[sector], 1024);
}

const char* get_file_ptr(const int sector)
{
	return nvm_buffer[sector];
}
