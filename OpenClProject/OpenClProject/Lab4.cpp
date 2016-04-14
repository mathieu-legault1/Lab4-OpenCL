#include <CL/cl.h>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <iostream>
#include <ctime>

float* getInitialMatrix();
void checkStatusAndOutputError(cl_int status);
void setKernelArgAndOutputError(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value);
void outputMatrix(float* matrix);
void copyMatrix(float* matrix1, float* matrix2);
char* oclLoadProgSource(const char* cFilename, const char* cPreamble, size_t* szFinalLength);
const char *getErrorString(cl_int error);
double parSolve();
double seqSolve();

int rowCount, columnCount, np;
float td, h;

const std::string PROGRAM_NAME = "Lab4.cl";
const std::string PROGRAM_METHOD_NAME = "OpenCLHeatTransfer";

int main(int argc, char *argv[]) {

	// probably not the best way in C++ but I'm kind of lazy so!
	if (argc != 6) {
		std::cout << "Invalid number of arguments" << std::endl;
		return -1;
	}
	
	rowCount = atoi(argv[1]);
	columnCount = atoi(argv[2]);
	np = atoi(argv[3]);
	td = (float)atof(argv[4]);
	h = (float)atof(argv[5]);
	
	double durationSeq = seqSolve();
	double durationPar = parSolve();

	std::cout << "Temps du traitement séquentielle(mili): " << durationSeq << std::endl;
	std::cout << "Temps du traitement parrallele(mili): " << durationPar << std::endl;
	std::cout << "L'acceleration du traitement est de: " << (durationSeq / durationPar) << std::endl;
}

double parSolve() {
    double start = clock();
	cl_int status;
	cl_platform_id platformID;

	cl_device_id deviceID;

	cl_context context;

	cl_command_queue commandQueue;

	cl_int size = sizeof(float)* rowCount * columnCount;
	cl_mem bufferPreviousMatrix;
	cl_mem bufferCurrentMatrix;

	size_t programLength = 0;
	cl_program program;

	cl_kernel kernel;

	// clGetPlatformIDs
	status = clGetPlatformIDs(1, &platformID, NULL);
	checkStatusAndOutputError(status);

	// clGetDeviceIDs
	status = clGetDeviceIDs(platformID, CL_DEVICE_TYPE_GPU, 1, &deviceID, NULL);
	checkStatusAndOutputError(status);

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
	char* progSource = oclLoadProgSource(PROGRAM_NAME.c_str(), "", &programLength); // could create a constant for Lab4.cl BUT HEY! :)

	program = clCreateProgramWithSource(context, 1, (const char **)&progSource, &progLength, &status);
	checkStatusAndOutputError(status);

	// clBuildProgram
	status = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	checkStatusAndOutputError(status);

	// clCreateKernel
	kernel = clCreateKernel(program, PROGRAM_METHOD_NAME.c_str(), &status);
	checkStatusAndOutputError(status);

	// clSetKernelArg
	setKernelArgAndOutputError(kernel, 0, sizeof(bufferPreviousMatrix), &bufferPreviousMatrix);
	setKernelArgAndOutputError(kernel, 1, sizeof(bufferCurrentMatrix), &bufferCurrentMatrix);
	setKernelArgAndOutputError(kernel, 2, sizeof(rowCount), &rowCount);
	setKernelArgAndOutputError(kernel, 3, sizeof(columnCount), &columnCount);
	setKernelArgAndOutputError(kernel, 4, sizeof(td), &td);
	setKernelArgAndOutputError(kernel, 5, sizeof(h), &h);

	float* previousMatrix = getInitialMatrix(); // could probably do something else for currentMatrix but ...
	float* currentMatrix = getInitialMatrix();
	size_t sizeT_size = rowCount * columnCount; // nice name ...

	// we only need to initialize de currentMatrix once 
	status = clEnqueueWriteBuffer(commandQueue, bufferCurrentMatrix, CL_FALSE, 0, size, currentMatrix, 0, NULL, NULL);
	checkStatusAndOutputError(status);


	for (int i = 0; i < np; i++) {
		// // clEnqueueWriteBuffer
		status = clEnqueueWriteBuffer(commandQueue, bufferPreviousMatrix, CL_FALSE, 0, size, previousMatrix, 0, NULL, NULL);
		checkStatusAndOutputError(status);
		
		// clEnqueueNDRangeKerne
		status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &sizeT_size, NULL, 0, NULL, NULL);
		checkStatusAndOutputError(status);

		// clEnqueueReadBuffer
		status = clEnqueueReadBuffer(commandQueue, bufferCurrentMatrix, CL_TRUE, 0, size, previousMatrix, 0, NULL, NULL);
		checkStatusAndOutputError(status);
	}

	double duration = clock() - start;
	std::cout << "Matrice Finale(parallele)" << std::endl;
	outputMatrix(previousMatrix);
	return duration;
}

double seqSolve() {
	double start = clock();
	// on résout le problème séquentiellement (PRIS DU LAB3)
	float* currentMatrix = getInitialMatrix();
	float* previousMatrix = getInitialMatrix();

	for (int k = 1; k <= np; k++) {
		for (int index = 0; index < rowCount * columnCount; index++) {
			int i = index / columnCount;
			int j = index % columnCount;

			if (j == 0 || j == columnCount - 1 || i == 0 || i == rowCount - 1)
			{
				continue;
			}
			
			currentMatrix[index] = (1 - 4 * td / (h*h)) * previousMatrix[index] + (td / (h*h)) * (previousMatrix[index - columnCount] + previousMatrix[index + columnCount] + previousMatrix[index - 1] + previousMatrix[index + 1]);
		}

		copyMatrix(previousMatrix, currentMatrix);
		//previousMatrix = currentMatrix;
	}
	double duration = clock() - start;
	std::cout << "Matrice Finale(séquentielle)" << std::endl;

	outputMatrix(previousMatrix);
	return duration;
}

float* getInitialMatrix() {
	float* matrix = new float[rowCount * columnCount];

	for (int i = 0; i < rowCount; i++) {
		for (int j = 0; j < columnCount; j++) {
			matrix[i * columnCount + j] = (float)(i * j * (rowCount - i - 1) * (columnCount - j - 1));
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

void copyMatrix(float* matrix1, float* matrix2) {
	for (int i = 0; i < rowCount * columnCount; i++) {
		matrix1[i] = matrix2[i];
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
	switch (error) {
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