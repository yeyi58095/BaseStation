// RandomVar.h  �X bcc32 �ۮe���]header-only�^
#pragma once
#include <cstdlib>
#include <cmath>
#include <ctime>

namespace rv {

// �i��G�ΩT�w�ؤl�ήɶ��ؤl
inline void reseed(unsigned seed)            { std::srand(seed); }
inline void reseed_with_time()               { std::srand(static_cast<unsigned>(std::time(NULL))); }

// 0..32767 �����k�A������A�쥻�� pow(2,15) ���k
inline double uniform(double a, double b) {
    int n = std::rand() & 0x7FFF;                       // 0..32767
    return a + (b - a) * (static_cast<double>(n) / 32768.0);
}

// rate = �f (>0), mean = 1/�f
inline double exponential(double rate) {
    double x = uniform(0.0, 1.0);
    if (x <= 1e-10) x = 1e-10;
    return -std::log(1.0 - x) / rate;
}

// �ĤG�ѼƵ����u�зǮt�vstddev�]��A�쥻�@�P�^
inline double normal(double mean, double stddev) {
    double x1 = uniform(0.0, 1.0);
    double x2 = uniform(0.0, 1.0);
    if (x2 <= 1e-10) x2 = 1e-10;
    const double pi = 3.14159265358979323846;
    return mean + stddev * std::cos(2.0 * pi * x1) * std::sqrt(-2.0 * std::log(x2));
}

// ��K������ ComboBox ������
inline double sample_by_index(int idx, double p1, double p2) {
    switch (idx) {
        case 0: return uniform(p1, p2);   // Uniform(a,b)
        case 1: return exponential(p1);   // Exponential(rate)
        case 2: return normal(p1, p2);    // Normal(mean, stddev)
        default: return uniform(p1, p2);
    }
}

} // namespace rv

