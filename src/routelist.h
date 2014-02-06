#pragma once
	
void routelist_init(void);
void routelist_show();
void routelist_destroy(void);
void routelist_in_received_handler(DictionaryIterator *iter);
bool routelist_is_on_top();
void getListOfRoutes();