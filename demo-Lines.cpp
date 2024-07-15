﻿/*
*  Author: wysaid
*    Mail: admin@wysaid.org
*    Blog: http://blog.wysaid.org
*/

#include "cgeMat.h"
#include "cgeVec.h"
#include "graphics.h"
#include <vector>

using namespace CGE;

class Object3d
{
public:
	Object3d()
	{
		//把下面的0(1)换成1(0)可以切换两种视图
#if 1
		m_matProj = Mat4::makePerspective(M_PI / 4.0f, 4.0f / 3.0f, 1.0, 1000.0f);
		m_matModelView = Mat4::makeLookAt(0.0f, 0.0f, 800.0f, 0.0f, 0.0f, -1000.0f, 0.0f, 1.0f, 0.0f);
#else
		m_matProj = Mat4::makeOrtho(-400.0f, 400.0f, -300.0f, 300.0f, -1.0f, 1.0f);
		m_matModelView.loadIdentity();
#endif
	}
	~Object3d() {}

	void render(float x, float y)
	{
		std::vector<Vec2f> vec;
		Mat4 mvp = m_matProj * m_matModelView;
		for(auto t = m_vec.begin(); t != m_vec.end(); ++t)
		{
			Vec2f coord;
			Mat4::projectM4f(*t, mvp, coord, 800.0f, 600.0f);
			vec.push_back(coord);
		}

		for(auto t1 = vec.begin(); t1 != vec.end(); ++t1)
		{
			for(auto t2 = vec.begin(); t2 != vec.end(); ++t2)
			{
				line(x + t1->x(), y + t1->y(), x + t2->x(), y + t2->y());
			}
//			outtextxy(x + t1->x(), y + t1->y(), '@');
		}


// 		for(auto t = m_vecTmp.begin()+1; t < m_vecTmp.end(); ++t)
// 		{
// 
// 			line(x + (t-1)->x(), y + (t-1)->y(), x + t->x(), y + t->y());
// 		}
// 		if(!m_vecTmp.empty())
// 		{
// 			line(x + m_vecTmp[0].x(), y + m_vecTmp[0].y(), x + m_vecTmp[m_vecTmp.size()-1].x(), y + m_vecTmp[m_vecTmp.size()-1].y());
// 		}
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

void getObj(Object3d& obj, float scale)
{
	obj.pushPoint(-scale, -scale, 0.0f);
	obj.pushPoint(scale, -scale, 0.0f);
	obj.pushPoint(scale, scale, 0.0f);
	obj.pushPoint(-scale, scale, 0.0f);
}

void getBox(Object3d& obj, float scale)
{
	obj.pushPoint(-scale, -scale, scale);
	obj.pushPoint(scale, -scale, scale);
	obj.pushPoint(scale, scale, scale);
	obj.pushPoint(-scale, scale, scale);
	obj.pushPoint(-scale, -scale, -scale);
	obj.pushPoint(scale, -scale, -scale);
	obj.pushPoint(scale, scale, -scale);
	obj.pushPoint(-scale, scale, -scale);
}

#define RANDOM_SIGN (random(2) == 0 ? -1 : 1)

void getObject(Object3d& obj, float scale)
{
	for(int i = 0; i != 40; ++i)
	{
		obj.pushPoint(RANDOM_SIGN*randomf()*scale, RANDOM_SIGN*randomf()*scale, RANDOM_SIGN*randomf()*scale);
	}
}


bool mouseFunc(Object3d& obj)
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
			obj.rotateZ((msg.x - s_lastX) / 100.0f);
			s_lastX = msg.x;
			s_lastY = msg.y;
		}
	}while(mousemsg());
	return true;
}

int main()
{
	initgraph(800, 600);
	setrendermode(RENDER_MANUAL);
	Object3d obj;
	Object3d obj2;
	getObject(obj, 300.0f);
	getBox(obj2, 50.0f);
	randomize();
//	obj2.translateZ(-500.0f);
	int i = 0;
	static bool s_useBlur = false;
	float x = 0.0f, y = 0.0f;
	float dx = randomf()* 5.0f + 1.0f, dy = randomf()* 5.0f + 1.0f;
	float radX = randomf()*10.0f + 0.001f, radY = randomf() * 10.0f, radZ = randomf() * 10.0f;
	setbkmode(TRANSPARENT);
	for(; is_run(); delay_fps(60))
	{
		if(s_useBlur) imagefilter_blurring(0, 0x80, 0x100);
		else cleardevice();
		mouseFunc(obj);

		obj.render(0, 0);
		outtextxy(20, 10, "点击空格键启用模糊滤镜,按回车随机生成模型");
		setcolor(WHITE);
		obj2.rotate(0.05f, radX, radY, radZ);

		obj2.render(x, y);
		setcolor(HSVtoRGB(i*2, 1.0f, 1.0f));

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
// 			obj.clear();
// 			getObject(obj, 300.0f);
		}
		if(kbhit())
		{
			switch (getch())
			{
			case ' ':
				s_useBlur = !s_useBlur;
				break;
			case '\r':
				obj.clear();
				getObject(obj, 300.0f);
				break;
			default:
				break;
			}
		}
	}
	closegraph();
	return 0;
}