#pragma once

#include <cstdint>

struct short2 {
	int16_t x;
	int16_t y;
};

struct float2 {
	float x;
	float y;
};

struct float3 {
	float x;
	float y;
	float z;
};

/*KER_ELE_BEG*/enum ELE_ENM {
	ELE_AIR   = 0,
	ELE_EARTH = 1,
	ELE_FIRE  = 3,
	ELE_WATER = 4
};

struct Element {
	float   den;
	uint8_t ela;
	uint8_t mad;
	uint8_t red;
	uint8_t los;
};

struct Interact {
	uint8_t itr;
	uint8_t des;
};

struct Cell {
	enum ELE_ENM ele;
	bool         sim;
	uint8_t      amt;
	uint8_t      msk;
	float3       vel;
};

struct Object {
	enum ELE_ENM ele;
	uint8_t      msk;
	short2       pos;
	uint32_t     mas;
	float2       cen;
};/*KER_ELE_END*/

const Element ELEMENTS[4] = {
	{},
	{},
	{},
	{}
};

const Interact INTERACT[4][4] = {
	{{}, {}, {}, {}},
	{{}, {}, {}, {}},
	{{}, {}, {}, {}},
	{{}, {}, {}, {}}
};