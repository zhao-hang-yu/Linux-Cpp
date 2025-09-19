#include <stdio.h>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

#define IN 1
#define OUT 0

int wordCount(char *filename) 
{
    int state = OUT;
    int count = 0;
    
    ifstream fin;
    fin.open(filename, ios::in);
    if (!fin.is_open()) 
    {
        return -1;
    }
    char c;
    while ((c = fin.get()) != EOF)
    {
        if (state == OUT)
        {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) //
            {
                state = IN;
                count++;
            }
        }
        else 
        {
            if (!(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z') && c != '-' && c != '\'') // 
            {
                state = OUT;
            }
        }
    }
    
    return count;
}

int main(int argc, char *argv[]) 
{
    if (argc < 2)
    {
        return -1;
    }
    cout << wordCount(argv[1]) << endl;
}