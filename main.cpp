#include<unistd.h>

int main(int argc, const char *argv[])
{
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}
