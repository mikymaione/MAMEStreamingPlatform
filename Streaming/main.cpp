#include <cstdio>

using namespace std;

int main()
{
    FILE *ffmpeg_in = popen("ffmpeg -i /dev/stdin -f h264 -c copy ...", "w");

    return 0;
}