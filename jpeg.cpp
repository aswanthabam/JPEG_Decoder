// #include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <math.h>
#include <map>
#include <vector>
#include "headers/huffman.h"
#include "headers/types.h"

using namespace logger;

class JPEG
{
  FileUtils *file;
  QuantizationTable *quantization_table[2];
  ColorComponent components[3];
  map<int, HuffmanTable *> huffman_table;
  HuffmanTable huffmanDCTTable[4];
  HuffmanTable huffmanACTable[4];
  short int quantization_table_index = 0;
  int image_width;
  int image_height;
  int quantMapping[4];
  vector<char> huffmanData;

  // IDCTAndCoeff buildMatrix(BitStream *st, int idx, QuantizationTable *quant, int olddccoeff)
  // {
  //   IDCT *i = new IDCT();
  //   int code = hex_to_int(this->huffman_table[0 + idx]->getCode(st), 1);
  //   int bits = st->getBitN(code);
  //   int dccoeff = decodeNumber(code, bits) + olddccoeff;
  //   show << "DC Coeff : " << (*quant[0][0]) << " " << i->base[0] >> cout;
  //   i->base[0] = dccoeff * (*quant[0][0]);
  //   int l = 1;
  //   while (l < 64)
  //   {
  //     code = hex_to_int(this->huffman_table[16 + idx]->getCode(st), 1);
  //     if (code == 0)
  //     {
  //       break;
  //     }
  //     if (code > 15)
  //     {
  //       l += code >> 4;
  //       code = code & 0xf0;
  //     }
  //     bits = st->getBitN(code);
  //     if (l < 64)
  //     {
  //       int coeff = decodeNumber(code, bits);
  //       i->base[l] = (coeff * *quant[l / 8][l % 8]);
  //       cout << " " << i->base[l] << " ";
  //       l++;
  //     }
  //     cout << endl;
  //   }
  //   i->rearrange_zig_zag();
  //   for (int k = 0; k < 64; k++)
  //   {
  //     cout << i->base[k] << " ";
  //   }
  //   cout << endl;
  //   i->perform();
  //   return {i, dccoeff};
  // }
  /* Read application headers of a given image */
  void read_application_headers(Marker *marker)
  {
    char *identifier = marker->read(5);
    if (identifier[0] != 'J' || identifier[1] != 'F' || identifier[2] != 'I' || identifier[3] != 'F' || identifier[4] != '\x00')
    {
      show(LogType::ERROR) << "APPLICATION HEADERS : Invalid JPEG file, not JEFI" >> cout;
      exit(1);
    }
    int major_version = (int)marker->read()[0];
    int minor_version = (int)marker->read()[0];
    int density_units = (int)marker->read()[0];
    int x_density = hex_to_int(marker->read(2), 2);
    int y_density = hex_to_int(marker->read(2), 2);
    if (x_density < 1 || y_density < 1)
    {
      show(LogType::ERROR) << "APPLICATION HEADERS : Invalid density" >> cout;
      exit(1);
    }
    int x_thumbnail = (int)marker->read()[0];
    int y_thumbnail = (int)marker->read()[0];
    if (x_thumbnail != 0 || y_thumbnail != 0)
    {
      show(LogType::ERROR) << "APPLICATION HEADERS : Thumbnail not supported" >> cout;
      exit(1);
    }
    show << "Application Headers" >> cout;
    show[1] << "JFIF Version : " << major_version << "." << minor_version >> cout;
    show[1] << "Density : " << x_density << "x" << y_density << " " << (density_units == 0 ? "DPI" : "DPC") >> cout;
  }

  void read_sof(Marker *marker)
  {
    if (marker->marker[0] == '\xc2')
    {
      show(LogType::ERROR) << "SOF : Not supported, Progressive baseline images are not supported" >> cout;
      exit(1);
    }
    int precision = (int)marker->read()[0];
    this->image_width = hex_to_int(marker->read(2), 2);
    this->image_height = hex_to_int(marker->read(2), 2);
    int components = (int)marker->read()[0];
    if (components != 3)
    {
      show(LogType::ERROR) << "SOF : Not supported, only 3 components are supported" >> cout;
      exit(1);
    }
    for (int i = 0; i < 3; i++ ) {
      int id = (int)marker->read()[0];
      char sampling = marker->read()[0];
      ColorComponent *comp = &this->components[i];
      comp->horizontal_sampling_factor = (sampling >> 4);
      comp->vertical_sampling_factor = (sampling & 0x0F);
      comp->quantizationTableID = (int)marker->read()[0];
    }
    show[0] << "Start of Frame" >> cout;
    show[1] << "Image Size : " << this->image_width << "x" << this->image_height >> cout;
    show[1] << "Components : " << components >> cout;
    for (int i = 0; i < components; i++)
    {
      show[2] << "Component " << i + 1 << " : " << this->components[i].horizontal_sampling_factor << "x" << this->components[i].vertical_sampling_factor >> cout;
      show[2] << "Quantization Table : " << this->components[i].quantizationTableID >> cout;
    }
  }

