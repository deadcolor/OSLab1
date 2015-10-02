#ifndef __FIXED_POINT_H
#define __FIXED_POINT_H

#define SHIFT_FLOAT (1<<14)
//Use concept in APPENDIX B
//x,y : fixed-point numbers n is integer

int INTtoFP(int n);
int FPtoINT(int x);
int FP_ROUND(int x);
int ADD_FP(int x, int y);
int ADD_FP_INT(int x,int n);
int SUB_FP(int x, int y);
int SUB_FP_INT(int x,int n);
int MUL_FP(int x, int y);
int MUL_FP_INT(int x, int n);
int DIV_FP(int x, int y);
int DIV_FP_INT(int x, int n);


//Convert n to fixed point
int INTtoFP(int n)
{
    return n*SHIFT_FLOAT;
}

//Convert x to integer(rounding toward zero)
int FPtoINT(int x)
{
    return x/SHIFT_FLOAT;
}

//Convert x to integer(rounding to nearest)
int FP_ROUND(int x)
{
    if(x>=0)
        return (x+SHIFT_FLOAT/2) /SHIFT_FLOAT;
    else 
        return (x-SHIFT_FLOAT/2) /SHIFT_FLOAT;
}
//Add x and y
int ADD_FP(int x, int y)
{
    return x+y;
}

//Add x and n
int ADD_FP_INT(int x, int n)
{
    return x+n*SHIFT_FLOAT;
}

//Subtract y from x
int SUB_FP(int x, int y)
{
    return x-y;
}

//Subtract n from x
int SUB_FP_INT(int x, int n)
{
    return x-n*SHIFT_FLOAT;
}

//Multiply x by y
int MUL_FP(int x, int y)
{
    return ((int64_t)x)*y / SHIFT_FLOAT;
}

//Multiply x by n
int MUL_FP_INT(int x, int n)
{
    return x*n;
}

//Devide x by y
int DIV_FP(int x, int y)
{
    return ((int64_t)x)*SHIFT_FLOAT/y;
}

//Ddvide x by n
int DIV_FP_INT(int x, int n)
{
    return x/n;
}


#endif //fixed_point.h
