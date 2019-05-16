#include "fpga_api.h"
#include <stdio.h>
#include <iostream>
#include <cstring>

using namespace std;

#define min(x, y) (((x) < (y)) ? (x) : (y))

FPGA::FPGA(off_t data_addr, off_t output_addr, int m_size, int v_size)
{
  m_size_ = m_size;
  v_size_ = v_size;
  data_size_ = (m_size_ + 1) * v_size_; // fpga bram data size

  output_ = new unsigned int[m_size_]; // use output_ as tempolar output
  data_ = new float[data_size_];

  num_block_call_ = 0;
}
FPGA::~FPGA()
{
  delete[] output_;
  delete[] data_;
}

float *FPGA::matrix(void)
{
  return data_ + v_size_;
}

float *FPGA::vector(void)
{
  return data_;
}

void FPGA::reset(void)
{
  num_block_call_ = 0;
}

int FPGA::num_block_call(void)
{
  return num_block_call_;
}

const float *FPGA::blockMV()
{
  num_block_call_ += 1;

  // cpu version
  float *vec = this->vector();
  float *mat = this->matrix();
  float *out = reinterpret_cast<float *>(output_);

  for (int i = 0; i < m_size_; ++i)
  {
    out[i] = 0;
    for (int j = 0; j < v_size_; ++j)
      out[i] += vec[j] * mat[v_size_ * i + j];
  }

  for (int i = 0; i < m_size_; ++i)
    data_[i] = out[i];

  return data_;
}

void FPGA::largeMV(const float* large_mat, const float* input, float* output, int num_input, int num_output)
{
    float* vec = this->vector();
    float* mat = this->matrix();

    // 0) Initialize output vector
    for(int i = 0; i < num_output; ++i)
    {
        output[i] = 0;
    }

    for(int i = 0; i < num_output; i += m_size_)
    {

        for(int j = 0; j < num_input; j += v_size_)
        {
            // 0) Initialize input vector
            int block_row = min(m_size_, num_output-i);
            int block_col = min(v_size_, num_input-j);

            // !) Assign a vector
            /* IMPLEMENT */

			for(int k = 0; k < block_col; k++){
				vec[k] = input[j + k];
			}
			for(int unk = block_col; unk < v_size_; unk++){
				vec[unk] = 0;
			}

            // 2) Assign a matrix
            /* IMPLEMENT */

			int block_start = i * num_input + j;
			for(int row = 0; row < block_row; row++){
				int block_cursor = block_start + row * num_input;
				for(int col = 0; col < block_col; col++){
					mat[row * v_size_ + col] = large_mat[block_cursor + col];
				}
				for(int uncol = block_col; uncol < v_size_; uncol++){
					mat[row * v_size_ + uncol] = 0;
				}

			}

			for(int unrow = block_row; unrow < m_size_; unrow++)
			{
				std::fill_n(mat + unrow * v_size_, v_size_, 0);
			}


            // 3) Call a function `block_call() to execute MV multiplication
            const float* ret = this->blockMV();

            // 4) Accumulate intermediate results
            for(int row = 0; row < block_row; ++row)
            {
                output[i + row] += ret[row];
            }
        }
    }
}

void FPGA::convLowering(const std::vector<std::vector<std::vector<std::vector<float>>>> &cnn_weights,
                        std::vector<std::vector<float>> &new_weights,
                        const std::vector<std::vector<std::vector<float>>> &inputs,
                        std::vector<std::vector<float>> &new_inputs)
{
  /*
   * Arguments:
   *
   * conv_weights: [conv_channel, input_channel, conv_height, conv_width]
   * new_weights: [?, ?]
   * inputs: [input_channel, input_height, input_width]
   * new_inputs: [?, ?]
   *
   */

  int conv_channel = cnn_weights.size();
  int input_channel = cnn_weights[0].size();
  int conv_height = cnn_weights[0][0].size();
  int conv_width = cnn_weights[0][0][0].size();
  //int input_channel = inputs.size();
  int input_height = inputs[0].size();
  int input_width = inputs[0][0].size();

  // IMPLEMENT THIS
  // For example,
  // new_weights[0][0] = cnn_weights[0][0][0][0];
  // new_inputs[0][0] = inputs[0][0][0];


  for(int c = 0; c < conv_channel; c++) {
    for(int rgb = 0; rgb < input_channel; rgb++){
      for(int h = 0; h < conv_height; h++) {
        for(int w = 0; w < conv_width; w++) {
          new_weights[c][rgb * conv_height * conv_width + h * conv_width + w] = cnn_weights[c][rgb][h][w];
        }
      }
    }
  }

  for (int rgb = 0; rgb < input_channel; rgb++) {
    for(int a = 0; a < input_height - conv_height + 1; a++){
      for(int b = 0; b < input_width - conv_width + 1; b++) {
        for(int h = a; h < conv_height + a; h++) {
          for(int w = b; w < conv_width + b; w++) {

            new_inputs[a * (input_width - conv_width + 1) + b][rgb * conv_height * conv_width + (h - a) * conv_width + (w - b)] = 0.0;
          }
        }
      }
    }
  }


}
