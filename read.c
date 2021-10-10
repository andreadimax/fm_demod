#include <stdio.h>

int main(){
    FILE* in = fopen("audio_prova.raw", "rb");
    float value;

    while (fread(&value, 4, 1,in))
    {
        printf("%f\n", value);
    }
    
}