#ifndef __OPENCL_VERSION__
#include "opencl.h"
#endif // __OPENCL_VERSION__

kernel void transmit(
    const uint in_size, const uint out_size, 
    global const float *input, global float *output,
    global const float *weight, global const float *bias
    )
{
	const uint size = out_size;
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		int i;
		float sum = 0.0;
		for(i = 0; i < in_size; ++i)
		{
			sum += input[i]*weight[in_size*pos + i];
		}
		output[pos] += sum + bias[pos];
	}
}

kernel void fill(const uint size, global float *buffer, const float number)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		buffer[pos] = number;
	}
}

kernel void update_uniform(const uint size, global const float *input, global float *output)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		output[pos] = input[pos];
	}
}

float sigma(float a)
{
	return 1.0f/(1.0f + exp(-a));
}

kernel void update_sigmoid(const uint size, global const float *input, global float *output)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		output[pos] = sigma(input[pos]);
	}
}

kernel void setError_quartic(const uint size, global const float *result, global const float *output, global float *error)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		error[pos] = result[pos] - output[pos];
	}
}

kernel void setErrorC_quartic(const uint size, global const float *output, global float *error)
{
	const uint pos = get_global_id(0);
	if(pos < size)
	{
		error[pos] -= output[pos];
	}
}
