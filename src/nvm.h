/*
 * nvm.h
 *
 *  Created on: 24.1.2017
 *      Author: Mika
 */

#ifndef NVM_H_
#define NVM_H_

void save_file(const int sector, char* src);
void load_file(const int sector, char* dest);
const char* get_file_ptr(const int sector);

#endif /* NVM_H_ */
