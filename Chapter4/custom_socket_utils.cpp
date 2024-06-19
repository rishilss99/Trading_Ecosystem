#include "custom_socket_utils.h"

int main()
{
    std::cout << Sockets::getIfaceIP("enp0s3") << "\n";
}