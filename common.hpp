#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include <iostream>
#include <ranges>
#include <algorithm>
#include <iomanip>
#include <random>

namespace ssw {

using Base     = uint8_t;
using Sequence = std::vector<Base>;
using Score    = int16_t;
using vScore   = std::vector<Score>;
using Workspace= std::vector<vScore>;

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
    Sequence s(length, Base{0});
    std::random_device rd;
    std::mt19937 gen(seed);
    std::uniform_int_distribution<uint8_t> dist(A,T);

    std::generate_n(std::back_inserter(s), length, 
        [&](){return dist(gen);});

    return s;
}

}