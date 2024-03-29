#include <iostream>
#include <fstream>
// #include "logger.h"
#include "utils.h"

using namespace std;
using namespace logger;

class JPEGFile
{
    char *buffer = nullptr;
    vector<unsigned char> *clean_data = new vector<unsigned char>();
    bool is_read = false;
    unsigned int read_index = 0;

public:
    unsigned int size = 0;
    ifstream *file;
    JPEGFile(string filename)
    {
        file = new ifstream(filename);
    }
    unsigned char *read(int n = 1)
    {
        this->buffer = new char[n]();
        if (!this->is_read)
        {
            file->read((char *)this->buffer, n * sizeof(char));
            return (unsigned char *)this->buffer;
        }
        else
        {
            if (this->read_index >= this->size)
            {
                cout << this->read_index << " " << this->size << endl;
                show(LogType::ERROR) << "Reading of unavailable data" >> cout;
                return nullptr;
            }
            else
            {
                for (int i = 0; i < n; i++)
                {
                    this->buffer[i] = this->clean_data->at(this->read_index);
                    this->read_index++;
                }
                return (unsigned char *) this->buffer;
            }
        }
    }
    void seek(int pos)
    {
        file->seekg(pos, ios::cur);
    }
    void readImageData()
    {
        if (this->is_read)
        {
            show(LogType::ERROR) << "Already Readed Image Data" >> cout;
            return;
        }
        unsigned char current = *this->read(1);
        unsigned char last;
        while (true)
        {
            last = current;
            current = *this->read(1);
            if (last == 0xff)
            {
                if (current == 0xd9)
                {
                    break;
                }
                else if (current == 0x00)
                {
                    this->clean_data->push_back(last);
                    current = *this->read(1);
                }
                else if (current >= 0xd0 && current <= 0xd7)
                {
                    current = *this->read(1);
                }
                else if (current == 0xff)
                {
                    continue;
                }
                else
                {
                    show(LogType::ERROR) << "Invalid marker, May be an unexpected format occured!(" << (char *)to_hex_string(&current, 1) << ")" >> cout;
                    return;
                }
            }
            else
            {
                this->clean_data->push_back(last);
            }
        }
        this->is_read = true;
        this->size = this->clean_data->size();
    }

    bool eof()
    {
        return file->eof();
    }

    ~JPEGFile()
    {
        delete this->file;
        delete this->clean_data;
    }
};

class BitStream : public JPEGFile
{
    unsigned char st;
    int st_c = -1;

public:
    BitStream(string filename) : JPEGFile(filename) {}
    
    int getBit()
    {

        if (st_c < 0)
        {
            if (this->eof())
            {
                cout << "END OF FILE\n";
                return -1;
            }
            else
                st = *this->read();
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
    void align()
    {
        this->st_c = -1;
    }
};

enum MarkerType
{
    SOI = 0xd8,
    APP = 0xe0,
    DQT = 0xdb,
    SOF = 0xc0,
    DHT = 0xc4,
    SOS = 0xda,
    EOI = 0xd9,
    DRI = 0xdd,
    DRM = 0xd0,
    META = -2,
    PAD = 0x00,
    INVALID = -1,
};

class Marker
{
    int _data_pointer = 0;

public:
    unsigned char *marker;
    int length;
    unsigned char *data;
    int type;
    Marker(JPEGFile *file)
    {
        this->marker = file->read(1);
        if (marker[0] == 0x00)
        {
            this->type = MarkerType::PAD;
            return;
        }
        int int_marker = (int)hex_to_int(marker, 1);
        if (int_marker >= 224 && int_marker < 240)
        {
            this->type = MarkerType::META;
            this->length = (file->read()[0] << 8) + file->read()[0];
            this->data = file->read(this->length - 2);
            return;
        }
        if (int_marker >= 0xd0 && int_marker < 0xd8)
        {
            this->type = MarkerType::DRM;
            this->length = int_marker - 0xd0;
            // this->data = file->read(this->length - 2);
            return;
        }
        switch (marker[0])
        {
        case MarkerType::APP:
            this->type = MarkerType::APP;
            break;
        case MarkerType::DQT:
            this->type = MarkerType::DQT;
            break;
        case 0xc0:
            this->type = MarkerType::SOF;
            break;
        case 0xc2:
            this->type = MarkerType::SOF;
            break;
        case 0xc4:
            this->type = MarkerType::DHT;
            break;
        case 0xda:
            this->type = MarkerType::SOS;
            break;
        case 0xd9:
            this->type = MarkerType::EOI;
            break;
        case 0xd8:
            this->type = MarkerType::SOI;
            break;
        case 0xdd:
            this->type = MarkerType::DRI;
            break;
        default:
            this->type = MarkerType::INVALID;
        }
        this->length = (file->file->get() << 8) + file->file->get();
        this->data = file->read(this->length - 2);
    }

    unsigned char *read(int n = 1)
    {
        if (_data_pointer + n > length)
        {
            show(LogType::ERROR) << "MARKER READER : Reading of unavailable data" >> cout;
            return nullptr;
        }
        unsigned char *res = new unsigned char[n];
        for (int i = 0; i < n; i++)
        {
            res[i] = this->data[_data_pointer + i];
        }
        _data_pointer += n;
        return res;
    }

    ~Marker()
    {
        delete[] this->data;
    }
};