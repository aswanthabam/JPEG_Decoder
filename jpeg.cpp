#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include "file.h"

using namespace std;

class JPEG
{
  FileUtils *file;
  char *first_quantization_table;
  char *second_quantization_table;

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
      if (sizeof(u) / sizeof(char) == 1)
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
        cout << "# Application headers : " << headers << endl;
        cout << "# Version: " << (int)file->read()[0] << "." << (int)file->read()[0] << endl;
        cout << "# Units: " << (int)file->read()[0] << " dpi" << endl;
        char *density = file->read(4);
        cout << "# Density: " << (int)(density[0] << 8 + density[1]) << "x" << (int)(density[2] << 8 + density[3]) << endl;
        cout << "# Thumbnail: " << (int)file->read()[0] << "x" << (int)file->read()[0] << endl;
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
        cout << "# Destination : " << (int)file->read()[0] << endl;
        first_quantization_table = file->read(64);
        cout << "# First Quantization Table : " << endl;
        for (int i = 0; i < 8; i++)
        {
          cout << "\t ";
          for (int j = 0; j < 8; j++)
            cout << (int)first_quantization_table[i * 8 + j] << " ";
          cout << endl;
        }
        cout << endl;
        cout << "# Destination : " << (int)file->read()[0] << endl;
        second_quantization_table = file->read(64);
        cout << "# Second Quantization Table : " << endl;
        for (int i = 0; i < 8; i++)
        {
          cout << "\t ";
          for (int j = 0; j < 8; j++)
            cout << (int)second_quantization_table[i * 8 + j] << " ";
          cout << endl;
        }
        cout << endl;
        delete length;
        delete marker;
      }
      else if (marker[0] == '\xff' && marker[1] == '\xc0')
      {
        cout << "\nStart of Frame\n\n";
        char *length = file->read(2);
        unsigned int len = hex_to_int(length, 2);
        cout << "# Length : " << len << endl;
        cout << "# Precision : " << hex_to_int(file->read(2), 2) << endl;
        unsigned int lineNb = hex_to_int(file->read(2), 2);
        unsigned int samples = hex_to_int(file->read(), 1);
        cout << "# Line NB x Samples : " << lineNb << "x" << samples << " Pixels" << endl;
        unsigned int components = hex_to_int(file->read(), 1);
        cout << "# Components : " << components << endl;
        for (int i = 0; i < components; i++)
        {
          cout << "\t# ID : " << (int)file->read()[0];
          char *sampling = file->read();
          cout << "\tSampling : " << (*sampling >> 4) << "x" << (*sampling & 0x0F);
          cout << "\tQuantization Table : " << hex_to_int(file->read(), 1) << endl;
          cout << endl;
        }
      }
      else if (marker[0] == '\xff' && marker[1] == '\xc4')
      {
        cout << "HUffman Table\n\n";
        char *length = file->read(2);
        unsigned int len = hex_to_int(length, 2);
        cout << "# Length : " << len << endl;
        char *classDest = file->read();
        cout << "Class and destination : " << (*classDest >> 4) << " & " << (*classDest & 0x0F) << endl;
        int arr[16];
        int n = 0;
        for (int i = 0; i < 16; i++)
        {
          int num = hex_to_int(file->read(), 1);
          arr[i] = num;
          n += num;
        }
        cout << "# Total number of codes : " << n << endl;
        for (int i = 0; i < 16; i++)
        {
          if (arr[i] == 0)
          {
            continue;
          }
          cout << "Code of length " << i + 1 << " : ";
          for (int j = 0; j < arr[i]; j++)
            cout << byteToBits(*(file->read())) << " ";
          cout << endl;
        }
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