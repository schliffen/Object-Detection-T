
#include <dlpack/dlpack.h>
#include <tvm/runtime/module.h>
#include <tvm/runtime/registry.h>
#include <tvm/runtime/packed_func.h>

#include <fstream>
#include <iterator>
#include <algorithm>

#include "src/tvm.hh"
#include "src/pipe.hh"
#include "src/executor.hh"

#include <opencv2/opencv.hpp> 
using namespace std;
using namespace cv;

int main(){
	 
	string so = "./test.tvm.so";
	string json = "./test.tvm.json";
	string params = "./test.tvm.params";
    
	tvm::runtime::Module mod_syslib = tvm::runtime::Module::LoadFromFile(so);

    // json graph
    std::ifstream json_in(json, std::ios::in);
    std::string json_data((std::istreambuf_iterator<char>(json_in)), std::istreambuf_iterator<char>());
    json_in.close();

    // parameters in binary
    std::ifstream params_in(params, std::ios::binary);
    std::string params_data((std::istreambuf_iterator<char>(params_in)), std::istreambuf_iterator<char>());
    params_in.close();

    // parameters need to be TVMByteArray type to indicate the binary data
    TVMByteArray params_arr;
    params_arr.data = params_data.c_str();
    params_arr.size = params_data.length();

    int dtype_code = kDLFloat;
    int dtype_bits = 32;
    int dtype_lanes = 1;
    int device_type = kDLGPU;
    int device_type_cpu = kDLCPU;
    int device_id = 0;
    
	tvm::runtime::Module mod = (*tvm::runtime::Registry::Get("tvm.graph_runtime.create"))(json_data, mod_syslib, device_type, device_id);

    DLTensor * x;

    int in_ndim = 4;
    
	int64_t in_shape[4] = {1, 3, 224, 224};
	
	Mat image = imread("dog.jpg", cv::IMREAD_COLOR);
	
	Mat resized;                                                                                                                                      
	
	vector <Mat> inputs;
	
	resize(image, resized, cv::Size(224, 224));

	TVMArrayAlloc(in_shape, in_ndim, dtype_code, dtype_bits, dtype_lanes, device_type_cpu, device_id, &x);

	string sdtype = "float32";
	
	auto pipein = TVMPipe(resized, "gpu", sdtype);

	float * fdata_in = new float [1 * 3 * 224 * 224];

	int result = TVMArrayCopyToBytes(x, fdata_in, 1 * 3 * 224 * 224 * 4);

	//auto x_iter = static_cast <float *> (x -> data);
    
	tvm::runtime::PackedFunc set_input = mod.GetFunction("set_input");
    
	set_input("data", x);

    // get the function from the module(load patameters)
    tvm::runtime::PackedFunc load_params = mod.GetFunction("load_params");
    
	load_params(params_arr);

    // get the function from the module(run it)
    tvm::runtime::PackedFunc run = mod.GetFunction("run");
    
	run();

    DLTensor * y;
    
	int out_ndim = 4;
    
	int64_t out_shape[4] = {1, 7, 222, 222};
    
	TVMArrayAlloc(out_shape, out_ndim, dtype_code, dtype_bits, dtype_lanes, device_type, device_id, &y);

    // get the function from the module(get output data)
    tvm::runtime::PackedFunc get_output = mod.GetFunction("get_output");
    
	get_output(0, y);

	cout << "[*] Get Output " << endl;

	vector <float> fdata (1 * 7 * 222 * 222, 7);
	//
	//float * fdata  = new float [1 * 7 * 222 * 222];

	//int sYresult =  TVMSynchronize	(device_type, device_id, stvm);
	
	int Yresult = TVMArrayCopyToBytes(y, fdata.data(), 1 * 7 * 222 * 222 * 4);

	//cout << "Sync Y Result : " << sYresult << endl;
	//cout << "Y Result : " << Yresult << endl;
	//cout << "Y output : " << fdata[0] << endl;


	//delete fdata;

	delete fdata_in;
    /* std::cout << *it; ... */

    //auto max_iter = std::max_element(y_iter, y_iter + 1000);
    
	//auto max_index = std::distance(y_iter, max_iter);
    
	//std::cout << "The maximum position in output vector is: " << max_index << std::endl;

    TVMArrayFree(x);
    
	TVMArrayFree(y);

    return 0;
}
