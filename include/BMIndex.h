#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>
#include <iostream>
#include <map>
#include "AES.h"

using namespace std;

class BMIndex { 
public:
    int id;
    int n; // the nubmer of bytes in a row of the bitmap
    map<string, unsigned char *> bitmap;
    
//    BMIndex(vector<string> &plaintext_keyword, unsigned char* records_key){
    BMIndex(vector<vector<string>> &records, int attribute_index, unsigned char* records_key){
        n = records.size() / 8 + 1;
        for (id = 0; id < records.size(); id++){
            unsigned char * Hmac_output1 = NULL;  
	        unsigned int len_of_Hmac_output1 = 0; 
//            HmacEncode("sha256", (const char*)records_key, 32, records[id][attribute_index].c_str(), 
//                records[id][attribute_index].size(), Hmac_output1, len_of_Hmac_output1);
//            string K((char *)Hmac_output1, len_of_Hmac_output1);
//            free(Hmac_output1);
            string K(records[id][attribute_index]);
            if (bitmap.find(K) == bitmap.end()){
                bitmap[K] = new unsigned char[n];
                memset(bitmap[K], 0, n * sizeof(char));
            }
            unsigned char *p = bitmap[K] + id / 8;
            *p = (*p) | (1 << (7 - id % 8));
        }
    }
    
    ~BMIndex(){
        for (auto itr = bitmap.begin(); itr != bitmap.end(); itr++)
            delete itr->second;
    }

    int search(string &search_keyword, unsigned char* records_key, unsigned char* output) {
//    cout << "in search**********************\n";
//    for (auto itr = bitmap.begin(); itr != bitmap.end(); itr++){
//        cout << itr->first;
//        for (int j =  0; j < n; j++)
//            printf(" %x ", itr->second[j]);
//        cout << endl;
//    }
        unsigned char * Hmac_output = NULL;  
        unsigned int len_of_Hmac_output = 0; 
//        HmacEncode("sha256", (const char*)records_key, 32, search_keyword.c_str(), search_keyword.size(), 
//		           Hmac_output, len_of_Hmac_output);
//        string K((char *)Hmac_output, len_of_Hmac_output);

        string K(search_keyword);
        if(bitmap.find(K) != bitmap.end()){
//            cout << "==================printf in search===================\n";
//            cout << K << endl;
            memcpy(output, bitmap[K], n);
//            for (int i = 0; i < n; i++)
//                printf("%x ", bitmap[K][i]);
//            cout << endl;
            
//            for (int i = 0; i < n; i++){
//                unsigned char cur_charater = *(bitmap[K] + i);
//                for (int j = 0; j < 8; j++){
//                    if ((cur_charater & (1 << 7)) != 0){
//                        result.push_back(i * 8 + j);
//                    }
//                    cur_charater <<= 1;
//                }
//            }
        }
        return 1;
    }        
};

#endif