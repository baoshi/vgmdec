#include <stdio.h>
#include <math.h>

int16_t fftdata[1024] = 
{
160, 104, 203, 470, 359, 34, 171, 238, 89, 110, 154, 87, 86, 206, 106, 151, 
230, 194, 35, 82, 123, 63, 51, 94, 38, 27, 77, 39, 29, 75, 40, 50, 
98, 75, 16, 14, 17, 19, 12, 30, 25, 3, 31, 15, 41, 16, 23, 23, 
34, 43, 10, 0, 8, 5, 7, 13, 14, 11, 24, 30, 5, 3, 13, 6, 
10, 17, 16, 2, 8, 8, 1, 10, 10, 20, 14, 5, 8, 2, 26, 19, 
18, 25, 29, 2, 6, 22, 22, 10, 21, 12, 2, 4, 8, 3, 6, 3, 
18, 25, 12, 2, 5, 9, 19, 9, 2, 5, 2, 6, 2, 6, 9, 1, 
5, 15, 12, 8, 3, 5, 14, 5, 8, 9, 4, 5, 1, 8, 6, 3, 
9, 7, 5, 13, 18, 4, 4, 4, 2, 4, 5, 1, 6, 2, 3, 3, 
7, 3, 24, 5, 2, 5, 1, 11, 1, 3, 11, 3, 5, 3, 5, 8, 
1, 12, 20, 4, 6, 0, 3, 5, 4, 4, 7, 3, 2, 5, 3, 5, 
6, 7, 14, 1, 5, 3, 3, 2, 4, 3, 6, 2, 4, 5, 10, 1, 
2, 4, 4, 8, 2, 5, 8, 1, 5, 1, 3, 9, 5, 10, 4, 4, 
7, 2, 6, 7, 8, 10, 1, 2, 5, 5, 3, 8, 2, 5, 7, 3, 
3, 7, 11, 11, 2, 6, 2, 2, 7, 4, 5, 7, 2, 6, 3, 4, 
7, 2, 2, 10, 6, 2, 4, 7, 2, 4, 6, 4, 2, 3, 7, 2, 
1, 6, 2, 4, 6, 5, 6, 6, 4, 2, 4, 1, 1, 6, 1, 5, 
2, 3, 4, 7, 3, 3, 0, 3, 6, 4, 3, 1, 2, 3, 3, 2, 
2, 3, 3, 7, 5, 1, 3, 5, 1, 2, 2, 3, 1, 1, 1, 3, 
7, 2, 3, 5, 5, 4, 2, 1, 4, 2, 3, 2, 2, 2, 5, 2, 
9, 3, 3, 5, 3, 2, 1, 1, 6, 2, 4, 1, 6, 5, 4, 1, 
1, 5, 5, 3, 6, 3, 5, 0, 1, 4, 4, 3, 2, 2, 1, 1, 
3, 2, 1, 1, 8, 5, 1, 1, 5, 3, 2, 2, 4, 4, 2, 5, 
1, 4, 1, 3, 3, 3, 1, 4, 2, 3, 4, 1, 2, 0, 4, 3, 
1, 4, 3, 2, 2, 3, 2, 4, 3, 0, 3, 4, 5, 4, 2, 2, 
3, 3, 3, 1, 3, 5, 2, 3, 1, 2, 3, 1, 5, 2, 1, 0, 
4, 3, 1, 1, 4, 3, 6, 2, 3, 3, 1, 1, 5, 3, 2, 7, 
1, 3, 5, 3, 0, 1, 3, 4, 2, 4, 2, 4, 3, 6, 2, 1, 
3, 3, 2, 2, 5, 1, 9, 3, 2, 2, 1, 3, 1, 2, 5, 2, 
2, 2, 5, 2, 0, 3, 1, 2, 7, 4, 1, 3, 3, 2, 2, 3, 
1, 2, 3, 3, 3, 3, 3, 5, 3, 3, 4, 1, 4, 3, 1, 3, 
5, 1, 4, 2, 2, 4, 2, 2, 1, 5, 3, 2, 1, 5, 1, 4, 
3, 2, 3, 3, 2, 1, 1, 1, 2, 2, 2, 1, 1, 2, 2, 2, 
2, 2, 1, 1, 2, 1, 3, 1, 3, 1, 2, 1, 2, 1, 1, 2, 
1, 1, 2, 1, 1, 2, 4, 2, 1, 1, 1, 1, 2, 2, 0, 1, 
2, 1, 1, 1, 2, 1, 1, 2, 1, 3, 0, 3, 1, 1, 1, 0, 
0, 1, 1, 2, 1, 2, 3, 1, 1, 1, 1, 0, 2, 0, 2, 1, 
1, 2, 1, 1, 0, 2, 1, 1, 3, 1, 1, 1, 3, 3, 1, 1, 
1, 2, 2, 0, 1, 3, 1, 2, 3, 1, 2, 2, 1, 1, 2, 1, 
1, 2, 2, 1, 1, 3, 1, 1, 2, 1, 2, 1, 1, 1, 2, 1, 
1, 1, 0, 1, 2, 2, 1, 1, 1, 2, 3, 1, 1, 1, 3, 1, 
1, 1, 1, 2, 2, 2, 1, 3, 3, 1, 2, 2, 2, 1, 2, 1, 
1, 2, 1, 1, 0, 0, 2, 2, 0, 3, 2, 1, 2, 2, 1, 2, 
2, 2, 2, 3, 2, 1, 2, 1, 1, 2, 1, 2, 1, 0, 2, 2, 
3, 1, 1, 1, 2, 2, 2, 1, 2, 1, 1, 2, 0, 2, 1, 1, 
2, 3, 3, 1, 1, 1, 3, 1, 4, 3, 0, 2, 1, 2, 1, 2, 
0, 2, 2, 1, 0, 1, 1, 2, 2, 3, 1, 1, 2, 2, 2, 0, 
1, 1, 1, 3, 1, 2, 2, 1, 1, 3, 2, 1, 2, 2, 2, 1, 
2, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 2, 1, 1, 
1, 3, 1, 2, 1, 1, 2, 2, 2, 0, 1, 1, 2, 1, 1, 2, 
1, 2, 0, 1, 2, 1, 1, 1, 1, 1, 3, 1, 4, 1, 3, 0, 
1, 0, 1, 2, 1, 3, 1, 1, 1, 1, 0, 1, 4, 1, 2, 1, 
2, 2, 1, 1, 1, 1, 1, 0, 2, 2, 1, 1, 0, 1, 2, 1, 
1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 3, 
0, 0, 1, 2, 1, 2, 1, 1, 2, 1, 2, 1, 1, 2, 0, 1, 
3, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 
1, 1, 2, 1, 1, 1, 2, 2, 1, 2, 2, 2, 2, 1, 2, 2, 
1, 0, 1, 2, 1, 1, 1, 0, 1, 3, 2, 1, 2, 1, 1, 1, 
1, 2, 1, 0, 1, 2, 1, 1, 2, 1, 2, 2, 1, 1, 1, 3, 
1, 1, 2, 2, 1, 0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 
2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 2, 2, 
2, 1, 2, 1, 2, 2, 2, 1, 1, 2, 1, 1, 2, 1, 2, 1, 
2, 2, 0, 2, 1, 2, 0, 1, 1, 1, 0, 1, 1, 1, 2, 1, 
2, 1, 1, 2, 1, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 1
};

