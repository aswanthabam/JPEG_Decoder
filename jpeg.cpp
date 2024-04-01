#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <math.h>
#include <map>
#include <vector>
#include <chrono>
#include "headers/huffman.hpp"
#include "headers/file.hpp"
#include "headers/display.hpp"
// #include "headers/types.h"

using namespace logger;

class Image;
void writeBMP(const Image *const image, const MCU *const mcus, const std::string &filename);

class Image
{
  BitStream *file;                                   // File object
  map<int, QuantizationTable *> quantization_tables; // Quantization Tables
  ColorComponent color_components[3];                // color components
  HuffmanTable huffmanDCTTable[4];                   // Huffman Tables DCT
  HuffmanTable huffmanACTable[4];                    // Huffman Tables AC
  vector<unsigned char> huffmanData;                 // Huffman Coded Data
  int restartInterval = 0;                           // Restart Interval
  bool zeroBased = false;                            // is the componet ids zero based or not
  short int numComponents = 0;                       // Number of components
  int mcuWidth = 0;                                  // MCU Width
  int mcuHeight = 0;                                 // MCU Height
  int horizontalSamplingFactor = 0;                  // Horizontal Sampling Factor
  int verticalSamplingFactor = 0;                    // Vertical Sampling Factor
  MCU *mcus = nullptr;                               // MCU array

  /* Read application headers of a given image */
  void read_application_headers(Marker *marker)
  {
    unsigned char *identifier = marker->read(5);
    if (identifier[0] != 'J' || identifier[1] != 'F' || identifier[2] != 'I' || identifier[3] != 'F' || identifier[4] != 0x00)
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
    if (marker->marker == 0xc2)
    {
      show(LogType::ERROR) << "SOF : Not supported, Progressive baseline images are not supported" >> cout;
      exit(1);
    }
    int precision = (int)marker->read()[0];
    this->image_height = hex_to_int(marker->read(2), 2);
    this->image_width = hex_to_int(marker->read(2), 2);
    this->mcuWidth = (this->image_width + 7) / 8;
    this->mcuHeight = (this->image_height + 7) / 8;
    this->mcuWidthReal = this->mcuWidth;
    this->mcuHeightReal = this->mcuHeight;
    int components = (int)marker->read()[0];
    this->numComponents = components;
    if (components != 3)
    {
      show(LogType::ERROR) << "SOF : Not supported, only 3 components are supported" >> cout;
      exit(1);
    }
    for (int i = 0; i < components; i++)
    {
      int id = (int)marker->read()[0];
      if (id == 0)
      {
        zeroBased = true;
      }
      if (!zeroBased && id > 3)
      {
        show(LogType::ERROR) << "SOF : Invalid component ID" >> cout;
        exit(1);
      }
      if (zeroBased && id > 2)
      {
        show(LogType::ERROR) << "SOF : Invalid component ID" >> cout;
        exit(1);
      }
      unsigned char sampling = marker->read()[0];
      ColorComponent *comp = &this->color_components[i];
      comp->horizontal_sampling_factor = (sampling >> 4);
      comp->vertical_sampling_factor = (sampling & 0x0F);
      comp->quantizationTableID = (int)marker->read()[0];
      if (id == 1)
      {
        if ((comp->horizontal_sampling_factor != 1 && comp->horizontal_sampling_factor != 2) ||
            (comp->vertical_sampling_factor != 1 && comp->vertical_sampling_factor != 2))
        {
          show(LogType::ERROR) << "SOF : Invalid sampling factors" >> cout;
          exit(1);
        }
        if (comp->horizontal_sampling_factor == 2 && this->mcuWidth % 2 == 1)
        {
          this->mcuWidthReal += 1;
        }
        if (comp->vertical_sampling_factor == 2 && this->mcuHeight % 2 == 1)
        {
          this->mcuHeightReal += 1;
        }
        this->horizontalSamplingFactor = comp->horizontal_sampling_factor;
        this->verticalSamplingFactor = comp->vertical_sampling_factor;
      }
      else
      {
        if (comp->horizontal_sampling_factor != 1 || comp->vertical_sampling_factor != 1)
        {
          show(LogType::ERROR) << "SOF : Invalid sampling factors" >> cout;
          exit(1);
        }
      }
    }
    show[0] << "Start of Frame" >> cout;
    show[1] << "Image Size : " << this->image_width << "x" << this->image_height >> cout;
    show[1] << "Components : " << components >> cout;
    for (int i = 0; i < components; i++)
    {
      show[2] << "Component " << i + 1 << " : " << this->color_components[i].horizontal_sampling_factor << "x" << this->color_components[i].vertical_sampling_factor >> cout;
      show[2] << "Quantization Table : " << this->color_components[i].quantizationTableID >> cout;
    }
  }

