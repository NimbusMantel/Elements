#include "host/elements.hpp"

#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <assert.h>
#include <chrono>

#define STR_VALUE(s) #s
#define TO_KERNEL(s) STR_VALUE(s)

// TO DO: Separate memory for cells, objects and updates
//        Update function with simulation steps

namespace Elements {

#define MAC_CEL_STR   \
struct Cell {         \
	enum ELE_ENM ele; \
	bool         sim; \
	uint8_t      msk; \
	uint8_t      amt; \
	float3       vel; \
};

#define MAC_OBJ_STR   \
struct Object {       \
	enum ELE_ENM ele; \
	uint8_t      msk; \
	short2       pos; \
	uint32_t     mas; \
	float2       cen; \
};

	Simulation& Simulation::sim = Simulation();

	Simulation::Simulation() : state(ReadWrite) {
		// Get an OpenCL GPU device

		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);

		cl::Platform platform = platforms.front();

		std::vector<cl::Device> devices;
		platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

		cl::Device device = devices.front();

		// Initialize the OpenCL context

		clContext = cl::Context(device);

		// Initialize the OpenCL buffers
		
		eleBuffer = cl::Buffer(clContext, (cl_mem_flags)(CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR), sizeof(Element) * 4, (Element*)(&ELES));
		actBuffer = cl::Buffer(clContext, (cl_mem_flags)(CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR), sizeof(Interact) * 4 * 4, (Interact*)(&ACTS));
		objBuffer = cl::Buffer(clContext, (cl_mem_flags)(CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR), 16 * 4 * 64, nullptr);

		// Compile the OpenCL code with a macroprocessor

		std::map<std::string, std::string> macros;

		macros["EXC"] = "";
		macros["ELE"] = std::string() + TO_KERNEL(MAC_ELE_ENM) + "\n\n" + TO_KERNEL(MAC_ELE_STR) + "\n\n" +
						TO_KERNEL(MAC_INT_STR) + "\n\n" + TO_KERNEL(MAC_CEL_STR) + "\n\n" + TO_KERNEL(MAC_OBJ_STR);

