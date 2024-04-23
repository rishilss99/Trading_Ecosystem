#include <iostream>
#include <cstdint>
#include <array>

int volatileFunc(volatile int &x) {
    x = 10;
    x = 20;
    auto y = x;
    y = x;
    return y;
}

long strictAliasing(long *a, int *b) {
    *a = 5;
    *b = 6;
    return *a;
}

struct booleanStruct{
    uint32_t var:1;
};

bool boolean(booleanStruct &a, booleanStruct &b) {
    bool c = a.var && b.var;
    return c;
}

void pointerAliasing(int *__restrict a, int *__restrict b, int n) {
    for(int i = 0; i<n; i++)
    {
        a[i] = *b;
    }
}

void loopUnrolling(int *a) {
    for(int i = 1; i < 5; ++i)
      a[i] = a[i-1] + 1;
}

struct PoorlyAlignedData
{
    char c;
    uint16_t u1;
    double d;
    int16_t i1;
};

int strengthReduction(int x)
{
    int divPrice = 128;
    return x * divPrice;
}

auto factorial(unsigned n) -> unsigned
{
    return n ? n * factorial(n-1) : 1;
}

void loopInvariantCode(std::array<int, 100> &a)
{
    auto doSomething = [](int r) {return 3 * r;};
    for(int i =0; i<100; i++)
    {
        a[i] = doSomething(1) + i;
    }
}

int main()
{
    factorial(12);
    long data = 2;
    std::cout << strictAliasing(&data, (int*)(&data));
    booleanStruct c{1};
    booleanStruct b{0};
    std::cout << boolean(c, b) << "\n";
    constexpr int conditionCheck = 2;
    if constexpr (conditionCheck == 2)
    {
        std::cout << "Compile-time branching" << "\n";
    }  
    std::cout << offsetof(PoorlyAlignedData, c) << "\n";
    std::cout << offsetof(PoorlyAlignedData, u1) << "\n";
    std::cout << offsetof(PoorlyAlignedData, d) << "\n";
    std::cout << offsetof(PoorlyAlignedData, i1) << "\n";
    std::cout << sizeof(PoorlyAlignedData) << "\n";
    std::array<int, 100> a;
    for(auto i = 0; i < 100; ++i)
    {
        a[i] = i * 10 + 12;
        std::cout << a[i];
    }
    loopInvariantCode(a);
}
        
    