#ifndef CL_MAGMA_RT_H
#define CL_MAGMA_RT_H
#pragma once

#include <vector>
#include <map>
#include <string>

#include "magma.h"

//#include "stdio.h"
//#include "stdlib.h"
//#include <string.h>
//
//#include <new>
//#include <map>
//#include <string>
//#include <math.h>
//#include <vector>
//#include <sstream>
//#include <iostream>
//#include <memory>
//#include <fstream>
//#include <exception>
//
//
//#include "cl.h"
//#include <oclUtils.h>
//
//#define QUEUE_COUNT 2 
//
//using namespace std;

class CL_MAGMA_RT
{
	private:
		unsigned int MAX_GPU_COUNT;
		
		cl_platform_id cpPlatform;
		cl_uint ciDeviceCount;
		
		cl_kernel ckKernel;             // OpenCL kernel
		cl_event ceEvent;               // OpenCL event
		size_t szParmDataBytes;         // Byte size of context information
		size_t szKernelLength;          // Byte size of kernel code
		cl_int ciErrNum;                // Error code var
		
		bool HasBeenInitialized;
		std::map<std::string, std::string> KernelMap;
		
		int GatherFilesToCompile(const char* FileNameList, std::vector<std::string>&);
		std::string fileToString(const char* FileName);
		cl_device_id* cdDevices;        // OpenCL device list    
		cl_context cxGPUContext;        // OpenCL context
		cl_command_queue *commandQueue;
		
	public:
		cl_device_id * GetDevicePtr();
		cl_context GetContext();
		cl_command_queue GetCommandQueue(int queueid);
		CL_MAGMA_RT();
		~CL_MAGMA_RT();
		bool Init ();
		bool Init(cl_platform_id gPlatform, cl_context gContext);
		bool Quit ();
		
		bool CompileFile(const char*FileName);
		bool CompileSourceFiles(const char* FileNameList);
		const char* GetErrorCode(cl_int err);
		bool BuildFromBinaries(const char*FileName);
		bool BuildKernelMap(const char* FileNameList);
		bool CreateKernel(const char* KernelName);
		
		std::map<std::string, std::string> Kernel2FileNamePool;	// kernel name -> file name 
		std::map<std::string, cl_program> ProgramPool;	// file name -> program
		std::map<std::string, cl_kernel> KernelPool;	// kernel name -> kernel
};

// declare global runtime object
extern CL_MAGMA_RT rt;

#endif        //  #ifndef CL_MAGMA_RT_H
