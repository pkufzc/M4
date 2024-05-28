#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include "include/test/sketch_test.hpp"
#include "include/meta/dd/ddsketch.hpp"
#include "include/meta/mreq/mreq_sketch.hpp"
#include "include/meta/tdigest/tdigest.hpp"

using namespace sketch;

#if defined(TEST_MREQ)
#define METATYPE mReqSketch
#define metaname "mreq"
#elif defined(TEST_TD)
#define METATYPE TDigest
#define metaname "tdigest"
#elif defined(TEST_DD)
#define METATYPE DDSketch
#define metaname "dd"
#endif

void print_usage(char* file) {
    cout << "usage: " << file
         << " <memory> <dataset> <hash-num> <repeat> [<seed>]" << endl;
    cout << endl;

    cout << "Meaning of arguments: " << endl;
    cout << "    memory          memory in KB" << endl;
    cout << "    dataset         caida, imc, seattle, or MAWI" << endl;
    cout << "    hash-num        number of hash functions per level" << endl;
    cout << "    repeat          times of test repetitions" << endl;
    cout << "    seed            random seed, by default 0" << endl;
}

struct main_args {
    bool valid;
    u32 memory;
    string dataset;
    u32 hash_num;
    u32 repeat;
    u32 seed;
};

main_args parse_args(int argc, char* argv[]) {
    main_args args;
    args.valid = false;

    if (argc != 5 && argc != 6) {
        return args;
    }

    args.memory = stoul(argv[1]) * 1024;

    args.dataset = argv[2];
    if (args.dataset != "caida" && args.dataset != "imc" &&
        args.dataset != "MAWI" &&
        args.dataset != "seattle" &&
        args.dataset != "web" &&
        args.dataset != "criteo") {
        return args;
    }

    args.hash_num = stoul(argv[3]);

    args.repeat = stoul(argv[4]);

    args.seed = 0;
    if (argc == 6) {
        args.seed = stoul(argv[5]);
    }

    args.valid = true;
    return args;
}


void output_res(const main_args& args, const SketchTest<METATYPE>& test) {
    string output_name = static_cast<string>(res_path) + "res_" + metaname
                            + "_" + args.dataset + ".txt";
    ofstream out(output_name, ios::app);
    assert(out.is_open());

    out << "memory: " << (args.memory / 1024) << " KB" << endl;
    out << "dataset: " << args.dataset << endl;
    out << "hash_num: " << args.hash_num << endl;
    out << "repeat: " << args.repeat << endl;
    out << "seed: " << args.seed << endl;
    out << endl;

    out << "ALE of andor: " << test.ALE(ANDOR) << endl;
    out << "ALE of dleft: " << test.ALE(DLEFT) << endl;
    out << "ALE of cuckoo: " << test.ALE(CUCKOO) << endl;
    out << "APE of andor: " << test.APE(ANDOR) << endl;
    out << "APE of dleft: " << test.APE(DLEFT) << endl;
    out << "APE of cuckoo: " << test.APE(CUCKOO) << endl;
    out << "AppendTp of andor: " << test.appendTp(ANDOR) << " Mops" << endl;
    out << "AppendTp of dleft: " << test.appendTp(DLEFT) << " Mops" << endl;
    out << "AppendTp of cuckoo: " << test.appendTp(CUCKOO) << " Mops" << endl;
    out << "QueryTp of andor: " << test.queryTp(ANDOR) << " Mops" << endl;
    out << "QueryTp of dleft: " << test.queryTp(DLEFT) << " Mops" << endl;
    out << "QueryTp of cuckoo: " << test.queryTp(CUCKOO) << " Mops" << endl;
    out << endl;
}

int main(int argc, char* argv[]) {
    main_args args = parse_args(argc, argv);
    if (!args.valid) {
        print_usage(argv[0]);
        return 1;
    }

    SketchTest<METATYPE> test(args.memory, args.hash_num, args.seed, 
                   args.dataset, args.repeat);

    test.run();

    output_res(args, test);
}

#undef META
#undef meta
