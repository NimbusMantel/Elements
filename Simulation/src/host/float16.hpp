#include <cstdint>

// Code provided by Phernost on stackoverflow: https://stackoverflow.com/a/3542975

class Float16Compressor {
	private:
		union Bits

		{
			float f;
			int32_t si;
			uint32_t ui;
		};

		static int const shift = 13;
		static int const shiftSign = 16;

		static int32_t const infN = 0x7F800000;
		static int32_t const maxN = 0x477FE000;
		static int32_t const minN = 0x38800000;
		static int32_t const signN = 0x80000000;

		static int32_t const infC = infN >> shift;
		static int32_t const nanN = (infC + 1) << shift;
		static int32_t const maxC = maxN >> shift;
		static int32_t const minC = minN >> shift;
		static int32_t const signC = signN >> shiftSign;

		static int32_t const mulN = 0x52000000;
		static int32_t const mulC = 0x33800000;

		static int32_t const subC = 0x003FF;
		static int32_t const norC = 0x00400;

		static int32_t const maxD = infC - maxC - 1;
		static int32_t const minD = minC - subC - 1;

	public:
		static const uint16_t float32To16(const float value) {
			Bits v, s;
			v.f = value;
			uint32_t sign = v.si & signN;
			v.si ^= sign;
			sign >>= shiftSign;
			s.si = mulN;
			s.si = s.f * v.f;
			v.si ^= (s.si ^ v.si) & -(minN > v.si);
			v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
			v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
			v.ui >>= shift;
			v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
			v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
			return v.ui | sign;
		}

		static const float float16To32(const uint16_t value) {
			Bits v;
			v.ui = value;
			int32_t sign = v.si & signC;
			v.si ^= sign;
			sign <<= shiftSign;
			v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
			v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
			Bits s;
			s.si = mulC;
			s.f *= v.si;
			int32_t mask = -(norC > v.si);
			v.si <<= shift;
			v.si ^= (s.si ^ v.si) & mask;
			v.si |= sign;
			return v.f;
		}
};