  void read_huffman_table(Marker *marker)
  {
    int length = marker->length - 2;
    while (length > 0)
    {
      unsigned char *tableInfo = marker->read();
      int tableId = (*tableInfo) & 0x0F;
      bool acTable = (*tableInfo) >> 4;
      if (tableId > 3)
      {
        show(LogType::ERROR) << "Huffman Table : Invalid table ID" >> cout;
        exit(1);
      }
      HuffmanTable *table;
      if (acTable)
      {
        table = &huffmanACTable[tableId];
      }
      else
      {
        table = &huffmanDCTTable[tableId];
      }
      table->offset[0] = 0;
      int allSymbols = 0;
      for (int i = 1; i <= 16; i++)
      {
        allSymbols += (int)marker->read()[0];
        table->offset[i] = allSymbols;
      }
      if (allSymbols > 176)
      {
        show(LogType::ERROR) << "Huffman Table : Invalid number of symbols" >> cout;
        exit(1);
      }
      for (int i = 0; i < allSymbols; i++)
      {
        table->symbols[i] = marker->read()[0];
      }
      table->set = true;
      table->generateCodes();
      length -= allSymbols + 17;
    }
    if (length != 0)
    {
      show(LogType::ERROR) << "Huffman Table : Invalid DHT" >> cout;
      exit(1);
    }
    return;
  }
  void read_quantization_table(Marker *marker)
  {
    int length = marker->length - 2;
    while (length > 0)
    {
      unsigned char *info = marker->read(1);
      length--;
      int tableID = (*info) & 0x0F;
      if (tableID > 3)
      {
        show(LogType::ERROR) << "Quantization Table : Invalid table ID" >> cout;
        exit(1);
      }
      unsigned char *arr;
      if (*info >> 4 != 0)
      {
        arr = new unsigned char[64];
        for (int i = 0; i < 64; i++)
        {
          unsigned char raw = (marker->read()[0] << 8) + marker->read()[0];
          arr[i] = raw;
        }
        length -= 128;
      }
      else
      {
        arr = marker->read(64);
        length -= 64;
      }
      this->quantization_tables[tableID] = new QuantizationTable(tableID, arr);
      this->quantization_tables[tableID]->set = true;
      show[0](LogType::INFO) << "Got Quantization Table [" << tableID << "]" >> cout;
      this->quantization_tables[tableID]->display();
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
      ColorComponent *comp = &this->color_components[i];
      unsigned char huff = marker->read()[0];
      comp->huffmanDCTTableID = huff >> 4;
      comp->huffmanACTableID = huff & 0x0f;
    }
    int startOfScan = (int)marker->read()[0];
    int endOfScan = (int)marker->read()[0];
    int approximation = (int)marker->read()[0];
    int successiveApproximationHigh = approximation >> 4;
    int successiveApproximationLow = approximation & 0x0f;

    if (startOfScan != 0 || endOfScan != 63)
    {
      show(LogType::ERROR) << "SOS : Invalid start or end of scan" >> cout;
      exit(1);
    }
    if (successiveApproximationHigh != 0 || successiveApproximationLow != 0)
    {
      show(LogType::ERROR) << "SOS : Invalid successive approximation" >> cout;
      exit(1);
    }
    show[0] << "Start of Scan" >> cout;
    show[1] << "Components : " << components >> cout;
    show[1] << "Start of selection: " << startOfScan >> cout;
    show[1] << "End of selection: " << endOfScan >> cout;
    show[1] << "Successive Approximation: " << successiveApproximationHigh << " : " << successiveApproximationLow >> cout;

    show[0] << "Color Components" >> cout;
    for (int i = 0; i < components; i++)
    {
      show[1] << "Component " << i + 1 >> cout;
      show[2] << "Huffman DC Table : " << this->color_components[i].huffmanDCTTableID >> cout;
      show[2] << " Huffman AC Table : " << this->color_components[i].huffmanACTableID >> cout;
      show[2] << "Quantization Table : " << this->color_components[i].quantizationTableID >> cout;
      show[2] << "Horizontal Sampling Factor : " << this->color_components[i].horizontal_sampling_factor >> cout;
      show[2] << "Vertical Sampling Factor : " << this->color_components[i].vertical_sampling_factor >> cout;
      show[2] << "Quantization Table : " << this->color_components[i].quantizationTableID >> cout;
    }
    if (marker->length - 6 - (2 * components) != 0)
    {
      show(LogType::ERROR) << "SOS : Invalid length of SOS" >> cout;
      exit(1);
    }
  }

