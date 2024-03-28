#include <string>
#include <cstring>
#include <iostream>

using namespace std;

unsigned int hex_to_int(unsigned char v)
{
    // Single unsigned character only
    return (0xff & v);
}

unsigned int hex_to_int(unsigned char *v, int n)
{
    unsigned char *tu = new unsigned char[20]();
    int k = 0;
    unsigned char *u = new unsigned char[2]();
    for (int i = 0; i < n; i++)
    {
        delete[] u;
        u = new unsigned char[2];
        sprintf((char *)u, "%X", hex_to_int(v[i]));
        if (strlen((char*)u) == 1)
        {
            u[1] = u[0];
            u[0] = '0';
        }
        tu[k + 0] = u[0];
        tu[k + 1] = u[1];
        k += 2;
    }
    unsigned int val = stoi((char *)tu, 0, 16);
    return val;
}

unsigned char *to_hex_string(unsigned char *v, int n)
{
    unsigned char *tu = new unsigned char[20]();
    int k = 0;
    unsigned char *u = new unsigned char[2]();
    for (int i = 0; i < n; i++)
    {
        delete[] u;
        u = new unsigned char[2];
        sprintf((char *)u, "%X", hex_to_int(v[i]));
        if (strlen((char *)u) == 1)
        {
            u[1] = u[0];
            u[0] = '0';
        }
        tu[k + 0] = u[0];
        tu[k + 1] = u[1];
        k += 2;
    }
    return tu;
}
string byteToBits(const unsigned char bytes)
{
    string result;
    for (int j = 7; j >= 0; --j)
    {
        bool bit = (bytes >> j) & 1;
        result += (bit ? '1' : '0');
    }

    return result;
}

unsigned int toInt(unsigned char *buffer)
{
    int num = (buffer[0] << 8) + buffer[1];
    if (num < 0)
        return (unsigned int)-num + 1;
    else
        return (unsigned int)num;
}

int decodeNumber(int code, int bits)
{
    int l = pow(2, code);
    if (bits >= l)
        return bits;
    else
        return bits - (2 * l - 1);
}

