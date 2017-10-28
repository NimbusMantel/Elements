#pragma once

#include <CL/cl.hpp>

#include <cstdint>
#include <memory>

#define OpenCLDebug 0

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

#define ELE           \
enum ELE_ENM {        \
	ELE_AIR = 0,      \
	ELE_EARTH = 1,    \
	ELE_FIRE = 2,     \
	ELE_WATER = 3     \
};                    \
                      \
struct Element {      \
	float   den;      \
	uint8_t ela;      \
	uint8_t mad;      \
	uint8_t red;      \
	uint8_t los;      \
};                    \
                      \
struct Interact {     \
	uint8_t itr;      \
	uint8_t des;      \
};                    \
                      \
struct Cell {         \
	enum ELE_ENM ele; \
	bool         sim; \
	uint8_t      amt; \
	uint8_t      msk; \
	float3       vel; \
};                    \
                      \
struct Object {       \
	enum ELE_ENM ele; \
	uint8_t      msk; \
	short2       pos; \
	uint32_t     mas; \
	float2       cen; \
};

namespace Elements {
	ELE;

	class Simulation {
		public:

			static inline Simulation* get() { return sim; }

			void mapSet();
			void areGet();
			void celSet();
			void celRds();
			void objCen();
			void objMas();
			void objSet();
			void objMov();
			void objSca();
			void objUbd();
			void eleGet();
			void eleSet();
			void intGet();
			void intSet();

		private:

			static Simulation* sim;

			Simulation();
			~Simulation();

			void initImage(uint16_t width, uint16_t height);

			cl::Context clContext;

			uint16_t width, height;

			Element ELES[4];
			Interact INT[4][4];

			cl::Buffer eleBuffer, actBuffer, objBuffer;

			std::unique_ptr<cl::Image2D> celImage;

			cl::Program clProgram;

			cl::Kernel actKernel, datKernel, setKernel, objKernel, movKernel, scaKernel, ubdKernel, rdsKernel;

			cl::CommandQueue clQueue;
	};
}