  void read_dri(Marker *marker)
  {
    this->restartInterval = hex_to_int(marker->read(2), 2);
    if (marker->length - 4 != 0)
    {
      show(LogType::ERROR) << "DRI : Invalid length" >> cout;
      exit(1);
    }
    show[0] << "Restart Interval : " << this->restartInterval >> cout;
  }

  unsigned char getNextSymbol(const HuffmanTable &table)
  {
    int cur = 0;
    for (int i = 0; i < 16; i++)
    {
      int bit = this->file->getBit();
      if (bit == -1)
      {
        return -1;
      }
      cur = (cur << 1) | bit;
      for (int j = table.offset[i]; j < table.offset[i + 1]; j++)
      {
        if (table.codes[j] == cur)
        {
          return table.symbols[j];
        }
      }
    }
    return -1;
  }

  bool decodeMCUComponent(int *const component, int &previousDC, HuffmanTable &dcTable, HuffmanTable &acTable)
  {
    int length = getNextSymbol(dcTable);
    if (length == -1)
    {
      show(LogType::ERROR) << "Invalid DC symbol (hmm)" >> cout;
      exit(1);
    }
    if (length > 11)
    {
      show(LogType::ERROR) << "Invalid DC length" >> cout;
      exit(1);
    }
    int coeff = this->file->getBitN(length);
    if (coeff == -1)
    {
      show(LogType::ERROR) << "Invalid DC coefficient" >> cout;
    }
    if (length != 0 && coeff < (1 << (length - 1)))
    {
      coeff -= (1 << length) - 1;
    }
    component[0] = coeff + previousDC;
    previousDC = component[0];
    int i = 1;
    while (i < 64)
    {
      int symbol = getNextSymbol(acTable);
      if (symbol == -1)
      {
        show(LogType::ERROR) << "Invalid AC symbol" >> cout;
        exit(1);
      }

      if (symbol == 0x00)
      {
        for (; i < 64; i++)
        {
          component[zigZagMap[i]] = 0;
        }
        return true;
      }
      int numZeros = symbol >> 4;
      int coeffLength = symbol & 0x0F;
      coeff = 0;

      if (symbol == 0xf0)
      {
        numZeros = 16;
      }
      if (i + numZeros >= 64)
      {
        show(LogType::ERROR) << "Invalid AC length" >> cout;
        exit(1);
      }
      for (int j = 0; j < numZeros; j++, i++)
      {
        component[zigZagMap[i]] = 0;
      }
      if (coeffLength > 10)
      {
        show(LogType::ERROR) << "Invalid AC length > 10" >> cout;
        exit(1);
      }
      if (coeffLength != 0)
      {
        coeff = this->file->getBitN(coeffLength);
        if (coeff == -1)
        {
          show(LogType::ERROR) << "Invalid AC coefficient" >> cout;
          exit(1);
        }
        if (coeff < (1 << (coeffLength - 1)))
        {
          coeff -= (1 << coeffLength) - 1;
        }
        component[zigZagMap[i]] = coeff;
        i++;
      }
    }
    return true;
  }

