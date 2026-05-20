//
// SPDX-FileCopyrightText: Copyright 2024-2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
//
// SPDX-License-Identifier: Apache-2.0
//

#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "benchmark/matmul/matmul_registry.hpp"
#include "kai/kai_common.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif  // __GNUC__

#include <benchmark/benchmark.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

namespace {
void print_usage(char* name) {
    std::ostringstream oss;
    oss << "Usage:\n";
    oss << "\t" << name << " -m 13 -n 17 -k 18 -b 32\n";
    oss << "Options:\n";
    oss << "\t-m,-n,-k";
    oss << "\tMatrix dimensions\n";
    oss << "\t-b";
    oss << "\t\t\t(Optional) Block size for blockwise quantization\n";
    oss << "For additional options:\n";
    oss << "\t--help\n";
    std::cerr << oss.str() << "\n";
}
}  // namespace

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);

    std::ostringstream oss;
    oss << "KleidiAI version: v" << kai_get_version() << "\n";

    bool mflag = false;
    bool nflag = false;
    bool kflag = false;
    bool bflag = false;

    size_t m = 1;
    size_t n = 1;
    size_t k = 1;
    size_t bl = 32;

    int opt;
    while ((opt = getopt(argc, argv, "m:n:k:b:")) != -1) {
        switch (opt) {
            case 'm':
                m = atoi(optarg);
                mflag = true;
                break;
            case 'n':
                n = atoi(optarg);
                nflag = true;
                break;
            case 'k':
                k = atoi(optarg);
                kflag = true;
                break;
            case 'b':
                bl = atoi(optarg);
                bflag = true;
                break;
            case '?':
                // Fallthrough
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!mflag || !nflag || !kflag) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!bflag) {
        oss << "Optional argument -b not specified. Defaulting to block size " << bl << "\n";
    }
    std::cerr << oss.str();

    kai::benchmark::RegisterMatMulBenchmarks({m, n, k}, bl);

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
