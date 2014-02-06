#pragma once
	
void stoplist_init(void);
void stoplist_show();
void stoplist_destroy(void);
void stoplist_in_received_handler(DictionaryIterator *iter);
bool stoplist_is_on_top();