  void read_huffman_table(Marker *marker)
  {
    int length = marker->length - 2;
    // cout << "Length : " << length << endl;
    while (length > 0) {
      char *tableInfo = marker->read();
      int tableId = (*tableInfo) & 0x0F;
      bool acTable = (*tableInfo) >> 4;
      // cout << "TABLE ID : " << tableId << " AC : " << acTable << endl;
      if (tableId > 3)
      {
        show(LogType::ERROR) << "Huffman Table : Invalid table ID" >> cout;
        exit(1);
      }
      HuffmanTable *table;
      if (acTable)
      {
         table = &huffmanACTable[tableId];

      }else {
        table = &huffmanDCTTable[tableId];
      }
      table->offset[0] = 0;
      int allSymbols = 0;
      for (int i = 1; i <= 16; i++)
      {
        allSymbols += (int) marker->read()[0];
        table->offset[i] = allSymbols;
      }
      if (allSymbols > 176) {
        show(LogType::ERROR) << "Huffman Table : Invalid number of symbols" >> cout;
        exit(1);
      }
      // cout << "ALL SYMBOLS : " << allSymbols << endl;
      for (int i = 0; i<allSymbols;i++) {
        table->symbols[i] = marker->read()[0];
      }
      length -= allSymbols + 17;
    }
    if (length != 0)
    {
      show(LogType::ERROR) << "Huffman Table : Invalid DHT" >> cout;
      exit(1);
    }
    return; 
    // char header = marker->read()[0];
    // int arr[16];
    // int n = 0;
    // for (int i = 0; i < 16; i++)
    // {
    //   int num = hex_to_int(marker->read(), 1);
    //   arr[i] = num;
    //   n += num;
    // }
    // HuffmanTable *table = new HuffmanTable();
    // char **elements = (char **)malloc(sizeof(char *) * n);
    // int k = 0;
    // for (int i = 0; i < 16; i++)
    // {
    //   if (arr[i] == 0)
    //   {
    //     continue;
    //   }
    //   for (int j = 0; j < arr[i]; j++)
    //   {
    //     elements[k] = marker->read();
    //     k++;
    //   }
    // }
    // // table->GetHuffmanBits(arr, 16, elements);
    // this->huffman_table[(int)header] = table;
    // show[0] << "Got Huffman Table [" << (int)header << "]" >> cout;
  }
  void read_quantization_table(Marker *marker)
  {
    int length = marker->length - 2;
    while (length > 0)
    {
      char *info = marker->read(1);
      length--;
      int tableID = (*info) & 0x0F;
      if (tableID > 3)
      {
        show(LogType::ERROR) << "Quantization Table : Invalid table ID" >> cout;
        exit(1);
      }
      char *arr;
      if (*info >> 4 != 0)
      {
        arr = new char[64];
        for (int i = 0; i < 64; i++)
        {
          char raw = (marker->read()[0] << 8) + marker->read()[0];
          arr[i] = raw;
        }
        length -= 128;
      }
      else
      {
        arr = marker->read(64);
        length -= 64;
      }
      quantization_table[tableID] = new QuantizationTable(tableID, arr);
      show[0](LogType::INFO) << "Got Quantization Table [" << tableID << "]" >> cout;
      // quantization_table[tableID]->display();
    }
  }

