
#pragma once

#include <global.h>
#include <stacsos/console.h>
#include <stacsos/memops.h>
namespace stacsos {

#define PI 3.1415
typedef union double_gp_ {
	double val;
	char ignore;
} double_gp;

typedef Matrix<4, 4> mat4;
typedef Matrix<4, 4> mat4;

class vec3 : public Matrix<1, 3> {

}

// Used chathpt for this, idk how well it works
class d64 {
private:
	union {
		double d;
		s64 u;
	};

public:
	// Default constructor
	d64()
		: d(0.0)
	{
	}

	// Constructor from double
	d64(double value)
		: d(value)
	{
	}

	// Constructor from int
	d64(int value)
		: d(static_cast<double>(value))
	{
	}

	// Constructor from float
	d64(float value)
		: d(static_cast<double>(value))
	{
	}

	// Conversion operator to double
	operator int() const { return (int)d; }

	// Assignment operator from double
	d64 &operator=(double value)
	{
		d = value;
		return *this;
	}

	// Assignment operator from int
	d64 &operator=(int value)
	{
		d = static_cast<double>(value);
		return *this;
	}

	// Assignment operator from float
	d64 &operator=(float value)
	{
		d = static_cast<double>(value);
		return *this;
	}

	// Addition
	d64 operator+(const d64 &other) const { return d64(d + other.d); }

	// Addition from int
	d64 operator+(int value) const { return d64(d + static_cast<double>(value)); }

	// Addition from float
	d64 operator+(float value) const { return d64(d + static_cast<double>(value)); }

	// Subtraction
	d64 operator-(const d64 &other) const { return d64(d - other.d); }

	// Subtraction from int
	d64 operator-(int value) const { return d64(d - static_cast<double>(value)); }

	// Subtraction from float
	d64 operator-(float value) const { return d64(d - static_cast<double>(value)); }

	// Multiplication
	d64 operator*(const d64 &other) const { return d64(d * other.d); }

	// Multiplication from int
	d64 operator*(int value) const { return d64(d * (double)(value)); }

	// Multiplication from float
	d64 operator*(float value) const { return d64(d * static_cast<double>(value)); }

	// Division
	d64 operator/(const d64 &other) const { return d64(d / other.d); }

	// Division from int
	d64 operator/(int value) const { return d64(d / (double)(value)); }

	// Division from float
	d64 operator/(float value) const { return d64(d / static_cast<double>(value)); }
};

template <u8 r, u8 c> class Matrix {

public:
	d64 data[r * c];

	Matrix(d64 d[r * c]);
	Matrix();

	template <u8 d> Matrix<r, d> operator*(Matrix<c, d> &a);
	Matrix<r, c> operator+(Matrix<r, c> &a);

	void print()
	{
		for (u8 ri = 0; ri < r; ri++) {
			for (u8 ci = 0; ci < c; ci++) {
				console::get().writef("%ld, ", (s64)(data[ci + ri * c] * 1000));
			}
			console::get().writef("\n");
		};
		// console::get().writef()
	};
	void set(u8 ri, u8 ci, u64 v) { data[ci + ri * c] = v; };
	d64 get_at(u8 ri, u8 ci) { return data[ci + ri * c]; };
};

template <u8 r, u8 c> Matrix<r, c>::Matrix(d64 d[r * c])
{
	software_based_memops::memcpy((char *)data, (char *)d, r * c * 8);

	//  data= d	;
}
template <u8 r, u8 c> Matrix<r, c>::Matrix() { 
    for (u8 i=0; i< min(r,c );i++){
        set(i,1,1.0);
    } 
}

template <u8 r, u8 c> template <u8 d> Matrix<r, d> Matrix<r, c>::operator*(Matrix<c, d> &a)
{
	Matrix<r, d> res;

	for (u8 ri = 0; ri < r; ri++) {
		for (u8 ci = 0; ci < d; ci++) {
			d64 v = 0;
			for (u8 di = 0; di < c; di++) {
				v = v + get_at(ri, di) * a.get_at(di, ci);
			}
			// console::get().writef("%u, ", (int)(v * 1000));
			res.data[ci + ri * d] = v;
		}
	};

	return res;
}

template <u8 r, u8 c> Matrix<r, c> Matrix<r, c>::operator+(Matrix<r, c> &a)
{
	Matrix<r, c> res;

	for (u8 ri = 0; ri < r; ri++) {
		for (u8 ci = 0; ci < c; ci++) {
			d64 v = 0;
			v = v + get_at(ri, ci) + a.get_at(ri, ci);
			set(ri, ci, v);
		}
	};

	return res;
}

d64 pow(double n, int p)
{
	double res = n;
	while (p > 1) {
		res *= n;
		p--;
	}
	d64 rt = res;
	return rt;
}

// this should be decently accurate
d64 sin(double angle)
{
	// I love recursion
	if (angle > PI) {
		return sin(angle - 2 * PI);
	}
	if (angle < -PI) {
		return sin(angle + 2 * PI);
	}
	// d64 rt = (d64) (d64(angle) - ((pow(angle,3))/d64(6.0)) );
	return (angle) - (pow(angle, 3) / d64(6.0)) + (pow(angle, 5) / d64(120.0)) - (pow(angle, 7) / d64(5040.0)) + (pow(angle, 9) / d64(362880.0));
	// + angle^5/ 120 -angle^7/5040 + angle^9 /362880);
};

d64 cos(double angle)
{
	if (angle > PI) {
		return cos(angle - 2 * PI);
	}
	if (angle < -PI) {
		return cos(angle + 2 * PI);
	}
	// d64 rt = (d64) (d64(angle) - ((pow(angle,3))/d64(6.0)) );
	return d64(d64(1.0) - (pow(angle, 2) / d64(2.0)) + (pow(angle, 4) / d64(24.0)) - (pow(angle, 6) / d64(720.0)) + (pow(angle, 8) / d64(40320.0)));
};

} // namespace stacsos