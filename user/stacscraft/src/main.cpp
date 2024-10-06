/* SPDX-License-Identifier: MIT */

/* StACSOS - stacscraft
 *
 * Copyright (c) Jade Robertson 2022
 * with reference to the mandelbrot utility
 */

/*
 * A program that prints a rather crude version of the Mandelbrot fractal to the StACSOS terminal.
 */

#include <global.h>
#include <stacsos/atomic.h>
#include <stacsos/console.h>
#include <stacsos/helpers.h>
#include <stacsos/list.h>
#include <stacsos/maths.h>
#include <stacsos/memops.h>
#include <stacsos/objects.h>
#include <stacsos/threads.h>
#include <stacsos/user-syscall.h>
// #include "main.h"
using namespace stacsos;

struct vec2i {
	int x, y;
	vec2i operator+(const vec2i &a) const { return vec2i(a.x + x, a.y + y); }
	vec2i operator-(const vec2i &a) const { return vec2i(x - a.x, y - a.y); }
	vec2i operator*(const double &a) const { return vec2i(a * x, a * y); }
};

struct vertex {
	Vec3 points[3];
	d64 depth;
};

struct triangle {
	Vec3 points[3];
};
struct object3d {
	list<triangle> tris;
	Transform trans;
};
struct colour {
	u8 r;
	u8 g;
	u8 b;
};
enum Mode { RENDER, WIREFRAME, NORMAL };
Mode RENDER_MODE;
const colour WHITE = colour(255, 255, 255);
const colour BLACK = colour(0, 0, 0);
const colour RED = colour(255, 0, 0);
const colour GREEN = colour(0, 255, 0);
const colour BLUE = colour(0, 0, 255);

#define ORIGIN Vec3(0, 0, 0)
#define UP Vec3(0, 1.0, 0)
#define DOWN Vec3(0, -1.0, 0)
#define LEFT Vec3(-1.0, 0, 0)
#define RIGHT Vec3(1.0, 0, 0)
#define FORWARD Vec3(0, 0, -1.0)
#define BACK Vec3(0, 0, 1.0)
const int WIDTH = 640;
const int HEIGHT = 480;
u32 frame_buffer[HEIGHT][WIDTH];
u32 depth_buffer[HEIGHT][WIDTH];
list<triangle> tris;
list<object3d *> objects;
object3d obj1;
object *fb;
atomic_u64 input_lock = 0;
static void send()
{
	// u32 u = (r<< 16) |(g << 8) | b;
	fb->pwrite((const char *)frame_buffer, sizeof(frame_buffer), 0);
}

static void set_pixel(u16 x, u16 y, colour c)
{
	if (x > WIDTH - 1 | y > HEIGHT - 1)
		return;
	u32 u = (c.r << 16) | (c.g << 8) | c.b;
	frame_buffer[y][x] = u;
}

// screen uvs vary from -1 to 1
static vec2i screen_uv_to_pixel(Vec3 in) { return vec2i(int(((in.x() + 1.0) / 2) * WIDTH), int(((in.y() + 1.0) / 2) * HEIGHT)); }

static void line(vec2i start, vec2i end, colour c)
{
	for (double t = 0.0; t < 1; t += 0.002) {
		int x = start.x + (end.x - start.x) * t;
		int y = start.y + (end.y - start.y) * t;
		set_pixel(x, y, c);
	}
}

