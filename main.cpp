#include<iostream>
#include<fstream>
using namespace std;

class JPEG {
private:
  char *img = nullptr;
  char *tmp2 = new char[2];
  char *tmp1 = new char;
  char *tmp = nullptr;
  fstream f;
  char* read_bytes(int n) {
    if(tmp != NULL) delete tmp;
    tmp = new char[n];
    f.read((char*) tmp,n*sizeof(char));
    return tmp;
  }
  bool compare(char *one,char *two,int n) {
    bool t = true;
    for(int i = 0;i < n;i++) {
      cout<<one[i]<<" "<<two[i]<<endl;
      if(one[i] != two[i]) {t = false;break;}}
    return t;
  }
  void read_application_headers() {
    //read_bytes(2)
  }
public:
  bool STATUS = true;
  JPEG(char *img) {
    this->img = img;
    f.open(img,ios::binary | ios::in);
    if(!f) {
      cout<<" - Invalid Image Provided"<<endl;
      STATUS = false;
    }else {
     cout<<" - File opened: "<<img<<endl;
     read_bytes(2);
     if(compare(tmp,(char*)"\xff\xd8",1)){ cout<<" - Not a valid JPEG image"<<endl;
       STATUS = false;
     }else cout<<" - Valid JPEG image"<<endl;
    }
  }
  void decode() {
    cout << " - Started Decoding of Image : " << img << endl;
    
  }
};
int main(int argc,char *argv[]) {
  if(argc < 2) {
    cout<<"Please specify an image"<<endl;
    return 0;
  }
  char *img = argv[1];
  JPEG obj(img);
  if(obj.STATUS) obj.decode();
}