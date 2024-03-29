#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <math.h>
#include <map>
#include <vector>
#include <chrono>
#include "headers/huffman.h"
// #include "headers/types.h"
#include "headers/file.h"
#include "headers/display.h"

using namespace logger;

class JPEG;
void writeBMP(const JPEG *const image, const MCU *const mcus, const std::string &filename);

class JPEG
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
    if (marker->marker[0] == 0xc2)
    {
      show(LogType::ERROR) << "SOF : Not supported, Progressive baseline images are not supported" >> cout;
      exit(1);
    }
    int precision = (int)marker->read()[0];
    this->image_height = hex_to_int(marker->read(2), 2);
    this->image_width = hex_to_int(marker->read(2), 2);
    this->mcuWidth = (this->image_width + 7) / 8;
    this->mcuHeight = (this->image_height + 7) / 8;
    this->mcuWidthReal = this->image_width;
    this->mcuHeightReal = this->image_height;
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
      // cout << "ALL SYMBOLS : " << allSymbols << endl;
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
      // cout << i << " 0s " << numZeros << " len " << coeffLength << endl;
      if (symbol == 0xf0)
      {
        numZeros = 16;
      }
      if (i + numZeros >= 64)
      {
        cout << i << " " << numZeros << endl;
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

public:
  int image_width;       // Image Width
  int image_height;      // Image Height
  int mcuWidthReal = 0;  // MCU Width Real
  int mcuHeightReal = 0; // MCU Height Real

  JPEG(string filename)
  {
    file = new BitStream(filename);
    unsigned char *type = file->read(2);
    if (type[0] != 0xff || type[1] != 0xd8)
    {
      show(LogType::WARNING) << "ERROR : Not a JPEG file" >> cout;
      exit(1);
    }
  }

  void decode()
  {
    while (1)
    {
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
          show(LogType::ERROR) << "Invalid marker (" << (int)*to_hex_string(marker->marker, 1) << ", " << hex_to_int(marker->marker, 1) << ")" >> cout;
          return;
        }
        else if (marker->type == MarkerType::PAD)
        {
          continue;
        }
        else
        {
          show(LogType::ERROR) << "Invalid marker type. Not Implemented (" << (char *)to_hex_string(marker->marker, 1) << ")" >> cout;
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

    auto start = std::chrono::high_resolution_clock::now();

    file->readImageData();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    double time_taken = duration.count();

    // Output the time taken
    std::cout << "Time taken by function: " << time_taken << " seconds" << std::endl;

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

    start = std::chrono::high_resolution_clock::now();

    MCU *mcus = decodeHuffman();

    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    time_taken = duration.count();
    std::cout << "Time taken by huffman decode function: " << time_taken << " seconds" << std::endl;

    cout << mcus[0].y << endl;
    string filename = "img.jpg";
    const std::size_t pos = filename.find_last_of('.');
    const std::string outFilename = (pos == std::string::npos) ? (filename + ".bmp") : (filename.substr(0, pos) + ".bmp");

    start = std::chrono::high_resolution_clock::now();
    // writeBMP(this, mcus, outFilename);
    Display display(mcus, this->image_width, this->image_height, this->mcuWidth, this->mcuHeight);

    display.display();
    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    time_taken = duration.count();

    // Output the time taken
    std::cout << "Time taken by write bmp function: " << time_taken << " seconds" << std::endl;
    cout << "HURAH!!!" << endl;
  }
};

// helper function to write a 4-byte integer in little-endian
void putInt(std::ofstream &outFile, const int v)
{
  outFile.put((v >> 0) & 0xFF);
  outFile.put((v >> 8) & 0xFF);
  outFile.put((v >> 16) & 0xFF);
  outFile.put((v >> 24) & 0xFF);
}

// helper function to write a 2-byte short integer in little-endian
void putShort(std::ofstream &outFile, const int v)
{
  outFile.put((v >> 0) & 0xFF);
  outFile.put((v >> 8) & 0xFF);
}

// write all the pixels in the MCUs to a BMP file
void writeBMP(const JPEG *const image, const MCU *const mcus, const std::string &filename)
{
  // open file
  std::ofstream outFile = std::ofstream(filename, std::ios::out | std::ios::binary);
  if (!outFile.is_open())
  {
    std::cout << "Error - Error opening output file\n";
    return;
  }

  const int paddingSize = image->image_width % 4;
  const int size = 14 + 12 + image->image_height * image->image_width * 3 + paddingSize * image->image_height;

  outFile.put('B');
  outFile.put('M');
  putInt(outFile, size);
  putInt(outFile, 0);
  putInt(outFile, 0x1A);
  putInt(outFile, 12);
  putShort(outFile, image->image_width);
  putShort(outFile, image->image_height);
  putShort(outFile, 1);
  putShort(outFile, 24);

  for (int y = image->image_height - 1; y >= 0; --y)
  {
    const int mcuRow = y / 8;
    const int pixelRow = y % 8;
    for (int x = 0; x < image->image_width; ++x)
    {
      const int mcuColumn = x / 8;
      const int pixelColumn = x % 8;
      const int mcuIndex = mcuRow * image->mcuWidthReal + mcuColumn;
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
}

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