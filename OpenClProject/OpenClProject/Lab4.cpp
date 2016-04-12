#include <CL/cl.h>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <iostream>

int rowCount, columnCount, td;

int main() {
	
	
	cl_int status = 0;
	cl_platform_id platformID;
	cl_uint nbPlatforms;

	cl_device_id deviceID;
	cl_uint nbDevices;

	cl_context context;

	cl_command_queue commandQueue = nullptr;

	cl_int size = rowCount * columnCount * sizeof(float);
	cl_mem sourceBuffer;
	cl_mem destinationBuffer;

	size_t programLength = 0;
	cl_program program;

	// clGetPlatformIDs
	status = clGetPlatformIDs(1, &platformID, &nbPlatforms);
	checkStatusAndOutputError(status);
	std::cout << "Nb platforms " << nbPlatforms << " and platformID " << platformID << std::endl;

	// clGetDeviceIDs
	status = clGetDeviceIDs(platformID, CL_DEVICE_TYPE_GPU, 1, &deviceID, &nbDevices);
	checkStatusAndOutputError(status);
	std::cout << "Nb devices " << nbDevices << " and devideID " << deviceID << std::endl;

	// clCreateContext
	context = clCreateContext(0, 1, &deviceID, NULL, NULL, &status);
	checkStatusAndOutputError(status);

	// clCreateCommandQueue
	commandQueue = clCreateCommandQueue(context, deviceID, 0, &status);
	checkStatusAndOutputError(status);

	// clCreateBuffer
	sourceBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, 0, &status);
	checkStatusAndOutputError(status);

	destinationBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, 0, &status);
	checkStatusAndOutputError(status);
	
	// clCreateProgramWithSource
	size_t progLength = 0;
	char* progSource = oclLoadProgSource("Lab4.cl", "", &programLength);

	program = clCreateProgramWithSource(context, 1, (const char **)&progSource, &progLength, &status);
	checkStatusAndOutputError(status);

	// clBuildProgram
	status = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	checkStatusAndOutputError(status);

	// clCreateKernel

	// clSetKernelArg

	// clEnqueueWriteBuffer

	// clEnqueueNDRangeKernel

	// clEnqueueReadBuffer
}

void checkStatusAndOutputError(cl_int status) {
	if (status != CL_SUCCESS) {
		std::cout << "Error at line number " << __LINE__ << std::endl;
		std::cout << "Error in code " << status << std::endl;
	}
}

//////////////////////////////////////////////////////////////////////////////
//! Loads a Program file and prepends the cPreamble to the code.
//!
//! @return the source string if succeeded, 0 otherwise
//! @param cFilename program filename
//! @param cPreamble code that is prepended to the loaded file, typically a set of
// #defines or a header
//! @param szFinalLength returned length of the code string
//////////////////////////////////////////////////////////////////////////////
char* oclLoadProgSource(const char* cFilename, const char* cPreamble, size_t*
	szFinalLength)
{
	// locals
	FILE* pFileStream = NULL;
	size_t szSourceLength;
	// open the OpenCL source code file
	if (fopen_s(&pFileStream, cFilename, "rb") != 0)
	{
		return NULL;
	}
	size_t szPreambleLength = strlen(cPreamble);
	// get the length of the source code
	fseek(pFileStream, 0, SEEK_END);
	szSourceLength = ftell(pFileStream);
	fseek(pFileStream, 0, SEEK_SET);
	// allocate a buffer for the source code string and read it in
	char* cSourceString = (char *)malloc(szSourceLength + szPreambleLength + 1);
	memcpy(cSourceString, cPreamble, szPreambleLength);
	if (fread((cSourceString)+szPreambleLength, szSourceLength, 1, pFileStream) != 1)
	{
		fclose(pFileStream);
		free(cSourceString);
		return 0;
	}
	// close the file and return the total length of the combined (preamble + source) string
	
	fclose(pFileStream);
	if (szFinalLength != 0)
	{
		*szFinalLength = szSourceLength + szPreambleLength;
	}
	cSourceString[szSourceLength + szPreambleLength] = '\0';
	return cSourceString;
}