  MCU *decodeHuffman()
  {
    MCU *mcus = new MCU[this->mcuWidthReal * this->mcuHeightReal];
    if (mcus == nullptr)
    {
      show(LogType::ERROR) << "Memory allocation failed" >> cout;
      exit(1);
    }
    int previousDC[3] = {0, 0, 0};
    int restartInterval = this->restartInterval * this->horizontalSamplingFactor * this->verticalSamplingFactor;
    for (int y = 0; y < this->mcuHeight; y += this->verticalSamplingFactor)
    {
      for (int x = 0; x < this->mcuWidth; x += this->horizontalSamplingFactor)
      {
        if (restartInterval != 0 && (y * this->mcuWidthReal + x) % restartInterval == 0)
        {
          previousDC[0] = 0;
          previousDC[1] = 0;
          previousDC[2] = 0;
          this->file->align();
        }
        for (int i = 0; i < this->numComponents; ++i)
        {
          for (int v = 0; v < this->color_components[i].vertical_sampling_factor; ++v)
          {
            for (int h = 0; h < this->color_components[i].horizontal_sampling_factor; ++h)
            {
              if (!decodeMCUComponent(mcus[(y + v) * this->mcuWidthReal + (x + h)][i],
                                      previousDC[i],
                                      this->huffmanDCTTable[this->color_components[i].huffmanDCTTableID],
                                      this->huffmanACTable[this->color_components[i].huffmanACTableID]))
              {
                delete[] mcus;
                return nullptr;
              }
            }
          }
        }
      }
    }
    return mcus;
  }

  void putInt(std::ofstream &outFile, const int v)
  {
    outFile.put((v >> 0) & 0xFF);
    outFile.put((v >> 8) & 0xFF);
    outFile.put((v >> 16) & 0xFF);
    outFile.put((v >> 24) & 0xFF);
  }

  void putShort(std::ofstream &outFile, const int v)
  {
    outFile.put((v >> 0) & 0xFF);
    outFile.put((v >> 8) & 0xFF);
  }

  bool writeBMP(const std::string &filename)
  {

    std::ofstream outFile = std::ofstream(filename, std::ios::out | std::ios::binary);
    if (!outFile.is_open())
    {
      show(LogType::ERROR) << "Error opening output file" >> cout;
      return false;
    }

    const int paddingSize = this->image_width % 4;
    const int size = 14 + 12 + this->image_height * this->image_width * 3 + paddingSize * this->image_height;

    outFile.put('B');
    outFile.put('M');
    putInt(outFile, size);
    putInt(outFile, 0);
    putInt(outFile, 0x1A);
    putInt(outFile, 12);
    putShort(outFile, this->image_width);
    putShort(outFile, this->image_height);
    putShort(outFile, 1);
    putShort(outFile, 24);

    for (int y = this->image_height - 1; y >= 0; --y)
    {
      const int mcuRow = y / 8;
      const int pixelRow = y % 8;
      for (int x = 0; x < this->image_width; ++x)
      {
        const int mcuColumn = x / 8;
        const int pixelColumn = x % 8;
        const int mcuIndex = mcuRow * this->mcuWidthReal + mcuColumn;
        const int pixelIndex = pixelRow * 8 + pixelColumn;

        outFile.put(mcus[mcuIndex].y[pixelIndex]);
        outFile.put(mcus[mcuIndex].cr[pixelIndex]);
        outFile.put(mcus[mcuIndex].cb[pixelIndex]);
      }
      for (int i = 0; i < paddingSize; ++i)
      {
        outFile.put(0);
      }
    }

    outFile.close();
    return true;
  }

