#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include "file.h"
#include "utils.h"

using namespace std;

/*
    Bit Stream class for streaming a bit
*/
class BitStream
{
    unsigned char st;
    int st_c = -1;
    FileUtils *file;
    vector<unsigned char> *st_v;
    int st_v_i = 0;
    int size = 0;

public:
    BitStream(char st)
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
            // if (st == '\xff') {
            //     char tmp;
            //     if (file == nullptr) {
            //         tmp = st_v->at(st_v_i);
            //         st_v_i++;
            //     }
            //     else tmp = *file->read();
            //     if (tmp == '\x00') {
            //         st = '\xff';
            //     } else {
            //         if (file == nullptr) {
            //             st_v_i--;
            //         } else {
            //             file->file->seekg(-1, ios::cur);
            //         }
            //     }
            // }
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
class Node
{
public:
    Node(char *value, int depth)
    {
        this->value = value;
        this->depth = depth;
        this->length = 2;
        this->leaf = 1;
    }
    Node(int depth)
    {
        this->depth = depth;
        this->left = new Node();
        this->right = new Node();
        this->left->depth = depth + 1;
        this->right->depth = depth + 1;
        this->length = 0;
    }
    Node() {}
    Node *left = nullptr, *right = nullptr;
    char *value = nullptr;
    int length = 0;
    int depth = 0;
    bool leaf = 0;

    bool append(Node *node)
    {
        if (this->length == 0)
        {
            this->left = node;
            this->length = 1;
            return true;
        }
        if (this->length == 1)
        {
            this->right = node;
            this->length = 2;
            return true;
        }
        return false;
    }

    Node *operator[](int index)
    {
        if (index == 0)
        {
            return this->left;
        }
        return this->right;
    }
};

bool BitsFromLength(Node *root1, char *element, int code_length);

class HuffmanTable2
{

public:
    Node *root;
    HuffmanTable2()
    {
        this->root = new Node(0);
    }

    void GetHuffmanBits(int *code_lengths, int size, char *elements[])
    {
        int k = 0;
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < code_lengths[i]; j++)
            {
                BitsFromLength(this->root, elements[k], i);
                k++;
            }
        }
    }

    void Traverse(Node *node)
    {
        if (node == nullptr)
            return;
        if (node->value != nullptr)
            cout << node->value << ":" << node->depth << endl;
        else
        {
            Traverse(node->left);
            Traverse(node->right);
        }
    }

    string toList(Node *node)
    {
        if (node == nullptr)
            return "";
        // if (node->value != nullptr)
        // {
        //     return to_string(hex_to_int(node->value, 1));
        // }
        else
        {
            string t = "[";
            if (node->left != nullptr)
            {
                if (node->left->leaf)
                {
                    t += to_string(hex_to_int(node->left->value, 1));
                }
                else
                {
                    t += toList(node->left);
                }
            }
            t += ",";
            if (node->right != nullptr)
            {
                if (node->right->leaf)
                {
                    t += to_string(hex_to_int(node->right->value, 1));
                }
                else
                {
                    t += toList(node->right);
                }
            }
            t += "]";
            // string t1 = node->left->leaf ? to_string(hex_to_int(node->left->value, 1)) : toList(node->left);
            // string t2 = node->right->leaf ? to_string(hex_to_int(node->right->value, 1)) : toList(node->right);
            // cout << "[]" << t1 << "[]" << t2 << endl;
            return t;
            return "[" + toList(node->left) + "," + toList(node->right) + "]";
        }
    }

    void display()
    {
        cout << toList(this->root) << endl;
    }

    Node *find(BitStream *st)
    {
        Node *r = this->root;
        while (!r->leaf)
        {
            int i = st->getBit();
            r = (i ? r->left : r->right);
        }
        return r;
    }
    char *getCode(BitStream *st)
    {
        Node *res = find(st);
        return res->value;
    }

    void printBT(const std::string &prefix, const Node *node, bool isLeft)
    {
        if (node != nullptr)
        {
            std::cout << prefix;
            std::cout << (isLeft ? "|.." : "L..");
            // print the value of the node
            std::cout << (node->value != nullptr ? to_string(hex_to_int(node->value, 1)) : "") << std::endl;
            // enter the next tree level - left and right branch
            printBT(prefix + (isLeft ? "|   " : "    "), node->right, true);
            printBT(prefix + (isLeft ? "|   " : "    "), node->left, false);
        }
    }

    void printBT()
    {
        printBT("", this->root, false);
    }
};

bool BitsFromLength(Node *root1, char *element, int code_length)
{
    if (root1 == nullptr)
        return false;
    if (code_length == 0)
    {
        if (root1->length < 2)
        {
            // cout << root1->length << endl;
            // cout << "Element: " << element << " @ L(" << root1->depth + 1 << ")" << endl;
            return root1->append(new Node(element, root1->depth + 1));
        }
        return false;
    }
    for (int i = 0; i < 2; i++)
    {
        if (root1->length == i)
        {
            root1->append(new Node(root1->depth + 1));
        }
        if (BitsFromLength(i == 0 ? root1->left : root1->right, element, code_length - 1) == true)
        {
            return true;
        }
    }
    return false;
}

// int main()
// {
//     HuffmanTable table;
//     int code_lengths[] = {0, 2, 2, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//     char* elements[] = {"5","6", "3", "4", "2", "7", "8", "1", "0", "9"};
//     table.GetHuffmanBits(code_lengths, 16, elements);
//     table.display();
//     return 0;
// }
