#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include <iostream>
#include <ranges>
#include <algorithm>
#include <iomanip>
#include <random>
#include <tuple>
#include <string>

namespace ssw {

using Base     = uint8_t;
using Sequence = std::vector<Base>;
using Score    = int16_t;
using vScore   = std::vector<Score>;
using Workspace= std::vector<vScore>;
using Result   = std::tuple<size_t,size_t,Score>;

constexpr Score gap_init   = Score{-1};
constexpr Score gap_extent = Score{-1};
constexpr Score success    = Score{2};
constexpr Score miss       = Score{-1};

constexpr size_t num_bases = 4;
constexpr size_t numB = 256;
constexpr Base A = Base{0};
constexpr Base C = Base{1};
constexpr Base G = Base{2};
constexpr Base T = Base{3};



constexpr std::array<Base,num_bases> all_bases {A,C,G,T};

constexpr std::array<std::array<Score,num_bases>,num_bases> generate_score () 
{
    std::array<std::array<Score,num_bases>,num_bases> w{};
    for (Base b: all_bases) {
        w[b].fill(miss);
        w[b][b] = success;
    }
    return w;
}

constexpr auto score = generate_score();

constexpr std::array<Base,numB> generate_char_to_base() {
    std::array<Base,numB> w;
    w.fill({Base{5}}); //default char
    w['A'] = A; w['a'] = A;
    w['C'] = C; w['c'] = C;
    w['G'] = G; w['g'] = G;
    w['T'] = T; w['t'] = T;
    return w;
}

constexpr auto char_to_base = generate_char_to_base();

constexpr std::array<char,numB> generate_base_to_char() {
    std::array<char,numB> w; 
    w.fill('-');//default char
    w[A] = 'A';
    w[C] = 'C';
    w[G] = 'G';
    w[T] = 'T';
    return w;
}
constexpr auto base_to_char = generate_base_to_char();

void from_char(const std::vector<char> origin, Sequence& dest){
    dest.resize(origin.size());
    std::transform(origin.begin(), origin.end(), dest.begin(), 
    [](char c){return char_to_base[c];});
}

void from_string(const std::string& s, Sequence& dest) {
    dest.resize(s.size());
    std::transform(s.begin(), s.end(), dest.begin(),
    [=](char c){return char_to_base[c];});
}

char inline to_char(const Base b) {
    return base_to_char[b];
}

void print_seq(const Sequence& seq, const char sep) {
    for (Base s : seq) std::cout << std::setw(1) << to_char(s);
    std::cout << std::endl;
}

void print_score(const vScore& vS) {
    for (Score s : vS) std::cout << std::setw(3) << s << " ";
    std::cout << std::endl;
}

void print_workspace(const Workspace& ws) {
    for (vScore vs: ws) print_score(vs);
}

Sequence generate_random_sequence(const size_t length, const size_t seed) {
    Sequence s;
    s.reserve(length);
    std::random_device rd;
    std::mt19937 gen(seed);
    std::uniform_int_distribution<uint8_t> dist(A,T);

    std::generate_n(std::back_inserter(s), length, 
        [&](){return dist(gen);});

    return s;
}

Sequence generateSeq(size_t length) {
    Sequence s(length);
    for (size_t i=0; i<length; ++i) s[i] = i % 4;
    return s;
}


Result smith_waterman(const Sequence& query, const Sequence& database,
        Workspace& ws)
{
    Score max_score = Score{0};
    size_t max_i=1, max_j=1;
    for (auto i=1; i<query.size(); ++i) {
        for (auto j=1; j<database.size(); ++j) {
            Base rowv = query[i]; 
            Base colv = database[j];
            ws[i][j] = std::max<int>({ws[i-1][j-1]+score[rowv][colv], 
            ws[i-1][j] + gap_init, ws[i][j-1] + gap_init, 0});
            if (ws[i][j] >= max_score) {
                max_score = ws[i][j];
                max_i = i; max_j=j;
            }
        }
    }
    return std::make_tuple(max_i-1,max_j-1,max_score);
    //std::cout << "======== SMITH WATERMAN =========\n";
    //std::cout << "MAX VAL FOUND " << max_score << std::endl;
    //std::cout << "POSITION AT ROW=" << max_i-1 << " COL=" << max_j-1 << std::endl;
    //std::cout << "=================================\n";
}


void backtrack(const Workspace& ws, 
    const Sequence& database, const Sequence& query,
    Sequence& align_database, Sequence& align_query) {

    size_t max_i=0, max_j=0;
    for (size_t i=0; i < query.size() ; ++i) {
        for (size_t j=0; j < database.size(); ++j) {
            if (ws[i][j] > ws[max_i][max_j]) {
                max_i=i; max_j=j;
            }
        }
    }

    //std::cout << "Max row=" << max_i << " col=" << max_j << " v= " << ws[max_i][max_j] << std::endl; 

    size_t i = max_i, j = max_j;
    Score v = ws[i][j];
    while (v > 0 && i > 0 && j > 0)
    {
        //std::cout << "i=" << i << " j=" << j << " q[i]=" << static_cast<uint32_t>(query[i]) 
        //    << " d[j]=" << static_cast<uint32_t>(database[j])
        //    << " w[i][j]=" << ws[i][j];
        if (ws[i-1][j-1]+score[query[i]][database[j]] == v)
        {
            //std::cout << "<-- main diag" << std::endl;
            if (query[i] == database[j]) {
                align_query.push_back(query[i]);
                align_database.push_back(database[j]); 
            } else {
                align_query.push_back('-');
                align_database.push_back('-');
            }
            --i; --j;
        } else if (ws[i][j-1]+gap_init == v) {
            //std::cout << "<-- query stride" << std::endl;
            align_query.push_back('E');
            align_database.push_back(database[j]);
            --j;
        } else if (ws[i-1][j]+gap_init == v) {
            //std::cout << "<-- database stride" << std::endl;
            align_query.push_back(query[i]);
            align_database.push_back('E');
            --i;
        }
        v = ws[i][j];
    }
    std::reverse(align_query.begin(),align_query.end());
    std::reverse(align_database.begin(),align_database.end());
}


}