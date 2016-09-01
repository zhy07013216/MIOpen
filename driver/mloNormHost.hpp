/**********************************************************************
Copyright (c)2016 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/


#ifndef MLO_NORMHOST_H_
#define MLO_NORMHOST_H_

#include <cmath>
#include <iomanip>

////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////

#ifndef MLO_LRN_WITHIN_CHANNEL
#define MLO_LRN_WITHIN_CHANNEL  0
#define MLO_LRN_ACROSS_CHANNELS 1
#endif

template<typename _T>
int mloLRNForwardRunHost(
	bool do_scale,
	int norm_region,
	int pad,
	int local_area,
	_T alphaoverarea,
	_T alpha,
	_T beta,
	_T K,
	int n_batchs,
	int n_outputs,
	int n_inputs,
	int bot_height,
	int bot_width,
	int bot_stride,
	int bot_channel_stride,
	int bot_batch_stride,
	int top_height,
	int top_width,
	int top_v_stride,
	int top_v_channel_stride,
	int	top_v_batch_stride,
	int scale_v_stride,
	int scale_v_channel_stride,
	int scale_v_batch_stride, 
	const _T * bot_ptr,
	_T * scale_v_ptr,
	_T * top_v_ptr
	)
{

	int ret = 0;
	if (norm_region == MLO_LRN_ACROSS_CHANNELS)
	{
		for (int b = 0; b < n_batchs; b++)
		{
			for (int j = 0; j < top_height; j++)
			{
				for (int i = 0; i < top_width; i++)
				{
					// c-emulator
					_T accum_scale = 0;
					int head = 0;
					_T bot_val;
					while (head < pad) {
						bot_val = (head < n_inputs) ? bot_ptr[b*bot_batch_stride + head * bot_channel_stride + j * bot_stride + i] : 0;
						accum_scale += bot_val  * bot_val;
						++head;
					}
					// until we reach size, nothing needs to be subtracted
					while (head < local_area && head - pad >= 0 && head < n_inputs) {
						bot_val = (head < n_inputs) ? bot_ptr[b*bot_batch_stride + head * bot_channel_stride + j * bot_stride + i] : 0;
						accum_scale += bot_val  * bot_val;
						_T scale = K + accum_scale * alphaoverarea;
						if ((head - pad) >= 0 && do_scale)
						{
							scale_v_ptr[b*scale_v_batch_stride + (head - pad) * scale_v_channel_stride + j * scale_v_stride + i] = scale;
						}
						bot_val = ((head - pad) >= 0 ) ? bot_ptr[b*bot_batch_stride + (head - pad) * bot_channel_stride + j * bot_stride + i] : 0;
						_T s = pow(scale, -beta);
						_T c_val = bot_val * s;
						if ((head - pad) >= 0)
						{
							top_v_ptr[b*top_v_batch_stride + (head - pad) * top_v_channel_stride + j * top_v_stride + i] = c_val;
						}
						++head;
					}
					// both add and subtract
					while (head < n_inputs ) {
						bot_val = bot_ptr[b*bot_batch_stride + head * bot_channel_stride + j * bot_stride + i];
						accum_scale += bot_val  * bot_val;
						bot_val = ((head - local_area) >= 0 ) ? bot_ptr[b*bot_batch_stride + (head - local_area) * bot_channel_stride + j * bot_stride + i] : 0;
						accum_scale -= bot_val  * bot_val;
						_T scale = K + accum_scale * alphaoverarea;
						if ((head - pad) >= 0 && do_scale)
						{
							scale_v_ptr[b*scale_v_batch_stride + (head - pad) * scale_v_channel_stride + j * scale_v_stride + i] = scale;
						}
						_T s = pow(scale, -beta);
						bot_val = ((head - pad) >= 0 ) ? bot_ptr[b*bot_batch_stride + (head - pad) * bot_channel_stride + j * bot_stride + i] : 0;
						_T c_val = bot_val * s;
						if ((head - pad) >= 0)
						{
							top_v_ptr[b*top_v_batch_stride + (head - pad) * top_v_channel_stride + j * top_v_stride + i] = c_val;
						}
						++head;
					}
					// subtract only
					while (head < n_inputs + pad) {
						bot_val = ((head - local_area) >= 0 && (head - local_area) < n_inputs) ? bot_ptr[b*bot_batch_stride + (head - local_area) * bot_channel_stride + j * bot_stride + i] : 0;
						accum_scale -= bot_val  * bot_val;
						_T scale = K + accum_scale * alphaoverarea;
						if ((head - pad) >= 0 && (head - pad) < n_outputs && do_scale)
						{
							scale_v_ptr[b*scale_v_batch_stride + (head - pad) * scale_v_channel_stride + j * scale_v_stride + i] = scale;
						}
						bot_val = ((head - pad) >= 0 && (head - pad) < n_inputs) ? bot_ptr[b*bot_batch_stride + (head - pad) * bot_channel_stride + j * bot_stride + i] : 0;
						_T s = pow(scale, -beta);
						_T c_val = bot_val * s;
						if ((head - pad) >= 0 && (head - pad) < n_outputs)
						{
							top_v_ptr[b*top_v_batch_stride + (head - pad) * top_v_channel_stride + j * top_v_stride + i] = c_val;
						}
						++head;
					}

				} // for (int i = 0; i < top_width; i++)
			} // for (int j = 0; j < top_height; j++)
		} // for (int b = 0; b < batch; b++)
	}
	else
	{
		for (int b = 0; b < n_batchs; b++)
		{
			for (int o = 0; o < n_outputs; o++)
			{
				for (int j = 0; j < top_height; j++)
				{
					for (int i = 0; i < top_width; i++)
					{
						// c-emulator
						_T scale = 0;
						int hstart = j - pad;
						int wstart = i - pad;
						int hend = std::min(hstart + local_area, bot_height + pad);
						int wend = std::min(wstart + local_area, bot_width + pad);
						int adj_area_size = (hend - hstart) * (wend - wstart);
						hstart = std::max(hstart, 0);
						wstart = std::max(wstart, 0);
						hend = std::min(hend, bot_height);
						wend = std::min(wend, bot_width);
						_T accum = 0;
						for (int h = hstart; h < hend; ++h)
						{
							for (int w = wstart; w < wend; ++w)
							{

								_T bot_val = bot_ptr[b*bot_batch_stride + o * bot_channel_stride + h * bot_stride + w];
								accum += bot_val * bot_val;

							}
						}

						alphaoverarea = alpha / adj_area_size;
						scale = K + accum* alphaoverarea;
						if (do_scale)
						{
							scale_v_ptr[b*scale_v_batch_stride + o * scale_v_channel_stride + j * scale_v_stride + i] = scale;
						}

						_T s = pow(scale, -beta);
						_T bot_val = bot_ptr[b*bot_batch_stride + o * bot_channel_stride + j * bot_stride + i];
						_T c_val = bot_val * s;


						top_v_ptr[b*top_v_batch_stride + o * top_v_channel_stride + j * top_v_stride + i] = c_val;

					} // for (int i = 0; i < top_width; i++)
				} // for (int j = 0; j < top_height; j++)
			} // for (int o = 0; o < outputs; o++)
		} // for (int b = 0; b < batch; b++)
	} // (norm_region == ACROSS_CHANNELS)


	return(ret);

}

template<typename _T>
int mloLRNBackwardRunHost(
	int norm_region,
	int pad,
	int local_area,
	_T /*alphaoverarea*/,
	_T alpha,
	_T beta,
	_T /*K*/,
	int n_batchs,
	int /*n_outputs*/,
	int n_inputs,
	int bot_height,
	int bot_width,
	int bot_stride,
	int bot_channel_stride,
	int bot_batch_stride,
	int bot_df_v_stride,
	int bot_df_v_channel_stride,
	int bot_df_v_batch_stride,
	int top_height,
	int top_width,
	int top_stride,
	int top_channel_stride,
	int	top_batch_stride,
	int top_df_stride,
	int top_df_channel_stride,
	int	top_df_batch_stride,
	int scale_stride,
	int scale_channel_stride,
	int scale_batch_stride,
	const _T * top_ptr,
	const _T * top_df_ptr,
	const _T * scale_ptr,
	const _T * bot_ptr,
	_T * bot_df_v_ptr
	)
{

	int ret = 0;
	_T negative_beta = -beta;

	if (norm_region == MLO_LRN_ACROSS_CHANNELS)
	{

		_T ratio_dta_bwd = (_T) 2. * alpha * beta / local_area;

		for (int b = 0; b < n_batchs; b++)
		{
			for (int j = 0; j < bot_height; j++)
			{
				for (int i = 0; i < bot_width; i++)
				{

					// c-emulator
					int head = 0;
					_T accum_ratio = 0;

					// accumulate values
					while (head < pad) {
						if (head < n_inputs)
						{
							_T adder = (top_df_ptr[b*top_df_batch_stride + head * top_df_channel_stride + j * top_df_stride + i]
								* top_ptr[b*top_batch_stride + head * top_channel_stride + j * top_stride + i])
								/ scale_ptr[b*scale_batch_stride + head * scale_channel_stride + j * scale_stride + i];

							accum_ratio += adder;
						}

						++head;
					}


					// until we reach size, nothing needs to be subtracted
					while (head < local_area) {

						if (head < n_inputs)
						{
							_T adder = (top_df_ptr[b*top_df_batch_stride + head * top_df_channel_stride + j * top_df_stride + i]
								* top_ptr[b*top_batch_stride + head * top_channel_stride + j * top_stride + i])
								/ scale_ptr[b*scale_batch_stride + head * scale_channel_stride + j * scale_stride + i];


							accum_ratio += adder;
						}

						if (head - pad >= 0 && head - pad < n_inputs)
						{
							bot_df_v_ptr[b*bot_df_v_batch_stride + (head - pad) * bot_df_v_channel_stride + j * bot_df_v_stride + i] =
								top_df_ptr[b*top_df_batch_stride + (head - pad) * top_df_channel_stride + j * top_df_stride + i]
								* pow(scale_ptr[b*scale_batch_stride + (head - pad) * scale_channel_stride + j * scale_stride + i], negative_beta)
								- ratio_dta_bwd * bot_ptr[b*bot_batch_stride + (head - pad) * bot_channel_stride + j * bot_stride + i] * accum_ratio;
						}
						++head;
					}


					// both add and subtract
					while (head < n_inputs) {

						_T adder = top_df_ptr[b*top_df_batch_stride + head * top_df_channel_stride + j * top_df_stride + i]
							* top_ptr[b*top_batch_stride + head * top_channel_stride + j * top_stride + i]
							/ scale_ptr[b*scale_batch_stride + head * scale_channel_stride + j * scale_stride + i];


						accum_ratio += adder;

						if (head - local_area >= 0)
						{
							_T subs = (top_df_ptr[b*top_df_batch_stride + (head - local_area) * top_df_channel_stride + j * top_df_stride + i]
								* top_ptr[b*top_batch_stride + (head - local_area) * top_channel_stride + j * top_stride + i])
								/ scale_ptr[b*scale_batch_stride + (head - local_area) * scale_channel_stride + j * scale_stride + i];



							accum_ratio -= subs;
						}
						if (head - pad >= 0)
						{
							bot_df_v_ptr[b*bot_df_v_batch_stride + (head - pad) * bot_df_v_channel_stride + j * bot_df_v_stride + i] =
								top_df_ptr[b*top_df_batch_stride + (head - pad) * top_df_channel_stride + j * top_df_stride + i]
								* pow(scale_ptr[b*scale_batch_stride + (head - pad) * scale_channel_stride + j * scale_stride + i], negative_beta)
								- ratio_dta_bwd * bot_ptr[b*bot_batch_stride + (head - pad) * bot_channel_stride + j * bot_stride + i] * accum_ratio;
						}

						++head;
					}
					// subtract only
					while (head < n_inputs + pad) {
						if (head - local_area >= 0 && head - local_area < n_inputs)
						{
							_T subs = (top_df_ptr[b*top_df_batch_stride + (head - local_area) * top_df_channel_stride + j * top_df_stride + i]
								* top_ptr[b*top_batch_stride + (head - local_area) * top_channel_stride + j * top_stride + i])
								/ scale_ptr[b*scale_batch_stride + (head - local_area) * scale_channel_stride + j * scale_stride + i];


							accum_ratio -= subs;
						}
						if (head - pad >= 0 && head - pad < n_inputs)
						{
							bot_df_v_ptr[b*bot_df_v_batch_stride + (head - pad) * bot_df_v_channel_stride + j * bot_df_v_stride + i] =
								top_df_ptr[b*top_df_batch_stride + (head - pad) * top_df_channel_stride + j * top_df_stride + i]
								* pow(scale_ptr[b*scale_batch_stride + (head - pad) * scale_channel_stride + j * scale_stride + i], negative_beta)
								- ratio_dta_bwd * bot_ptr[b*bot_batch_stride + (head - pad) * bot_channel_stride + j * bot_stride + i] * accum_ratio;
						}

						++head;
					}




				} // for (int i = 0; i < bot_width; i++)
			} // for (int j = 0; j < bot_height; j++)
		} // for (int b = 0; b < n_batchs; b++)
	} // if (norm_region == MLO_LRN_ACROSS_CHANNELS)
	else
	{
		for (int b = 0; b < n_batchs; b++)
		{
			for (int o = 0; o < n_inputs; o++)
			{
				for (int j = 0; j < bot_height; j++)
				{

					for (int i = 0; i < bot_width; i++)
					{
						_T accum_ratio = 0;

						int hstart = j - pad;
						int wstart = i - pad;
						int hend = std::min(hstart + local_area, top_height + pad);
						int wend = std::min(wstart + local_area, top_width + pad);
						int adj_area_size = (hend - hstart) * (wend - wstart);
						hstart = std::max(hstart, 0);
						wstart = std::max(wstart, 0);
						hend = std::min(hend, top_height);
						wend = std::min(wend, top_width);
						for (int h = hstart; h < hend; ++h)
						{
							for (int w = wstart; w < wend; ++w)
							{
								_T adder = top_df_ptr[b*top_df_batch_stride + o * top_df_channel_stride + h * top_df_stride + w]
									* top_ptr[b*top_batch_stride + o * top_channel_stride + h * top_stride + w]
									/ scale_ptr[b*scale_batch_stride + o * scale_channel_stride + h * scale_stride + w];

								accum_ratio += adder;

							}
						}

						_T ratio_dta_bwd = (_T) 2. * alpha * beta / adj_area_size;

						bot_df_v_ptr[b*bot_df_v_batch_stride + o * bot_df_v_channel_stride + j * bot_df_v_stride + i] =
							top_df_ptr[b*top_df_batch_stride + o * top_df_channel_stride + j * top_df_stride + i]
							* pow(scale_ptr[b*scale_batch_stride + o * scale_channel_stride + j * scale_stride + i], negative_beta)
							- ratio_dta_bwd * bot_ptr[b*bot_batch_stride + o * bot_channel_stride + j * bot_stride + i] * accum_ratio;

					}
				}
			}
		}


	} // if (norm_region == MLO_LRN_ACROSS_CHANNELS)

	return(ret);
}

#endif