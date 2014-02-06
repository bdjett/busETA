#include <pebble.h>
#include "pebble-assist.h"
#include "common.h"
#include "stoplist.h"
#include "routelist.h"
#include "eta.h"
	
#define MAX_ROUTES 10

static DoublemapRoute routes[MAX_ROUTES];

static int num_routes;
static char routeerror[128];
static char routeid[128];
static char currentstopid[512];
static char currentstopname[512];

static void routes_clean_list();
static uint16_t routes_menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context);
static uint16_t routes_menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t routes_menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t routes_menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void routes_menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void routes_menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void routes_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void routes_menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);

static Window *routes_window;
static MenuLayer *routes_menu_layer;

void getListOfRoutes(char stopid[512], char stopname[512]) {
	strncpy(currentstopid, stopid, sizeof(currentstopid));
	strncpy(currentstopname, stopname, sizeof(currentstopname));
	Tuplet get_routes_tuple = TupletInteger(DOUBLEMAP_GET_ROUTES, 1);
	Tuplet stop_id_tuple = TupletCString(DOUBLEMAP_STOP_ID, stopid);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	dict_write_tuplet(iter, &get_routes_tuple);
	dict_write_tuplet(iter, &stop_id_tuple);
	dict_write_end(iter);
	app_message_outbox_send();
}

void routelist_init(void) {
	routes_window = window_create();
	
	routes_menu_layer = menu_layer_create_fullscreen(routes_window);
	menu_layer_set_callbacks(routes_menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_sections = routes_menu_get_num_sections_callback,
        .get_num_rows = routes_menu_get_num_rows_callback,
        .get_header_height = routes_menu_get_header_height_callback,
        .get_cell_height = routes_menu_get_cell_height_callback,
        .draw_header = routes_menu_draw_header_callback,
        .draw_row = routes_menu_draw_row_callback,
        .select_click = routes_menu_select_callback,
        .select_long_click = routes_menu_select_long_callback,
	});
	
	menu_layer_set_click_config_onto_window(routes_menu_layer, routes_window);
	menu_layer_add_to_window(routes_menu_layer, routes_window);
}

void routelist_show() {
	routes_clean_list();
	window_stack_push(routes_window, true);
}

void routelist_destroy(void) {
	layer_remove_from_parent(menu_layer_get_layer(routes_menu_layer));
	menu_layer_destroy_safe(routes_menu_layer);
	window_destroy_safe(routes_window);
}

static void routes_clean_list() {
	memset(routes, 0x0, sizeof(routes));
	num_routes = 0;
	routeerror[0] = '\0';
	menu_layer_set_selected_index(routes_menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
	menu_layer_reload_data_and_mark_dirty(routes_menu_layer);
}

bool routelist_is_on_top() {
	return routes_window == window_stack_get_top_window();
}

void routelist_in_received_handler(DictionaryIterator *iter) {

	Tuple *index_tuple = dict_find(iter, DOUBLEMAP_INDEX);
	Tuple *id_tuple = dict_find(iter, DOUBLEMAP_ROUTE_ID);
	Tuple *name_tuple = dict_find(iter, DOUBLEMAP_ROUTE_NAME);

	if (id_tuple) {
		DoublemapRoute route;
		route.index = index_tuple->value->int16;
		strncpy(route.name, name_tuple->value->cstring, sizeof(route.name));
		strncpy(route.id, id_tuple->value->cstring, sizeof(route.id));
					APP_LOG(APP_LOG_LEVEL_DEBUG, "Made it here");
		routes[route.index] = route;
		num_routes++;
		menu_layer_reload_data_and_mark_dirty(routes_menu_layer);
	}
}

static uint16_t routes_menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
        return 1;
}

static uint16_t routes_menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
        return (num_routes) ? num_routes : 1;
}

static int16_t routes_menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
        return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t routes_menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
        return MENU_CELL_BASIC_CELL_HEIGHT;
}

static void routes_menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
        menu_cell_basic_header_draw(ctx, cell_layer, currentstopname);
}

static void routes_menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
        if (strlen(routeerror) != 0) {
                menu_cell_basic_draw(ctx, cell_layer, "Error!", routeerror, NULL);
        } else if (num_routes == 0) {
                menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
        } else {
                menu_cell_basic_draw(ctx, cell_layer, routes[cell_index->row].name, "", NULL);
        }
}

static void routes_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	strncpy(routeid, routes[cell_index->row].id, sizeof(routeid));
	getEtas(currentstopid, routeid);	
}

static void routes_menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
        vibes_double_pulse();
        
}