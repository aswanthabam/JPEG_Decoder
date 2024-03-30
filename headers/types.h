// IDCT constants

const float m0 = 2.0 * std::cos(1.0 / 16.0 * 2.0 * M_PI);
const float m1 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * M_PI);
const float m3 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * M_PI);
const float m5 = 2.0 * std::cos(3.0 / 16.0 * 2.0 * M_PI);
const float m2 = m0 - m5;
const float m4 = m0 + m5;

const float s0 = std::cos(0.0 / 16.0 * M_PI) / std::sqrt(8);
const float s1 = std::cos(1.0 / 16.0 * M_PI) / 2.0;
const float s2 = std::cos(2.0 / 16.0 * M_PI) / 2.0;
const float s3 = std::cos(3.0 / 16.0 * M_PI) / 2.0;
const float s4 = std::cos(4.0 / 16.0 * M_PI) / 2.0;
const float s5 = std::cos(5.0 / 16.0 * M_PI) / 2.0;
const float s6 = std::cos(6.0 / 16.0 * M_PI) / 2.0;
const float s7 = std::cos(7.0 / 16.0 * M_PI) / 2.0;

// Zig zag map

const int zigZagMap[] = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63};
struct MCU
{
  union
  {
    int y[64] = {0};
    int r[64];
  };

  union
  {
    int cb[64] = {0};
    int g[64];
  };

  union
  {
    int cr[64] = {0};
    int b[64];
  };

  int *operator[](int index)
  {
    switch (index)
    {
    case 0:
      return y;
    case 1:
      return cb;
    case 2:
      return cr;

    default:
      return nullptr;
    }
  }
};

struct ColorComponent
{
  int horizontal_sampling_factor = 0;
  int vertical_sampling_factor = 0;
  int quantizationTableID = 0;
  int huffmanDCTTableID = 0;
  int huffmanACTableID = 0;
  bool set = false;
};

class QuantizationTable
{
  int table[64];
  int header;

public:
  bool set = false;
  QuantizationTable(int header, unsigned char *raw)
  {
    for (int i = 0; i < 64; i++){
        this->table[zigZagMap[i]] = (int)raw[i];
    }
    this->header = header;
  }
  int operator[](int index)
  {
    return table[index];
  }
  void display()
  {
    for (int i = 0; i < 8; i++)
    {
      cout << "\t";
      for (int j = 0; j < 8; j++)
      {
        cout << table[i * 8 + j] << " ";
      }
      cout << endl;
    }
  }
};
