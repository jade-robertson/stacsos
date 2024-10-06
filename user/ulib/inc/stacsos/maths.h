
#pragma once

#include <global.h>
#include <stacsos/console.h>
#include <stacsos/memops.h>
namespace stacsos {

#define PI 3.1415

// typedef Matrix<4, 4> mat4;
// typedef Matrix<4, 4> mat4;

// Evil hack to avoid using flaoting point registers
// Should probably ask spink about this at some point
//  Used chathpt for this, idk how well it works
class d64 {


public:
	union {
		double d;
		s64 u;
	};

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
	operator u8() const { return (u8)d; }
	operator s64() const { return (s64)d; }
	// operator double() const { return (double)d; }

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
	d64 operator+(double value) const { return d64(d + static_cast<double>(value)); }

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
d64 pow(d64 n, int p);

d64 sin(d64 angle);

d64 cos(d64 angle);
d64 tan(d64 angle);

template <u8 r, u8 c> class Matrix {

public:
	d64 data[r * c];

	Matrix(d64 d[r * c]);
	Matrix();

	template <u8 d> Matrix<r, d> operator*(const Matrix<c, d> &a) const;
	Matrix<r, c> operator*(const d64 &a) const;
	Matrix<r, c> operator+(const Matrix<r, c> &a)const;

	void print()
	{
		for (u8 ri = 0; ri < r; ri++) {
			for (u8 ci = 0; ci < c; ci++) {
				stacsos::console::get().writef("%ld, ", (s64)(data[ci + ri * c] * 1000));
			}
			console::get().writef("\n");
		};
		// console::get().writef()
	};
	void set(u8 ri, u8 ci, d64 v) { data[ci + ri * c] = v; };
	d64 get_at(u8 ri, u8 ci) const { return data[ci + ri * c]; };
};

template <u8 r, u8 c> Matrix<r, c>::Matrix(d64 d[r * c])
{
	software_based_memops::memcpy((char *)data, (char *)d, r * c * 8);

	//  data= d	;
}
template <u8 r, u8 c> Matrix<r, c>::Matrix() { 
    for (u8 i=0; i< min(r,c );i++){
        set(i,i,1.0);
    } 
}
template <u8 r, u8 c>  Matrix<r, c> Matrix<r, c>::operator*(const d64&a) const {
		Matrix<r, c> res;

	for (u8 ri = 0; ri < r; ri++) {
		for (u8 ci = 0; ci < c; ci++) {
			res.set(ri, ci, get_at(ri, ci) * a);
		}
	};

	return res;

}

template <u8 r, u8 c> template <u8 d> Matrix<r, d> Matrix<r, c>::operator*(const Matrix<c, d> &a) const
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

template <u8 r, u8 c> Matrix<r, c> Matrix<r, c>::operator+(const Matrix<r, c> &a) const
{
	Matrix<r, c> res;

	for (u8 ri = 0; ri < r; ri++) {
		for (u8 ci = 0; ci < c; ci++) {
			d64 v = 0;
			v = v + get_at(ri, ci) + a.get_at(ri, ci);

			res.set(ri, ci, v);
		}
	};

	return res;
}

// class Vec4 : public Matrix<1, 4>{};
class Vec3 : public Matrix<1, 3> {
public:
	Vec3(d64 x, d64 y, d64 z)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
	};
	Vec3(double x, double y, double z)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
	};
	Vec3(Matrix<1, 3> m)
	{
		data[0] = m.get_at(0,0);
		data[1] =  m.get_at(0,1);
		data[2] =  m.get_at(0,2);
	};
	d64 dot(Vec3 other){
		return (x() *other.x()) + (y() *other.y()) + (z() *other.z())  ;
	}
	d64 x() const { return data[0]; };
	d64 y() const { return data[1]; };
	d64 z() const { return data[2]; };
};
class Vec4 : public Matrix<1, 4> {
public:
	Vec4(d64 x, d64 y, d64 z, d64 w)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
		data[3] = w;
	};
	Vec4(double x, double y, double z, double w)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
		data[3] = w;
	};
	Vec4(Vec3 v, d64 w)
	{
		data[0] = v.x();
		data[1] = v.y();
		data[2] = v.z();
		data[3] = w;
	};
	Vec4(Matrix<1, 4> m)
	{
		data[0] = m.data[0];
		data[1] = m.data[1];
		data[2] = m.data[2];
		data[3] = m.data[3];
	};
	d64 x() const { return data[0]; };
	d64 y() const { return data[1]; };
	d64 z() const { return data[2]; };
	d64 w() const { return data[3]; };

	Vec3 toVec3() { return Vec3(data[0]/data[3], data[1]/data[3], data[2]/data[3]); }
};

// 4 by 4 matrix, where the
class Transform : public Matrix<4, 4> {
public:
	Transform(){
		
	}
	void translate(Vec3 p)
	{
		set(0, 3, get_at(0, 3) + p.x());
		set(1, 3, get_at(1, 3) + p.y());
		set(2, 3, get_at(2, 3) + p.z());

		set(3, 0, get_at(3, 0) + p.x());
		set(3, 1, get_at(3, 1) + p.y());
		set(3, 2, get_at(3, 2) + p.z());
	};
	void scale() const {
		// return data[1];
	};
	// Vector p represents n radian rotation around axis, applied
	void rotate_x(d64 theta) const {
		// return data[2];
	};

	// Vector p represents n radian rotation around axis, applied
	void rotate_y(d64 theta) const {
		// return data[2];
	};

	// Vector p represents n radian rotation around axis, applied
	void rotate_z(d64 theta) const {
		// return data[2];
	};

	operator Matrix<4,4>() { return Matrix<4,4>(data);};

};

Transform translation_mat(Vec3 p);
Transform rotate_x_mat(d64 theta);
Transform rotate_y_mat(d64 theta);
Transform rotate_z_mat(d64 theta);









} // namespace stacsos

