#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <math.h>
#include <map>            // std::map
#include <vector>         // std::vector 
#include <sys/time.h>     // gettimeofday
#include <stack>          // std::stack
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <stdexcept>

#include "PHIndex.h"
#include "BFIndex.h"
#include "AES.h"
#include "BMIndex.h"


using namespace std;


/*
	srcStr:原始string
	Keyword:分割符
	destVect:分割后放入容器中
 */ 
int splitStringToVect(const string & srcStr, vector<string> & destVect, const string & Keyword)  
{  
	string Tempstr;			//to store current substring
	int Pos = 0;			
	int Templen;		
	while(1)
	{
		Templen = srcStr.find(Keyword, Pos) - Pos;		
		if (Templen < 0)								
			break;
        else if (Templen > 0){
            Tempstr = srcStr.substr(Pos, Templen);			
			destVect.push_back(Tempstr);
        }
		Pos += Templen + Keyword.length();				
	}
    if(Pos < srcStr.length())
        destVect.push_back(srcStr.substr(Pos, srcStr.length() - Pos)); //The last substring
    return destVect.size();  
}


void readKeywords(vector<vector<string> > &attributes_list, char* file_name, int records_size, int attributes_size){
    string line;
    ifstream readfile(file_name);
    int count = 0;
    if(readfile.is_open()){
        while(count != records_size && getline(readfile, line, '\n')){
            if(line.find('#') != -1) throw invalid_argument("find a # in file\n");
            if (line[line.size() - 1] == 13)
                line.erase(line.end()  - 1, line.end());
            vector<string> keywords;
            splitStringToVect(line, keywords, " ");
            if (keywords.size() < attributes_size + 1)
                continue;
            attributes_list.push_back(keywords);
            count ++;
        }
        readfile.close();
    }
    else
        printf("file open error\n");
    return;
}

int keywords_to_str(vector<vector<string>> &records, int attributes_size, vector<string> &string_set){

    for (int j = 1; j < attributes_size + 1; j++){

        int total_len = 0;
        string keywords_string;
        
        for(int i = 0; i < records.size(); i++)
            total_len += 2 * (records[i][j].size());

        keywords_string.resize(total_len + 2 * records.size());
        
        int current_position = 0;
        for(int i = 0; i < records.size(); i++){
            keywords_string.replace(current_position, records[i][j].size(), records[i][j]);
            current_position += records[i][j].size();
            keywords_string.replace(current_position, 1, "_");
            current_position ++;
            keywords_string.replace(current_position, records[i][j].size(), records[i][j]);
            current_position += records[i][j].size();
            keywords_string.replace(current_position, 1, "#");
            current_position ++;
        }
        
        string_set.push_back(keywords_string);
    }
    return 1;
}

struct time_count{
    float total_time;
    int test_num;
};


// Test the proposed scheme
#define AttributesSize 5
int TestNewSolution(char *file_name)
{
  
    // initial AES key
    unsigned char aes_key[32];
    unsigned char records_key[32];
    RAND_bytes(aes_key, 32);
    RAND_bytes(records_key, 32);

    // time calculation
    struct timeval time1, time2;
    float evaluate_time = 0;
    
    // read keywords to keywords_list from the file
    vector<vector<string>> records;
    readKeywords(records, file_name, 2000, AttributesSize); 


    // transfer records to a set of string
    vector<string> string_set;
    keywords_to_str(records, AttributesSize, string_set);
    

    // Build a set of position heap
    vector<PositionHeap *> keywords_index;
    for (int i = 0; i < AttributesSize; i++){
        PositionHeap *heap = new PositionHeap(string_set[i].c_str(), aes_key, records, i + 1);
        keywords_index.push_back(heap);
    }
   

    // Input query keywords
//    vector<string> query_keywords;
//    vector<vector<string> > matching_keywords;
//    for (int i = 0; i < AttributesSize; i++){
//        string query_keyword;
//        cout << "input keyword " << i << ":";
//        getline(cin, query_keyword);
//        query_keywords.push_back(query_keyword);
//    }


//    map<int,struct time_count> test_count;
    vector<vector<string>> matching_keywords;
    for(int j = 0; j < records.size(); j++){
        
        // Multi-substring query
        for (int i = 0; i < AttributesSize; i++){            
            vector<string> cur_keywords = keywords_index[i]->search(records[j][i+1].c_str(), aes_key);
            matching_keywords.push_back(cur_keywords);
        }
    }

    
//    for (int i = 0; i < matching_keywords.size(); i++){
//        cout << "Attributes " << i << ": ";
//        for (int j = 0; j < matching_keywords[i].size(); j++){
//            cout << matching_keywords[i][j] << " ";
//        }
//        cout << endl;
//    }

    
    // Build a set of bitmap index
    vector<BMIndex *> records_index;
    for (int i = 0; i < AttributesSize; i++){
        BMIndex *cur_record_index = new BMIndex(records, i+1, records_key);
        records_index.push_back(cur_record_index);
    }
 
    
    gettimeofday(&time1,NULL);
    for (int z = 0; z < records.size(); z++){
        //Multi-keyword query
        unsigned char * all_records_bitmap = new unsigned char[records.size()/8 + 1];
        memset(all_records_bitmap, 0, records.size()/8 + 1);
        unsigned char * cur_records_bitmap = new unsigned char[records.size()/8 + 1];
        memset(cur_records_bitmap, 0, records.size()/8 + 1);
        for (int i = 0; i < AttributesSize; i++){
            records_index[i]->search(records[z][i + 1], aes_key, cur_records_bitmap);
            for (int j = 0; j < records.size()/8 + 1; j++){
                all_records_bitmap[j] |= cur_records_bitmap[j];
            }
        }

        vector<int> matching_records;
        for (int i = 0; i < records.size()/8 + 1; i++){
            unsigned char cur_charater = *(all_records_bitmap + i);
            for (int j = 0; j < 8; j++){
                if ((cur_charater & (1 << 7)) != 0){
                    matching_records.push_back(i * 8 + j);
                }
                cur_charater <<= 1;
            }
        }
        
        delete all_records_bitmap;
        delete cur_records_bitmap;
    }
    gettimeofday(&time2,NULL);

    
    //msec
    evaluate_time =  1000 * ((time2.tv_sec-time1.tv_sec)+((double)(time2.tv_usec-time1.tv_usec))/1000000);
    printf("records_size(): %d  evaluate_time: %f\n",  records.size(), evaluate_time/records.size());
    
//    cout << "mathcaingrecpd.size():" << matching_records.size() << endl;
//    for (int i = 0; i < matching_records.size(); i++){
//        cout << matching_records[i] << " ";
//    }
//    cout << endl;
    
    return 0;  
    
}