  void YCbCrToRGBMCU(MCU &mcu, const MCU &cbcr, const uint v, const uint h)
  {
    for (uint y = 7; y < 8; --y)
    {
      for (uint x = 7; x < 8; --x)
      {
        const uint pixel = y * 8 + x;
        const uint cbcrPixelRow = y / this->verticalSamplingFactor + 4 * v;
        const uint cbcrPixelColumn = x / this->horizontalSamplingFactor + 4 * h;
        const uint cbcrPixel = cbcrPixelRow * 8 + cbcrPixelColumn;
        int r = mcu.y[pixel] + 1.402f * cbcr.cr[cbcrPixel] + 128;
        int g = mcu.y[pixel] - 0.344f * cbcr.cb[cbcrPixel] - 0.714f * cbcr.cr[cbcrPixel] + 128;
        int b = mcu.y[pixel] + 1.772f * cbcr.cb[cbcrPixel] + 128;
        if (r < 0)
          r = 0;
        if (r > 255)
          r = 255;
        if (g < 0)
          g = 0;
        if (g > 255)
          g = 255;
        if (b < 0)
          b = 0;
        if (b > 255)
          b = 255;
        mcu.r[pixel] = r;
        mcu.g[pixel] = g;
        mcu.b[pixel] = b;
      }
    }
  }

  // convert all pixels from YCbCr color space to RGB
  void YCbCrToRGB()
  {
    for (uint y = 0; y < this->mcuHeight; y += this->verticalSamplingFactor)
    {
      for (uint x = 0; x < this->mcuWidth; x += this->horizontalSamplingFactor)
      {
        const MCU &cbcr = mcus[y * this->mcuWidthReal + x];
        for (uint v = this->verticalSamplingFactor - 1; v < this->verticalSamplingFactor; --v)
        {
          for (uint h = this->horizontalSamplingFactor - 1; h < this->horizontalSamplingFactor; --h)
          {
            MCU &mcu = this->mcus[(y + v) * this->mcuWidthReal + (x + h)];
            YCbCrToRGBMCU(mcu, cbcr, v, h);
          }
        }
      }
    }
  }

  void dequantizeMCUComponent(QuantizationTable *qTable, int *component)
  {
    for (uint i = 0; i < 64; ++i)
    {
      component[i] *= (*qTable)[i];
    }
  }

  // dequantize all MCUs
  void dequantize()
  {
    for (uint y = 0; y < this->mcuHeight; y += this->verticalSamplingFactor)
    {
      for (uint x = 0; x < this->mcuWidth; x += this->horizontalSamplingFactor)
      {
        for (uint i = 0; i < this->numComponents; ++i)
        {
          for (uint v = 0; v < this->color_components[i].vertical_sampling_factor; ++v)
          {
            for (uint h = 0; h < this->color_components[i].horizontal_sampling_factor; ++h)
            {
              dequantizeMCUComponent(this->quantization_tables[this->color_components[i].quantizationTableID], this->mcus[(y + v) * this->mcuWidthReal + (x + h)][i]);
            }
          }
        }
      }
    }
  }

