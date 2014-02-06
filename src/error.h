#pragma once
	
void error_init(void);
void error_show();
void error_destroy(void);
void error_in_received_handler(DictionaryIterator *iter);
bool error_is_on_top();