// Typedefs

typedef signed   char   int8_t;
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;
typedef signed   int	int32_t;
typedef unsigned long	uint64_t;

// Macros

#define ELEMENTS(ele) { { as_float(ele[0x00]), ele[0x01] }, { as_float(ele[0x02]), ele[0x03] }, { as_float(ele[0x04]), ele[0x05] }, { as_float(ele[0x06]), ele[0x07] } }
/*KER_EXC_BEG*/#define OpenCLDebug 0/*KER_EXC_END*/
#define INTERACTS(act) { { { act[0x00] }, { act[0x01] }, { act[0x02] }, { act[0x03] } }, { { act[0x04] },{ act[0x05] },{ act[0x06] },{ act[0x07] } }, { { act[0x08] },{ act[0x09] },{ act[0x0A] },{ act[0x0B] } }, { { act[0x0C] },{ act[0x0D] },{ act[0x0E] },{ act[0x0F] } } }

// Enum and struct declaration

/*KER_ELE_BEG*/

// Placeholder declarations

enum ELE_ENM { ELE_AIR, ELE_EARTH, ELE_FIRE, ELE_WATER };

struct Element { float den; uint8_t ela; uint8_t mxd; uint8_t red; uint8_t los; };

struct Interact { uint8_t itr; uint8_t des; int8_t bal; uint8_t msk; };

struct Cell { enum ELE_ENM ele; bool sim; uint8_t msk; uint8_t amt; float3 vel; }; /*Stored in memory as ele(2), amt(8), msk(6), vel(48)*/

struct Object { enum ELE_ENM ele; uint8_t msk; ushort2 pos; uint32_t mas; float2 cen; }; /*Stored in memory as pos(32), mas(32), dis(64)*/

/*KER_ELE_END*/

// Function declarations

struct Cell loadCell(const float4 mem);
float4 storeCell(const struct Cell cel);

// Kernels

kernel void actKernel(read_only image2d_t inp, write_only image2d_t oup, write_only image2d_t upd, constant read_only uint32_t* ele, constant read_only uint32_t* act, short2 stp, uint16_t siz) {
	const struct Element elements[4] = ELEMENTS(ele);
	const struct Interact interacts[4][4] = INTERACTS(act);

	// Simulate interaction between two adjacent cells
}

kernel void datKernel(read_only image2d_t inp, write_only image2d_t oup, constant read_only uint32_t* ele, global read_write uint32_t* obj, short2 stp, uint16_t siz) {
	const struct Element elements[4] = ELEMENTS(ele);

	// Simulate cell amount loss and update object data
}

// Function definitions

inline struct Cell loadCell(const float4 m) {
	uint32_t v = as_uint(m.x);
	v = (((v & 0x80000000) >> 16) | (((v & 0x7F800000) - 0x38000000) >> 13) | ((v & 0x007FE000) >> 13));

	struct Cell c = { 0x01 << ((v & 0x0000C000) >> 14), (bool)(v & 0x00003F00), (v & 0x00003F00) >> 8, v & 0x000000FF, m.yzw };

	return c;
}

inline float4 storeCell(const struct Cell c) {
	uint32_t v = ((uint32_t)(((0x02 & -(bool)(c.ele & 0x0A)) | (bool)(c.ele & 0x04)) << 14) | ((c.msk & 0x3F & -c.sim) << 8) | c.amt);

	return (float4)(as_float(((v & 0x00008000) << 16) | (((v & 0x00007C00) << 13) + 0x38000000) | ((v & 0x000003FF) << 13)), c.vel);
}