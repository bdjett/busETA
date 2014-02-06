#pragma once
	
void etalist_init(void);
void etalist_show();
void etalist_destroy(void);
void etalist_in_received_handler(DictionaryIterator *iter);
bool etalist_is_on_top();
void getEtas(char stopid[512], char routeid[512]);