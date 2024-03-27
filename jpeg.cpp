#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <math.h>
#include <map>
#include "huffman.h"
#include "idct.h"

using namespace std;

class QuantizationTable
{
  int table[8][8];
  int header;

public:
  QuantizationTable(int header, char *raw)
  {
    for (int i = 0; i < 8; i++)
    {
      for (int j = 0; j < 8; j++)
        this->table[i][j] = (int)raw[i * 8 + j];
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

class JPEG
{
  FileUtils *file;
  QuantizationTable *quantization_table[2];
  short int quantization_table_index = 0;
  int image_width;
  int image_height;
  int quantMapping[4];
  map<int, HuffmanTable *> huffman_table;
  struct IDCTAndCoeff {
    IDCT *idct;
    int dc_coeff;
  };
  string byteToBits(const char bytes)
  {
    string result;
    for (int j = 7; j >= 0; --j)
    {
      bool bit = (bytes >> j) & 1;
      result += (bit ? '1' : '0');
    }

    return result;
  }

  int hex_to_int(char v)
  {
    // Single character only
    return (0xff & v);
  }
  int hex_to_int(char *v, int n)
  {
    char *tu = new char[20]();
    int k = 0;
    char *u = new char[2]();
    for (int i = 0; i < n; i++)
    {
      delete[] u;
      u = new char[2];
      sprintf(u, "%X", hex_to_int(v[i]));
      if (strlen(u) == 1)
      {
        u[1] = u[0];
        u[0] = '0';
      }
      tu[k + 0] = u[0];
      tu[k + 1] = u[1];
      k += 2;
    }
    unsigned int val = stoi(tu, 0, 16);
    return val;
  }
  unsigned int toInt(char *buffer)
  {
    cout << "Buffer: " << (int)buffer[0] << " " << (int)buffer[1] << endl;
    int num = (buffer[0] << 8) + buffer[1];
    if (num < 0)
      return (unsigned int)-num + 1;
    else
      return (unsigned int)num;
  }

  int decodeNumber(int code, int bits)
  {
    int l = pow(2, code);
    if (bits >= l)
      return bits;
    else
      return bits - (2 * l - 1);
  }
  IDCTAndCoeff buildMatrix(BitStream *st, int idx, QuantizationTable *quant, int olddccoeff)
  {
    IDCT *i = new IDCT();
    int code = hex_to_int(this->huffman_table[0 + idx]->getCode(st), 1);
    int bits = st->getBitN(code);
    int dccoeff = decodeNumber(code, bits) + olddccoeff;
    i->base[0] = dccoeff * (*quant[0][0]);
    // cout << i->base[0];
    int l = 1;
    while (l < 64)
    {
      code = hex_to_int(this->huffman_table[16 + idx]->getCode(st), 1);
      if (code == 0)
      {
        break;
      }
      if (code > 15) {
        l += code >> 4;
        code = code & 0xf0;
      }
      cout << "CODE : " << code << " ";
      bits = st->getBitN(code);
      cout << " BITS : " << bits << " ";
      if (l < 64) {
        int coeff = decodeNumber(code, bits);
        cout << " COEFF : " << coeff * *quant[l / 8][l % 8] << " L : " << l;
        i->base[l] = (coeff * *quant[l / 8][l % 8]);
        cout << " " << i->base[l] << " ";
        l++;
      }
      cout << endl;
    }
    i->rearrange_zig_zag();
    for (int k = 0;k < 64; k++) {
        cout << i->base[k] << " ";
    }
    cout << endl;
    displayMatrix(i->zig_zag, 8, 8);
    return {i, dccoeff};
    i->perform();
    cout << endl;
    return {i, dccoeff};
  }

public:
  JPEG(string filename)
  {
    file = new FileUtils(filename);
    char *type = file->read(2);
    if (type[0] != '\xff' || type[1] != '\xd8')
    {
      cout << "Not a JPEG file" << endl;
      exit(1);
    }
  }
  void decode()
  {
    while (1)
    {
      char *marker = file->read(2);
      if (marker[0] == '\xff' && marker[1] == '\xd9')
      {
        cout << "# End of file" << endl;
        delete marker;
        return;
      }
      if (marker[0] == '\xff' && marker[1] == '\xe0')
      {
        cout << "Application Headers\n\n";
        char *length = file->read(2);
        int len = (length[0] << 8) + length[1];
        cout << "# Length : " << len << endl;
        char *headers = file->read(5);
        // cout << "# Application headers : " << headers << endl;
        (int)file->read()[0];
        (int)file->read()[0]; // Version x.x
        (int)file->read()[0]; // Units dpi
        char *density = file->read(4);
        (int)file->read()[0];
        (int)file->read()[0]; // Thumbnail
        delete length;
        delete headers;
        delete density;
        delete marker;
      }
      else if (marker[0] == '\xff' && marker[1] == '\xdb')
      {
        cout << "\nQuantization Tables\n\n";
        char *length = file->read(2);
        unsigned int len = hex_to_int(length, 2);
        cout << "# Length : " << len << endl;
        char header = file->read()[0];
        cout << "# Header : " << ((header << 4))<< endl;
        quantization_table[quantization_table_index] = new QuantizationTable(header, file->read(64));
        quantization_table[quantization_table_index]->display();
        cout << endl;
        delete length;
        delete marker;
        // break;
      }
      else if (marker[0] == '\xff' && marker[1] == '\xc0')
      {break;
        cout << "\nStart of Frame\n\n";
        char *length = file->read(2);
        int len = hex_to_int(length, 2);
        unsigned int precision = hex_to_int(file->read(1), 1); // Precision
        char *height_b = file->read(2);
        char *width_b = file->read(2);
        this->image_width = hex_to_int(width_b, 2);
        this->image_height = hex_to_int(height_b, 2);
        cout << "# Image Size : " << this->image_width << "x" << this->image_height << endl;
        // unsigned int samples = hex_to_int(file->read(), 1);
        unsigned int components = hex_to_int(file->read(), 1);
        for (int i = 0; i < components; i++)
        {
          (int)file->read()[0]; // ID
          char *sampling = file->read();
          // cout << "\tSampling : " << (*sampling >> 4) << "x" << (*sampling & 0x0F);
          int table = hex_to_int(file->read(), 1); // Quantization Table
          this->quantMapping[i] = table;
        }
        delete length;
        delete marker;
      }
      else if (marker[0] == '\xff' && marker[1] == '\xc4')
      {
        char *length = file->read(2);
        unsigned int len = hex_to_int(length, 2);
        // cout << "# Length : " << len << endl;
        char header = file->read()[0];
        cout << "HUffman Table [" << (int)header << "]\n";
        // cout << "Class and destination : " << (int) header << ":" << (header & 0x0f) << ":" << ((header >> 4) & 0x0f) << endl;
        int arr[16];
        int n = 0;
        for (int i = 0; i < 16; i++)
        {
          int num = hex_to_int(file->read(), 1);
          arr[i] = num;
          n += num;
        }
        HuffmanTable *table = new HuffmanTable();
        // cout << "# Total number of codes : " << n << endl;
        char **elements = (char **)malloc(sizeof(char *) * n);
        int k = 0;
        for (int i = 0; i < 16; i++)
        {
          if (arr[i] == 0)
          {
            continue;
          }
          // cout << "Code of length " << i + 1 << " : ";
          for (int j = 0; j < arr[i]; j++)
          {
            elements[k] = file->read();
            // cout << hex_to_int(elements[k], 1) << " ";
            k++;
          }
          // cout << endl;
        }
        table->GetHuffmanBits(arr, 16, elements);
        // table.printBT();
        this->huffman_table[(int)header] = table;
      }
      else if (marker[0] == '\xff' && marker[1] == '\xda')
      {
        cout << "\nStart of scan\n\n";
        char *length = file->read(2);
        unsigned int len = hex_to_int(length, 2);
        cout << "# Length : " << len << endl;
        file->read(len > 2 ? len - 2 : 0);
        BitStream *st = new BitStream(file);

        int oldLumdccoeff = 0, oldCbdccoeff = 0, oldCrdccoeff = 0;
        IDCT *matL, *matCr, *matCb;
        for (int i = 0;i < this->image_height / 8; i++ ) {
          for (int j = 0; j < this->image_width / 8; i++)
          {
            IDCTAndCoeff l = buildMatrix(st, 0, this->quantization_table[this->quantMapping[0]], oldLumdccoeff);
            // IDCTAndCoeff cr = buildMatrix(st, 1, this->quantization_table[this->quantMapping[1]], oldCrdccoeff);
            // IDCTAndCoeff cb = buildMatrix(st, 1, this->quantization_table[this->quantMapping[2]], oldCbdccoeff);
            cout << "FIRST ITER" << endl;
            return;
          }
        }
      }
      else if (marker[0] == '\xff' && marker[1] == '\x00')
      {
        cout << "FF00\n\n";
        delete marker;
        break;
      }
      else
      {
        delete marker;
        break;
      }
    }
  }
};

int main()
{
  JPEG jpeg("profile.jpg");
  jpeg.decode();
  return 0;
}