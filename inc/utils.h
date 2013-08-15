/*
 * BLELabs BLE112-Protostick Demo Application for Tutorial #3
 * Contact: support@blelabs.com
 * Author: PeterVanPansen
 * Date: 12.08.2013
 * License: See license file in package
 *
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <apitypes.h>
#include <config.h>

void printHexdump(uint8 *data, int len, int offset);
void printAddr6(uint8 *addr, char token);
void printUUID(uint8 *uuid, int len);
uint8 *reverseArray(uint8 *array, size_t len);
void uuid128StrToArray(char *str_uuid, uint8 *arr_uuid);

#endif /* UTILS_H_ */
