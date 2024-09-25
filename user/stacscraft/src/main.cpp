/* SPDX-License-Identifier: MIT */

/* StACSOS - stacscraft
 *
 * Copyright (c) Jade Robertson 2022
 * with reference to the mandelbrot utility
 */

/*
 * A program that prints a rather crude version of the Mandelbrot fractal to the StACSOS terminal.
 */

#include <stacsos/atomic.h>
#include <stacsos/console.h>
#include <global.h>
#include <stacsos/helpers.h>
#include <stacsos/objects.h>
#include <stacsos/threads.h>
#include <stacsos/user-syscall.h>
#include <stacsos/list.h>

using namespace stacsos;

// struct mat{
	
// }
struct vec3
{
	double x,y,z;
};
struct vec2i
{
	int x,y;
	vec2i operator+(const vec2i& a) const
    {
        return vec2i(a.x+x, a.y+y);
    }
	vec2i operator-(const vec2i& a) const
    {
        return vec2i(x- a.x, y-a.y);
}	
	vec2i operator*(const double& a) const
    {
        return vec2i(a*x, a*y);
    }
};

struct triangle
{
	vec3 p1;
	vec3 p2;
	vec3 p3;
};

struct colour
{
	u8 r;
	u8 g;
	u8 b;
};

// template <u8 r, u8 c> class Matrix{
// 	private:
// 	double data[r*c];
// public:

// 	Matrix();
// 	template <int d>
// 	Matrix< r,  c> operator+(const Matrix< d, c>& a) const ;
// 	// template <int r2, int d, int c2>
// 	// // template <int d>
// 	// Matrix< r2,  d> Matrix< r2,  c2>::operator+(Matrix< d, c2>& a) const {
// 	// 	Matrix<r2,c2> res;
		
// 	// 	return res;
// 	// }
// };


// template <int r, int c>
// template <int d>
// Matrix< r,  c> Matrix< r,  d>::operator+(const Matrix< d, d>& a)  {
// 		Matrix<r,c> res;
		
// 		return res;
// 	}
// template <int r2, int d, int c2>
// // template <int d>
// Matrix< r2,  d> operator+(Matrix< d, c2>& a){
// 	Matrix<r2,c2> res;
	
// 	return res;
// }

// template <u8 r, u8 c> class	Matrix< r,  c>& operator+(Matrix< u8 s,u8 d>& a){
// 		return a;
// 	}

// template < r,  c> ;
// Matrix<int r, int c>& operator+(Matrix<int r, int c>& a){
// 		return a;
// 	}

// typedef Matrix<4,4> mat4x4; 
const colour WHITE = colour(255,255,255);
const colour BLACK = colour(0,0,0);
const colour RED = colour(255,0,0);
const colour GREEN = colour(0,255,0);
const colour BLUE = colour(0,0,255);
const int WIDTH = 640; // frame is 80x25
const int HEIGHT =480;
u32 frame_buffer[HEIGHT][WIDTH];
list<triangle> tris;

object *fb;

static void send()
{
	// u32 u = (r<< 16) |(g << 8) | b;
	fb->pwrite((const char *)frame_buffer, sizeof(frame_buffer),0);
}

static void set_pixel(u16 x, u16 y, colour c){
	u32 u =(c.r<< 16) |(c.g << 8) | c.b;
	frame_buffer[y][x] = u;
}


//screen uvs vary from -1 to 1
static vec2i screen_uv_to_pixel(vec3 in){
	return vec2i(((in.x+1.0)/2)*WIDTH,((in.y+1.0)/2)*HEIGHT);
}

static void line(vec2i start, vec2i end, colour c){
	for (double t=0.0; t< 1; t+=0.002){
		int x = start.x + (end.x-start.x) * t;
		int y = start.y + (end.y-start.y) * t;
		set_pixel(x,y,c);
	}
}

