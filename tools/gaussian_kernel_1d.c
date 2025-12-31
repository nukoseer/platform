#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        printf("Usage: %s <kernel_size> <sigma>\n", argv[0]);
        return 1;
    }

    int ksize = atoi(argv[1]);
    if (ksize % 2 == 0)
    {
        printf("Kernel size must be an odd number.\n");
        return 1;
    }

    char* end = 0;
    float sgma = strtof(argv[2], &end);
    if (argv[2] == end)
    {
        printf("Sigma could not parse.\n");
        return 1;
    }
    
    int kernel_size = ksize; // Must be odd
    float sigma = sgma;
    float* kernel = (float*)malloc(kernel_size * sizeof(float));
    float sum = 0.0f;

    for (int i = 0; i < kernel_size; i++)
    {
        kernel[i] = expf(-(i * i) / (2 * sigma * sigma));
    }

    // Normalize the kernel
    sum += kernel[0];
    for (int i = 1; i < kernel_size; i++)
    {
        sum += kernel[i] * 2;
    }

    for (int i = 0; i < kernel_size; i++)
    {
        kernel[i] /= sum;
    }

    // Print the kernel
    printf("Gaussian Kernel (1D):\n");
    for (int i = 0; i < kernel_size; i++)
    {
        printf("%12d ", i);
    }
    printf("\n");

    for (int i = 0; i < kernel_size; i++)
    {
        printf("%12f ", kernel[i]);
    }
    printf("\n");

    printf("{ ");
    for (int i = 0; i < kernel_size; i++)
    {
        if (i == kernel_size - 1)
        {
            printf("%f", kernel[i]);
        }
        else
        {
            printf("%f, ", kernel[i]);
        }
    }
    printf(" }");

    return 0;
}


