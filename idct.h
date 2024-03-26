#include <iostream>
#include <cmath>
#define M_PI 3.14159265358979323846

using namespace std;

class IDCT
{
    int zig_zag[8][8] = {
        {0, 1, 5, 6, 14, 15, 27, 28},
        {2, 4, 7, 13, 16, 26, 29, 42},
        {3, 8, 12, 17, 25, 30, 41, 43},
        {9, 11, 18, 24, 31, 40, 44, 53},
        {10, 19, 23, 32, 39, 45, 52, 54},
        {20, 22, 33, 38, 46, 51, 55, 60},
        {21, 34, 37, 47, 50, 56, 59, 61},
        {35, 36, 48, 49, 57, 58, 62, 63}};
    int idct_precision = 8;
    int idct_table[8][8];

public:
    int base[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0};
    int *base2[8][8] = {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0}};

    IDCT()
    {
        for (int i = 0; i < this->idct_precision; i++)
        {
            for (int j = 0; j < this->idct_precision; j++)
            {
                this->idct_table[i][j] = (i == 0 ? (1.0 / sqrt(2.0)) : 1.0) * cos((2.0 * j + 1.0) * i * M_PI / 16);
            }
        }
    }

    void rearrange_zig_zag()
    {
        for (int i = 0; i < this->idct_precision; i++)
        {
            for (int j = 0; j < this->idct_precision; j++)
            {
                this->zig_zag[i][j] = this->base[this->zig_zag[i][j]];
            }
        }
    }

    void perform()
    {
        int out[8][8];
        for (int i = 0; i < this->idct_precision; i++)
        {
            for (int j = 0; j < this->idct_precision; j++)
            {
                int local_sum = 0;
                for (int x = 0; x < this->idct_precision; x++)
                {
                    for (int y = 0; y < this->idct_precision; y++)
                    {
                        local_sum += this->zig_zag[y][x] * this->idct_table[x][i] * this->idct_table[y][j];
                    }
                }
                out[j][i] = local_sum / 4;
            }
        }
        for (int i = 0; i < this->idct_precision; ++i) {
            for (int j = 0; j < this->idct_precision; ++j) {
                this->base[i * idct_precision + j] = out[i][j];
            }
        }
    }
};