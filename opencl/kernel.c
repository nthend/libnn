#ifndef __OPENCL_VERSION__
#include "opencl.h"
#endif // __OPENCL_VERSION__

kernel void fill(const uint size, global float *buffer, const float number)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		buffer[pos] = number;
	}
}

kernel void copy(const uint size, global const float *src, global float *dst)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		dst[pos] = src[pos];
	}
}

kernel void add(const uint size, global const float *src, global float *dst)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		dst[pos] += src[pos];
	}
}

kernel void transmit(
    const uint in_size, const uint out_size, 
    global const float *input, global float *output,
    global const float *weight, global const float *bias
    )
{
	const uint pos = get_global_id(0);
	if(pos < out_size)
	{
		float sum = 0.0;
		for(int i = 0; i < in_size; ++i)
		{
			sum += input[i]*weight[in_size*pos + i];
		}
		output[pos] += sum + bias[pos];
	}
}

kernel void transmit_reduce_init(
    const uint in_size, const uint2 out_size,
    global const float *input, global float *output,
    global const float *weight
    )
{
	const uint2 pos = (uint2) (get_global_id(0), get_global_id(1));
	const line = (in_size - 1)/out_size.x + 1;
	if(pos.y < out_size.y && pos.x < out_size.x)
	{
		float sum = 0.0;
		for(int i = pos.x*line; i < min((pos.x + 1)*line, in_size); ++i)
		{
			sum += input[i]*weight[in_size*pos.y + i];
		}
		output[out_size.x*pos.y + pos.x] = sum;
	}
}

kernel void transmit_reduce(
    const uint in_size, const uint2 out_size,
    global const float *input, global float *output
    )
{
	const uint2 pos = (uint2) (get_global_id(0), get_global_id(1));
	const uint line = (in_size - 1)/out_size.x + 1;
	if(pos.y < out_size.y && pos.x < out_size.x)
	{
		float sum = 0.0;
		for(int i = pos.x*line; i < min((pos.x + 1)*line, in_size); ++i)
		{
			sum += input[in_size*pos.y + i];
		}
		output[out_size.x*pos.y + pos.x] = sum;
	}
}

kernel void transmit_reduce_finalize(
    const uint in_size, const uint out_size, 
    global const float *input, global float *output,
    global const float *bias
    )
{
	const uint pos = get_global_id(0);
	if(pos < out_size)
	{
		float sum = 0.0;
		for(int i = 0; i < in_size; ++i)
		{
			sum += input[in_size*pos + i];
		}
		output[pos] += sum + bias[pos];
	}
}

kernel void update(const uint size, global const float *input, global float *output)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		output[pos] = input[pos];
	}
}

float sigma(float a)
{
	return 1.0/(1.0 + exp(-a));
}

float sigma_deriv(float a)
{
	float e = exp(a);
	float d = (1.0 + e);
	return e/(d*d);
}

kernel void update_sigmoid(const uint size, global const float *input, global float *output)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		output[pos] = sigma(input[pos]);
	}
}

kernel void setError(const uint size, global const float *result, global const float *output, global float *error)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		error[pos] = result[pos] - output[pos];
	}
}

kernel void setErrorReuse(const uint size, global const float *output, global float *error)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		error[pos] -= output[pos];
	}
}

kernel void updateError(const uint size, global float *input_error, global const float *output_error)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		input_error[pos] = output_error[pos];
	}
}

kernel void updateError_sigmoid(const uint size, global float *input_error, global const float *output_error, global const float *input)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		input_error[pos] = output_error[pos]*sigma_deriv(input[pos]);
	}
}

kernel void updateError_sigmoid_crossEntropy(const uint size, global float *input_error, global const float *output_error, global const float *input, global const float *output)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		input_error[pos] = output_error[pos]*output[pos]*(1.0 - output[pos]);
	}
}

kernel void backpropWeightGrad(
    const uint2 size2, global const float *src_input_error, global const float *dst_output,
    global float *weight_grad
    )
{
	const uint pos = get_global_id(0);
	const uint size = size2.x*size2.y;
	const uint2 pos2 = (uint2) (pos % size2.x, pos / size2.x);
	if(pos < size)
	{
		weight_grad[pos] += dst_output[pos2.x]*src_input_error[pos2.y];
	}
}

kernel void backpropBiasGrad(
    const uint size, global const float *src_input_error,
    global float *bias_grad
    )
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		bias_grad[pos] += src_input_error[pos];
	}
}

kernel void backpropError(
    const uint in_size, const uint out_size, 
    global const float *src_error, global float *dst_error,
    global const float *weight
    )
{
	const uint pos = get_global_id(0);
	if(pos < in_size)
	{
		float sum = 0.0;
		for(int i = 0; i < out_size; ++i)
		{
			// TODO: fence to optimize caching
			sum += weight[i*in_size + pos]*src_error[i];
		}
		dst_error[pos] += sum;
	}
}

kernel void commitWeightGrad(const uint2 size, const float delta, global const float *weight_grad, global float *weight)
{
	const uint2 pos = (uint2) (get_global_id(0), get_global_id(1));
	if(pos.x < size.x && pos.y < size.y)
	{
		weight[pos.y*size.x + pos.x] += delta*weight_grad[pos.y*size.x + pos.x];
	}
}

kernel void commitBiasGrad(const uint size, const float delta, global const float *bias_grad, global float *bias)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		bias[pos] += delta*bias_grad[pos];
	}
}
