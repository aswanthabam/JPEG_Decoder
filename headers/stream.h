#include <string>
#include <vector>
#include "file.h"

class BitStream
{
    unsigned char st;
    int st_c = -1;
    FileUtils *file;
    vector<unsigned char> *st_v;
    unsigned int st_v_i = 0;
    int size = 0;

public:
    BitStream(unsigned char st)
    {
        this->st = st;
    }
    BitStream(vector<unsigned char> *st)
    {
        this->st = st->at(0);
        this->size = st->size();
        this->st_v = st;
    }
    BitStream(FileUtils *file)
    {
        this->file = file;
    }
    int getBit()
    {

        if (st_c < 0)
        {
            if (file == nullptr && st_v_i >= size || file != nullptr && file->file->eof())
            {
                cout << "END OF FILE\n";
                return -1;
            }
            if (file == nullptr)
            {
                st = st_v->at(st_v_i);
                st_v_i++;
            }
            else
                st = *file->read();
            st_c = 7;
        }
        int bit = (st >> st_c) & 1;
        st_c--;
        return bit;
    }

    int getBitN(int n)
    {
        int val = 0;
        for (int i = 0; i < n; i++)
        {
            int bit = getBit();
            if (bit == -1)
                break;
            val = (val << 1) | bit;
        }
        return val;
    }
    void align() {
        this->st_c = -1;
    }
    string byteToBits()
    {
        string result;
        for (int j = 7; j >= 0; --j)
        {
            bool bit = (st >> j) & 1;
            result += (bit ? '1' : '0');
        }

        return result;
    }
};