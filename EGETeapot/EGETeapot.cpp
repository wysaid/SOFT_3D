//Author: wysaid
//blog: http://blog.wysaid.org

#include "cgeMat.h"
#include "cgeVec.h"
#include "graphics.h"
#include <vector>
#include "teapot.h"

using namespace CGE;

#define USE_DEPTH_TEST 0
#define USE_PERSPECTIVE_PROJ 1

#define RENDER_TRIANGLE_HALF_1 1
#define RENDER_TRIANGLE_HALF_2 0

color_t g_color;


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#if USE_DEPTH_TEST
//depth 还存在一点问题， 没有效果
unsigned char g_depthMask[SCREEN_WIDTH][SCREEN_HEIGHT];
const int g_maskSize = sizeof(g_depthMask);

#endif

template<class Type>
class Triangle
{
public:

	static inline void fillTriangle(const Type& v0, const Type& v1, const Type& v2)
	{
		if(v0[1] == v2[1])
		{
			_fillSimpleTriangle(v0, v1, v2);
		}
		else if(v1[1] == v2[1])
		{
			_fillSimpleTriangle(v1, v0, v2);
		}
		else if(v0[1] == v1[1])
		{
			_fillSimpleTriangle(v0, v2, v1);
		}
		else
		{
			_fillNormalTriangle(v0, v1, v2);
		}
	}

private:

#if USE_DEPTH_TEST

	static inline void drawPixel(int x, int y, int depth, color_t color)
	{
		if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
			return;

		if(depth >=0 && depth < 256 && g_depthMask[x][y] <= depth)
		{
			putpixel_f(x, y, color);
			g_depthMask[x][y] = depth;
		}
	}

	static inline void drawLineL2R(int x0, int depth0, int x1, int depth1, int y, color_t color)
	{
		if(x0 == x1)
		{
			drawPixel(x0, y, depth0, color);
			return;
		}

		int left, right;
		int depthL, depthR;

		if(x0 < x1)
		{
			left = x0;
			right = x1;
			depthL = depth0;
			depthR = depth1;
		}
		else
		{
			left = x1;
			right = x0;
			depthL = depth1;
			depthR = depth0;
		}

		float delta = float(depthR - depthL) / (right - left);
		for(int i = left; i <= right; ++i)
		{
			drawPixel(i, y, depthL, color);
			depthL += delta;
		}
	}

#endif

	// 平顶/平底三角形, v0[1] == v2[1], v1[1] 最大
	static inline void _fillSimpleTriangle(const Type& v0, const Type& v1, const Type& v2)
	{
		assert(v0[1] == v2[1]);
		float h = v1[1] - v0[1];

		float dL = (v1[0] - v0[0]) / h;
		float dR = (v1[0] - v2[0]) / h;

		float dDepthL = (v1[2] - v0[2]) / h;
		float dDepthR = (v1[2] - v2[2]) / h;

		float xL = v0[0], xR = v2[0];
		float depthL = v0[2], depthR = v2[2];

		if(v0[1] < v1[1])
		{
#if RENDER_TRIANGLE_HALF_1

			for(int i = v0[1]; i <= v1[1]; ++i)
			{
#if USE_DEPTH_TEST
				drawLineL2R(xL, depthL, xR, depthR, i, RED);
#else
				line(xL, i, xR, i); //A Simple Function That Just Draw A Line
#endif //USE_DEPTH_TEST

				xL += dL;
				xR += dR;

				depthL += dDepthL;
				depthR += dDepthR;
			}
#endif //RENDER_TRIANGLE_HALF_1
		}
		else
		{
#if RENDER_TRIANGLE_HALF_2

			for(int i = v0[1]; i >= v1[1]; --i)
			{
#if USE_DEPTH_TEST
				drawLineL2R(xL, depthL, xR, depthR, i, YELLOW);
#else
				line(xL, i, xR, i); //A Simple Function That Just Draw A Line
#endif //USE_DEPTH_TEST	

				xL -= dL;
				xR -= dR;

				depthL -= dDepthL;
				depthR -= dDepthR;
			}

#endif
		}
	}


