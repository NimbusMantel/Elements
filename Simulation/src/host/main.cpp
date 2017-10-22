#include <CL/cl.hpp>

#include <vector>
#include <fstream>

#include <iostream>

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

	// Compile the OpenCL code with a macroprocessor

	std::ifstream strm("src/kernel/kernel.cl");
	std::string kstr(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));

	std::string rstr = "";

	while (kstr.find("/*KER_EXC_BEG*/") != -1) {
		kstr.replace(kstr.find("/*KER_EXC_BEG*/"), kstr.find("/*KER_EXC_END*/") - kstr.find("/*KER_EXC_BEG*/") + 15, rstr);
	}
	
	cl::Program::Sources sources = cl::Program::Sources(1, std::make_pair(kstr.c_str(), kstr.length() + 1));

	cl::Program clProgram(clContext, sources);

	char* clBuild = new char[30]();
	sprintf_s(clBuild, 31, "-cl-std=CL1.2 -D OpenCLDebug=%u", (bool)OpenCLDebug);
	clProgram.build(clBuild);

	// Initialize an OpenCL test kernel

	cl::Kernel testKernel = cl::Kernel(clProgram, "testKernel");

	// Initialize an OpenCL command queue
	
	cl::CommandQueue clQueue = cl::CommandQueue(clContext, device);

	// Enqueue the test kernel

	clQueue.enqueueTask(testKernel);

	// Wait for user input, then exit program

	system("pause");

	return 0;
}