// test naive solution
int TestNaiveSolution(char *file_name)
{   
    // time calculation
    struct timeval time1, time2;
    float evaluate_time = 0;
    
    // Read keywords to keywords_list from the file
    vector<vector<string>> records;
    readKeywords(records, file_name, 2000, AttributesSize); 

    // Initial AES key
    unsigned char aes_key[32];
    RAND_bytes(aes_key, 32);

//    // Build bloom filters   
    vector<vector<BFIndex *>> index;
    for (int i = 0; i < records.size(); i++){
        vector<BFIndex *> row;
        for (int j = 0; j < AttributesSize; j++){
            row.push_back(new BFIndex(records[i][j + 1], aes_key));
        }
        index.push_back(row);
    }
   
/*
    
    // Input query keywords
    vector<string> query_keywords;
    for (int j = 0; j < AttributesSize; j++){
        string query_keyword;
        cout << "input keyword " << j << ":";
        getline(cin, query_keyword);
        query_keywords.push_back(query_keyword);
    }
*/
    // Multi-substring query
    map<int,struct time_count> test_count;
    for (int z = 0; z < records.size(); z++){
         // Search
//        vector<string> matching_sets[AttributesSize];
         
        for (int j = 0; j < AttributesSize; j++){
            cout << "z:" << z << ", " << "j:" << j << endl;
            int count = 0;
            gettimeofday(&time1,NULL);
            for (int i = 0; i < records.size(); i++){
                string keyword = index[i][j] -> search(records[z][j + 1], aes_key);
                if (keyword.size() > 0)
//                    matching_sets[j].push_back(keyword);
                    count++;
            }
            gettimeofday(&time2,NULL);
            
            //msec
            evaluate_time = 1000 * ((time2.tv_sec-time1.tv_sec)+((double)(time2.tv_usec-time1.tv_usec))/1000000);
            if (test_count.count(count)==0){
                time_count map_value = {evaluate_time, 1};
                test_count[count] = map_value;
            }
            else{
                test_count[count].total_time += evaluate_time;
                test_count[count].test_num += 1;
            }
        }
    }

    for(auto itr = test_count.begin(); itr != test_count.end(); itr++)
        cout << "matching_keywords size: " << (itr)->first << ", " << "count: " << (itr)->second.test_num << ", "<<
        "time: " << (itr)->second.total_time/(itr)->second.test_num << endl;
    
/*
    for (int j = 0; j < AttributesSize; j++){
        cout << "attribute:" << j << endl;
        for (int i = 0; i < matching_sets[j].size(); i++)
            cout << matching_sets[j][i] << " ";
        cout << endl;
    }
    */
/*
    // Intersect all the sets
    map<string, int> intersect_set;
    for (int j = 0; j < AttributesSize; j++){
        for (int i = 0; i < matching_sets[j].size(); i++){
            if (intersect_set.find(matching_sets[j][i]) == intersect_set.end())
                intersect_set[matching_sets[j][i]] = 1;
            else
                intersect_set[matching_sets[j][i]] += 1;
        }
    }
*/

    // Multi-keyword query
    
    return 0;
}

int main(int argc, char * argv[])
{
    // the format must be unix style (ending style: '\n') 
//    char file_name[30] = "./Testfile/test";
    char file_name[50] = "./Testfile/records_with_5_attributes";
    
//    TestNewSolution(file_name);;
    TestNaiveSolution(file_name);

    return 0;
}


