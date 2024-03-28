#include<iostream>
#include<fstream>
using namespace std;

class FileUtils {
  char *buffer = nullptr;
public:
  ifstream *file;
  FileUtils(string filename) {
    file = new ifstream(filename);
  }
  unsigned char* read(int size = 1) {
    this->buffer= new char[size]();
    file->read((char *) this->buffer,size * sizeof(char));
    return (unsigned char*) this->buffer;
  }
  byte* readByte(int size = 1) {
    byte *buffer = new byte[size]();
    file->read(( char*)buffer,size * sizeof(byte));
    return buffer;
  }
};