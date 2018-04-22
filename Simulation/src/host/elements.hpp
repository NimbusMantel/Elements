#pragma once

#include <CL/cl.hpp>

#include <cstdint>
#include <memory>

#include "host/float16.hpp"

#define OpenCLDebug 0

struct ushort2 {
	uint16_t x;
	uint16_t y;
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
    int8_t  bal;      \
    uint8_t msk;      \
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
				private:
					bool* upd;
				protected:
					CelPRX(uint16_t* d, bool* u) : data(d), upd(u) {}
					uint16_t* data;
					void flag() { *upd = true; };

				public:
					virtual operator const T&() const = 0;
					virtual const T& operator=(const T&) = 0;
			};

			class CelELE : CelPRX<ELE_ENM> {
				friend class SimCell;

				public:
					operator const ELE_ENM&() const override { return (ELE_ENM)(0x01 << (((*data) & 0xC000) >> 14)); }
					const ELE_ENM& operator=(const ELE_ENM& e) override { *data = (((0x02 & -(bool)(e & 0x0A)) | (bool)(e & 0x04)) << 14) | ((*data) & 0x3FFF); flag(); return e; }

				private: CelELE(uint16_t* d, bool* u) : CelPRX(d, u) {}
			};

			class CelSIM : CelPRX<bool> {
				friend class SimCell;

				public:
					operator const bool&() const override { return (bool)((*data) & 0x3F00); }
					const bool& operator=(const bool& s) override { *data = (((*data) & 0x3F00) & -s) | (((!(bool)((*data) & 0x3F00)) && s) << 8) | ((*data) & 0xC0FF); flag(); return s; }

				private: CelSIM(uint16_t* d, bool* u) : CelPRX(d, u) {}
			};

			class CelMSK : CelPRX<uint8_t> {
				friend class SimCell;

				public:
					operator const uint8_t&() const override { return (uint8_t)(((*data) & 0x3F00) >> 8); }
					const uint8_t& operator=(const uint8_t& m) override { *data = ((m & 0x003F) << 8) | ((*data) & 0xC0FF); flag(); return m; }

				private: CelMSK(uint16_t* d, bool* u) : CelPRX(d, u) {}
			};

			class CelAMT : CelPRX<uint8_t> {
				friend class SimCell;

				public:
					operator const uint8_t&() const override { return (uint8_t)(*data); }
					const uint8_t& operator=(const uint8_t& a) override { *data = a | ((*data) & 0xFF00); flag(); return a; }

				private: CelAMT(uint16_t* d, bool* u) : CelPRX(d, u) {}
			};

			class CelVEL : CelPRX<float3> {
				friend class SimCell;

				public:
					operator const float3&() const override { return { Float16Compressor::float16To32(*data), Float16Compressor::float16To32(*(data + 1)), Float16Compressor::float16To32(*(data + 2)) }; }
					const float3& operator=(const float3& v) override { data[0] = Float16Compressor::float32To16(v.x);
																		data[1] = Float16Compressor::float32To16(v.y);
																		data[2] = Float16Compressor::float32To16(v.z);
																		flag();
																		return v;
																	  }
				private: CelVEL(uint16_t* d, bool* u) : CelPRX(d, u) {}
			};

			CelELE ele;
			CelSIM sim;
			CelMSK msk;
			CelAMT amt;
			CelVEL vel;

			SimCell& operator= (const SimCell&) = delete;
			SimCell(const SimCell&) = delete;

		private:
			SimCell(uint16_t* d, bool* u) : ele(d, u), sim(d, u), msk(d, u), amt(d, u), vel((uint16_t*)(d + 1), u) {}
	};

	class SimObject {
		friend class Simulation;

		public:
			const ELE_ENM& ele() const { return ELE; }
			const uint8_t& msk() const { return MSK; }
			const ushort2& pos() const { return{ (data[0] & 0xFFFF0000) >> 16, data[0] & 0x0000FFFF }; }
			const uint32_t& mas() const { return data[1]; }
			const float2& cen() const { return { data[2] / (float)data[1], data[2] / (float)data[1] }; }

			SimObject& operator= (const SimObject&) = delete;
			SimObject(const SimObject&) = delete;

		private:
			SimObject(ELE_ENM e, uint8_t m, uint32_t* d) : ELE(e), MSK(m & 0x3F), data(d) {}

			const ELE_ENM ELE;
			const uint8_t MSK;

			uint32_t* data;
	};

	class SimElement {
		friend class Simulation;

		public:
			template <class T>
			class ElePRX {
				private:
					Simulation& sim;
				protected:
					ElePRX(T* d) : data(d), sim(Simulation::get()) {}
					T* data;
					void flag() { sim.eleDirty = true; }
				public:
					virtual operator const T&() const = 0;
					virtual const T& operator=(const T&) = 0;
			};

			class EleDEN : ElePRX<float> {
				friend class SimElement;

				public:
					operator const float&() const override { return *data; }
					const float& operator=(const float& d) override { *data = d; flag(); return d; };

				private: EleDEN(float* d) : ElePRX(d) {}
			};

			class EleELA : ElePRX<uint8_t> {
				friend class SimElement;

				public:
					operator const uint8_t&() const override { return *data; }
					const uint8_t& operator=(const uint8_t& e) override { *data = e; flag(); return e; };

				private: EleELA(uint8_t* d) : ElePRX(d) {}
			};

			class EleMXD : ElePRX<uint8_t> {
				friend class SimElement;

				public:
					operator const uint8_t&() const override { return *data; }
					const uint8_t& operator=(const uint8_t& m) override { *data = m; flag(); return m; };

				private: EleMXD(uint8_t* d) : ElePRX(d) {}
			};

			class EleRED : ElePRX<uint8_t> {
				friend class SimElement;

				public:
					operator const uint8_t&() const override { return *data; }
					const uint8_t& operator=(const uint8_t& r) override { *data = r; flag(); return r; };

				private: EleRED(uint8_t* d) : ElePRX(d) {}
			};

			class EleLOS : ElePRX<uint8_t> {
				friend class SimElement;

				public:
					operator const uint8_t&() const override { return *data; }
					const uint8_t& operator=(const uint8_t& l) override { *data = l; flag(); return l; };

				private: EleLOS(uint8_t* d) : ElePRX(d) {}
			};

			EleDEN den;
			EleELA ela;
			EleMXD mxd;
			EleRED red;
			EleLOS los;

			SimElement& operator= (const SimElement&) = delete;
			SimElement(const SimElement&) = delete;

		private:
			SimElement(Element* e) : den(&(e->den)), ela(&(e->ela)), mxd(&(e->mxd)), red(&(e->red)), los(&(e->los)) {}
	};

	class SimInteract {
		friend class Simulation;

		private:
			template <class T>
			class ActPRX {
				private:
					Simulation& sim;
				protected:
					ActPRX(T* d) : data(d), sim(Simulation::get()) {}
					T* data;
					void flag() { sim.actDirty = true; }
				public:
					virtual operator const T&() const = 0;
					virtual const T& operator=(const T&) = 0;
			};

			class ActIDM : ActPRX<uint8_t> {
				friend class SimInteract;

				public:
					operator const uint8_t&() const override { return *data; }
					const uint8_t& operator=(const uint8_t& d) override { *data = d; flag(); return d; }

				private: ActIDM(uint8_t* d) : ActPRX(d) {}
			};

			class ActBAL : ActPRX<int8_t> {
				friend class SimInteract;

				public:
					operator const int8_t&() const override { return *data; }
					const int8_t& operator=(const int8_t& d) override { *data = d; flag(); return d; }

				private: ActBAL(int8_t* d) : ActPRX(d) {}
			};

			ActIDM itr;
			ActIDM des;
			ActBAL bal;
			ActIDM msk;

			SimInteract& operator= (const SimInteract&) = delete;
			SimInteract(const SimInteract&) = delete;

		private:
			SimInteract(Interact* i) : itr(&(i->itr)), des(&(i->des)), bal(&(i->bal)), msk(&(i->msk)) {}
	};

	class Simulation {
		friend class SimCell;
		friend class SimElement;
		friend class SimInteract;

		public:

			static inline Simulation& get() { return sim; }

			void load(std::unique_ptr<uint16_t*> data, uint16_t width, uint16_t height);
			static std::unique_ptr<uint16_t*> store(ELE_ENM ele, bool sim, uint8_t msk, uint8_t amt, float3 vel);

			void update();
			void finish();

			SimCell& cel(uint16_t x, uint16_t y) const;
			SimObject& obj(ELE_ENM ele, uint8_t msk) const;
			SimElement& ele(ELE_ENM ele) const;
			SimInteract& act(ELE_ENM efr, ELE_ENM eto) const;

			SimCell& operator() (uint16_t x, uint16_t y) const { return this->cel(x, y); }

		private:

			static Simulation& sim;

			Simulation();
			~Simulation();

			void initImage(uint16_t width, uint16_t height);

			cl::Context clContext;

			uint16_t width, height;
			
			Element ELES[4];
			Interact ACTS[4][4];

			cl::Buffer eleBuffer, actBuffer, objBuffer;

			std::unique_ptr<cl::Image2D> celImage, updImage;

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
			uint8_t*  updMemory;

			bool eleDirty = false;
			bool actDirty = false;
	};
}