	static inline void _fillNormalTriangle(const Type& v0, const Type& v1, const Type& v2)
	{
		const Type *pnts[] = {&v0, &v1, &v2};

		if((*pnts[0])[1] > (*pnts[1])[1])
			std::swap(pnts[0], pnts[1]);

		if((*pnts[0])[1] > (*pnts[2])[1])
			std::swap(pnts[0], pnts[2]);

		if((*pnts[1])[1] > (*pnts[2])[1])
			std::swap(pnts[1], pnts[2]);		

		const Type &vv0 = *pnts[0], &vv1 = *pnts[1], &vv2 = *pnts[2];
		Type newPoint(vv1);

		newPoint[0] = vv0[0] + (vv2[0] - vv0[0]) * (vv1[1] - vv0[1]) / (vv2[1] - vv0[1]);

		_fillSimpleTriangle(newPoint, vv0, vv1);
		_fillSimpleTriangle(newPoint, vv2, vv1);
	}
};

class Object
{
public:
	Object()
	{
		//把下面的0(1)换成1(0)可以切换两种视图
#if USE_PERSPECTIVE_PROJ
 		m_matProj = Mat4::makePerspective(M_PI / 4.0f, 4.0f / 3.0f, 1.0f, 1000.0f);
 		m_matModelView = Mat4::makeLookAt(0.0f, 0.0f, 800.0f, 0.0f, 0.0f, -500.0f, 0.0f, 1.0f, 0.0f);
#else
 		m_matProj = Mat4::makeOrtho(-400.0f, 400.0f, -300.0f, 300.0f, -300.0f, 300.0f);
 		m_matModelView.loadIdentity();
#endif
	}
	~Object() {}

	void render(float x, float y)
	{
		std::vector<Vec3f> vec;
		const Mat4 mvp = m_matProj * m_matModelView;
		const Vec4f viewPort(0.0f, 0.0f, 800.0f, 600.0f);
		for(auto t = m_vec.begin(); t != m_vec.end(); ++t)
		{
			Vec3f coord;
			Mat4::projectM4f(*t, mvp, viewPort, coord);
			//Mat4::projectM4fPerspective(*t, m_matModelView, m_matProj, viewPort, coord);
			vec.push_back(coord);
		}

		for(int i = 0; i <= g_teapotIndicesNum; i += 3)
		{
			const int index1 = g_teapotIndices[i];
			const int index2 = g_teapotIndices[i + 1];
			const int index3 = g_teapotIndices[i + 2];
			
			const Vec3i polyPoints[] = {
				Vec3i(x + vec[index1][0], y + vec[index1][1], vec[index1][2] * 255.0f),
				Vec3i(x + vec[index2][0], y + vec[index2][1], vec[index2][2] * 255.0f),
				Vec3i(x + vec[index3][0], y + vec[index3][1], vec[index3][2] * 255.0f) };

//设置是否显示模拟填充
#if RENDER_TRIANGLE_HALF_1 || RENDER_TRIANGLE_HALF_2
			
			{
				float ax = polyPoints[1][0] - polyPoints[0][0];
				float ay = polyPoints[1][1] - polyPoints[0][1];
				float bx = polyPoints[2][0] - polyPoints[1][0];
				float by = polyPoints[2][1] - polyPoints[1][1];
				float zNorm = ax * by - ay * bx;

				//背面剔除
				if(zNorm > 0.0f)
				{
					setcolor(g_color ^ 0xffffff);
#if USE_DEPTH_TEST
					memset(g_depthMask, 0, g_maskSize);
#endif
					Triangle<Vec3i>::fillTriangle(polyPoints[0], polyPoints[1], polyPoints[2]);
				}
			}

#endif
			setcolor(g_color);
			line(polyPoints[0][0], polyPoints[0][1], polyPoints[1][0], polyPoints[1][1]);
			line(polyPoints[1][0], polyPoints[1][1], polyPoints[2][0], polyPoints[2][1]);
			line(polyPoints[2][0], polyPoints[2][1], polyPoints[0][0], polyPoints[0][1]);
		}

	}