  // perform 1-D IDCT on all columns and rows of an MCU component
  //   resulting in 2-D IDCT
  void inverseDCTComponent(int *const component)
  {
    for (uint i = 0; i < 8; ++i)
    {
      const float g0 = component[0 * 8 + i] * s0;
      const float g1 = component[4 * 8 + i] * s4;
      const float g2 = component[2 * 8 + i] * s2;
      const float g3 = component[6 * 8 + i] * s6;
      const float g4 = component[5 * 8 + i] * s5;
      const float g5 = component[1 * 8 + i] * s1;
      const float g6 = component[7 * 8 + i] * s7;
      const float g7 = component[3 * 8 + i] * s3;

      const float f0 = g0;
      const float f1 = g1;
      const float f2 = g2;
      const float f3 = g3;
      const float f4 = g4 - g7;
      const float f5 = g5 + g6;
      const float f6 = g5 - g6;
      const float f7 = g4 + g7;

      const float e0 = f0;
      const float e1 = f1;
      const float e2 = f2 - f3;
      const float e3 = f2 + f3;
      const float e4 = f4;
      const float e5 = f5 - f7;
      const float e6 = f6;
      const float e7 = f5 + f7;
      const float e8 = f4 + f6;

      const float d0 = e0;
      const float d1 = e1;
      const float d2 = e2 * m1;
      const float d3 = e3;
      const float d4 = e4 * m2;
      const float d5 = e5 * m3;
      const float d6 = e6 * m4;
      const float d7 = e7;
      const float d8 = e8 * m5;

      const float c0 = d0 + d1;
      const float c1 = d0 - d1;
      const float c2 = d2 - d3;
      const float c3 = d3;
      const float c4 = d4 + d8;
      const float c5 = d5 + d7;
      const float c6 = d6 - d8;
      const float c7 = d7;
      const float c8 = c5 - c6;

      const float b0 = c0 + c3;
      const float b1 = c1 + c2;
      const float b2 = c1 - c2;
      const float b3 = c0 - c3;
      const float b4 = c4 - c8;
      const float b5 = c8;
      const float b6 = c6 - c7;
      const float b7 = c7;

      component[0 * 8 + i] = b0 + b7;
      component[1 * 8 + i] = b1 + b6;
      component[2 * 8 + i] = b2 + b5;
      component[3 * 8 + i] = b3 + b4;
      component[4 * 8 + i] = b3 - b4;
      component[5 * 8 + i] = b2 - b5;
      component[6 * 8 + i] = b1 - b6;
      component[7 * 8 + i] = b0 - b7;
    }
    for (uint i = 0; i < 8; ++i)
    {
      const float g0 = component[i * 8 + 0] * s0;
      const float g1 = component[i * 8 + 4] * s4;
      const float g2 = component[i * 8 + 2] * s2;
      const float g3 = component[i * 8 + 6] * s6;
      const float g4 = component[i * 8 + 5] * s5;
      const float g5 = component[i * 8 + 1] * s1;
      const float g6 = component[i * 8 + 7] * s7;
      const float g7 = component[i * 8 + 3] * s3;

      const float f0 = g0;
      const float f1 = g1;
      const float f2 = g2;
      const float f3 = g3;
      const float f4 = g4 - g7;
      const float f5 = g5 + g6;
      const float f6 = g5 - g6;
      const float f7 = g4 + g7;

      const float e0 = f0;
      const float e1 = f1;
      const float e2 = f2 - f3;
      const float e3 = f2 + f3;
      const float e4 = f4;
      const float e5 = f5 - f7;
      const float e6 = f6;
      const float e7 = f5 + f7;
      const float e8 = f4 + f6;

      const float d0 = e0;
      const float d1 = e1;
      const float d2 = e2 * m1;
      const float d3 = e3;
      const float d4 = e4 * m2;
      const float d5 = e5 * m3;
      const float d6 = e6 * m4;
      const float d7 = e7;
      const float d8 = e8 * m5;

      const float c0 = d0 + d1;
      const float c1 = d0 - d1;
      const float c2 = d2 - d3;
      const float c3 = d3;
      const float c4 = d4 + d8;
      const float c5 = d5 + d7;
      const float c6 = d6 - d8;
      const float c7 = d7;
      const float c8 = c5 - c6;

      const float b0 = c0 + c3;
      const float b1 = c1 + c2;
      const float b2 = c1 - c2;
      const float b3 = c0 - c3;
      const float b4 = c4 - c8;
      const float b5 = c8;
      const float b6 = c6 - c7;
      const float b7 = c7;

      component[i * 8 + 0] = b0 + b7;
      component[i * 8 + 1] = b1 + b6;
      component[i * 8 + 2] = b2 + b5;
      component[i * 8 + 3] = b3 + b4;
      component[i * 8 + 4] = b3 - b4;
      component[i * 8 + 5] = b2 - b5;
      component[i * 8 + 6] = b1 - b6;
      component[i * 8 + 7] = b0 - b7;
    }
  }

  // perform IDCT on all MCUs
  void inverseDCT()
  {
    for (uint y = 0; y < this->mcuHeight; y += this->verticalSamplingFactor)
    {
      for (uint x = 0; x < this->mcuWidth; x += this->horizontalSamplingFactor)
      {
        for (uint i = 0; i < this->numComponents; ++i)
        {
          for (uint v = 0; v < this->color_components[i].vertical_sampling_factor; ++v)
          {
            for (uint h = 0; h < this->color_components[i].horizontal_sampling_factor; ++h)
            {
              inverseDCTComponent(this->mcus[(y + v) * this->mcuWidthReal + (x + h)][i]);
            }
          }
        }
      }
    }
  }

