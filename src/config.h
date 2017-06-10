/*
 * config.h
 *
 *  Created on: 11.2.2017
 *      Author: Mika
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define GPREG0 (*((volatile unsigned long *)0x40020004))
#define GPREG1 (*((volatile unsigned long *)0x40020008))
#define GPREG2 (*((volatile unsigned long *)0x4002000C))
#define GPREG3 (*((volatile unsigned long *)0x40020010))

#define program_number	(GPREG0)
#define brightness 		(GPREG1)

#endif /* CONFIG_H_ */
