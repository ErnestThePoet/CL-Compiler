#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "CL/cl.h"
#include "cl_device_helper.h"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::vector;

#define ERROR_LINE(S)       "CL Compiler Error: "<<S<<endl
#define USAGE_TEXT          "Usage: aclc|iclc <input files> [-o <output file>] [options]"
#define COMPILE_SUCCESS     0
#define COMPILE_FAILURE     1

#define CHECK_STATUS(M) \
do{if(status!=CL_SUCCESS){std::cerr<<ERROR_LINE(M<<" Error code "<<status);return COMPILE_FAILURE;}} while(false)


int CompileAndSaveCLProgram(
    const cl_device_id device_id,
    vector<const char*>& sources, 
    const string& options,
    const string& output_file_name)
{
	cl_int status = CL_SUCCESS;

	cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &status);
	CHECK_STATUS("Failed to create context.");

	cl_command_queue command_queue = 
		clCreateCommandQueueWithProperties(context, device_id, NULL, &status);
	CHECK_STATUS("Failed to create command queue.");

	cl_program program = 
		clCreateProgramWithSource(context, sources.size(), sources.data(), NULL, &status);
	CHECK_STATUS("Failed to create program with source.");

	status = clBuildProgram(program, 1, &device_id, options.c_str(), NULL, NULL);

	if (status == CL_BUILD_PROGRAM_FAILURE)
	{
		size_t build_log_size = 0;
		clGetProgramBuildInfo(
			program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_size);

		vector<char> build_log(build_log_size);

		clGetProgramBuildInfo(
			program, device_id, CL_PROGRAM_BUILD_LOG, build_log_size, build_log.data(), NULL);

		cerr << "Failed to build program. Build log:\n" << build_log.data() << endl;

		return COMPILE_FAILURE;
	}
	CHECK_STATUS("Failed to build program.");

	size_t binary_size = 0;
	status = clGetProgramInfo(
		program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
	CHECK_STATUS("Failed to get program binary size.");

	std::vector<unsigned char> program_binary(binary_size);
	unsigned char* binary_ptrs[] = { program_binary.data() };
	status = clGetProgramInfo(
		program,
		CL_PROGRAM_BINARIES,
		sizeof(program_binary.data()),
		binary_ptrs,
		NULL);
	CHECK_STATUS("Failed to get program binary.");

	ofstream ofs(output_file_name, std::ios::out | std::ios::binary);
	if (!ofs.is_open())
	{
		std::cerr << ERROR_LINE("Failed to open output file: " << output_file_name);
		return COMPILE_FAILURE;
	}

	ofs.write((const char*)program_binary.data(), binary_size);
	ofs.close();

	cout << "Successful! Wrote " << binary_size << " bytes to " << output_file_name << endl;

	clReleaseProgram(program);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);

	return COMPILE_SUCCESS;
}

int main(int argc, char* argv[])
{
    if(argc<2)
    {
        cout << ERROR_LINE("No input files.");
        cout << USAGE_TEXT << endl;
        return COMPILE_FAILURE;
    }

    vector<string> input_file_names;
    string output_file_name;
	string options;

    for (int i = 1; i < argc; i++)
    {
		// once we encounter "-o", all args next will be taken over here
        if (!strcmp(argv[i], "-o"))
        {
            if (i + 1 < argc)
            {
                output_file_name = argv[i + 1];

				if (i + 2 < argc)
				{
					for (int j = i + 2; j < argc; j++)
					{
						options += argv[j];
						options += ' ';
					}
				}
            }
            else
            {
                cout << ERROR_LINE("No output file name.");
                cout << USAGE_TEXT << endl;
                return COMPILE_FAILURE;
            }
            break;
        }

		// handle single argument
		if (argv[i][0] == '-')
		{
			options += argv[i];
			options += ' ';
		}
		else
		{
			input_file_names.emplace_back(argv[i]);
		}

		// we have arrived at the last argument and no output file was given (if there is a "-o", we won't 
		// arrive here because the if block above will take over all args next).
		// now we use first input file name as output file name.
		if (i == argc - 1)
		{
			if (input_file_names.size() == 0)
			{
				cout << ERROR_LINE("No input files.");
				cout << USAGE_TEXT << endl;
				return COMPILE_FAILURE;
			}

			if (input_file_names[0].substr(input_file_names[0].length() - 3, 3) == ".cl")
			{
				output_file_name = 
					input_file_names[0].substr(0, input_file_names[0].length() - 3) + ".bin";
			}
			else
			{
				output_file_name = input_file_names[0] + ".bin";
			}
		}
    }

	vector<const char*> sources;
	vector<vector<char>> source_chars;

	for (auto& i : input_file_names)
	{
		ifstream ifs(i, std::ios::in);
		if (!ifs.is_open())
		{
			cerr << ERROR_LINE("Failed to open input file: " << i);
			return COMPILE_FAILURE;
		}

		ifs.seekg(0, std::ios::end);
		size_t file_char_count = ifs.tellg();
		ifs.seekg(0, std::ios::beg);

		source_chars.emplace_back(file_char_count + 1);
		ifs.read(source_chars[source_chars.size() - 1].data(), file_char_count);

		ifs.close();

		source_chars[source_chars.size() - 1][file_char_count] = '\0';

		sources.push_back(source_chars[source_chars.size() - 1].data());
	}

	CLDeviceHelper helper;
	cl_device_id device_id = nullptr;

	cout << "Choose your target device:" << endl;

	try
	{
		helper.Initialize();
		helper.PrintAllDevices();
		device_id = helper.GetDeviceIdFromInput();
	}
	catch (const std::runtime_error& e)
	{
		cerr << ERROR_LINE(e.what());
		return COMPILE_FAILURE;
	}

	cout << endl;

	if (CompileAndSaveCLProgram(
		device_id, 
		sources, 
		options, 
		output_file_name) 
		!= COMPILE_SUCCESS)
	{
		return COMPILE_FAILURE;
	}

    return COMPILE_SUCCESS;
}
