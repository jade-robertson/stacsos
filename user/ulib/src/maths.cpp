#include <stacsos/maths.h>

using namespace stacsos;





d64 stacsos::pow(d64 n, int p)
{
	double res = n.d;
	while (p > 1) {
		res *= n.d;
		p--;
	}
	d64 rt = res;
	return rt;
}

// this should be decently accurate
d64 stacsos::sin(d64 angle)
{
	// I love recursion
	if (angle.d > PI) {
		return sin(angle.d - 2 * PI);
	}
	if (angle.d < -PI) {
		return sin(angle.d + 2 * PI);
	}
	// d64 rt = (d64) (d64(angle) - ((pow(angle,3))/d64(6.0)) );
	return d64(d64(angle) - (pow(angle, 3) / d64(6.0))) + (pow(angle, 5) / d64(120.0)) - (pow(angle, 7) / d64(5040.0)) + (pow(angle, 9) / d64(362880.0));
	// + angle^5/ 120 -angle^7/5040 + angle^9 /362880);
};

d64 stacsos::cos(d64 angle)
{
	if (angle.d > PI) {
		return cos(angle.d - 2 * PI);
	}
	if (angle.d < -PI) {
		return cos(angle.d + 2 * PI);
	}
	// d64 rt = (d64) (d64(angle) - ((pow(angle,3))/d64(6.0)) );
	return d64(d64(1.0) - (pow(angle, 2) / d64(2.0)) + (pow(angle, 4) / d64(24.0)) - (pow(angle, 6) / d64(720.0)) + (pow(angle, 8) / d64(40320.0)));
};
d64 stacsos::tan(d64 angle)
{
	return sin(angle)/cos(angle);
};
d64 stacsos::sqroot(d64 square)
{
    d64 root=square/3;
    int i;
    if (square.d <= 0.0) return 0.0;
    for (i=0; i<32; i++)
        root = (root + square / root) / 2;
    return root;
};

d64 stacsos::abs(d64 a){
	return  (a.d>0.0) ? a : d64(-a.d);
};
Transform stacsos::translation_mat(Vec3 p ){
	Transform t = Transform();
	t.set(3, 0, p.x());
	t.set(3, 1, p.y());
	t.set(3, 2, p.z());
	return t;
}

Transform stacsos::rotate_x_mat(d64 theta ){
	Transform t = Transform();
	t.set(1, 1, cos(theta));
	t.set(1, 2, -(sin(theta).d));
	t.set(2, 1, sin(theta));
	t.set(2, 2, cos(theta));
	return t;
}

Transform stacsos::rotate_y_mat(d64 theta ){
	Transform t = Transform();
	t.set(0, 0, cos(theta));
	t.set(0, 2, sin(theta));
	t.set(2, 0, -(sin(theta)).d);
	t.set(2, 2, cos(theta));
	return t;
}

Transform stacsos::rotate_z_mat(d64 theta ){
	Transform t = Transform();
					// stacsos::console::get().writef("%ld, ", (s64)(theta* 1000));

	t.set(0, 0, cos(theta));
	t.set(1, 0, sin(theta));
	t.set(0, 1, -(sin(theta)).d);
	t.set(1, 1, cos(theta));
	return t;
}