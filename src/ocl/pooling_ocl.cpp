#include <mlopen/pooling.hpp>
#include <mlopen/mlo_internal.hpp>
#include <mlopen/kernel_cache.hpp>

namespace mlopen {
mlopenStatus_t PoolingDescriptor::Forward(
		Handle								&handle,
		const void							*alpha,
		const TensorDescriptor				&xDesc,
		const cl_mem						x,
		const void							*beta,
		const TensorDescriptor				&yDesc,
		cl_mem								y,
		bool								do_backward,
		cl_mem								workSpace,
		size_t								workSpaceSize) {

	mlopenStatus_t status = mlopenStatusSuccess;
	printf("in pooling forward\n");
	mlo_construct_pooling2D construct_params(1); // forward

	std::string kernel_path = "../src/Kernels/";

	construct_params.setKernelPath(kernel_path);

	construct_params.setStream(handle.GetStream());

	{
		int nOut;
		int cOut;
		int hOut;
		int wOut;
		int nOutStride;
		int cOutStride;
		int hOutStride;
		int wOutStride;

		std::tie(nOut, cOut, hOut, wOut) = tie4(yDesc.GetLengths());
		std::tie(nOutStride, cOutStride, hOutStride, wOutStride) = tie4(yDesc.GetStrides());


		construct_params.setTopDescr(
			"NCHW",
			"FP32",
			nOut,
			cOut,
			hOut,
			wOut,
			nOutStride,
			cOutStride,
			hOutStride,
			wOutStride);
	}

	{
		int nIn;
		int cIn;
		int hIn;
		int wIn;
		int nInStride;
		int cInStride;
		int hInStride;
		int wInStride;

		std::tie(nIn, cIn, hIn, wIn) = tie4(xDesc.GetLengths());
		std::tie(nInStride, cInStride, hInStride, wInStride) = tie4(xDesc.GetStrides());

		construct_params.setBotDescr(
			"NCHW",
			"FP32",
			nIn,
			cIn,
			hIn,
			wIn,
			nInStride,
			cInStride,
			hInStride,
			wInStride);
	}

	mlopenPoolingMode_t mode = GetMode();
	const std::vector<int> & lengths = GetLengths();
	const std::vector<int> & strides = GetStrides();
	const std::vector<int> & pads = GetPads();
	int pooling_method = (mode == mlopenPoolingMax) ? MLO_POOLING_OP_MAX : MLO_POOLING_OP_AVE;
	construct_params.setPoolingDescr(pooling_method, lengths[0], lengths[1], pads[0], pads[1], strides[0], strides[1]);

	status = (mlopenStatus_t)construct_params.mloConstruct();

	std::string program_name = kernel_path + construct_params.getKernelFile();  // CL kernel filename
	std::string kernel_name = construct_params.getKernelName(); // kernel name
	std::string parms = construct_params.getCompilerOptions(); // kernel parameters

	std::string network_config;
	construct_params.mloBuildConf_Key(network_config);

	const std::vector<size_t> & vld = construct_params.getLocalWkSize();
	const std::vector<size_t> & vgd = construct_params.getGlobalWkSize();

	handle.GetKernel("mlopenPooling2dDForward",
		network_config,
		program_name,
		kernel_name,
		vld,
		vgd,
		parms)(x, y);


	handle.Finish();

	std::cout << "Pooling Forward Finished !!" << std::endl;

	return mlopenStatusSuccess;
}

mlopenStatus_t PoolingDescriptor::Backward(
		Handle								&handle,
		const void							*alpha,
		const TensorDescriptor				&yDesc,
		const cl_mem						y,
		const TensorDescriptor				&dyDesc,
		const cl_mem						dy,
		const TensorDescriptor				&xDesc,
		const cl_mem						x,
		const void							*beta,
		const TensorDescriptor				&dxDesc,
		cl_mem								dx,
		const cl_mem						workSpace) {


	mlopenStatus_t status = mlopenStatusSuccess;
	printf("in pooling backward\n");
	mlo_construct_pooling2D construct_params(0); // backward

	std::string kernel_path = "../src/Kernels/";

	construct_params.setKernelPath(kernel_path);

	construct_params.setStream(handle.GetStream());

	{
		int ndOut;
		int cdOut;
		int hdOut;
		int wdOut;
		int ndOutStride;
		int cdOutStride;
		int hdOutStride;
		int wdOutStride;

		std::tie(ndOut, cdOut, hdOut, wdOut) = tie4(dyDesc.GetLengths());
		std::tie(ndOutStride, cdOutStride, hdOutStride, wdOutStride) = tie4(dyDesc.GetStrides());


		construct_params.setTopDfDescr(
			"NCHW",
			"FP32",
			ndOut,
			cdOut,
			hdOut,
			wdOut,
			ndOutStride,
			cdOutStride,
			hdOutStride,
			wdOutStride);
	}

	{
		int nOut;
		int cOut;
		int hOut;
		int wOut;
		int nOutStride;
		int cOutStride;
		int hOutStride;
		int wOutStride;

		std::tie(nOut, cOut, hOut, wOut) = tie4(yDesc.GetLengths());
		std::tie(nOutStride, cOutStride, hOutStride, wOutStride) = tie4(yDesc.GetStrides());


		construct_params.setTopDescr(
			"NCHW",
			"FP32",
			nOut,
			cOut,
			hOut,
			wOut,
			nOutStride,
			cOutStride,
			hOutStride,
			wOutStride);
	}


	{
		int ndIn;
		int cdIn;
		int hdIn;
		int wdIn;
		int ndInStride;
		int cdInStride;
		int hdInStride;
		int wdInStride;

		std::tie(ndIn, cdIn, hdIn, wdIn) = tie4(dxDesc.GetLengths());
		std::tie(ndInStride, cdInStride, hdInStride, wdInStride) = tie4(dxDesc.GetStrides());

		construct_params.setBotDfDescr(
			"NCHW",
			"FP32",
			ndIn,
			cdIn,
			hdIn,
			wdIn,
			ndInStride,
			cdInStride,
			hdInStride,
			wdInStride);
	}

	{
		int nIn;
		int cIn;
		int hIn;
		int wIn;
		int nInStride;
		int cInStride;
		int hInStride;
		int wInStride;

		std::tie(nIn, cIn, hIn, wIn) = tie4(xDesc.GetLengths());
		std::tie(nInStride, cInStride, hInStride, wInStride) = tie4(xDesc.GetStrides());

		construct_params.setBotDescr(
			"NCHW",
			"FP32",
			nIn,
			cIn,
			hIn,
			wIn,
			nInStride,
			cInStride,
			hInStride,
			wInStride);
	}

	mlopenPoolingMode_t mode = mode;
	const std::vector<int> & lengths = GetLengths();
	const std::vector<int> & strides = GetStrides();
	const std::vector<int> & pads = GetPads();
	int pooling_method = (mode == mlopenPoolingMax) ? MLO_POOLING_OP_MAX : MLO_POOLING_OP_AVE;
	construct_params.setPoolingDescr(pooling_method, lengths[0], lengths[1], pads[0], pads[1], strides[0], strides[1]);

	status = (mlopenStatus_t)construct_params.mloConstruct();


	std::string program_name = kernel_path + construct_params.getKernelFile();  // CL kernel filename
	std::string kernel_name = construct_params.getKernelName(); // kernel name
	std::string parms = construct_params.getCompilerOptions(); // kernel parameters

	std::string network_config;
	construct_params.mloBuildConf_Key(network_config);

	const std::vector<size_t> & vld = construct_params.getLocalWkSize();
	const std::vector<size_t> & vgd = construct_params.getGlobalWkSize();

	// Compile the kernel if not aleady compiled
	auto queue = handle.GetStream();
	OCLKernel obj = KernelCache::get(queue,
		"mlopenPooling2dBackward",
		network_config,
		program_name,
		kernel_name,
		vld,
		vgd,
		parms);

	std::string kernName;
	obj.GetKernelName(kernName);

	// Set kernel arguments
	// Use proper arguments
	if (!kernName.compare("mloPoolingMaxBwd"))
	{
		obj.SetArgs(0, dy, dx, y, x);
	}
	else
	{
		obj.SetArgs(0, dy, dx);
	}

	int dim = (int)vld.size();

	// Run the kernel
	obj.run(queue, dim, 0, vgd.data(), vld.data(), NULL);

	clFinish(queue);

	std::cout << "Pooling Backward Finished !!" << std::endl;


	return(status);
}
}
