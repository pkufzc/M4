#pragma once
#include <random>
#include <stdexcept>
#include <cstdio>
#include <fstream>
#include <climits>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include <iostream>
#include <string>
#include <sstream>
#include "sketch_defs.hpp"
#include "file_path.hpp"

struct Seattle_Tuple {
    uint64_t timestamp;
    uint64_t id;
};

struct Webget_Tuple {
    uint64_t id;
    uint64_t timestamp;
};

namespace sketch {
    // Random number generation.

    /// @brief Generate a random bit.
    /// @warning This function is not thread-safe.
    bool rand_bit() {
        static std::random_device rd;
        static std::default_random_engine gen(rd());
        static std::bernoulli_distribution dis(0.5);
        return dis(gen);
    }

    /// @brief Random u32 generator.
    struct rand_u32_generator {
        /// @brief Construct a generator with given seed and max value.
        rand_u32_generator(u32 seed = 0, u32 max = UINT32_MAX)
            : gen(seed), dis(0, max) { }

        /// @brief Generate a random u32 in [0, max].
        u32 operator()() {
            return dis(gen);
        }

    private:
        std::default_random_engine gen;
        std::uniform_int_distribution<u32> dis;
    };

    // Others.

    bool f64_equal(f64 a, f64 b) {
        return std::fabs(a - b) < 1e-5;
    }


    vector<FlowItem> load_dataset(const string& dataset) {
        const char* filename;
        if(dataset=="caida") {
            filename = caida_path;
        } else if (dataset=="imc") {
            filename = imc_path;
        }
        else if (dataset == "seattle") {
            filename = seattle_path;
        }
        else if (dataset == "web") {
            filename = web_path;
        }
        else if (dataset == "criteo") {
            filename = criteo_path;
        }
        else if (dataset=="MAWI") {
            filename = mawi_path;
        } else {
            throw std::invalid_argument("unknown dataset");
        }    

        FILE* pf = fopen(filename, "rb");
        if(!pf){
            throw std::runtime_error("cannot open file");
        }

        vector<FlowItem> vec;
        double ftime = -1;
        long long fime = -1;
        char trace[30];

        if(filename == caida_path){
            while(fread(trace, 1, 21, pf)){
                uint32_t tkey = *(uint32_t*) (trace);
                double ttime = *(double*) (trace+13);
                if(ftime<0)ftime=ttime;
                vec.emplace_back((FlowItem){tkey, u32((ttime-ftime)*10000000)+1});
            }
        }else if(filename == imc_path){
            while(fread(trace, 1, 26, pf)){
                uint32_t tkey = *(uint32_t*) (trace);
                long long ttimee = *(long long*) (trace+18);
                if(fime<0)fime=ttimee;
                vec.emplace_back((FlowItem){tkey, u32((ttimee-fime)/100)+1});
            }
        }
        else if (filename == criteo_path) {
            int i = 0;
           // string temp1; 
           // string temp2;
            uint64_t temp1; 
            uint64_t temp2;
            string input;
            std::ifstream fin;        
            fin.open( criteo_path);         
            while (i<1000000)
            {
                getline(fin, input);
                if (!fin) break; 

                std::istringstream buffer(input);
                buffer >> temp1 >> temp2;
                FlowItem fi;
                fi.value = temp1;
                fi.id = temp2;
                if(fi.value!=0){
                  vec.push_back(fi);
                   /* if (i < 10) {
                        cout << temp1 << '\t' << temp2 << endl;
                        cout << fi.id << '\t' << fi.value << endl;
                    }*/
                ++i;}
            }
            cout << "datalen: " << i << endl;
        }
        else if (filename == web_path) {
            int datalen = 0;
            std::vector<uint64_t> num1;
            std::vector<uint64_t> num2;

            for (int i = 0; i < 2; ++i)
            {
                std::ifstream file(web_path);
                while (!file.eof())
                {
                    uint64_t temp1;
                    uint64_t temp2;
                    file >> temp1 >> temp2;
                    if (temp2 == 0) continue;

                    num1.push_back(temp1);
                    //num.push_back(temp2 * 123465);
                    num2.push_back(temp2);

                    datalen += 1;
                }
                file.close();
            }

            std::cout << datalen << " " << num1.size() << " " << num2.size() << "\n";

            //Webget: 9368 flow,some timestamp is 0

            Webget_Tuple* dataset = new Webget_Tuple[datalen];
            for (int i = 0; i < datalen; i++)
            {
                dataset[i].id = num1[i];
                dataset[i].timestamp = num2[i];
                FlowItem k;
                k.id = dataset[i].id;
                k.value = dataset[i].timestamp;
                vec.push_back(k);
            }

            //  int length = datalen;
          //    int running_length = length;
            delete[] dataset;
        }
        else if (filename == seattle_path) {
            int datalen = 0;
            std::vector<double> num;
            std::ifstream file(seattle_path);
            while (!file.eof())
            {
                double temp;
                file >> temp;
                num.push_back(temp * 100000);
                datalen++;
            }
            file.close();
            std::cout << datalen << " " << num.size() << "\n";
            //seattle: 688file, each file has 99*99 item
            Seattle_Tuple* dataset = new Seattle_Tuple[datalen];
            int index = 0;
            for (int filenum = 0; filenum < 688; filenum++)
            {
                for (int flow_id = 0; flow_id < 99; flow_id++)
                {
                    for (int i = 0; i < 99; i++)
                    {
                        dataset[index].id = index % (9801) * 41 + 41; //use [node i to node j] as id
                        dataset[index].timestamp = num[index];
                        index++;
                    }

                }
            }
            int length = datalen;
            int running_length = length;
            for (int i = 0; i < running_length; ++i) {
                if (dataset[i].timestamp == 0) continue;
                FlowItem k;
                k.id = dataset[i].id;
                k.value = dataset[i].timestamp;
                vec.push_back(k);
            }
        }
        else{
            while(fread(trace, 1, 21, pf)){
                uint32_t tkey = *(uint32_t*) (trace);
                long long ttime = *(long long*) (trace+13);
                if(fime<0)fime=ttime;
                vec.emplace_back((FlowItem){tkey, u32((ttime-fime)*100000)+1});
            }
        }
        fclose(pf);

        return vec;
    }

} // namespace sketch