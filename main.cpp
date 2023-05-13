#include <iostream>
#include <fstream>
#include <string.h>
using namespace std;

class JPEG
{
private:
  char *img = nullptr;
  char *tmp2 = new char[2];
  char *tmp1 = new char;
  char *tmp = nullptr;
  fstream f;
  // Function to convert hexa decimal character / array of characters to integer value
  int hex_to_int(char v)
  {
    // Single character only
    return (0xff & v);
  }
  int hex_to_int(char *v, int n)
  {
    char *tu = new char[20];
    int k = 0;
    for (int i = 0; i < n; i++)
    {
      char u[2];
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

  // Function to read n no of bytes from the file
  char *read_bytes(int n)
  {
    if (tmp != NULL)
      delete tmp;
    tmp = new char[n];
    f.read((char *)tmp, n * sizeof(char));
    return tmp;
  }

  // Function to seek the position of the file pointer
  void seek(int n)
  {
    f.seekg(f.tellg() / sizeof(char) + n * sizeof(char));
  }

  // print a hexa decimal value (char)
  void print(char *c, int n)
  {
    for (int i = 0; i < n; i++)
      cout << hex << uppercase << (0xff & c[i]) << (i == n - 1 ? " " : "|");
  }

  // Compare two hex characters*
  bool compare(char *one, char *two, int n)
  {
    bool t = true;
    for (int i = 0; i < n; i++)
    {
      if (hex_to_int(one[i]) != hex_to_int(two[i]))
      {
        t = false;
        break;
      }
    }
    return t;
  }

  // Read the Application specific headers of the image
  void read_application_headers()
  {
    cout << "\n - GOT : Application Headers \n";
    int length = hex_to_int(read_bytes(2), 2);
    cout << "\tLength : " << length << endl;
    seek(length - 2);
  }

  void read_quantization_table()
  {
    cout << "\n - GOT : Quantization Table \n";
    int length = hex_to_int(read_bytes(2), 2);
    cout << "\tLength : " << length << endl;
    seek(length - 2);
  }

  void read_start_of_frame()
  {
    cout << "\n - GOT : Start of frame \n";
    int length = hex_to_int(read_bytes(2), 2);
    cout << "\tLength : " << length << endl;
    seek(length - 2);
  }

  void read_huffman_table()
  {
    cout << "\n - GOT : Huffman Table \n";
    int length = hex_to_int(read_bytes(2), 2);
    cout << "\tLength : " << length << endl;
    seek(length - 2);
  }

  void read_start_of_scan()
  {
    cout << "\n - GOT : Start of scan \n";
    int length = hex_to_int(read_bytes(2), 2);
    cout << "\tLength : " << length << endl;
    seek(length - 2);
    read_image_data();
  }

  void read_image_data()
  {
    cout << "\n - GOT : Image Data \n";
    do
    {
      read_bytes(1);
      if (compare(tmp, (char *)"\xff", 1))
      {
        read_bytes(1);
        if (compare(tmp, (char *)"\xd9", 1))
        {
          break;
        }
      }
      print(tmp, 1);
    } while (true);
    cout << "\nDone" << endl;
  }

public:
  bool STATUS = true;
  JPEG(char *img)
  {
    this->img = img;
    f.open(img, ios::binary | ios::in);
    if (!f)
    {
      cout << " - Invalid Image Provided" << endl;
      STATUS = false;
    }
    else
    {
      cout << " - File opened: " << img << endl;
      read_bytes(2);
      if (!compare(tmp, (char *)"\xff\xd8", 2))
      {
        cout << " - Not a valid JPEG image" << endl;
        STATUS = false;
      }
      else
        cout << " - Valid JPEG image" << endl;
    }
  }
  void decode()
  {
    cout << " - Started Decoding of Image : " << img << endl;
    char *marker;
    do
    {
      marker = read_bytes(2);
      if (compare(marker, (char *)"\xff\xe0", 2))
        read_application_headers();
      else if (compare(marker, (char *)"\xff\xdb", 2))
        read_quantization_table();
      else if (compare(marker, (char *)"\xff\xc0", 2))
        read_start_of_frame();
      else if (compare(marker, (char *)"\xff\xc4", 2))
        read_huffman_table();
      else if (compare(marker, (char *)"\xff\xda", 2))
      {
        read_start_of_scan();
        return;
      }
      else
      {
        cout << "Something idk" << endl;
      }
    } while (!compare(marker, (char *)"\xff\xd9", 2));
  }
};
int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    cout << "Please specify an image" << endl;
    return 0;
  }
  char *img = argv[1];
  JPEG obj(img);
  if (obj.STATUS)
    obj.decode();
}