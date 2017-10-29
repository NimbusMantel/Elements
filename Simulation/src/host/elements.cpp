#include "host/elements.hpp"

#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <assert.h>
#include <chrono>

#define STR_VALUE(s) #s
#define TO_KERNEL(s) STR_VALUE(s)

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

		eleBuffer = cl::Buffer(clContext, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(Element) * 4, (Element*)(&ELES));
		actBuffer = cl::Buffer(clContext, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(Interact) * 4 * 4, (Interact*)(&INT));
		objBuffer = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, 16 * 4 * 64);

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
		actKernel.setArg(2, eleBuffer);
		actKernel.setArg(3, actBuffer);

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

		size_t row_pitch = sizeof(cl_half) * 4 * width;
		size_t slice_pitch = 0;

		celMemory = (uint16_t*)clQueue.enqueueMapImage(*celImage, false, CL_MAP_READ | CL_MAP_WRITE, origin, region, &row_pitch, &slice_pitch);
		clQueue.enqueueUnmapMemObject(*celImage, celMemory);

		clQueue.finish();
	}

	void Simulation::update() {
		assert(state == PreUpdate);

		state = Undefined;

		clQueue.finish();

		cl::size_t<3> origin = cl::size_t<3>();

		cl::size_t<3> region = cl::size_t<3>();
		region[0] = width; region[1] = height; region[2] = 1;

		size_t row_pitch = sizeof(cl_half) * 4 * width;
		size_t slice_pitch = 0;
		
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

	SimCell& Simulation::operator()(uint16_t x, uint16_t y) const {
		return SimCell(celMemory + (y * width + x) * 4);
	}
}