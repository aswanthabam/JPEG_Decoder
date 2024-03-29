#include "constants.h"
// #include "logger.h"
#include "idct.h"

// using namespace logger;

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

struct IDCTAndCoeff
{
  IDCT *idct;
  int dc_coeff;
};

class QuantizationTable
{
  int table[8][8];
  int header;

public:
  bool set = false;
  QuantizationTable(int header, unsigned char *raw)
  {
    for (int i = 0; i < 8; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        this->table[i][j] = (int)raw[zigZagMap[i * 8 + j]];
      }
    }
    this->header = header;
  }
  int *operator[](int index)
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
        cout << table[i][j] << " ";
      }
      cout << endl;
    }
  }
};
