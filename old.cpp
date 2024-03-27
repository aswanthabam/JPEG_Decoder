#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
using namespace std;
class Stack {
public:
  short int *array = new short int[16]();
  int top = -1;
  void push(int b) {
    if(top == 16) return;
    top++;
    array[top] = b;
  }
  int pop() {
    if(top == -1) return -1;
    int val = array[top];
    top--;
    return val;
  }
  void print() {
    if(top == -1) return;
    for(int i = 0;i <= top;i++) {
      cout<<array[i];
    }
  }
};

class Node {
public:
  Node *left=nullptr,*right=nullptr,*parent=nullptr;
  bool leaf=false;
  char value;
  void insertLeft() {
    leaf = false;
    left = new Node;
    left->parent = this;
    left->leaf = true;
  }
  void insertRight() {
    leaf = false;
    right = new Node;
    right->parent = this;
    right->leaf = true;
  }
  Node* getRightMost() {
    cout<<"\tFinding right most"<<endl;
    if(parent != nullptr && parent->left == this){
      cout<<"\tJust the right element"<<endl;
      return parent->right;
    }
    
    Node *ptr = this;
    int c = 0;
    cout<<"\tFinding the non-right parent"<<(ptr->parent == nullptr)<<endl;
    
    while(ptr->parent != nullptr && ptr->parent->right == ptr) {
      cout<<"\t\tLoop"<<c<<" "<<ptr->parent<<endl;
      ptr = ptr->parent;
      c++;
    }
    cout<<"\tChecking if null"<<endl;
    if(ptr->parent == nullptr) {
      cout<<"\t\tFinally got null :("<<endl;
      return nullptr;
    }
    cout<<"\tSetting right and going on"<<endl;
    ptr = ptr->parent->right;
    cout <<"\tSetted to the right element"<<endl;
    cout<<"\tStarted looing for same level "<<c<<endl;
    while(c > 0) {
      ptr = ptr->left;
      c--;
      cout<<"\t\tLoop"<<c<<endl;
    }
    return ptr;
  }
};

class HuffmanTree {
public:
  Node *root=nullptr;
  HuffmanTree() {
    root = new Node;
    root->insertLeft();
    root->insertRight();
  }
  void traverse() {
    Stack *s1=new Stack,*s2=new Stack;
    s1->push(0);
    s2->push(1);
    traverse(root->left,s1);
    traverse(root->right,s2);
    
  }
  void traverse(Node* node,Stack* s) {
    if(node == nullptr) return ;
    if(node->leaf) {
      s->print();
      s->pop();
      return;
    }
    else {
      s->push(0);
      traverse(node->left,s);
      s->push(1);
      traverse(node->right,s);
    }
  }
  
};

class HuffmanTable {
private:
  int *lengths;
  char *elements;
  HuffmanTree tree;
public:
  HuffmanTable(int *lengths,char *elements) {
    this->lengths = lengths;
    this->elements = elements;
  }
  
  // Decode Huffman Table
  void decode() {
    int k = 0;
    char elem;
    Node *current,*leftMost;
    cout<<"Seted Left Most"<<endl;
    leftMost = tree.root->left;
    for(int i = 0;i < 16;i++) {
      if(lengths[i] == 0) {
        cout<<"No element with that length"<<endl;
        current = leftMost;
        cout<<"Setted current to leftmost"<<endl;
        while(current != nullptr) {
          current->insertLeft();
          current->insertRight();
          cout<<"Created left & rigt"<<endl;
          current = current->getRightMost();
          cout<<"Setted current to rightmost"<<endl;
        }
        leftMost = leftMost->left;
        cout<<"Setted leftmost to the left of leftmost"<<endl;
      }
      else {
        cout<<"Non zero length\nAdding all elements from left to right in same level"<<endl;
        for(int i = 0;i < lengths[i];i++) {
          elem = elements[k];
          cout<<"Element: "<<elem<<endl;
          leftMost->value = elem;
          leftMost=leftMost->getRightMost();
          k++;
          if(leftMost == nullptr) break;
          
        }
        leftMost->insertLeft();
        leftMost->insertRight();
        current = leftMost->getRightMost();
        leftMost = leftMost->left;
        while(current != nullptr) {
          // Some eeror here
          current->insertLeft();
          current->insertRight();
          current = current->getRightMost();
          // break;
        }
      }
    }
  }
};
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
    char *tu = new char[20]();
    int k = 0;
    char *u=new char[2]();
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
    unsigned int val = stoi(tu,0,16);
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
      cout << hex << uppercase << (0xff & c[i]) <<dec<< (i == n - 1 ? " " : "|");
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
  
  // Read Quantization table
  void read_quantization_table()
  {
    cout << "\n - GOT : Quantization Table \n";
    char* buf = read_bytes(2);
    cout<<"Buffer: "<<(int)buf[0]<<" "<<(int)buf[1]<<endl;
    int length = hex_to_int(read_bytes(2), 2);
    cout << "\tLength : " << length << endl;
    seek(length - 2);
  }
  
  // Read Start of frame segment
  void read_start_of_frame()
  {
    cout << "\n - GOT : Start of frame \n";
    int length = hex_to_int(read_bytes(2), 2);
    cout << "\tLength : " << length << endl;
    int pre = hex_to_int(read_bytes(1),1);
    cout<< "\tPrecision : "<<pre<<endl;
    this->height = hex_to_int(read_bytes(2),2);
    this->width = hex_to_int(read_bytes(2),2);
    cout<<"\twidth*height : "<<this->width<<"*"<<this->height<<" Pixels"<<endl;
    seek(hex_to_int(read_bytes(1),1)*3);
  }
  
  // Read huffman Table
  void read_huffman_table()
  {
    cout << "\n - GOT : Huffman Table \n";
    int length = hex_to_int(read_bytes(2), 2);
    cout << "\tLength : " << length << endl;
    int info = hex_to_int(read_bytes(1),1);
    int lengths[16];
    char *elements = new char[length-19]();
    for(int i = 0;i < 16;i++)
      lengths[i] = hex_to_int(read_bytes(1),1);
    for(int i = 0;i < (length-19);i++)
      elements[i] = *read_bytes(1);
    
    cout<<"Lengths : [ ";
    for(int i = 0;i < 16;i++)
      cout<<lengths[i]<<" ";
    cout<<"]"<<endl<<"Elements: [ ";
    for(int i = 0;i < (length - 19);i++){
      print(&elements[i],1);
      cout<<" ";
    }
    cout<<"] "<<endl;
  }
  
  // Read start of scan segment
  void read_start_of_scan()
  {
    cout << "\n - GOT : Start of scan \n";
    int length = hex_to_int(read_bytes(2), 2);
    cout << "\tLength : " << length << endl;
    seek(length - 2);
    // read_image_data();
  }
  
  // Read image data
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
  int width=0,height=0;
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
      // if (compare(marker, (char *)"\xff\xe0", 2))
      //   read_application_headers();
      // else if (compare(marker, (char *)"\xff\xdb", 2))
      //   read_quantization_table();
      if (compare(marker, (char *)"\xff\xc0", 2))
        read_start_of_frame();
      // else if (compare(marker, (char *)"\xff\xc4", 2))
      //   read_huffman_table();
      // else if (compare(marker, (char *)"\xff\xda", 2))
      // {
      //   read_start_of_scan();
      //   return;
      // }
      // else
      // {
      //   cout << "Something idk" << endl;
      // }
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
  
  // HuffmanTable table(new int[16]{0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0},new char[12]{'a','b','c','d','e','f','g','h','i','j','k','l'});
  // table.decode();
}