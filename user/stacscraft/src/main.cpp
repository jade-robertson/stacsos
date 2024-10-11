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

class triangle {
public:
	triangle() {};
	triangle(Vec3 x, Vec3 y, Vec3 z)
	{
		points[0] = x;
		points[1] = y;
		points[2] = z;
	};

	Vec3 points[3];
};
enum Material { DIRT, WOOD, LEAVES };

struct object3d {
public:
	triangle tris[12];
	int tri_count = 0;
	Material mat = DIRT;
	Transform trans;
	object3d() {};
	void add_tri(triangle t) { tris[tri_count++] = t; }
};

struct colour {
	u8 r;
	u8 g;
	u8 b;
};
enum Mode { RENDER, WIREFRAME, NORMAL, DEPTH, SHADING };
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
float depth_buffer[HEIGHT][WIDTH];
object3d objects[100];
object *fb;
atomic_u64 input_lock = 0;
Vec3 light_dir;
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
// assumes counterclockwise winding
static void draw_triangle(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 norm, colour c)
{

	d64 depth = (p0.z() + p1.z() + p2.z()) / d64(3.0);
	d64 light_str = norm.dot(light_dir);
	light_str = light_str.d/4.0+0.75;
		c = colour((u8)(light_str.d * c.r),(u8)(light_str.d * c.g),(u8)(light_str.d * c.b));

	if (RENDER_MODE == DEPTH) {
		u8 d_col = (u8)((depth.d * 256.0));
		c = colour(d_col, d_col, d_col);
	}
	vec2i p0i = screen_uv_to_pixel(p0);
	vec2i p1i = screen_uv_to_pixel(p1);
	vec2i p2i = screen_uv_to_pixel(p2);
	if (p0i.y == p1i.y && p0i.y == p2i.y)
		return;
	// Sort the vertices, t0 lower
	if (p0i.y > p1i.y)
		swap(p1i, p0i);
	if (p0i.y > p2i.y)
		swap(p0i, p2i);
	if (p1i.y > p2i.y)
		swap(p1i, p2i);
	int total_height = p2i.y - p0i.y;
	// console::get().writef("%d\n",total_height);
	//total height is all of tri, so skip parts where less than 0 or greater than HEIGHT
	// p0i.y +i <0 0 skip
	//p0i.y + i > HEIGHT SKIP
	//if p0.y is negative, i start at -p0i.y
	//if 
	// consider p0i.y =-600, and total hiehgt =900
	//height start = 600
	//height emd = 600
	if (total_height <0) return;
	// if t0.y + total_height > HEIGHT, then max_height = HEIGHT - t0.y
	int height_start = p0i.y<0 ? -p0i.y : 0;
	int height_end = p0i.y+ total_height > HEIGHT ? HEIGHT - p0i.y : total_height;
	for (int i =height_start; i < height_end; i++) {
		int line_y = p0i.y + i;
		bool second_half = i > p1i.y - p0i.y || p1i.y == p0i.y;
		int segment_height = second_half ? p2i.y - p1i.y : p1i.y - p0i.y;
		double alpha = (double)i / total_height;
		double beta = (double)(i - (second_half ? p1i.y - p0i.y : 0)) / segment_height;
		vec2i A = p0i + (p2i - p0i) * alpha; //how far up thorugh the triangle we are, on the long side
		vec2i B = second_half ? p1i + (p2i - p1i) * beta : p0i + (p1i - p0i) * beta; //how far up thorugh the triangle we are, on the short side
		// if (p0i.y +i < 0 | p0i.y +i >= HEIGHT) continue;
		if (A.x > B.x)
			swap(A, B);
		for (int j = max(A.x,0); j <= min(B.x,WIDTH); j++) {
			if (float(depth_buffer[p0i.y + i][j]) < float(depth.d)) {
				depth_buffer[line_y][j] = float(depth.d);
				if (RENDER_MODE == NORMAL) {
					set_pixel(j, line_y,
						colour(u8(norm.x() * 128 + 256), u8(norm.y() * 128 + 256), u8(norm.z() * 128 + 256))); // attention, due to int casts t0.y+i != A.y
				} else if (RENDER_MODE == SHADING) { 
					set_pixel(j, line_y, colour(u8(light_str * 256), u8(light_str * 256), u8(light_str * 256)));
				} else {
					set_pixel(j, line_y, c); // attention, due to int casts t0.y+i != A.y
				}
			}
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

static void render(int frame, Matrix<4, 4> cam_matrix, d64 delta)
{
	for (unsigned int y = 0; y < HEIGHT; y++) {
		for (unsigned int x = 0; x < WIDTH; x++) {
			set_pixel(x, y, WHITE);
			depth_buffer[y][x] = ((float)-9999999.0);
		}
	}

	Matrix<4, 4> proj_mat = proj_matrix(10.0, 1.0, 90.0);
	for (auto &o : objects) {

		int t_count = 0;
		Matrix<4,4> tr = ( (o).trans * cam_matrix * proj_mat);
		
		for (auto &t : (o).tris) {
			t_count++;
			Vec3 sideA = t.points[0] - t.points[1];
			Vec3 sideB = t.points[0] - t.points[2];
			Vec3 norm = sideA.cross(sideB).normalise();
			
			Vec4 p1_ = (Vec4(t.points[0], 1.0)) *tr;
			Vec4 p2_ = (Vec4(t.points[1], 1.0)) *tr;
			Vec4 p3_ = (Vec4(t.points[2], 1.0)) *tr;

			Vec3 p1 = p1_.toVec3();
			Vec3 p2 = p2_.toVec3();
			Vec3 p3 = p3_.toVec3();
			int num_outside=0;
			if ((p1.z()).d < 0.0) {
				num_outside ++;
				continue;
			}
			if ((p2.z()).d < 0.0) {
				num_outside++;
				continue;
			}
			if ((p3.z()).d < 0.0) {
				num_outside++;
				continue;
			}

			colour tri_col;
			switch (o.mat) {
			case DIRT:
				tri_col = colour(u8(32), u8(61), u8(40));
				break;
			case WOOD:
				tri_col = colour(u8(128), u8(116), u8(97));
				break;
			case LEAVES:
				tri_col = colour(u8(82), u8(204), u8(78));
				break;
			default:
				break;
			}
			// colour tri_col = colour(u8(t.points[2].x() * 256 + t_count * 10), u8(t.points[2].z() * 256), (frame % 25600) / 100);
			if (RENDER_MODE != WIREFRAME) {
				draw_triangle(p1, p2, p3, norm, tri_col);

			} else if (RENDER_MODE == WIREFRAME) {
				line(screen_uv_to_pixel(p1), screen_uv_to_pixel(p2), tri_col);
				line(screen_uv_to_pixel(p2), screen_uv_to_pixel(p3), tri_col);
				line(screen_uv_to_pixel(p3), screen_uv_to_pixel(p1), tri_col);
			}
		}
	}
	send();
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
// I think this needs to be flipped?
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

void setup_cube(object3d *o, Vec3 pos, Material m)
{
	//top
	o->tris[0] = (triangle({ Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0) }));

	o->tris[1] = (triangle({ Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 1.0, 1.0) }));
	// bottom face
	o->tris[2] = (triangle({ Vec3(0.0, 0.0, 0.0), Vec3(1.0, 0.0, 1.0), Vec3(1.0, 0.0, 0.0) }));
	o->tris[3] = (triangle({ Vec3(0.0, 0.0, 0.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 0.0, 1.0) }));

	// front face
	o->tris[4] = (triangle({ Vec3(0.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.0), Vec3(1.0, 0.0, 0.0) }));
	o->tris[5] = (triangle({ Vec3(0.0, 0.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.0) }));
	// back face
	o->tris[6] = (triangle({ Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(1.0, 0.0, 1.0) }));
	o->tris[7] = (triangle({ Vec3(0.0, 0.0, 1.0), Vec3(0.0, 1.0, 1.0), Vec3(1.0, 1.0, 1.0) }));

	// left face
	o->tris[8] = (triangle({ Vec3(0.0, 0.0, 0.0), Vec3(0.0, 1.0, 1.0), Vec3(0.0, 0.0, 1.0) }));

	o->tris[9] = (triangle({ Vec3(0.0, 0.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(0.0, 1.0, 1.0) }));

	// right face
	o->tris[10] = (triangle({ Vec3(1.0, 0.0, 0.0), Vec3(1.0, 0.0, 1.0), Vec3(1.0, 1.0, 1.0) }));
	o->tris[11] = (triangle({ Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(1.0, 1.0, 0.0) }));
	o->trans = stacsos::translation_mat(pos);
	o->mat = m;
}
int main(const char *cmdline)
{
	light_dir =Vec3(DOWN*3+LEFT*2+FORWARD).normalise();
	RENDER_MODE = RENDER;
	fb = object::open("/dev/virtcon0");
	// t = object::open("/dev/c");
	object *t = object::open("/dev/cmos-rtc0");

	u16 t_buf[6];
	if (!fb) {
		console::get().write("error: unable to open virtual console\n");
		return 1;
	}
	if (!t) {
		console::get().write("error: unable to open rtc\n");
		return 1;
	}
	console::get().writef("got here\n");
	console::get().writef("sqrtoot 16 = %ld\n", int(stacsos::sqroot(16)));
	console::get().writef("sqrtoot 1 = %ld\n", int(stacsos::sqroot(1)));
	console::get().writef("sqrtoot 0.5 = %ld\n", int(stacsos::sqroot(0.5) * 10));
	console::get().writef("got here\n");
	console::get().writef("got here\n");
	// object3d c3[64] ;

	for (int x = 0; x < 8; x++) {
		for (int z = 0; z < 8; z++) {

			setup_cube(&objects[z + x * 8], Vec3(x, -1, z), DIRT);
			objects[z + x * 8].trans = stacsos::translation_mat(Vec3(x, -1, z));
		}
	}
	setup_cube(&objects[65], Vec3(4, -1, 4), WOOD);
	setup_cube(&objects[66], Vec3(4, 0, 4), WOOD);
	setup_cube(&objects[67], Vec3(4, 1, 4), WOOD);
	setup_cube(&objects[68], Vec3(4, 2, 4), WOOD);
	setup_cube(&objects[69], Vec3(4, 3, 4), WOOD);

	setup_cube(&objects[70], Vec3(5, 3, 4), LEAVES);
	setup_cube(&objects[71], Vec3(3, 3, 4), LEAVES);
	setup_cube(&objects[72], Vec3(4, 3, 5), LEAVES);
	setup_cube(&objects[73], Vec3(4, 3, 3), LEAVES);
	setup_cube(&objects[74], Vec3(4, 4, 4), LEAVES);

	// object3d c1 = setup_cube(stacsos::translation_mat(Vec3(0, 0, 5.0)));
	// object3d c2 = setup_cube(stacsos::translation_mat(Vec3(2, 0, 5.0)));
	console::get().writef("got her2e\n");

	// objects.push(&c1);
	// objects.push(&(c2));
	int frame = 0;
	thread *input_thr;
	char input_buffer[256];
	input_thr = thread::start(input_thread, (void *)(input_buffer));
	d64 speed = 100.0;
	d64 delta = 0.01;
	d64 pitch = 0.01;
	d64 yaw = 0.0;
	Vec3 camera_pos = Vec3(0, 0, 0);
	u16 second;
	u32 frames_per_second = 100.0;
	console::get().writef("got here\n");

	while (true) {
		t->pread(t_buf, 12, 0);
		if (t_buf[0] != second) {
			delta = (1.0 / frames_per_second);
			console::get().writef("2%dfps, %d mspf\n", frames_per_second, (s64)((delta) * 1000));

			second = t_buf[0];
			frames_per_second = 0;
		}
		Matrix<4, 4> cam_mat = gen_view_mat(camera_pos, 0.0, pitch);
		render(frame, cam_mat, delta);
		frame++;
		frames_per_second++;

		if (input_lock.fetch_and_add(0) != 0) {
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
					RENDER_MODE = DEPTH;
					break;
				};
				case DEPTH: {
					RENDER_MODE = NORMAL;
					break;
				};
				case NORMAL: {
					RENDER_MODE = SHADING;
					break;
				};
				case SHADING: {
					RENDER_MODE = RENDER;
					break;
				};
				}
			}
			Vec4 cam_forward_ = Vec4(BACK, 0.0) * stacsos::rotate_y_mat(-pitch.d);
			// cam_forward_.print();
			Vec3 cam_forward = Vec3(cam_forward_.x(), cam_forward_.y(), cam_forward_.z()).normalise();
			// cam_forward.print();

			Vec3 cam_right = cam_forward.cross(UP).normalise();
			// cam_right.print();
			if (c == 'w') {
				camera_pos = (camera_pos + (cam_forward)*speed * delta);
			};
			if (c == 's') {
				camera_pos = (camera_pos + (cam_forward * (-1)) * speed * delta);
			};
			if (c == 'a') {
				camera_pos = (camera_pos + (cam_right * (-1)) * speed * delta);
			};
			if (c == 'd') {
				camera_pos = (camera_pos + (cam_right)*speed * delta);
			};

			if (c == 'z') {
				camera_pos = (camera_pos + (UP * speed * delta));
			};
			if (c == 'x') {
				camera_pos = (camera_pos + (DOWN)*speed * delta);
			};
			if (c == 'q') {
				pitch = pitch + (speed * delta * d64(0.3));
			};
			if (c == 'e') {
				pitch = pitch - (speed * delta * d64(0.3));
			};
		}
	}
	// wait for input so the prompt doesn't ruin the lovely image
	// remove this when timing!
	console::get().read_char();

	delete fb;

	return 0;
}
