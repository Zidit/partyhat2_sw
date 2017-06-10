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
"1  let c = 0\n"
"5  let c = c + 1\n"
"6  if c = 3 then let c = 0\n"
"10 let a = c\n"
"20 poke a, 128\n"
"30 let a = a + 3\n"
"40 if a > 270 then goto 110\n"
"50 update 90\n"
"70 goto 20\n"
"110 let a = c\n"
"120 poke a, 0\n"
"130 let a = a + 3\n"
"140 if a > 270 then goto 5\n"
"150 update 90\n"
"170 goto 120\n",

"10 s = 10\n"
"20 for i = 0 to s\n"
"30 gosub 200\n"
"40 poke i * 3 + 1, 255\n"
"50 update 90\n"
"60 sleep 50\n"
"70 next i\n"
"100 for j = 0 to s\n"
"110 gosub 200\n"
"120 poke (s - j)*3 + 1, 255\n"
"130 update 90\n"
"131 sleep 50\n"
"135 next j\n"
"140 goto 10\n"
"200 for k = 0 to s * 3 + 1\n"
"210 peek k, a\n"
"220 let a = a - 64\n"
"230 if a < 0 then a = 0\n"
"240 poke k, a\n"
"250 next k\n"
"260 return\n",

"10 for a = 0 to 270\n"
"30 poke a, rnd % 255\n"
"40 next a\n"
"50 update 90\n"
"70 goto 10\n",

"1  let r = 1\n"
"2  let g = 0\n"
"3  let b = 2\n"
"10 clr\n"
"20 poke r, 128\n"
"21 poke g, 128\n"
"22 poke b, 128\n"
"30 let r = (r + 1 * 3) % 270\n"
"31 let g = (g + 2 * 3) % 270\n"
"32 let b = (b + 3 * 3) % 270\n"
"40 for i = 0 to 2\n"
"50 update 90\n"
"70 next i\n"
"100 goto 10\n",

"3  let r = 0\n"
"4  let g = 0\n"
"5  let b = 0\n"
"10 clr\n"
"20 for i = 0 to 31\n"
"21 let s = (sin(4 * i) - 128) * 2\n"
"32 poke ((r + i) * 3) % 270, s\n"
"33 poke ((g + i) * 3) % 270 + 1, s\n"
"34 poke ((b + i) * 3) % 270 + 2, s\n"
"40 next i\n"
"43 let r = r + 1\n"
"44 let g = g + 2\n"
"45 let b = b + 3\n"
"50 update 90\n"
"60 sleep 20\n"
"100 goto 10\n",

"10 s = 10\n"
"20 for i = 0 to s\n"
"30 gosub 200\n"
"40 poke i * 3 + 1, 255\n"
"50 update 90\n"
"60 sleep 50\n"
"70 next i\n"
"100 for j = 0 to s\n"
"110 gosub 200\n"
"120 poke (s - j)*3 + 1, 255\n"
"130 update 90\n"
"131 sleep 50\n"
"135 next j\n"
"140 goto 10\n"
"200 for k = 0 to s * 3 + 1\n"
"210 peek k, a\n"
"220 let a = a - 64\n"
"230 if a < 0 then a = 0\n"
"240 poke k, a\n"
"250 next k\n"
"260 return\n",

"10 let l = 90\n"
"20 poke 0, (rnd % 255) / 3 + 128\n"
"30 poke 1, (rnd % 255) / 3 + 128\n"
"40 poke 2, (rnd % 255) / 3 + 128\n"
"50 gosub 200\n"
"100 update l\n"
"105 sleep 10\n"
"110 goto 50\n"
"200 for k = 0 to l * 3\n"
"210 peek (l*3 - k), a\n"
"240 poke (l*3 - k + 3), a\n"
"250 next k\n"
"260 for i = 0 to 2\n"
"270 peek i + 3, a\n"
"280 let a = a - 7 + rnd % 15\n"
"285 poke i, a\n"
"290 next i\n"
"300 return\n"
,

"10 sleep 100\n"
"20 gosub 10\n"
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
