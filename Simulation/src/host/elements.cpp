#include "host/elements.hpp"

#include <iostream>
#include <vector>
#include <map>
#include <fstream>

#define STR_VALUE(s) #s
#define TO_KERNEL(s) STR_VALUE(s)

namespace Elements {
	Simulation* Simulation::sim = new Simulation();

	Simulation::Simulation() {
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
		objBuffer = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, 16 * 4 * 64);

		// Compile the OpenCL code with a macroprocessor

		std::map<std::string, std::string> macros;

		macros["EXC"] = "";
		macros["ELE"] = TO_KERNEL(ELE);

		std::ifstream strm("src/kernel/kernel.cl");
		std::string kstr(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));

		std::pair<size_t, size_t> sres;
		
		for (std::pair<std::string, std::string> rep : macros) {
			sres.first = kstr.find("/*KER_" + rep.first + "_BEG*/");

			while (sres.first != -1) {
				sres.second = kstr.find("/KER_" + rep.first + "_END*/");
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

		setKernel = cl::Kernel(clProgram, "setKernel");

		objKernel = cl::Kernel(clProgram, "objKernel");
		objKernel.setArg(2, objBuffer);

		movKernel = cl::Kernel(clProgram, "movKernel");
		movKernel.setArg(2, objBuffer);

		scaKernel = cl::Kernel(clProgram, "scaKernel");
		scaKernel.setArg(2, objBuffer);

		ubdKernel = cl::Kernel(clProgram, "ubdKernel");
		ubdKernel.setArg(2, objBuffer);

		rdsKernel = cl::Kernel(clProgram, "rdsKernel");

		// Initialize the Image2D cell storage

		initImage(640, 360);

		// Initialize the OpenCL command queue

		clQueue = cl::CommandQueue(clContext, device);
	}

	Simulation::~Simulation() {
		celImage.release();
	}

	void Simulation::initImage(uint16_t w, uint16_t h) {
		width = w;
		height = h;

		celImage.reset(new cl::Image2D(clContext, CL_MEM_READ_WRITE, { CL_RGBA, CL_HALF_FLOAT }, width, height));

		actKernel.setArg(0, celImage);
		actKernel.setArg(1, celImage);

		datKernel.setArg(0, celImage);
		datKernel.setArg(1, celImage);

		setKernel.setArg(0, celImage);
		setKernel.setArg(1, celImage);

		objKernel.setArg(0, celImage);
		objKernel.setArg(1, celImage);

		movKernel.setArg(0, celImage);
		movKernel.setArg(1, celImage);

		scaKernel.setArg(0, celImage);
		scaKernel.setArg(1, celImage);

		ubdKernel.setArg(0, celImage);
		ubdKernel.setArg(1, celImage);

		rdsKernel.setArg(0, celImage);
		rdsKernel.setArg(1, celImage);
	}

	void mapSet() {

	}

	void areGet() {

	}

	void celSet() {

	}

	void celRds() {

	}

	void objCen() {

	}

	void objMas() {

	}

	void objSet() {

	}

	void objMov() {

	}

	void objSca() {

	}

	void objUbd() {

	}

	void eleGet() {

	}

	void eleSet() {

	}

	void intGet() {

	}

	void intSet() {

	}
}