// Draw triangle, in screen pixel space
static void draw_triangle(vec2i p0, vec2i p1, vec2i p2, colour c)
{
	if (p0.y == p1.y && p0.y == p2.y)
		return;
	// Sort the vertices, t0 lower
	if (p0.y > p1.y)
		swap(p1, p0);
	if (p0.y > p2.y)
		swap(p0, p2);
	if (p1.y > p2.y)
		swap(p1, p2);
	int total_height = p2.y - p0.y;
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > p1.y - p0.y || p1.y == p0.y;
		int segment_height = second_half ? p2.y - p1.y : p1.y - p0.y;
		double alpha = (double)i / total_height;
		double beta = (double)(i - (second_half ? p1.y - p0.y : 0)) / segment_height;
		vec2i A = p0 + (p2 - p0) * alpha;
		vec2i B = second_half ? p1 + (p2 - p1) * beta : p0 + (p1 - p0) * beta;
		if (A.x > B.x)
			swap(A, B);
		for (int j = A.x; j <= B.x; j++) {
			set_pixel(j, p0.y + i, c); // attention, due to int casts t0.y+i != A.y
		}
	}
}
static Matrix<4, 4> proj_matrix(d64 far, d64 near, d64 fov)
{
	Matrix<4, 4> proj_mat = Matrix<4, 4>();
	d64 scale = d64(1.0) / tan(fov * d64(0.5) * d64(PI) / d64(180.0));

	proj_mat.set(0, 0, scale);
	proj_mat.set(1, 1, scale);
	proj_mat.set(2, 2, d64(-(far.d)) / (far - near));
	proj_mat.set(3, 2, d64(-(far.d * near.d)) / (far - near));
	proj_mat.set(2, 3, -1.0);
	proj_mat.set(3, 3, 0.0);
	return proj_mat;
}

static void render(int frame, Matrix<4, 4> cam_matrix)
{
	// clear frame buffer
	for (unsigned int y = 0; y < HEIGHT; y++) {
		for (unsigned int x = 0; x < WIDTH; x++) {
			set_pixel(x, y, WHITE);
		}
	}

	Matrix<4, 4> proj_mat = proj_matrix(100.0, 0.1, 90.0);
	for (auto &o : objects) {

		Transform ttr = stacsos::rotate_z_mat(frame * 0.0001);
		Transform r = stacsos::rotate_x_mat(frame * 0.0002);
		Transform sr = stacsos::translation_mat(Vec3(0.0, 0.0, -4.0));
		auto tr = (ttr) * (r)*sr;
		// tr.print();
		int t_count = 0;
		for (auto &t : (*o).tris) {
			t_count++;
			Vec4 p1 = (Vec4(t.points[0], 1.0)) * (tr) * (*o).trans * cam_matrix * proj_mat;
			Vec4 p2 = (Vec4(t.points[1], 1.0)) * (tr) * (*o).trans * cam_matrix * proj_mat;
			Vec4 p3 = (Vec4(t.points[2], 1.0)) * (tr) * (*o).trans * cam_matrix * proj_mat;

			Vec3 camera_point = Vec3(0.0, 0.0, 0.0);
			Vec3 camera_look_point = Vec3(0.0, 0.0, 1.0);

			colour tri_col = colour(u8(t.points[2].x() * 256 + t_count * 10), u8(t.points[2].z() * 256), (frame % 25600) / 100);
			if (RENDER_MODE == RENDER) {
				draw_triangle(screen_uv_to_pixel(p1.toVec3()), screen_uv_to_pixel(p2.toVec3()), screen_uv_to_pixel(p3.toVec3()), tri_col);

			} else if (RENDER_MODE == WIREFRAME) {
				line(screen_uv_to_pixel(p1.toVec3()), screen_uv_to_pixel(p2.toVec3()), tri_col);
				line(screen_uv_to_pixel(p2.toVec3()), screen_uv_to_pixel(p3.toVec3()), tri_col);
				line(screen_uv_to_pixel(p3.toVec3()), screen_uv_to_pixel(p1.toVec3()), tri_col);
			}
		}
	}
	// syscalls::sleep(10);
	send();

	return;
}

static void *input_thread(void *input_queue)
{
	console::get().write("thisis from input thread");
	char *q = (char *)input_queue;
	while (true) {
		char c = console::get().read_char();


		q[0] = (c);
		input_lock++;
	}
}

Matrix<4, 4> gen_view_mat(Vec3 pos, d64 pitch, d64 yaw)
{
	Vec3 xaxis = Vec3(cos(yaw).d, 0.0, -(sin(yaw)).d);
	Vec3 yaxis = Vec3(sin(yaw).d * sin(pitch).d, cos(pitch).d, cos(yaw).d * sin(pitch).d);
	Vec3 zaxis = Vec3(sin(yaw).d * cos(pitch).d, -sin(pitch).d, cos(yaw).d * cos(pitch).d);
	d64 mat_i[16] = { xaxis.x(), yaxis.x(), zaxis.x(), 0.0, xaxis.y(), yaxis.y(), zaxis.y(), 0.0, xaxis.z(), yaxis.z(), zaxis.z(), 0.0, -xaxis.dot(pos).d,
		-yaxis.dot(pos).d, -zaxis.dot(pos).d, 1.0 };
	Matrix<4, 4> viewMat = Matrix<4, 4>(mat_i);
	return viewMat;
}

