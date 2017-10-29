#pragma once

#include <CL/cl.hpp>

#include <cstdint>
#include <memory>

#include "host/float16.hpp"

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

#define MAC_ELE_ENM   \
enum ELE_ENM {        \
	ELE_AIR   = 0x01, \
	ELE_EARTH = 0x02, \
	ELE_FIRE  = 0x04, \
	ELE_WATER = 0x08  \
};

#define MAC_ELE_STR   \
struct Element {      \
	float   den;      \
	uint8_t ela;      \
	uint8_t mxd;      \
	uint8_t red;      \
	uint8_t los;      \
};

#define MAC_INT_STR   \
struct Interact {     \
	uint8_t itr;      \
	uint8_t des;      \
};

namespace Elements {
	MAC_ELE_ENM;
	MAC_ELE_STR;
	MAC_INT_STR;

	class SimCell {
		friend class Simulation;

		public:
			template <class T>
			class CelPRX {
				protected:
					CelPRX(uint16_t* d) : data(d) {}
					uint16_t* data;
				public:
					virtual operator const T&() const = 0;
					virtual const T& operator=(const T&) = 0;
			};

			class CelELE : CelPRX<ELE_ENM> {
				friend class SimCell;

				public:
					operator const ELE_ENM&() const override { return (ELE_ENM)(0x01 << (((*data) & 0xC000) >> 14)); }
					const ELE_ENM& operator=(const ELE_ENM& e) override { *data = (((0x02 & -(bool)(e & 0x0A)) | (bool)(e & 0x04)) << 14) | ((*data) & 0x3FFF); return e; }

				private: CelELE(uint16_t* d) : CelPRX(d) {}
			};

			class CelMSK : CelPRX<uint8_t> {
				friend class SimCell;

				public:
					operator const uint8_t&() const override { return (uint8_t)(((*data) & 0x3F00) >> 8); }
					const uint8_t& operator=(const uint8_t& m) override { *data = ((m & 0x003F) << 8) | ((*data) & 0xC0FF); return m; }

				private: CelMSK(uint16_t* d) : CelPRX(d) {}
			};

			class CelAMT : CelPRX<uint8_t> {
				friend class SimCell;

				public:
					operator const uint8_t&() const override { return (uint8_t)(*data); }
					const uint8_t& operator=(const uint8_t& a) override { *data = a | ((*data) & 0xFF00); return a; }

				private: CelAMT(uint16_t* d) : CelPRX(d) {}
			};

			class CelVEL : CelPRX<float3> {
				friend class SimCell;

				public:
					operator const float3&() const override { return { Float16Compressor::float16To32(*data), Float16Compressor::float16To32(*(data + 1)), Float16Compressor::float16To32(*(data + 2)) }; }
					const float3& operator=(const float3& v) override { *data = Float16Compressor::float32To16(v.x);
																		*(data + 1) = Float16Compressor::float32To16(v.y);
																		*(data + 2) = Float16Compressor::float32To16(v.z);
																		return v;
																	  }
				private: CelVEL(uint16_t* d) : CelPRX(d) {}
			};

			CelELE ele;
			CelMSK msk;
			CelAMT amt;
			CelVEL vel;

			SimCell& operator= (const SimCell&) = delete;
			SimCell(const SimCell&) = delete;

		private:
			SimCell(uint16_t* d) : ele(d), msk(d), amt(d), vel((uint16_t*)(d + 1)) {}
	};

	class Simulation {
		public:

			static inline Simulation& get() { return sim; }

			void update();
			void finish();

			SimCell& operator() (uint16_t x, uint16_t y) const;

			/*void mapSet();
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
			void intSet();*/

		private:

			static Simulation& sim;

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

			cl::Kernel actKernel, datKernel;

			cl::CommandQueue clQueue;

			enum UpdateState {
				PreUpdate,
				ReadWrite,
				Undefined
			};

			UpdateState state;

			uint16_t* celMemory;
			uint32_t* objMemory;
	};
}