  void process_image_data()
  {
    for (int i = 0; i < this->numComponents; i++)
    {
      if (this->quantization_tables[this->color_components[i].quantizationTableID]->set == false)
      {
        show(LogType::ERROR) << "Quantization Table not found" >> cout;
        exit(1);
      }
      if (this->huffmanDCTTable[this->color_components[i].huffmanDCTTableID].set == false)
      {
        show(LogType::ERROR) << "Huffman DCT Table not found" >> cout;
        exit(1);
      }
      if (this->huffmanACTable[this->color_components[i].huffmanACTableID].set == false)
      {
        show(LogType::ERROR) << "Huffman AC Table not found" >> cout;
        exit(1);
      }
    }
    show[0] << "Image is valid and ready to decode" >> cout;
    show[1] << "Got Huffman tables DC and AC" >> cout;
    show[1] << "Got Quantization tables" >> cout;
    show[1] << "Got Color Components" >> cout;
    show[1] << "Got Image Size : " << this->image_width << "x" << this->image_height >> cout;
    show[1] << "Got Restart Interval : " << this->restartInterval >> cout;
    show[1] << "Got Zero Based : " << this->zeroBased >> cout;
    show[1] << "Got Number of Components : " << this->numComponents >> cout;
    show[1] << "Got Huffman Data of size(" << (int)file->size << ")" >> cout;

    this->mcus = decodeHuffman();
    dequantize();
    inverseDCT();
    YCbCrToRGB();
  }

public:
  int image_width;       // Image Width
  int image_height;      // Image Height
  int mcuWidthReal = 0;  // MCU Width Real
  int mcuHeightReal = 0; // MCU Height Real

  Image(string filename)
  {
    file = new BitStream(filename);
    unsigned char *type = file->read(2);
    if (type[0] != 0xff || type[1] != 0xd8)
    {
      show(LogType::WARNING) << "ERROR : Not a JPEG file" >> cout;
      exit(1);
    }

  }

  void saveToBMP(string filename)
  {
    if(writeBMP(filename)) {
      show[0] << "Image Saved" >> cout;
    } else {
      show(LogType::ERROR) << "Image Save Failed" >> cout;
    }
  }

  void display() {
    if(displayImage(this->mcus, this->image_width, this->image_height, this->mcuWidth, this->mcuHeight, this->mcuWidthReal)) {
      show[0] << "Image Displayed" >> cout;
    } else {
      show(LogType::ERROR) << "Image Display Failed" >> cout;
    }
  }

  void readJPEG()
  {
    while (1)
    {
      if(file->eof()) {
        show(LogType::WARNING) << "File Readed Completely!" >> cout;
        break;
      }
      unsigned char *marker = file->read(1);
      if (marker[0] == 0xff)
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
        }
        else if (marker->type == MarkerType::SOS)
        {
          read_sos(marker);
          process_image_data();
          break;
        }
        else if (marker->type == MarkerType::DRI)
        {
          read_dri(marker);
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
          show(LogType::ERROR) << "Invalid marker (" << (int)marker->marker << ", " << marker->marker << ")" >> cout;
          return;
        }
        else if (marker->type == MarkerType::PAD)
        {
          continue;
        }
        else
        {
          show(LogType::ERROR) << "Invalid marker type. Not Implemented (" << marker->marker << ")" >> cout;
          return;
        }
        delete marker;
      }
      else
      {
        show(LogType::ERROR) << "Invalid marker, May be an unexpected format occured!(" << (char *)to_hex_string(marker, 1) << ")" >> cout;
        return;
      }
      delete marker;
    }
    return;
  }

  ~Image()
  {
    show[0] << "Exiting ..." >> cout;
    file->close();
  }
};

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    show(LogType::ERROR) << "Usage : jpeg <filename>" >> cout;
    return 1;
  }

  Image *jpeg = new Image(argv[1]);
  jpeg->readJPEG();
  jpeg->display();
  delete jpeg;
  return 0;
}