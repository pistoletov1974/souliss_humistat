
    void float32(float* __restrict out, const uint16_t in);

    // float16
    // Martin Kallman
    //
    // Fast single-precision to half-precision floating point conversion
    //  - Supports signed zero, denormals-as-zero (DAZ), flush-to-zero (FTZ),
    //    clamp-to-max
    //  - Does not support infinities or NaN
    //  - Few, partially pipelinable, non-branching instructions,
    //  - Core opreations ~10 clock cycles on modern x86-64
    void float16(uint16_t* __restrict out, const float in);