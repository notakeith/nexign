#include <cstddef>
#include <iostream>

struct Mixed {
    char a;
    int b;
    char c;
    double d;
    short e;
};

struct __attribute__((packed)) PackedMixed {
    char a;
    int b;
    char c;
    double d;
    short e;
};

int main() {
    PackedMixed p{};
    double* field = &p.d;
    std::cout << "Mixed: " << sizeof(Mixed) << " bytes\n";
    std::cout << "PackedMixed: " << sizeof(PackedMixed) << " bytes\n";
    std::cout << "d offset: " << offsetof(Mixed, d) << " / "
              << offsetof(PackedMixed, d) << "\n";
    std::cout << "Address of packed d: " << static_cast<void*>(field) << '\n';
}