object3d gen_cube(Transform t)
{
	object3d o = object3d();
	console::get().writef("%");
	o.tris.append(triangle({ Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0) }));
	o.tris.append(triangle({ Vec3(0.0, 1.0, 0.0), Vec3(0.0, 1.0, 1.0), Vec3(1.0, 1.0, 1.0) }));
	// bottom face
	o.tris.append(triangle({ Vec3(0.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 0.0, 1.0) }));
	o.tris.append(triangle({ Vec3(0.0, 0.0, 0.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 0.0, 1.0) }));

	// front face
	o.tris.append(triangle({ Vec3(0.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.0) }));
	o.tris.append(triangle({ Vec3(0.0, 0.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.0) }));
	// back face
	o.tris.append(triangle({ Vec3(0.0, 0.0, 1.0), Vec3(1.0, 0.0, 1.0), Vec3(1.0, 1.0, 1.0) }));
	o.tris.append(triangle({ Vec3(0.0, 0.0, 1.0), Vec3(0.0, 1.0, 1.0), Vec3(1.0, 1.0, 1.0) }));

	// left face
	o.tris.append(triangle({ Vec3(0.0, 0.0, 0.0), Vec3(0.0, 0.0, 1.0), Vec3(0.0, 1.0, 1.0) }));
	o.tris.append(triangle({ Vec3(0.0, 0.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(0.0, 1.0, 1.0) }));
	// back face
	o.tris.append(triangle({ Vec3(1.0, 0.0, 0.0), Vec3(1.0, 0.0, 1.0), Vec3(1.0, 1.0, 1.0) }));
	o.tris.append(triangle({ Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0) }));
	o.trans = t;
	return o;
}
int main(const char *cmdline)
{
	RENDER_MODE = RENDER;
	fb = object::open("/dev/virtcon0");
	// t = object::open("/dev/c");

	if (!fb) {
		console::get().write("error: unable to open virtual console\n");
		return 1;
	}

	object3d c1 = gen_cube(stacsos::translation_mat(Vec3(0, 0, 0)));
	object3d c2 = gen_cube(stacsos::translation_mat(Vec3(2, 0, 0)));

	objects.push(&c1);
	objects.push(&(c2));
	int frame = 0;
	thread *input_thr;
	char input_buffer[256];
	input_thr = thread::start(input_thread, (void *)(input_buffer));
	d64 speed = 10.0;
	d64 delta = 0.01;
	Vec3 camera_pos = Vec3(0, 0, 0);
	while (true) {
		Matrix<4, 4> cam_mat = gen_view_mat(camera_pos, 0.0, 0.0);
		render(frame, cam_mat);
		frame++;

		if (input_lock.fetch_and_add(0)!=0) {
			// console::get().writef("char reacived: %d\n",input_lock.fetch_and_add(0));
			int i = input_lock.fetch_and_add(-1);
			char c = input_buffer[0];
			// console::get().writef("char reacived: %c\n",c);
						// console::get().writef("char reacived: %d\n",input_lock.fetch_and_add(0));


			if (c == ' ') {

				switch (RENDER_MODE) {
				case RENDER: {
					RENDER_MODE = WIREFRAME;
					break;
				};
				case WIREFRAME: {
					RENDER_MODE = RENDER;
					break;
				};
				}
			}
			if (c == 'w') {
				camera_pos = (camera_pos + (BACK)*speed * delta);
			};
			if (c == 's') {
				camera_pos = (camera_pos + (FORWARD)*speed * delta);
			};
			if (c == 'a') {
				camera_pos = (camera_pos + (LEFT)*speed * delta);
			};
			if (c == 'd') {
				camera_pos = (camera_pos + (RIGHT)*speed * delta);
			};
		}
	}
	// wait for input so the prompt doesn't ruin the lovely image
	// remove this when timing!
	console::get().read_char();

	delete fb;

	return 0;
}
