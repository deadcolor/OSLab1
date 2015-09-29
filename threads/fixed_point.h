#define F (1 << 14)

/* x and y denote fixed point numbers.
 * n denotes an integer.
 */

int int2fp(int n);
int fp2int_zero(int x);
int fp2int_nearest(int x);
int add_fp(int x, int y);
int add_fp_int(int x, int n);
int sub_fp(int x, int y);
int sub_fp_int(int x, int n);
int mul_fp(int x, int y);
int mul_fp_int(int x, int n);
int div_fp(int x, int y);
int div_fp_int(int x, int n);


int int2fp(int n){
    return n * F
}

// rounding toward zero
int fp2int_zero(int x){
    return x / F
}

//rounding to nearest
int fp2int_nearest(int x){
    if (x >= 0) 
        return (x + F / 2) / F
    if (x <= 0) 
        return (x - F / 2) / F
}

int add_fp(int x, int y){
    return x + y
}

int add_fp_int(int x, int n){
    return x + n * F
}

int sub_fp(int x, int y){
    return x - y
}

int sub_fp_int(int x, int n){
    return x - n * F
}

int mul_fp(int x, int y){
    return ((int64_t) x) * y / F
}

int mul_fp_int(int x, int n){
    return x * n
}

int div_fp(int x1, int x2){
    return ((int64_t) x) * f / y
}

int div_fp_int(int x, int n){
    return x / n
}

