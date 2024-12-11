/*
 * torch_api.h
 *
 *  Created on: Nov 14, 2024
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_DATASET_TORCH_API_H_
#define ALPHAGOMOKU_DATASET_TORCH_API_H_

namespace ag
{
#ifdef __cplusplus
	extern "C"
	{
#endif
		typedef struct
		{
				int buffer_index;
				int game_index;
				int sample_index;
				int augmentation;
		} Sample_t;

		typedef struct
		{
				int rank;
				int dim[4];
		} TensorSize_t;

		void load_dataset_fragment(int i, const char *path);
		void unload_dataset_fragment(int i);
		void print_dataset_info();
		void get_dataset_size(TensorSize_t *shape, int *size);

		void get_tensor_shapes(int batch_size, const Sample_t *samples, TensorSize_t *input, TensorSize_t *policy_target, TensorSize_t *value_target,
				TensorSize_t *moves_left_target, TensorSize_t *action_values_target);
		void load_batch(int batch_size, const Sample_t *samples, float *input, float *policy_target, float *value_target, float *moves_left_target,
				float *action_values_target);

#ifdef __cplusplus
	}
#endif
} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_TORCH_API_H_ */
