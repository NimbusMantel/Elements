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

// Function declarations

float uint16Tofloat16(const uint16_t val);
uint16_t float16Touint16(const float val);

// Kernels

kernel void testKernel() {
	printf("Simulation test\n");
}

// Function definitions

inline float uint16Tofloat16(const uint16_t v) {
	uint32_t val = v;

	return as_float(((val & 0x00008000) << 16) | (((val & 0x00007C00) << 13) + 0x38000000) | ((val & 0x000003FF) << 13));
}

inline uint16_t float16Touint16(const float v) {
	uint32_t val = as_uint(v);

	return (((val & 0x80000000) >> 16) | (((val & 0x7F800000) - 0x38000000) >> 13) | ((val & 0x007FE000) >> 13));
}