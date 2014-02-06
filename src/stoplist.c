#include <pebble.h>
#include "pebble-assist.h"
#include "common.h"
#include "stoplist.h"
#include "routelist.h"
#include "eta.h"
	
#define MAX_STOPS 5

static DoublemapStop stops[MAX_STOPS];

static int num_stops;
static char stoperror[128];
static char stopid[512];
static char stopname[512];

static void stops_clean_list();
static uint16_t stops_menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context);
static uint16_t stops_menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t stops_menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t stops_menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void stops_menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void stops_menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void stops_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void stops_menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);

static Window *stops_window;
static MenuLayer *stops_menu_layer;

void stoplist_init(void) {
	stops_window = window_create();
	
	stops_menu_layer = menu_layer_create_fullscreen(stops_window);
	menu_layer_set_callbacks(stops_menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_sections = stops_menu_get_num_sections_callback,
        .get_num_rows = stops_menu_get_num_rows_callback,
        .get_header_height = stops_menu_get_header_height_callback,
        .get_cell_height = stops_menu_get_cell_height_callback,
        .draw_header = stops_menu_draw_header_callback,
        .draw_row = stops_menu_draw_row_callback,
        .select_click = stops_menu_select_callback,
        .select_long_click = stops_menu_select_long_callback,
	});
	
	menu_layer_set_click_config_onto_window(stops_menu_layer, stops_window);
	menu_layer_add_to_window(stops_menu_layer, stops_window);
}

void stoplist_show() {
	stops_clean_list();
	window_stack_push(stops_window, true);
}

void stoplist_destroy(void) {
	layer_remove_from_parent(menu_layer_get_layer(stops_menu_layer));
	menu_layer_destroy_safe(stops_menu_layer);
	window_destroy_safe(stops_window);
}

static void stops_clean_list() {
	memset(stops, 0x0, sizeof(stops));
	num_stops = 0;
	stoperror[0] = '\0';
	menu_layer_set_selected_index(stops_menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
	menu_layer_reload_data_and_mark_dirty(stops_menu_layer);
}

bool stoplist_is_on_top() {
	return stops_window == window_stack_get_top_window();
}

void stoplist_in_received_handler(DictionaryIterator *iter) {
	Tuple *index_tuple = dict_find(iter, DOUBLEMAP_INDEX);
	Tuple *id_tuple = dict_find(iter, DOUBLEMAP_STOP_ID);
	Tuple *name_tuple = dict_find(iter, DOUBLEMAP_STOP_NAME);
	
	if (index_tuple && name_tuple) {
		DoublemapStop stop;
		stop.index = index_tuple->value->int16;
		strncpy(stop.id, id_tuple->value->cstring, sizeof(stop.id));
		strncpy(stop.name, name_tuple->value->cstring, sizeof(stop.name));
		stops[stop.index] = stop;
		num_stops++;
		menu_layer_reload_data_and_mark_dirty(stops_menu_layer);
	}
}

static uint16_t stops_menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
        return 1;
}

static uint16_t stops_menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
        return (num_stops) ? num_stops : 1;
}

static int16_t stops_menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
        return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t stops_menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
        return MENU_CELL_BASIC_CELL_HEIGHT;
}

static void stops_menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
        menu_cell_basic_header_draw(ctx, cell_layer, "Nearest Stops");
}

static void stops_menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
        if (strlen(stoperror) != 0) {
                menu_cell_basic_draw(ctx, cell_layer, "Error!", stoperror, NULL);
        } else if (num_stops == 0) {
                menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
        } else {
                menu_cell_basic_draw(ctx, cell_layer, stops[cell_index->row].name, "", NULL);
        }
}

static void stops_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
        strncpy(stopid, stops[cell_index->row].id, sizeof(stopid));
        strncpy(stopname, stops[cell_index->row].name, sizeof(stopname));
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Getting routes for %s - %s", stopname, stopid);
		getListOfRoutes(stopid, stopname);
}

static void stops_menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
        vibes_double_pulse();
        
}