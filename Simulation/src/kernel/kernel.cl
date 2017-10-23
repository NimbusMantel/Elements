// Typedefs

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;
typedef signed   int	int32_t;
typedef unsigned long	uint64_t;

// Macros

/*KER_EXC_BEG*/#define OpenCLDebug 0/*KER_EXC_END*/

// Placeholder declarations

/*KER_ELE_BEG*/enum ELE_ENM {
	ELE_NULL, ELE_AIR, ELE_EARTH, ELE_FIRE, ELE_WATER = 4
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

// Function declarations

struct Cell loadCell(const float4 mem);
float4 storeCell(const struct Cell cel);

// Kernels

kernel void testKernel() {
	printf("Cell storage test:\n");

	struct Cell a = { ELE_FIRE, 43, 17, (float3)(1.0f, -3.51f, 6.0f) };

	printf("  Cell O: %u %u 0x%.6X (%f|%f|%f)\n", a.ele, a.amt, a.msk, a.vel.x, a.vel.y, a.vel.z);

	float4 b = storeCell(a);

	printf("  Float4: (%f|%f|%f|%f)\n", b.x, b.y, b.z, b.w);

	struct Cell c = loadCell(b);

	printf("  Cell D: %u %u 0x%.6X (%f|%f|%f)\n", c.ele, c.amt, c.msk, c.vel.x, c.vel.y, c.vel.z);

	printf("\n");
}

kernel void actKernel(read_only image2d_t inp, write_only image2d_t oup, constant uint32_t* ele, constant uint16_t* act, int2 stp, uint16_t siz) {
	const struct Element elements[4] = {
		{ as_float(ele[0x00]), ele[0x01] },
		{ as_float(ele[0x02]), ele[0x03] },
		{ as_float(ele[0x04]), ele[0x05] },
		{ as_float(ele[0x06]), ele[0x07] }
	};

	const struct Interact interacts[4][4] = {
		{ { act[0x00] }, { act[0x01] }, { act[0x02] }, { act[0x03] } },
		{ { act[0x04] }, { act[0x05] }, { act[0x06] }, { act[0x07] } },
		{ { act[0x08] }, { act[0x09] }, { act[0x0A] }, { act[0x0B] } },
		{ { act[0x0C] }, { act[0x0D] }, { act[0x0E] }, { act[0x0F] } }
	};
}

kernel void losKernel(read_only image2d_t inp, write_only image2d_t oup, constant uint32_t* ele, int2 stp, uint16_t siz);

// Function definitions

inline struct Cell loadCell(const float4 m) {
	uint32_t v = as_uint(m.x);
	v = (((v & 0x80000000) >> 16) | (((v & 0x7F800000) - 0x38000000) >> 13) | ((v & 0x007FE000) >> 13));

	struct Cell c = { ((((v & 0x0000C000) >> 14) + 1) & -(bool)(v & 0x00003FFF)), (v & 0x00003FC0) >> 6, v & 0x0000003F, m.yzw };

	return c;
}

inline float4 storeCell(const struct Cell c) {
	uint32_t v = (((uint32_t)((c.ele - 1) & -(bool)(c.ele)) << 14) | ((uint32_t)(c.amt) << 6) | (c.msk & 0x3F)) & -(bool)(c.ele);

	return (float4)(as_float(((v & 0x00008000) << 16) | (((v & 0x00007C00) << 13) + 0x38000000) | ((v & 0x000003FF) << 13)), c.vel);
}