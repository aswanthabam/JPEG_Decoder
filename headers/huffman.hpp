#include <iostream>
#include <string>
#include <cstring>
#include <vector>

using namespace std;

class HuffmanTable
{

public:
    unsigned char offset[17] = {0};
    char symbols[162] = {0};
    unsigned int codes[162] = {0};
    bool set = false;

    void generateCodes()
    {
        int code = 0;
        for (int i = 0; i < 16; i++)
        {
            for (int j = this->offset[i]; j < this->offset[i + 1]; j++)
            {
                this->codes[j] = code;
                code++;
            }
            code <<= 1;
        }
    }

    void display()
    {
        for (int i = 0; i < 16; i++)
        {
            cout << (int)offset[i] << " ";
        }
        cout << endl;
        for (int i = 0; i < 162; i++)
        {
            cout << (int)symbols[i] << " ";
        }
        cout << endl;
        for (int i = 0; i < 162; i++)
        {
            cout << codes[i] << " ";
        }
        cout << endl;
    }
};