// --------------------------------------------------------
// Dragon
// Copyright(c) 2017 SeetaTech
// Written by Ting Pan
// --------------------------------------------------------

#ifndef DRAGON_OPERATORS_VISION_CONV_OP_H_
#define DRAGON_OPERATORS_VISION_CONV_OP_H_

#include "operators/vision/conv_op_base.h"

namespace dragon {

template <class Context>
class ConvOp : public ConvOpBase<Context> {
 public:
    ConvOp(const OperatorDef& def, Workspace* ws) 
        : ConvOpBase<Context>(def, ws) {}

    void ComputeOutputShape() override;
    bool ReverseDimensions() override { return false; }

    void RunOnDevice() override;
    template <typename T> void RunWithType();
};

template <class Context>
class ConvGradientOp : public ConvOp<Context> {
 public:
    ConvGradientOp(const OperatorDef& def, Workspace* ws) 
        : ConvOp<Context>(def, ws) {}

    void ShareBeforeRun() override;
    void RunOnDevice() override;
    void ClearAfterRun() override;
    template <typename T> void RunWithType();
};

#ifdef WITH_CUDNN

#include "utils/cudnn_device.h"

template <class Context>
class CuDNNConvOp : public ConvOp<Context> {
 public:
    CuDNNConvOp(const OperatorDef& def, Workspace* ws)
        : ConvOp<Context>(def, ws) {
        handle = new cudnnHandle_t[this->group];
        stream = new cudaStream_t[this->group];
        ctx().SwitchToDevice();
        for (int g = 0; g < this->group; g++) {
            CUDA_CHECK(cudaStreamCreate(&stream[g]));
            CUDNN_CHECK(cudnnCreate(&handle[g]));
            CUDNN_CHECK(cudnnSetStream(handle[g], stream[g]));
        }
        CUDNN_CHECK(cudnnCreateFilterDescriptor(&filter_desc));
        CUDNN_CHECK(cudnnCreateTensorDescriptor(&input_desc));
        CUDNN_CHECK(cudnnCreateTensorDescriptor(&output_desc));
        CUDNN_CHECK(cudnnCreateConvolutionDescriptor(&conv_desc));
        if (InputSize() > 2)
            CUDNN_CHECK(cudnnCreateTensorDescriptor(&bias_desc));
    }

    void RunOnDevice() override;
    template <typename T> void RunWithType();

 protected:
    cudnnHandle_t* handle;
    cudaStream_t*  stream;
    cudnnConvolutionFwdAlgo_t fwd_algo;
    cudnnTensorDescriptor_t input_desc, output_desc, bias_desc;
    cudnnConvolutionDescriptor_t conv_desc;
    cudnnFilterDescriptor_t filter_desc;
    size_t workspace_fwd_data_size;
    int bias_offset;
};

template <class Context>
class CuDNNConvGradientOp : public ConvGradientOp<Context> {
 public:
    CuDNNConvGradientOp(const OperatorDef& def, Workspace* ws) 
        : ConvGradientOp<Context>(def, ws) {
        handle = new cudnnHandle_t[this->group * 3];
        stream = new cudaStream_t[this->group * 3];
        for (int g = 0; g < this->group * 3; g++) {
            CUDA_CHECK(cudaStreamCreate(&stream[g]));
            CUDNN_CHECK(cudnnCreate(&handle[g]));
            CUDNN_CHECK(cudnnSetStream(handle[g], stream[g]));
        }
        CUDNN_CHECK(cudnnCreateFilterDescriptor(&filter_desc));
        CUDNN_CHECK(cudnnCreateTensorDescriptor(&input_desc));
        CUDNN_CHECK(cudnnCreateTensorDescriptor(&output_desc));
        CUDNN_CHECK(cudnnCreateConvolutionDescriptor(&conv_desc));
        if (InputSize() > 2)
            CUDNN_CHECK(cudnnCreateTensorDescriptor(&bias_desc));
    }

    void RunOnDevice() override;
    template <typename T> void RunWithType();

 protected:
    cudnnHandle_t* handle;
    cudaStream_t*  stream;
    cudnnConvolutionBwdFilterAlgo_t bwd_filter_algo;
    cudnnConvolutionBwdDataAlgo_t bwd_data_algo;
    cudnnTensorDescriptor_t input_desc, output_desc, bias_desc;
    cudnnConvolutionDescriptor_t conv_desc;
    cudnnFilterDescriptor_t filter_desc;
    size_t workspace_bwd_filter_size, workspace_bwd_data_size;
    int bias_offset;
};

#endif    // WITH_CUDNN

}    // namespace dragon

#endif    // DRAGON_OPERATORS_VISION_CONV_OP_H_