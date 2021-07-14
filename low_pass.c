#include <stdio.h>
#include <stdlib.h>

#include "low_pass.h"

#include <string.h>

#define ORDER 100

/*  LOW PASS FILTER IMPLEMENTING
    HAMMING WINDOW COEFFICIENTS
    
    FIR FORMULA: y(n) = sum[k: 0 - N-1]{ h(k)*x(n-k)}
    N -> Number of filter coefficients (ORDER of the filter)
    h() -> array of filter coefficient
    x() -> array of inputs

*/

int low_pass( double *input, int length)
{
    const int totalLenght = (length + (ORDER-1));
    double *buffer = malloc(totalLenght * sizeof(double));

    if(buffer == NULL){
        printf("Error in allocating input buffer\n");
        return -1;
    }

    //Erasing buffer
    for(int i = 0; i< totalLenght; i++){
        buffer[i] = 0;
    }
    
    //Copy input array from position ORDER (previous cells needed for x(n-k))
    memmove(&buffer[ORDER-1], input, length * sizeof(double));

    double coeffp[ORDER] = {0.0003, 0.0001, -0.0000, -0.0002, -0.0003, -0.0005, -0.0007, -0.0009, -0.0010, -0.0011, -0.0012, -0.0011, -0.0009, -0.0005, 0.0000, 0.0006, 0.0014,
                            0.0022, 0.0030, 0.0037, 0.0042, 0.0045, 0.0045, 0.0040, 0.0031, 0.0018, -0.0000, -0.0021, -0.0045, -0.0070, -0.0094, -0.0115, -0.0130, -0.0138,
                            -0.0136, -0.0122, -0.0095, -0.0054, 0.0000, 0.0067, 0.0145, 0.0233, 0.0326, 0.0421, 0.0514, 0.0602, 0.0681, 0.0746, 0.0794, 0.0825, 0.0835,
                            0.0825, 0.0794, 0.0746, 0.0681, 0.0602, 0.0514, 0.0421, 0.0326, 0.0233, 0.0145, 0.0067, 0.0000, -0.0054, -0.0095, -0.0122, -0.0136, -0.0138,
                            -0.0130, -0.0115, -0.0094, -0.0070, -0.0045, -0.0021, -0.0000, 0.0018, 0.0031, 0.0040, 0.0045, 0.0045, 0.0042, 0.0037, 0.0030, 0.0022, 0.0014,
                            0.0006, 0.0000, -0.0005, -0.0009, -0.0011, -0.0012, -0.0011, -0.0010, -0.0009, -0.0007, -0.0005, -0.0003, -0.0002, -0.0000, 0.0001}; //0.0003

    double acc;     // accumulator for MACs
    int n;
    int k;

    
 
    // apply the filter to each input sample
    for ( n = 0; n < length; n++ ) {
        double *buffer_init = &buffer[(ORDER-1)+n];
        acc = 0;
        for ( k = 0; k < ORDER; k++ ) {
            acc += coeffp[k] * (*buffer_init--);
        }
        input[n] = acc;
    }

    free(buffer);

    return 1;
 
}