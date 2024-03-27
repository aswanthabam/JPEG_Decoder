#include <string>
#include <cstring>
#include <iostream>

using namespace std;

int hex_to_int(char v)
{
    // Single character only
    return (0xff & v);
}

int hex_to_int(char *v, int n)
{
    char *tu = new char[20]();
    int k = 0;
    char *u = new char[2]();
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
    unsigned int val = stoi(tu, 0, 16);
    return val;
}

char *to_hex_string(char *v, int n)
{
    char *tu = new char[20]();
    int k = 0;
    char *u = new char[2]();
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
    return tu;
}
string byteToBits(const char bytes)
{
    string result;
    for (int j = 7; j >= 0; --j)
    {
        bool bit = (bytes >> j) & 1;
        result += (bit ? '1' : '0');
    }

    return result;
}

unsigned int toInt(char *buffer)
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

