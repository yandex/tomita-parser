#pragma once

class TRand {
    public:
        TRand();
        char* initstate(unsigned long seed, char* arg_state, long n);
        char* setstate(char* arg_state);
        void srandom(unsigned long x);
        long random();

    private:
        long randtbl[32];
        long *fptr, *rptr, *state, rand_type, rand_deg, rand_sep, *end_ptr;
};
