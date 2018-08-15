/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <vector>
#include "paddle/fluid/framework/op_registry.h"

namespace paddle {
namespace operators {

using Tensor = framework::Tensor;

template <typename DeviceContext, typename T>
class SamplingIdKernel : public framework::OpKernel<T> {
 public:
  void Compute(const framework::ExecutionContext& context) const override {
    const Tensor* input = context.Input<Tensor>("X");
    const int batch_size = static_cast<int>(input->dims()[0]);
    const int width = static_cast<int>(input->dims()[1]);

    std::vector<T> ins_vector;
    framework::TensorToVector(*input, context.device_context(), &ins_vector);

    std::vector<T> ids(batch_size);
    for (size_t i = 0; i < batch_size; ++i) {
      double r = getRandReal();
      int idx = width - 1;
      for (int j = 0; j < width; ++j) {
        if ((r -= ins_vector[i * width + j]) < 0) {
          idx = j;
          break;
        }
      }
      ids[i] = ins_vector[i * width + idx];
    }

    std::vector<int64_t> out_dim;
    out_dim.push_back(static_cast<int64_t>(batch_size));

    Tensor* output = context.Output<Tensor>("Out");
    output->Resize(framework::make_ddim(out_dim));
    output->mutable_data<T>(context.GetPlace());
    framework::TensorFromVector(ids, context.device_context(), output);
  }

 private:
  double getRandReal() const {
    std::random_device
        rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with
                             // rd()
    std::uniform_real_distribution<> dis(1.0, 2.0);
    return dis(gen);
  }
};

class SamplingIdOp : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

  void InferShape(framework::InferShapeContext* ctx) const override {
    PADDLE_ENFORCE(ctx->HasInput("X"),
                   "Input(X) of SamplingIdOp should not be null.");
    PADDLE_ENFORCE(ctx->HasOutput("Out"),
                   "Output(Out) of SamplingIdOp should not be null.");

    auto input_dims = ctx->GetInputDim("X");
    PADDLE_ENFORCE(input_dims.size() == 2,
                   "Input(X, Filter) should be 2-D tensor.");

    framework::DDim dims = input_dims;
    ctx->SetOutputDim("Out", dims);
    ctx->ShareLoD("X", "Out");
  }
};

class SamplingIdOpMaker : public framework::OpProtoAndCheckerMaker {
 public:
  void Make() override {
    AddInput("X",
             "The input tensor of softmax. "
             "2-D with shape [batch_size, input_feature_dimensions].");
    AddOutput("Out", "SamplingId data tensor.");
    AddComment(R"DOC(
SamplingId Operator.
A layer for sampling id from multinomial distribution from the
 input layer. Sampling one id for one sample.)DOC");
  }
};
}  // namespace operators
}  // namespace paddle

namespace ops = paddle::operators;
REGISTER_OPERATOR(sampling_id, ops::SamplingIdOp, ops::SamplingIdOpMaker,
                  paddle::framework::EmptyGradOpMaker);

REGISTER_OP_CPU_KERNEL(
    sampling_id, ops::SamplingIdKernel<paddle::platform::CPUDeviceContext, int>,
    ops::SamplingIdKernel<paddle::platform::CPUDeviceContext, int64_t>,
    ops::SamplingIdKernel<paddle::platform::CPUDeviceContext, float>,
    ops::SamplingIdKernel<paddle::platform::CPUDeviceContext, double>);
