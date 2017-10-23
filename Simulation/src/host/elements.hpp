#pragma once

#include <cstdint>

struct float3 {
	float x;
	float y;
	float z;
};

/*KER_ELE_BEG*/enum ELE_ENM {
	ELE_NULL  = 0,
	ELE_AIR   = 1,
	ELE_EARTH = 2,
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
	uint8_t      amt;
	uint8_t      msk;
	float3       vel;
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