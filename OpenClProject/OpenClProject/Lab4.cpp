#include <CL/cl.h>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <iostream>

float* getInitialMatrix();
void checkStatusAndOutputError(cl_int status);
void setKernelArgAndOutputError(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value);
void outputMatrix(float* matrix);
char* oclLoadProgSource(const char* cFilename, const char* cPreamble, size_t* szFinalLength);
const char *getErrorString(cl_int error);

int rowCount, columnCount, np;

float td, h;

int main() {
	rowCount = 5;
	columnCount = 10;
	np = 1;
	td = 1;
	h = 1;

	cl_int status = 0;
	cl_platform_id platformID;
	cl_uint nbPlatforms;

	cl_device_id deviceID;
	cl_uint nbDevices;

	cl_context context;

	cl_command_queue commandQueue = nullptr;

	cl_int size = sizeof(float) * rowCount * columnCount;
	cl_mem bufferPreviousMatrix;
	cl_mem bufferCurrentMatrix;

	size_t programLength = 0;
	cl_program program;

	cl_kernel kernel;

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
	bufferPreviousMatrix = clCreateBuffer(context, CL_MEM_READ_WRITE, size, 0, &status);
	checkStatusAndOutputError(status);

	bufferCurrentMatrix = clCreateBuffer(context, CL_MEM_READ_WRITE, size, 0, &status);
	checkStatusAndOutputError(status);
	
	// clCreateProgramWithSource
	size_t progLength = 0;
	char* progSource = oclLoadProgSource("Lab4.cl", "", &programLength);

	program = clCreateProgramWithSource(context, 1, (const char **)&progSource, &progLength, &status);
	checkStatusAndOutputError(status);
	std::cout << "yo" << std::endl;
	// clBuildProgram
	status = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	checkStatusAndOutputError(status);
	std::cout << "yo2" << std::endl;

	// clCreateKernel
	kernel = clCreateKernel(program, "OpenCLHeatTransfer", &status);
	checkStatusAndOutputError(status);

	// clSetKernelArg
	setKernelArgAndOutputError(kernel, 0, sizeof(bufferPreviousMatrix), &bufferPreviousMatrix);
	setKernelArgAndOutputError(kernel, 1, sizeof(bufferCurrentMatrix), &bufferCurrentMatrix);
	setKernelArgAndOutputError(kernel, 2, sizeof(rowCount), &rowCount);
	setKernelArgAndOutputError(kernel, 3, sizeof(columnCount), &columnCount);
	setKernelArgAndOutputError(kernel, 4, sizeof(td), &td);
	setKernelArgAndOutputError(kernel, 5, sizeof(h), &h);

	// clEnqueueWriteBuffer
	float* previousMatrix = getInitialMatrix(); // could probably do something else for currentMatrix but ...
	float* currentMatrix = getInitialMatrix();
	size_t sizeWithoutBorders = (rowCount - 2) * (columnCount - 2);
	float* finalMatrix = previousMatrix;

	for (int i = 0; i < np; i++) {
		// for this, we will use the fact that each "even" i means the the previousMatrix is the previousMatrix
		// and "even" i means the the previous matrix is the currentMatrix

		/* look if we can do something here (yes we can!)*/
		if ((i % 2) == 0) {
			status = clEnqueueWriteBuffer(commandQueue, bufferPreviousMatrix, CL_FALSE, 0, size, previousMatrix, 0, NULL, NULL);
			checkStatusAndOutputError(status);
			status = clEnqueueWriteBuffer(commandQueue, bufferCurrentMatrix, CL_FALSE, 0, size, currentMatrix, 0, NULL, NULL);
			checkStatusAndOutputError(status);
		}
		std::cout << "yaazaa" << std::endl;
		/*else {
			status = clEnqueueWriteBuffer(commandQueue, bufferPreviousMatrix, CL_FALSE, 0, size, currentMatrix, 0, NULL, NULL);
			checkStatusAndOutputError(status);
			status = clEnqueueWriteBuffer(commandQueue, bufferCurrentMatrix, CL_FALSE, 0, size, previousMatrix, 0, NULL, NULL);
			checkStatusAndOutputError(status);
		}*/

		status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &sizeWithoutBorders, NULL, 0, NULL, NULL);
		checkStatusAndOutputError(status);


		//if ((i % 2) == 0) {
			status = clEnqueueReadBuffer(commandQueue, bufferCurrentMatrix, CL_TRUE, 0, size, finalMatrix, 0, NULL, NULL);
		//}
		/*else {
			status = clEnqueueReadBuffer(commandQueue, bufferPreviousMatrix, CL_TRUE, 0, size, finalMatrix, 0, NULL, NULL);
		}*/
		checkStatusAndOutputError(status);

		

	}
	std::cout << "end" << std::endl;
	outputMatrix(finalMatrix);
	std::cout << std::endl;
}

