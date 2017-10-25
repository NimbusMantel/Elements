#include <CL/cl.hpp>

#include <vector>
#include <fstream>

#include <iostream>

#include "host/elements.hpp"

#define OpenCLDebug 0

bool HOST_BIG_ENDIAN, DEVICE_BIG_ENDIAN;

const uint16_t width = 640;
const uint16_t height = 360;

int main(int argc, char* argv[]) {
	// Get an OpenCL GPU device

	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	cl::Platform platform = platforms.front();

	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

	cl::Device device = devices.front();

	// Check the endianess of host and device

	union { uint32_t i; char c[4]; } bint = { 0x01020304 }; HOST_BIG_ENDIAN = (bint.c[0] == 0x01);
	device.getInfo(CL_DEVICE_ENDIAN_LITTLE, &DEVICE_BIG_ENDIAN); DEVICE_BIG_ENDIAN = !DEVICE_BIG_ENDIAN;

	// Initialize the OpenCL context

	cl::Context clContext = cl::Context(device);

	// Initialize the OpenCL buffers

	cl::Buffer eleBuffer = cl::Buffer(clContext, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(Element) * 4, (Element*)(&ELEMENTS));
	cl::Buffer actBuffer = cl::Buffer(clContext, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(Interact) * 4 * 4, (Interact*)(&INTERACT));
	
	cl::Image2D celImage = cl::Image2D(clContext, CL_MEM_READ_WRITE, { CL_RGBA, CL_HALF_FLOAT }, width, height);

	cl::Buffer objBuffer = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, 16 * 4 * 64);

	// Compile the OpenCL code with a macroprocessor

	std::ifstream strm("src/kernel/kernel.cl");
	std::string kstr(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));

	std::string rstr = "";

	while (kstr.find("/*KER_EXC_BEG*/") != -1) {
		kstr.replace(kstr.find("/*KER_EXC_BEG*/"), kstr.find("/*KER_EXC_END*/") - kstr.find("/*KER_EXC_BEG*/") + 15, rstr);
	}
	
	strm = std::ifstream("src/host/elements.hpp");
	rstr = std::string(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));
	rstr = rstr.substr(rstr.find("/*KER_ELE_BEG*/") + 15, rstr.find("/*KER_ELE_END*/") - rstr.find("/*KER_ELE_BEG*/") - 15);
	kstr.replace(kstr.find("/*KER_ELE_BEG*/"), kstr.find("/*KER_ELE_END*/") - kstr.find("/*KER_ELE_BEG*/") + 15, rstr);
	
	cl::Program::Sources sources = cl::Program::Sources(1, std::make_pair(kstr.c_str(), kstr.length() + 1));

	cl::Program clProgram(clContext, sources);

	char* clBuild = new char[30]();
	sprintf_s(clBuild, 31, "-cl-std=CL1.2 -D OpenCLDebug=%u", (bool)OpenCLDebug);
	clProgram.build(clBuild);

	// Initialize the OpenCL kernels

	cl::Kernel actKernel = cl::Kernel(clProgram, "actKernel");
	actKernel.setArg(0, celImage);
	actKernel.setArg(1, celImage);
	actKernel.setArg(2, eleBuffer);
	actKernel.setArg(3, actBuffer);

	cl::Kernel datKernel = cl::Kernel(clProgram, "datKernel");
	datKernel.setArg(0, celImage);
	datKernel.setArg(1, celImage);
	datKernel.setArg(2, eleBuffer);
	datKernel.setArg(3, objBuffer);

	cl::Kernel setKernel = cl::Kernel(clProgram, "setKernel");
	setKernel.setArg(0, celImage);
	setKernel.setArg(1, celImage);

	cl::Kernel objKernel = cl::Kernel(clProgram, "objKernel");
	objKernel.setArg(0, celImage);
	objKernel.setArg(1, celImage);
	objKernel.setArg(2, objBuffer);

	cl::Kernel movKernel = cl::Kernel(clProgram, "movKernel");
	movKernel.setArg(0, celImage);
	movKernel.setArg(1, celImage);
	movKernel.setArg(2, objBuffer);

	cl::Kernel scaKernel = cl::Kernel(clProgram, "scaKernel");
	scaKernel.setArg(0, celImage);
	scaKernel.setArg(1, celImage);
	scaKernel.setArg(2, objBuffer);

	cl::Kernel ubdKernel = cl::Kernel(clProgram, "ubdKernel");
	ubdKernel.setArg(0, celImage);
	ubdKernel.setArg(1, celImage);
	ubdKernel.setArg(2, objBuffer);

	cl::Kernel rdsKernel = cl::Kernel(clProgram, "rdsKernel");
	rdsKernel.setArg(0, celImage);
	rdsKernel.setArg(1, celImage);

	// Initialize an OpenCL command queue
	
	cl::CommandQueue clQueue = cl::CommandQueue(clContext, device);

	// Wait for user input, then exit program

	system("pause");

	return 0;
}