// Draw triangle, in screen pixel space
static void draw_triangle(vec2i p0 ,vec2i p1, vec2i p2, colour c){
	if (p0.y==p1.y && p0.y==p2.y) return;
	//Sort the vertices, t0 lower
	if (p0.y > p1.y) swap(p1,p0);
	if (p0.y > p2.y) swap(p0,p2);
	if (p1.y > p2.y) swap(p1,p2);
	int total_height = p2.y-p0.y;
	for (int i=0; i <total_height; i++){
		bool second_half = i> p1.y-p0.y || p1.y==p0.y;
		int segment_height = second_half ? p2.y-p1.y : p1.y-p0.y;
		double alpha = (double)i/total_height;
		double beta = (double)(i-(second_half ? p1.y-p0.y : 0))/segment_height;
		vec2i A =               p0 + (p2-p0)*alpha; 
        vec2i B = second_half ? p1 + (p2-p1)*beta : p0 + (p1-p0)*beta; 
        if (A.x>B.x) swap(A, B); 
        for (int j=A.x; j<=B.x; j++) { 
            set_pixel(j, p0.y+i, c); // attention, due to int casts t0.y+i != A.y 
        } 
	} 
}
static void render(int frame)
{



    for (unsigned int y=0; y< HEIGHT; y++){ 
        for (unsigned int x=0; x< WIDTH; x++){
            // set_pixel(x,y,WHITE);
            // drawchar(x, y , (u8)x, (u8)y, (u8)(frame %100),'c');

        }
    }
	for(auto& t: tris){
		// console::get().writef("%d",t.p1.x);
		draw_triangle(screen_uv_to_pixel(t.p1),screen_uv_to_pixel(t.p2),screen_uv_to_pixel(t.p3),colour(t.p3.x*256,t.p3.y*256,(frame%25600)/100));
		// line(screen_uv_to_pixel(t.p1),screen_uv_to_pixel(t.p2),RED);
		// line(screen_uv_to_pixel(t.p2),screen_uv_to_pixel(t.p3),GREEN);
		// line(screen_uv_to_pixel(t.p3),screen_uv_to_pixel(t.p1),BLUE);
	}
	// syscalls::sleep(10);
	send();

	return ;
}


int main(const char *cmdline)
{
	// Matrix<1,4> a;
	// Matrix<4,1> b;
	// Matrix<4,4> c = a+b;
	fb = object::open("/dev/virtcon2");

	if (!fb) {
		console::get().write("error: unable to open virtual console\n");
		return 1;
	}
	//    printf("\033\x09How many threads would you like to use?\n");
	//    int numThreads = getch();
	// int numThreads = 8;
	// thread *threads[numThreads];

	// for (int k = 0; k < numThreads; k++) {
	// 	threads[k] = thread::start(mandelbrot, (void *)(u64)k);
	// }

	// for (int k = 0; k < numThreads; k++) {
	// 	threads[k]->join();
	// }

	// tris.append(triangle(vec3(.1,0.0,-0.1), vec3(0.9,0.9,0.0),vec3((frame%1000)/1000.0,0.0,0.0)));
	// tris.append(triangle(vec3(-0.3,0.0,0.1), vec3(0.8,-0.8,0.0),vec3(1.0,0.5,0.0)));
	// tris.append(triangle(vec3(-0.2,0.0,0.0), vec3(-0.5,1.0,0.0),vec3(1.0,-0.2,0.0)));


	tris.append(triangle(vec3(0.0,0.0,0.0), vec3(0,0.3,0.0),vec3(0.3,0.15,0.0)));
	tris.append(triangle(vec3(0.0,0.0,0.0), vec3(0,0.3,0.0),vec3(-0.3,0.15,0.0)));

	tris.append(triangle(vec3(0.0,0.0,0.0), vec3(0.3,-0.15,0.0),vec3(0.0,-0.3,0.0)));
	tris.append(triangle(vec3(0.0,0.0,0.0), vec3(0.3,-0.15,0.0),vec3(0.3,0.15,0.0)));

	tris.append(triangle(vec3(0.0,0.0,0.0), vec3(-0.3,-0.15,0.0),vec3(0.0,-0.3,0.0)));
	tris.append(triangle(vec3(0.0,0.0,0.0), vec3(-0.3,-0.15,0.0),vec3(-0.3,0.15,0.0)));
	int frame = 0 ;
	while (true){
    render( frame);
	frame++;
	// console::get().writef("frame %d\n", frame);
	}
	// wait for input so the prompt doesn't ruin the lovely image
	// remove this when timing!
	console::get().read_char();

	delete fb;

	return 0;
}
