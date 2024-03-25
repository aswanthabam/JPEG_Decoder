#include<iostream>
#include<fstream>
using namespace std;

class FileUtils {
  ifstream *file;
public:
  FileUtils(string filename) {
    file = new ifstream(filename);
  }
  char* read(int size = 1) {
    char *buffer = new char[size]();
    file->read(buffer,size * sizeof(char));
    return buffer;
  }
  byte* readByte(int size = 1) {
    byte *buffer = new byte[size]();
    file->read((char*)buffer,size * sizeof(byte));
    return buffer;
  }
};