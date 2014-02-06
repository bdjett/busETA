#pragma once

typedef struct {
	char id[25];
	char name[128];
	int index;
} DoublemapStop;

typedef struct {
	char id[25];
	char name[128];
	int index;
} DoublemapRoute;

typedef struct {
	char time[25];
	int index;
} DoublemapEta;

enum {
	DOUBLEMAP_AGENCY = 0x0,
	DOUBLEMAP_INDEX = 0x1,
	DOUBLEMAP_STOP_ID = 0x2,
	DOUBLEMAP_STOP_NAME = 0x3,
	DOUBLEMAP_REFRESH = 0x4,
	DOUBLEMAP_GET_ROUTES = 0x5,
	DOUBLEMAP_ROUTE_ID = 0x6,
	DOUBLEMAP_ROUTE_NAME = 0x7,
	DOUBLEMAP_ETA = 0x8,
	DOUBLEMAP_GET_ETA = 0x9,
	DOUBLEMAP_ERROR = 10,
	DOUBLEMAP_CLEAN_STOPS = 11
};