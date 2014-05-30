#include "htMat.h"
#include "htVec.h"
#include "graphics.h"
#include <vector>
#include "teapot.h"

using namespace HTAlgorithm;

class Object
{
public:
	Object()
	{
		m_matProj = Mat4::makeOrtho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
//		m_matProj = Mat4::makeFrustum(0.0f, 800.0f, 0.0f, 600.0f, -100.0f, 100.0f);
//		m_matProj = Mat4::makePerspective(45.0f, 4.0f / 3.0f, -10.0f, 10.0f);
//		m_matProj.transPose();
//		m_matModelView = Mat4::makeLookAt(0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
//		m_matModelView.translate(-400, -150, -700);
		m_matModelView.loadIdentity();
	}
	~Object() {}

	void render(float x, float y)
	{
		std::vector<Vec2f> vec;
		for(auto t = m_vec.begin(); t != m_vec.end(); ++t)
		{
			Vec2f coord;
			Mat4::projectM4f(*t, m_matModelView, m_matProj, coord, 800.0f, 600.0f);
			vec.push_back(coord);
		}

		for(int i = 0; i <= g_teapotIndicesNum; i += 3)
		{
			const int index1 = g_teapotIndices[i];
			const int index2 = g_teapotIndices[i + 1];
			const int index3 = g_teapotIndices[i + 2];
			line(x + vec[index1][0], y + vec[index1][1], x + vec[index2][0], y + vec[index2][1]);
			line(x + vec[index3][0], y + vec[index3][1], x + vec[index2][0], y + vec[index2][1]);
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

int main()
{
	initgraph(800, 600);
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
	for(; is_run(); delay_fps(60))
	{
		if(s_useBlur) imagefilter_blurring(0, 0x80, 0x100);
		else cleardevice();
		mouseFunc(obj);

		obj.render(400, 300);
		outtextxy(20, 10, "点击空格键启用模糊滤镜,按回车随机生成模型，滚轮移动模型Z值");
		
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