#define SAMPLE_RATE 44100
#define SAMPLE_POINTS 2048

#define FIRST_BIN_FREQ 20.0
#define LAST_BIN_FREQ 22050.0
#define NUM_BINS 32

static struct freq_bin_param_s
{
    int base_bin;
    int16_t scale;
} bin_param_q15[NUM_BINS] = 
{
    {2, 1154},
    {2, 9455},
    {2, 19789},
    {2, 32651},
    {3, 15893},
    {4, 3053},
    {4, 27858},
    {5, 25965},
    {6, 31629},
    {8, 13930},
    {10, 7937},
    {12, 16517},
    {15, 10467},
    {18, 26994},
    {23, 6087},
    {28, 20160},
    {35, 12238},
    {43, 25743},
    {54, 8403},
    {67, 9495},
    {83, 16800},
    {103, 23129},
    {128, 27551},
    {160, 4160},
    {199, 2285},
    {247, 17789},
    {307, 28794},
    {382, 32127},
    {476, 15126},
    {592, 26877},
    {737, 21462},
    {917, 30632}
};


int main()
{
    double c_freq_data_points[SAMPLE_POINTS / 2];
    double c_freq_bin_center[NUM_BINS];

    int c_bin_in_data[NUM_BINS];
    int16_t bin_data[NUM_BINS] = {0.0};

    double c_data_width = (double)SAMPLE_RATE / (SAMPLE_POINTS - 2);
    for (int i = 0; i < SAMPLE_POINTS / 2; ++i)
    {
        c_freq_data_points[i] = (float)i * c_data_width; // this maps c_freq_data_points[0] to 0Hz and c_freq_data_points[1023] to SAMPLE_RATE/2
    }
    double bin_ratio;
    bin_ratio = pow((LAST_BIN_FREQ / FIRST_BIN_FREQ), 1.0 / (NUM_BINS + NUM_BINS));

    c_freq_bin_center[0] = FIRST_BIN_FREQ * bin_ratio;
    for (int i = 1; i < NUM_BINS; ++i)
    {
        c_freq_bin_center[i] = c_freq_bin_center[i - 1] * bin_ratio * bin_ratio;
    }
    /*
    for (int bin = 0; bin < NUM_BINS; ++bin)
    {
        printf("Bin center freq = %.1f\n", c_freq_bin_center[bin]);
    }
    */

    // bind where the bin point falls into
    printf("{\n", NUM_BINS);
    int data_index = 0;
    for (int bin = 0; bin < NUM_BINS; ++bin)
    {
        while (c_freq_bin_center[bin] > c_freq_data_points[data_index])
            ++data_index;
        double c_x0 = c_freq_data_points[data_index - 1];
        double c_x1 = c_freq_data_points[data_index];
        // bin_data[bin] = fftdata[data_index - 1] + (c_freq_bin_center[bin] - c_x0) * (fftdata[data_index] - fftdata[data_index - 1]) / (c_x1 - c_x0);
        //printf("bin%d = fftdata[%d] + %.5f * fftdata[%d]-fftdata[%d]\n", bin, data_index-1, (c_freq_bin_center[bin] - c_x0) / (c_x1 - c_x0), data_index, data_index - 1);
        printf("{%d, %d},\n", data_index, (int)(32768 * (c_freq_bin_center[bin] - c_x0) / (c_x1 - c_x0)));  // convert to q15
    }
    printf("}\n");

    for (int bin = 0; bin < NUM_BINS; ++bin)
    {
        int32_t temp = bin_param_q15[bin].scale * (fftdata[bin_param_q15[bin].base_bin] - fftdata[bin_param_q15[bin].base_bin - 1]);
        bin_data[bin] = fftdata[bin_param_q15[bin].base_bin - 1] + (temp >> 15);
        printf("%d\n", bin_data[bin]);
    }

    return 0;
}

