/*
 * GENERATED FILE - DO NOT EDIT
 * (c) Code Red Technologies Ltd, 2008-2013
 * (c) NXP Semiconductors 2013-2017
 * Generated linker script file for LPC824
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.23
 * LPCXpresso v8.2.2 [Build 650] [2016-09-09]  on Jun 10, 2017 1:39:44 PM
 */

MEMORY
{
  /* Define each memory region */
  MFlash32 (rx) : ORIGIN = 0x0, LENGTH = 0x6000 /* 24K bytes (alias Flash) */  
  MFlash32_16 (rx) : ORIGIN = 0x6000, LENGTH = 0x2000 /* 8K bytes (alias Flash2) */  
  RamLoc8 (rwx) : ORIGIN = 0x10000000, LENGTH = 0x2000 /* 8K bytes (alias RAM) */  
}

  /* Define a symbol for the top of each memory region */
  __base_MFlash32 = 0x0  ; /* MFlash32 */  
  __base_Flash = 0x0 ; /* Flash */  
  __top_MFlash32 = 0x0 + 0x6000 ; /* 24K bytes */  
  __top_Flash = 0x0 + 0x6000 ; /* 24K bytes */  
  __base_MFlash32_16 = 0x6000  ; /* MFlash32_16 */  
  __base_Flash2 = 0x6000 ; /* Flash2 */  
  __top_MFlash32_16 = 0x6000 + 0x2000 ; /* 8K bytes */  
  __top_Flash2 = 0x6000 + 0x2000 ; /* 8K bytes */  
  __base_RamLoc8 = 0x10000000  ; /* RamLoc8 */  
  __base_RAM = 0x10000000 ; /* RAM */  
  __top_RamLoc8 = 0x10000000 + 0x2000 ; /* 8K bytes */  
  __top_RAM = 0x10000000 + 0x2000 ; /* 8K bytes */  
