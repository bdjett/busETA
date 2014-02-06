#include <pebble.h>
#include "pebble-assist.h"
#include "common.h"
#include "stoplist.h"
#include "routelist.h"
#include "eta.h"
#include "error.h"

#define MAX_STOPS 5
#define KEY_AGENCY 10

static Window *window;
static TextLayer *text_layer;
static BitmapLayer *image_layer;
static GBitmap *image_logo;

void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
}

void getListOfStops() {
	Tuplet refresh_tuple = TupletInteger(DOUBLEMAP_REFRESH, 1);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	text_layer_set_text(text_layer, "Getting nearest stops...");
	
	dict_write_tuplet(iter, &refresh_tuple);
	dict_write_end(iter);
	app_message_outbox_send();
}

void in_received_handler(DictionaryIterator *iter, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Received message!");
	Tuple *text_tuple_agency = dict_find(iter, DOUBLEMAP_AGENCY);
	Tuple *text_tuple_stop_name = dict_find(iter, DOUBLEMAP_STOP_NAME);
	Tuple *text_tuple_route_name = dict_find(iter, DOUBLEMAP_ROUTE_NAME);
	Tuple *text_tuple_eta = dict_find(iter, DOUBLEMAP_ETA);
	Tuple *text_tuple_error = dict_find(iter, DOUBLEMAP_ERROR);
	Tuple *text_tuple_clean_stops = dict_find(iter, DOUBLEMAP_CLEAN_STOPS);
	
	if (text_tuple_agency) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "GOT AN AGENCY");
		window_stack_pop_all(true);
		window_stack_push(window, true);
	} else if (text_tuple_stop_name) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Got an stop!");
		if (!stoplist_is_on_top()) {
			window_stack_pop_all(true);
			stoplist_show();
		}
		Tuple *text_tuple_stop_id = dict_find(iter, DOUBLEMAP_STOP_ID);
		if (stoplist_is_on_top()) {
			stoplist_in_received_handler(iter);
		}
	} else if (text_tuple_route_name) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Got an route!");
		if (!routelist_is_on_top()) {
			routelist_show();
		}
		Tuple *text_tuple_route_id = dict_find(iter, DOUBLEMAP_ROUTE_ID);
		if (routelist_is_on_top()) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Passing to route list");
			routelist_in_received_handler(iter);
		}
	} else if (text_tuple_eta) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Got an eta!");
		if (!etalist_is_on_top()) {
			etalist_show();
		}
		if (etalist_is_on_top()) {
			etalist_in_received_handler(iter);
		}
	} else if (text_tuple_error) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Got an error!");
		if (!error_is_on_top()) {
			error_show();
		}
		if (error_is_on_top()) {
			error_in_received_handler(iter);
		}
	} else if (text_tuple_clean_stops) {
		stoplist_show();
	}
}

void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App message dropped!");
}

static void init(void) {
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	stoplist_init();
	routelist_init();
	etalist_init();
	error_init();
}

int main(void) {
	init();
	window = window_create();
	window_stack_push(window, true);

	Layer *window_layer = window_get_root_layer(window);

	GRect bounds = layer_get_frame(window_layer);

	image_logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_APP_LOGO_LONG);
	image_layer = bitmap_layer_create(GRect(0, 5, bounds.size.w, 20));
	bitmap_layer_set_bitmap(image_layer, image_logo);
	layer_add_child(window_layer, bitmap_layer_get_layer(image_layer));

	text_layer = text_layer_create(GRect(0, 25, bounds.size.w, bounds.size.h));
	text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
	text_layer_set_overflow_mode(text_layer, GTextOverflowModeWordWrap);
	text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text(text_layer, "Welcome to BusETA!");
	layer_add_child(window_layer, text_layer_get_layer(text_layer));

	app_event_loop();

	text_layer_destroy(text_layer);
	window_destroy(window);
	gbitmap_destroy(image_logo);
	bitmap_layer_destroy(image_layer);
	stoplist_destroy();
	routelist_destroy();
	etalist_destroy();
	error_destroy();
} 