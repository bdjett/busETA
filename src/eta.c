#include <pebble.h>
#include "pebble-assist.h"
#include "common.h"
#include "stoplist.h"
#include "routelist.h"
#include "eta.h"
	
#define MAX_ETAS 5

static DoublemapEta etas[MAX_ETAS];

static int num_etas;
static char etaerror[128];
static char etatime[128];

static void eta_clean_list();
static uint16_t eta_menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context);
static uint16_t eta_menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t eta_menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t eta_menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void eta_menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void eta_menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void eta_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void eta_menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);

static Window *eta_window;
static MenuLayer *eta_menu_layer;

void getEtas(char stopid[512], char routeid[512]) {
	Tuplet get_eta_tuple = TupletInteger(DOUBLEMAP_GET_ETA, 1);
	Tuplet stop_id_tuple = TupletCString(DOUBLEMAP_STOP_ID, stopid);
	Tuplet route_id_tuple = TupletCString(DOUBLEMAP_ROUTE_ID, routeid);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	dict_write_tuplet(iter, &get_eta_tuple);
	dict_write_tuplet(iter, &stop_id_tuple);
	dict_write_tuplet(iter, &route_id_tuple);
	dict_write_end(iter);
	app_message_outbox_send();
}

void etalist_init(void) {
	eta_window = window_create();
	
	eta_menu_layer = menu_layer_create_fullscreen(eta_window);
	menu_layer_set_callbacks(eta_menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_sections = eta_menu_get_num_sections_callback,
        .get_num_rows = eta_menu_get_num_rows_callback,
        .get_header_height = eta_menu_get_header_height_callback,
        .get_cell_height = eta_menu_get_cell_height_callback,
        .draw_header = eta_menu_draw_header_callback,
        .draw_row = eta_menu_draw_row_callback,
        .select_click = eta_menu_select_callback,
        .select_long_click = eta_menu_select_long_callback,
	});
	
	menu_layer_set_click_config_onto_window(eta_menu_layer, eta_window);
	menu_layer_add_to_window(eta_menu_layer, eta_window);
}

void etalist_show() {
	eta_clean_list();
	window_stack_push(eta_window, true);
}

void etalist_destroy(void) {
	layer_remove_from_parent(menu_layer_get_layer(eta_menu_layer));
	menu_layer_destroy_safe(eta_menu_layer);
	window_destroy_safe(eta_window);
}

static void eta_clean_list() {
	memset(etas, 0x0, sizeof(etas));
	num_etas = 0;
	etaerror[0] = '\0';
	menu_layer_set_selected_index(eta_menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
	menu_layer_reload_data_and_mark_dirty(eta_menu_layer);
}

bool etalist_is_on_top() {
	return eta_window == window_stack_get_top_window();
}

void etalist_in_received_handler(DictionaryIterator *iter) {
	Tuple *index_tuple = dict_find(iter, DOUBLEMAP_INDEX);
	Tuple *eta_tuple = dict_find(iter, DOUBLEMAP_ETA);
	
	if (index_tuple && eta_tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %d %s", index_tuple->value->int16, eta_tuple->value->cstring);
		DoublemapEta eta;
		eta.index = index_tuple->value->int16;
		strncpy(eta.time, eta_tuple->value->cstring, sizeof(eta.time));
		etas[eta.index] = eta;
		num_etas++;
		menu_layer_reload_data_and_mark_dirty(eta_menu_layer);
	}
}

static uint16_t eta_menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
        return 1;
}

static uint16_t eta_menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
        return (num_etas) ? num_etas : 1;
}

static int16_t eta_menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
        return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t eta_menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
        return MENU_CELL_BASIC_CELL_HEIGHT;
}

static void eta_menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
        menu_cell_basic_header_draw(ctx, cell_layer, "Current ETA");
}

static void eta_menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
        if (strlen(etaerror) != 0) {
                menu_cell_basic_draw(ctx, cell_layer, "Error!", etaerror, NULL);
        } else if (num_etas == 0) {
                menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
        } else {
                menu_cell_basic_draw(ctx, cell_layer, etas[cell_index->row].time, "minutes", NULL);
        }
}

static void eta_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
        //
}

static void eta_menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
        vibes_double_pulse();
        
}