float* getInitialMatrix() {
	float* matrix = new float[rowCount * columnCount];

	for (int i = 0; i < rowCount; i++) {
		for (int j = 0; j < columnCount; j++) {
			matrix[i * columnCount + j] = i * j * (rowCount - i - 1) * (columnCount - j - 1);
		}
	}

	return matrix;
}

void checkStatusAndOutputError(cl_int status) {
	if (status != CL_SUCCESS) {
		std::cout << "Error in code " << getErrorString(status) << std::endl;
	}
}

void setKernelArgAndOutputError(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value) {
	cl_int status = clSetKernelArg(kernel, arg_index, arg_size, arg_value);
	checkStatusAndOutputError(status);
}

void outputMatrix(float* matrix) {
	// wasn't able to reproduce same output with std::left :(
	for (int i = 0; i < rowCount; i++) {
		for (int j = 0; j < columnCount; j++) {
			printf("%5.1f ", matrix[i * columnCount + j]);
		}
		printf("\n");
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

// http://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
const char *getErrorString(cl_int error)
{
	switch (error){
		// run-time and JIT compiler errors
	case 0: return "CL_SUCCESS";
	case -1: return "CL_DEVICE_NOT_FOUND";
	case -2: return "CL_DEVICE_NOT_AVAILABLE";
	case -3: return "CL_COMPILER_NOT_AVAILABLE";
	case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case -5: return "CL_OUT_OF_RESOURCES";
	case -6: return "CL_OUT_OF_HOST_MEMORY";
	case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case -8: return "CL_MEM_COPY_OVERLAP";
	case -9: return "CL_IMAGE_FORMAT_MISMATCH";
	case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case -11: return "CL_BUILD_PROGRAM_FAILURE";
	case -12: return "CL_MAP_FAILURE";
	case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
	case -15: return "CL_COMPILE_PROGRAM_FAILURE";
	case -16: return "CL_LINKER_NOT_AVAILABLE";
	case -17: return "CL_LINK_PROGRAM_FAILURE";
	case -18: return "CL_DEVICE_PARTITION_FAILED";
	case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

		// compile-time errors
	case -30: return "CL_INVALID_VALUE";
	case -31: return "CL_INVALID_DEVICE_TYPE";
	case -32: return "CL_INVALID_PLATFORM";
	case -33: return "CL_INVALID_DEVICE";
	case -34: return "CL_INVALID_CONTEXT";
	case -35: return "CL_INVALID_QUEUE_PROPERTIES";
	case -36: return "CL_INVALID_COMMAND_QUEUE";
	case -37: return "CL_INVALID_HOST_PTR";
	case -38: return "CL_INVALID_MEM_OBJECT";
	case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case -40: return "CL_INVALID_IMAGE_SIZE";
	case -41: return "CL_INVALID_SAMPLER";
	case -42: return "CL_INVALID_BINARY";
	case -43: return "CL_INVALID_BUILD_OPTIONS";
	case -44: return "CL_INVALID_PROGRAM";
	case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
	case -46: return "CL_INVALID_KERNEL_NAME";
	case -47: return "CL_INVALID_KERNEL_DEFINITION";
	case -48: return "CL_INVALID_KERNEL";
	case -49: return "CL_INVALID_ARG_INDEX";
	case -50: return "CL_INVALID_ARG_VALUE";
	case -51: return "CL_INVALID_ARG_SIZE";
	case -52: return "CL_INVALID_KERNEL_ARGS";
	case -53: return "CL_INVALID_WORK_DIMENSION";
	case -54: return "CL_INVALID_WORK_GROUP_SIZE";
	case -55: return "CL_INVALID_WORK_ITEM_SIZE";
	case -56: return "CL_INVALID_GLOBAL_OFFSET";
	case -57: return "CL_INVALID_EVENT_WAIT_LIST";
	case -58: return "CL_INVALID_EVENT";
	case -59: return "CL_INVALID_OPERATION";
	case -60: return "CL_INVALID_GL_OBJECT";
	case -61: return "CL_INVALID_BUFFER_SIZE";
	case -62: return "CL_INVALID_MIP_LEVEL";
	case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
	case -64: return "CL_INVALID_PROPERTY";
	case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
	case -66: return "CL_INVALID_COMPILER_OPTIONS";
	case -67: return "CL_INVALID_LINKER_OPTIONS";
	case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

		// extension errors
	case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
	case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
	case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
	case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
	case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
	case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
	default: return "Unknown OpenCL error";
	}
}