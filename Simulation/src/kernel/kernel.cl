// Typedefs

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;
typedef signed   int	int32_t;
typedef unsigned long	uint64_t;

// Macros

/*KER_EXC_BEG*/
#define OpenCLDebug 0
/*KER_EXC_END*/

// Enum declarations

enum ELEMENT {
	ELE_NULL  = 0,
	ELE_AIR   = 1,
	ELE_EARTH = 2,
	ELE_FIRE  = 3,
	ELE_WATER = 4
};

// Struct declarations

struct Cell {
	enum ELEMENT ele;
	uint8_t      amt;
	uint8_t      msk;
	float3       vel;
};

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

// Function definitions

inline struct Cell loadCell(const float4 m) {
	uint32_t v = as_uint(m.x);
	v = (((v & 0x80000000) >> 16) | (((v & 0x7F800000) - 0x38000000) >> 13) | ((v & 0x007FE000) >> 13));

	struct Cell c = { ((((v & 0x0000C000) >> 14) + 1) & -(bool)(v & 0x00003FC0)), (v & 0x00003FC0) >> 6, v & 0x0000003F, m.yzw };

	return c;
}

inline float4 storeCell(const struct Cell c) {
	uint32_t v = (((uint32_t)((c.ele - 1) & -(bool)(c.ele)) << 14) | ((uint32_t)(c.amt) << 6) | (c.msk & 0x3F)) & -(bool)(c.ele);

	return (float4)(as_float(((v & 0x00008000) << 16) | (((v & 0x00007C00) << 13) + 0x38000000) | ((v & 0x000003FF) << 13)), c.vel);
}