	void pushPoint(Vec3f pnt)
	{
		m_vec.push_back(pnt);
	}

	void pushPoints(Vec3f* pnt, int n)
	{
		for(int i = 0; i != n; ++i)
		{
			m_vec.push_back(pnt[i]);
		}
	}

	void pushPoint(float x, float y, float z)
	{
		m_vec.push_back(Vec3f(x, y, z));
	}

	void rotate(float rad, float x, float y, float z)
	{
		m_matModelView.rotate(rad, x, y, z);
	}

	void rotateX(float rad)
	{
		m_matModelView.rotateX(rad);
	}

	void rotateY(float rad)
	{
		m_matModelView.rotateY(rad);
	}

	void rotateZ(float rad)
	{
		m_matModelView.rotateZ(rad);
	}

	void translateZ(float z)
	{
		m_matModelView.translateZ(z);
	}

	void clear() { m_vec.clear(); }

private:
	std::vector<Vec3f> m_vec;
	Mat4 m_matProj, m_matModelView;
};


bool mouseFunc(Object& obj)
{
	if(!mousemsg()) return false;
	do
	{
		static int s_lastX, s_lastY;
		static int s_bIsDown = false;
		mouse_msg msg = getmouse();
		switch (msg.msg)
		{
		case mouse_msg_down:
			s_lastX = msg.x;
			s_lastY = msg.y;
			s_bIsDown = true;
			break;
		case mouse_msg_up:
			s_bIsDown = false;
			break;
		case mouse_msg_wheel:
			obj.translateZ(msg.wheel / 12.0f);
			break;
		default:
			break;
		}

		if(s_bIsDown && msg.is_move())
		{
			obj.rotateX((msg.y - s_lastY) / 100.0f);
			obj.rotateY((msg.x - s_lastX) / 100.0f);
			s_lastX = msg.x;
			s_lastY = msg.y;
		}
	}while(mousemsg());
	return true;
}

void getObj(Object& obj, float scale)
{
	const int num = g_positionNum / 3;
	for(int i = 0; i < g_positionNum; i += 3)
	{
		obj.pushPoint(g_teapotPositions[i] * 10.0f, g_teapotPositions[i + 1] * 10.0f, g_teapotPositions[i + 2] * 10.0f);
	}
}

// template<class Type>
// void drawTriangle(const Type& t0, const Type& t1, const Type& t2)
// {
// 	line(t0[0], t0[1], t1[0], t1[1]);
// 	line(t2[0], t2[1], t1[0], t1[1]);
// 	line(t0[0], t0[1], t2[0], t2[1]);
// }

int main()
{
	initgraph(SCREEN_WIDTH, SCREEN_HEIGHT);
	setrendermode(RENDER_MANUAL);

	Object obj;
	randomize();
	
	getObj(obj, 100);

	int i = 0;
	static bool s_useBlur = false;
	float x = 0.0f, y = 0.0f;
	float dx = randomf()* 5.0f + 1.0f, dy = randomf()* 5.0f + 1.0f;
	float radX = randomf()*10.0f + 0.001f, radY = randomf() * 10.0f, radZ = randomf() * 10.0f;
	setbkmode(TRANSPARENT);
	setfillcolor(DARKGRAY);
	for(; is_run(); delay_fps(60))
	{
		if(s_useBlur) imagefilter_blurring(0, 0x80, 0x100);
		else cleardevice();
		mouseFunc(obj);

		obj.render(0, 0);

		g_color = HSVtoRGB(i*2, 1.0f, 1.0f);
		outtextxy(20, 10, "点击空格键启用模糊滤镜, 滚轮移动模型Z值");

		x += dx;
		y += dy;

		if(x < 0.0f || x > 800.0f)
		{
			dx = -dx;
			x += dx;
		}
		if(y < 0.0f || y > 600.0f)
		{
			dy = -dy;
			y += dy;
		}
		if(++i > 360)
		{
			i = 0;
		}
		if(kbhit())
		{
			switch (getch())
			{
			case ' ':
				s_useBlur = !s_useBlur;
				break;
			
			default:
				break;
			}
		}
	}
	closegraph();
	return 0;
}