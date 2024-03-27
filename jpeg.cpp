#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <math.h>
#include <map>
#include <vector>
#include "headers/huffman.h"
#include "headers/types.h"

using namespace logger;

class JPEG;
void writeBMP(JPEG *image, const MCU* const mcus, const std::string& filename);









class JPEG
{
  FileUtils *file;
  QuantizationTable *quantization_table[2];
  ColorComponent components[3];
  map<int, HuffmanTable *> huffman_table;
  HuffmanTable huffmanDCTTable[4];
  HuffmanTable huffmanACTable[4];
  short int quantization_table_index = 0;
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
    for (int i = 0; i < 3; i++)
    {
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
    while (length > 0)
    {
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
    show[1] << "Start of selection: " << (int)marker->read()[0] >> cout;
    show[1] << "End of selection: " << (int)marker->read()[0] >> cout;
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
    while (true)
    {
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
  void generateCodes(HuffmanTable &table)
  {
    int code = 0;
    for (int i = 0; i < 16; i++)
    {
      for (int j = table.offset[i]; j < table.offset[i + 1]; j++)
      {
        table.codes[j] = code;
        code++;
      }
      code <<= 1;
    }
  }
  char getNextSymbol(BitStream *st, const HuffmanTable &table)
  {
    int cur = 0;
    for (int i = 0; i < 16; i++)
    {
      int bit = st->getBit();
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
  bool decodeMCUComponent(BitStream *st, int *const component, int &previousDC, const HuffmanTable &dcTable, const HuffmanTable &acTable)
  {
    int length = getNextSymbol(st, dcTable);
    if (length == -1)
    {
      show(LogType::ERROR) << "Invalid DC symbol" >> cout;
      exit(1);
    }
    if (length > 11)
    {
      show(LogType::ERROR) << "Invalid DC length" >> cout;
      exit(1);
    }
    int coeff = st->getBitN(length);
    if (coeff == -1)
    {
      show(LogType::ERROR) << "Invalid DC coefficient" >> cout;
    }
    if (length != 0 && coeff < (1 << (length - 1)))
    {
      coeff += (-1 << length) + 1;
    }
    component[0] = coeff + previousDC;
    previousDC = component[0];
    int i = 1;
    while (i < 64)
    {
      int symbol = getNextSymbol(st, acTable);
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
      int coeff = 0;
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
        i++;
      }
      if (coeffLength > 10)
      {
        show(LogType::ERROR) << "Invalid AC length > 10" >> cout;
        exit(1);
      }
      if (coeffLength != 0)
      {
        coeff = st->getBitN(coeffLength);
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
    int mcuHeight = (this->image_height + 7) / 8;
    int mcuWidth = (this->image_width + 7) / 8;
    MCU *mcus = new MCU[mcuHeight * mcuWidth];
    if (mcus == nullptr)
    {
      show(LogType::ERROR) << "Memory allocation failed" >> cout;
      exit(1);
    }
    for (int i = 0; i < 4; i++)
    {
      generateCodes(huffmanDCTTable[i]);
      generateCodes(huffmanACTable[i]);
    }
    BitStream *st = new BitStream(&this->huffmanData);
    int previousDC[3] = {0, 0, 0};
    for (int i = 0; i < mcuHeight * mcuWidth; i++)
    {
      // DO Restart DC HERE
      for (int j = 0; j < 3; j++)
      {
        if (!decodeMCUComponent(st, mcus[i][j],
                                previousDC[j],
                                this->huffmanDCTTable[this->components[j].huffmanDCTTableID],
                                this->huffmanACTable[this->components[j].huffmanACTableID]))
        {
          cout << "FALSE HIT" << endl;
          delete[] mcus;
          return nullptr;
        }
        for(int k = 0; k < 64; k++)
        {
          // std::cout << mcus[i][j][k] << " ";
        }
      }
    }
    return mcus;
  }

public:
  int image_width;
  int image_height;
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
          // read_entropy_image(file);
          break;
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
      char current = *file->read(1);
      char last;
    while (true) {
      last = current;
      current = *file->read(1);
      // cout << to_hex_string(&last, 1) << " " << to_hex_string(&current, 1) << endl;
      if (last == '\xff') {
        if (current == '\xd9') {
          break;
        } else if (current == '\x00') {
          huffmanData.push_back(last);
          current = *file->read(1);
        } else if (current >= '\xd0' && current <= '\xd7') {
          current = *file->read(1);
        } else if (current == '\xff') {
          //
        }
        else {
          show(LogType::ERROR) << "Invalid marker, May be an unexpected format occured!(" << to_hex_string(&current, 1) << ")" >> cout;
          return;
        }
      }else {
        huffmanData.push_back(last);
      }
    }
    MCU *mcus = decodeHuffman();
    cout <<  mcus[0].y << endl;
    string filename = "img.jpg";
    const std::size_t pos = filename.find_last_of('.');
    const std::string outFilename = (pos == std::string::npos) ? (filename + ".bmp") : (filename.substr(0, pos) + ".bmp");
        
    writeBMP(this, mcus, outFilename);
    cout << "HURAH!!!"  << endl;
  }
};




// helper function to write a 4-byte integer in little-endian
void putInt(std::ofstream& outFile, const int v) {
    outFile.put((v >>  0) & 0xFF);
    outFile.put((v >>  8) & 0xFF);
    outFile.put((v >> 16) & 0xFF);
    outFile.put((v >> 24) & 0xFF);
}

// helper function to write a 2-byte short integer in little-endian
void putShort(std::ofstream& outFile, const int v) {
    outFile.put((v >>  0) & 0xFF);
    outFile.put((v >>  8) & 0xFF);
}

// write all the pixels in the MCUs to a BMP file
void writeBMP(JPEG *image, const MCU* const mcus, const std::string& filename) {
    cout << "WRITING BMP FILE" << endl;
    std::ofstream outFile = std::ofstream(filename, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        std::cout << "Error - Error opening output file\n";
        return;
    }

    const int mcuHeight = (image->image_height + 7) / 8;
    const int mcuWidth = (image->image_width + 7) / 8;
    const int paddingSize = (4 - (image->image_width * 3) % 4) % 4;
    const int size = 14 + 12 + image->image_height * image->image_width * 3 + image->image_height * paddingSize;

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

    for (int i = image->image_height - 1; i < image->image_height; --i) {
        const int mcuRow = i / 8;
        const int pixelRow = i % 8;
        for (int j = 0; j < image->image_width; ++j) {
            const int mcuColumn = j / 8;
            const int pixelColumn = j % 8;
            const int mcuIndex = mcuRow * mcuWidth + mcuColumn;
            const int pixelIndex = pixelRow * 8 + pixelColumn;
    cout << "HERE" << mcus[mcuIndex].b << " " << pixelIndex << endl;
            outFile.put(mcus[mcuIndex].y[pixelIndex]);
            outFile.put(mcus[mcuIndex].cb[pixelIndex]);
            outFile.put(mcus[mcuIndex].cr[pixelIndex]);
        }
        for (int j = 0; j < paddingSize; ++j) {
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