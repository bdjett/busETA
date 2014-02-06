#include <pebble.h>
#include "pebble-assist.h"
#include "common.h"
	
static Window *error_window;
static TextLayer *error_text_layer;
static BitmapLayer *image_layer;
static GBitmap *image_logo;

void error_init(void) {
	error_window = window_create();
	Layer *error_window_layer = window_get_root_layer(error_window);
	GRect bounds = layer_get_frame(error_window_layer);
	image_logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_APP_LOGO_LONG);
	image_layer = bitmap_layer_create(GRect(0, 5, bounds.size.w, 20));
	bitmap_layer_set_bitmap(image_layer, image_logo);
	layer_add_child(error_window_layer, bitmap_layer_get_layer(image_layer));
	
	error_text_layer = text_layer_create(GRect(0, 25, bounds.size.w, bounds.size.h));
	text_layer_set_text_alignment(error_text_layer, GTextAlignmentCenter);
	text_layer_set_overflow_mode(error_text_layer, GTextOverflowModeWordWrap);
	text_layer_set_font(error_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(error_window_layer, text_layer_get_layer(error_text_layer));
}

void error_show() {
	window_stack_push(error_window, true);
}

void error_destroy(void) {
	layer_remove_from_parent(text_layer_get_layer(error_text_layer));
	layer_remove_from_parent(bitmap_layer_get_layer(image_layer));
	text_layer_destroy(error_text_layer);
	bitmap_layer_destroy(image_layer);
	gbitmap_destroy(image_logo);
	window_destroy_safe(error_window);
}

bool error_is_on_top() {
	return error_window == window_stack_get_top_window();
}

void error_in_received_handler(DictionaryIterator *iter) {
	Tuple *error_tuple = dict_find(iter, DOUBLEMAP_ERROR);
	
	if (error_tuple) {
		text_layer_set_text(error_text_layer, error_tuple->value->cstring);
	}
}