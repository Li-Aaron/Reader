#ifndef __JSON_DATA_H__
#define __JSON_DATA_H__

#include "types.h"

char* create_json(header_t* data);
void create_json_free(char* json);
BOOL parser_json(const char* json, header_t* defhdr, void **data, int *size);

#endif