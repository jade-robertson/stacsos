
#pragma once
#include <global.h>
#include <stacsos/console.h>
#include <stacsos/memops.h>
namespace stacsos {


template <u8 r, u8 c> class Matrix {

public:
	double data[r * c];

	Matrix(double d[r * c]);
	Matrix();
	template <u8 d> Matrix<r, d> operator*(Matrix<c, d> &a);
	Matrix<r, c> operator+(Matrix<r,c> &a);

	void print()
	{
		for (u8 ri = 0; ri < r; ri++) {
			for (u8 ci = 0; ci < c; ci++) {
				console::get().writef("%u, ", (int)(data[ci + ri * c] * 1000));
			}
			console::get().writef("\n");
		};
		// console::get().writef()
	};
    void set(u8 ri, u8 ci, u64 v){
        data[ci + ri * c] = v;
    };
    u64 get_at(u8 ri, u8 ci){
        return  * ( u64 * )&(data[ci + ri * c]);
    };
};

template <u8 r, u8 c> Matrix<r, c>::Matrix(double d[r * c])
{
	software_based_memops::memcpy((char *)data, (char *)d, r * c * 8);

	//  data= d	;
}
template <u8 r, u8 c> Matrix<r, c>::Matrix() { }

template <u8 r, u8 c> template <u8 d> Matrix<r, d> Matrix<r, c>::operator*(Matrix<c, d> &a)
{
	Matrix<r, d> res;

	for (u8 ri = 0; ri < r; ri++) {
		for (u8 ci = 0; ci < d; ci++) {
			double v = 0;
			for (u8 di = 0; di < c; di++) {
				u64 a1 =get_at(ri, di);
				u64 b2 =a.get_at(di,ci);
				v += (*(double*) &(a1) ) * (*( double * ) &(b2));
                // v += data[di + ri * c] * a.data[ci + di * d];

			}
			console::get().writef("%u, ", (int)(v * 1000));
			res.data[ci + ri * d] = v;
		}
		// console::get().writef("\n");
	};

	return res;
}


template <u8 r, u8 c> Matrix<r, c> Matrix<r, c>::operator+(Matrix<r,c > &a)
{
	Matrix<r, c> res;

	for (u8 ri = 0; ri < r; ri++) {
		for (u8 ci = 0; ci < c; ci++) {
			double v = 0;
            // res.data
				u64 a1 =get_at(ri, ci);
				u64 b2 =a.get_at(ri,ci);
				v += (*(double*) &(a1) ) * (*( double * ) &(b2));
				set(ri,ci,v);

			}
			// console::get().writef("%u, ", (int)(v * 1000));
			// res.data[ci + ri * d] = v;
		
		// console::get().writef("\n");
	};

	return res;
}

u64 sin(double angle){
	
}


} // namespace stacsos