		std::ifstream strm("src/kernel/kernel.cl");
		std::string kstr(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));
		
		std::pair<size_t, size_t> sres;
		
		for (std::pair<std::string, std::string> rep : macros) {
			sres.first = kstr.find("/*KER_" + rep.first + "_BEG*/");

			while (sres.first != std::string::npos) {
				sres.second = kstr.find("/*KER_" + rep.first + "_END*/");
				kstr.replace(sres.first, sres.second - sres.first + 15, rep.second);
				sres.first = kstr.find("/*KER_" + rep.first + "_BEG*/");
			}
		}
		
		cl::Program::Sources sources = cl::Program::Sources(1, std::make_pair(kstr.c_str(), kstr.length() + 1));
		
		clProgram = cl::Program(clContext, sources);
		
		char* build = new char[30]();
		sprintf_s(build, 31, "-cl-std=CL1.2 -D OpenCLDebug=%u", (bool)OpenCLDebug);
		clProgram.build(build);

		// Initialize the OpenCL kernels

		actKernel = cl::Kernel(clProgram, "actKernel");
		actKernel.setArg(3, eleBuffer);
		actKernel.setArg(4, actBuffer);

		datKernel = cl::Kernel(clProgram, "datKernel");
		datKernel.setArg(2, eleBuffer);
		datKernel.setArg(3, objBuffer);

		// Initialize the OpenCL command queue

		clQueue = cl::CommandQueue(clContext, device);

		// Initialize the Image2D cell storage and memory

		initImage(640, 360);

		// Initialize the object memory

		objMemory = (uint32_t*)clQueue.enqueueMapBuffer(objBuffer, false, CL_MAP_READ, 0x00, 16 * 4 * 64);
		clQueue.enqueueUnmapMemObject(objBuffer, objMemory);

		clQueue.finish();
	}

	Simulation::~Simulation() {
		celImage.release();
	}

	void Simulation::initImage(uint16_t w, uint16_t h) {
		if (w == width && h == height) return;

		width = w;
		height = h;
		
		celImage.reset(new cl::Image2D(clContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, { CL_RGBA, CL_HALF_FLOAT }, width, height));

		actKernel.setArg(0, *celImage);
		actKernel.setArg(1, *celImage);

		datKernel.setArg(0, *celImage);
		datKernel.setArg(1, *celImage);

		cl::size_t<3> origin = cl::size_t<3>();

		cl::size_t<3> region = cl::size_t<3>();
		region[0] = width; region[1] = height; region[2] = 1;

		size_t row_pitch, slice_pitch;

		cl_uint4 fill_col = { 0, 0, 0, 0 };

		clQueue.enqueueFillImage(*celImage, fill_col, origin, region);

		celMemory = (uint16_t*)clQueue.enqueueMapImage(*celImage, false, CL_MAP_READ, origin, region, &row_pitch, &slice_pitch);
		clQueue.enqueueUnmapMemObject(*celImage, celMemory);

		region[0] = (size_t)ceilf(width / 16.0f);
		region[1] = (size_t)ceilf(height / 16.0f);

		updImage.reset(new cl::Image2D(clContext, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, { CL_A, CL_UNORM_INT8 }, region[0], region[1]));

		actKernel.setArg(2, *updImage);

		fill_col = { 0, 0, 0, 1 };

		clQueue.enqueueFillImage(*updImage, fill_col, origin, region);

		updMemory = (uint8_t*)clQueue.enqueueMapImage(*updImage, false, CL_MAP_READ, origin, region, &row_pitch, &slice_pitch);
		clQueue.enqueueUnmapMemObject(*updImage, updMemory);

		clQueue.finish();
	}

	void Simulation::load(std::unique_ptr<uint16_t*> data, uint16_t w, uint16_t h) {
		initImage(w, h);

		memcpy(celMemory, *data, sizeof(uint16_t) * 4 * w * h);
	}

	std::unique_ptr<uint16_t*> Simulation::store(ELE_ENM ele, bool sim, uint8_t msk, uint8_t amt, float3 vel) {
		std::unique_ptr<uint16_t*> tmp = std::make_unique<uint16_t*>(new uint16_t[4]);

		(*tmp)[0] = (((0x02 & -(bool)(ele & 0x0A)) | (bool)(ele & 0x04)) << 14) | (((msk & 0x003F) & -sim) << 8) | amt;
		(*tmp)[1] = Float16Compressor::float32To16(vel.x);
		(*tmp)[2] = Float16Compressor::float32To16(vel.y);
		(*tmp)[3] = Float16Compressor::float32To16(vel.z);

		return tmp;
	}

	void Simulation::update() {
		assert(state == PreUpdate);

		state = Undefined;

		clQueue.finish();

		if (eleDirty) {
			eleDirty = false;

			clQueue.enqueueWriteBuffer(eleBuffer, false, 0x00, sizeof(Element) * 4, &ELES);
		}

		if (actDirty) {
			actDirty = false;

			clQueue.enqueueWriteBuffer(actBuffer, false, 0x00, sizeof(Interact) * 4 * 4, &ACTS);
		}

		// Queue interaction kernels

		cl::size_t<3> origin = cl::size_t<3>();
		cl::size_t<3> region = cl::size_t<3>();

		region[0] = (size_t)ceilf(width / 16.0f);
		region[1] = (size_t)ceilf(height / 16.0f);

		size_t row_pitch, slice_pitch;

		updMemory = (uint8_t*)clQueue.enqueueMapImage(*updImage, false, CL_MAP_READ | CL_MAP_WRITE, origin, region, &row_pitch, &slice_pitch);
		memset(updMemory, 0x00, sizeof(uint8_t) * region[0] * region[1]);
		clQueue.enqueueUnmapMemObject(*updImage, updMemory);

		// Enqueue kernels to the clQueue

		clQueue.finish();

		region[0] = width; region[1] = height; region[2] = 1;
		
		celMemory = (uint16_t*)clQueue.enqueueMapImage(*celImage, false, CL_MAP_READ | CL_MAP_WRITE, origin, region, &row_pitch, &slice_pitch);
		objMemory = (uint32_t*)clQueue.enqueueMapBuffer(objBuffer, false, CL_MAP_READ, 0x00, 16 * 4 * 64);

		clQueue.finish();

		state = ReadWrite;
	}

	void Simulation::finish() {
		assert(state == ReadWrite);

		state = Undefined;

		clQueue.finish();

		clQueue.enqueueUnmapMemObject(*celImage, celMemory);
		clQueue.enqueueUnmapMemObject(objBuffer, objMemory);

		clQueue.finish();

		state = PreUpdate;
	}

	SimCell& Simulation::cel(uint16_t x, uint16_t y) const {
		return SimCell(celMemory + (y * width + x) * 4, (bool*)(updMemory + (size_t)(ceilf(y * width / 16.0f) + ceilf(x / 16.0f))));
	}

	SimObject& Simulation::obj(ELE_ENM ele, uint8_t msk) const {
		return SimObject(ele, (msk & 0x3F), objMemory + (((0x02 & -(bool)(ele & 0x0A)) | (bool)(ele & 0x04)) * 64 + (msk & 0x3F)) * 4);
	}

	SimElement& Simulation::ele(ELE_ENM ele) const {
		return SimElement((Element*)(&ELES[(0x02 & -(bool)(ele & 0x0A)) | (bool)(ele & 0x04)]));
	}

	SimInteract& Simulation::act(ELE_ENM efr, ELE_ENM eto) const {
		return SimInteract((Interact*)(&(ACTS[(0x02 & -(bool)(efr & 0x0A)) | (bool)(efr & 0x04)][(0x02 & -(bool)(eto & 0x0A)) | (bool)(eto & 0x04)])));
	}
}