  void read_sos(Marker *marker)
  {
    int components = (int)marker->read()[0];
    if (components != 3)
    {
      show(LogType::ERROR) << "SOS : Not supported, only 3 components are supported" >> cout;
      exit(1);
    }
    for (int i = 0; i < components; i++)
    {
      int id = (int)marker->read()[0]; // ID
      ColorComponent *comp = &this->components[i];
      char huff = marker->read()[0];
      comp->huffmanDCTTableID = huff >> 4;
      comp->huffmanACTableID = huff & 0x0f;
    }
    show[0] << "Start of Scan" >> cout;
    show[1] << "Components : " << components >> cout;
    show[1] << "Start of selection: " << (int) marker->read()[0] >> cout;
    show[1] << "End of selection: " << (int) marker->read()[0] >> cout;
    show[1] << "Successive Approximation: " << hex_to_int(marker->read(), 1) >> cout;
    show[0] << "Color Components" >> cout;
    for (int i = 0; i < components; i++)
    {
      show[1] << "Component " << i + 1 >> cout;
      show[2] << "Huffman DC Table : " << this->components[i].huffmanDCTTableID >> cout;
      show[2] << " Huffman AC Table : " << this->components[i].huffmanACTableID >> cout;
      show[2] << "Quantization Table : " << this->components[i].quantizationTableID >> cout;
      show[2] << "Horizontal Sampling Factor : " << this->components[i].horizontal_sampling_factor >> cout;
      show[2] << "Vertical Sampling Factor : " << this->components[i].vertical_sampling_factor >> cout;
      show[2] << "Quantization Table : " << this->components[i].quantizationTableID >> cout;
    }
  }
  void read_entropy_image(FileUtils *file)
  {
    while(true) {

    }
    return;
    // show[0] << "Reading Entropy Data" >> cout;
    // BitStream *st = new BitStream(file);
    // int oldLumdccoeff = 0, oldCbdccoeff = 0, oldCrdccoeff = 0;
    // IDCT *matL, *matCr, *matCb;
    // for (int i = 0; i < this->image_height / 8; i++)
    // {
    //   for (int j = 0; j < this->image_width / 8; i++)
    //   {
    //     // IDCTAndCoeff l = buildMatrix(st, 0, this->quantization_table[this->quantMapping[0]], oldLumdccoeff);
    //     // IDCTAndCoeff cr = buildMatrix(st, 1, this->quantization_table[this->quantMapping[1]], oldCrdccoeff);
    //     // IDCTAndCoeff cb = buildMatrix(st, 1, this->quantization_table[this->quantMapping[2]], oldCbdccoeff);
    //     cout << "FIRST ITER" << endl;
    //     return;
    //   }
    // }
  }

public:
  JPEG(string filename)
  {
    file = new FileUtils(filename);
    char *type = file->read(2);
    if (type[0] != '\xff' || type[1] != '\xd8')
    {
      show(LogType::WARNING) << "ERROR : Not a JPEG file" >> cout;
      exit(1);
    }
  }
  void decode()
  {
    while (1)
    {
      char *marker = file->read(1);
      if (marker[0] == '\xff')
      {
        Marker *marker = new Marker(file);
        if (marker->type == MarkerType::SOI)
        {
          show << "Start of Image" >> cout;
        }
        else if (marker->type == MarkerType::APP)
        {
          read_application_headers(marker);
        }
        else if (marker->type == MarkerType::SOF)
        {
          read_sof(marker);
        }
        else if (marker->type == MarkerType::DHT)
        {
          read_huffman_table(marker);
        }
        else if (marker->type == MarkerType::DQT)
        {
          read_quantization_table(marker);
          // quantization_table[0]->display();
          // quantization_table[1]->display();
          // return;
        }
        else if (marker->type == MarkerType::SOS)
        {
          read_sos(marker);
          read_entropy_image(file);
        }

        else if (marker->type == MarkerType::DRI)
        {
          show(LogType::WARNING) << "Found DRI marker, ignoring ..." >> cout;
          show[1] << "Length : " << marker->length >> cout;
        }
        else if (marker->type == MarkerType::DRM)
        {
          show(LogType::WARNING) << "Found DRM marker, ignoring ..." >> cout;
          show[1] << "Length : " << marker->length >> cout;
        }
        else if (marker->type == MarkerType::META)
        {
          show(LogType::WARNING) << "Found Meta marker, ignoring ..." >> cout;
          show[1] << "Length : " << marker->length >> cout;
        }
        else if (marker->type == MarkerType::EOI)
        {
          show << "End of file" >> cout;
          return;
        }
        else if (marker->type == MarkerType::INVALID)
        {
          show(LogType::ERROR) << "Invalid marker (" << to_hex_string(marker->marker, 1) << ", " << hex_to_int(marker->marker, 1) << ")" >> cout;
          return;
        }
        else if (marker->type == MarkerType::PAD)
        {
          continue;
        }
        else
        {
          show(LogType::ERROR) << "Invalid marker type. Not Implemented (" << to_hex_string(marker->marker, 1) << ")" >> cout;
          return;
        }
        delete marker;
      }
      else
      {
        show(LogType::ERROR) << "Invalid marker, May be an unexpected format occured!(" << to_hex_string(marker, 1) << ")" >> cout;
        return;
      }
      delete marker;
    }
  }
};

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    show(LogType::ERROR) << "Usage: jpeg <filename>" >> cout;
    return 1;
  }
  JPEG jpeg(argv[1]);
  jpeg.decode();
  return 0;
}