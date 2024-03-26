#include <iostream>
#include <fstream>
#include <string>
#include<cstring>
#include <math.h>
#include "huffman.h"

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
        this->table[i][j] = (int) raw[i * 8 + j];
    }
    this->header = header;
  }

  void display() {
    for (int i = 0;i < 8; i++) {
      cout << "\t";
      for (int j = 0; j < 8; j++) {
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
  int image_width;
  int image_height;
  int quantMapping[4];
  HuffmanTable *huffman_table[4];
  int _huffman_table_count = 0;

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
        quantization_table[0] = new QuantizationTable((int)file->read()[0], file->read(64));
        quantization_table[1] = new QuantizationTable((int)file->read()[0], file->read(64));
        // quantization_table[1]->display();
        cout << endl;
        delete length;
        delete marker;
      }
      else if (marker[0] == '\xff' && marker[1] == '\xc0')
      {
        cout << "\nStart of Frame\n\n";
        char *length = file->read(2);
        int len = hex_to_int(length, 2);
        cout << "# Length : " << len << endl;
        unsigned int precision =  hex_to_int(file->read(1), 1); // Precision
        cout << "# Precision : " << precision << endl;
        char *height_b = file->read(2);
        char *width_b = file->read(2);
        unsigned int width = hex_to_int(width_b, 2);
        unsigned int height = hex_to_int(height_b, 2);
        cout << "# Image Size : " << width<< "x" << height << endl;
        // unsigned int samples = hex_to_int(file->read(), 1);
        unsigned int components = hex_to_int(file->read(), 1);
        cout << "# Components : " << components << endl;
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
        cout << "HUffman Table ["<<this->_huffman_table_count<<"]\n";
        char *length = file->read(2);
        unsigned int len = hex_to_int(length, 2);
        // cout << "# Length : " << len << endl;
        char *classDest = file->read();
        // cout << "Class and destination : " << (*classDest >> 4) << " & " << (*classDest & 0x0F) << endl;
        int arr[16];
        int n = 0;
        for (int i = 0; i < 16; i++)
        {
          int num = hex_to_int(file->read(), 1);
          arr[i] = num;
          n += num;
        }
        HuffmanTable table;
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
        table.GetHuffmanBits(arr, 16, elements);
        this->huffman_table[this->_huffman_table_count] = &table;
        this->_huffman_table_count++;
      }
      else if (marker[0] == '\xff' && marker[1] == '\xda') {
        cout << "\nStart of scan\n\n";
        char *length = file->read(2);
        unsigned int len = hex_to_int(length, 2);
        cout << "# Length : " << len << endl;
        file->read(len > 2 ? len - 2 : 0);
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
  JPEG jpeg("img.jpeg");
  jpeg.decode();
  return 0;
}