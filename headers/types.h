#include "constants.h"
#include "logger.h"
#include "idct.h"

using namespace logger;

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

class HuffmanTable
{

public:
  unsigned char offset[17] = {0};
  char symbols[162] = {0};
  unsigned int codes[162] = {0};
  bool set = false;
  void display()
  {
    for (int i = 0; i < 16; i++)
    {
      cout << (int)offset[i] << " ";
    }
    cout << endl;
    for (int i = 0; i < 162; i++)
    {
      cout << (int)symbols[i] << " ";
    }
    cout << endl;
    for (int i = 0; i < 162; i++)
    {
      cout << codes[i] << " ";
    }
    cout << endl;
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

enum MarkerType
{
  SOI = 0xd8,
  APP = 0xe0,
  DQT = 0xdb,
  SOF = 0xc0,
  DHT = 0xc4,
  SOS = 0xda,
  EOI = 0xd9,
  DRI = 0xdd,
  DRM = 0xd0,
  META = -2,
  PAD = 0x00,
  INVALID = -1,
};

class Marker
{
  int _data_pointer = 0;

public:
  unsigned char *marker;
  int length;
  unsigned char *data;
  int type;
  Marker(FileUtils *file)
  {
    this->marker = file->read(1);
    if (marker[0] == 0x00)
    {
      this->type = MarkerType::PAD;
      return;
    }
    int int_marker = (int) hex_to_int(marker, 1);
    if (int_marker >= 224 && int_marker < 240)
    {
      this->type = MarkerType::META;
      this->length = (file->read()[0] << 8) + file->read()[0];
      this->data = file->read(this->length - 2);
      return;
    }
    if (int_marker >= 0xd0 && int_marker < 0xd8)
    {
      this->type = MarkerType::DRM;
      this->length = int_marker - 0xd0;
      // this->data = file->read(this->length - 2);
      return;
    }
    switch (marker[0])
    {
    case MarkerType::APP:
      this->type = MarkerType::APP;
      break;
    case MarkerType::DQT:
      this->type = MarkerType::DQT;
      break;
    case 0xc0:
      this->type = MarkerType::SOF;
      break;
    case 0xc2:
      this->type = MarkerType::SOF;
      break;
    case 0xc4:
      this->type = MarkerType::DHT;
      break;
    case 0xda:
      this->type = MarkerType::SOS;
      break;
    case 0xd9:
      this->type = MarkerType::EOI;
      break;
    case 0xd8:
      this->type = MarkerType::SOI;
      break;
    case 0xdd:
      this->type = MarkerType::DRI;
      break;
    default:
      this->type = MarkerType::INVALID;
    }
    this->length = (file->file->get() << 8) + file->file->get();
    this->data = file->read(this->length - 2);
  }

  unsigned char *read(int n = 1)
  {
    if (_data_pointer + n > length)
    {
      show(LogType::ERROR) << "MARKER READER : Reading of unavailable data" >> cout;
      return nullptr;
    }
    unsigned char *res = new unsigned char[n];
    for (int i = 0; i < n; i++)
    {
      res[i] = this->data[_data_pointer + i];
    }
    _data_pointer += n;
    return res;
  }

  ~Marker()
  {
    // delete[] this->marker;
    delete[] this->data;
  }
};