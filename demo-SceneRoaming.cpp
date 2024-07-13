/*
*  Author: wysaid
*    Mail: admin@wysaid.org
*    Blog: http://blog.wysaid.org
*/

#define SHOW_CONSOLE
#define _CRT_SECURE_NO_WARNINGS
#include "graphics.h"
#include "cgeScene.h"
#include "cgeMat.h"
#include <vector>

using namespace CGE;
using namespace std;

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define CREATE_FUNC(className) static inline className* create(){ return new className;}

template<class T>
static inline void line(T p1, T p2)
{
	//3d坐标系与窗口坐标系y方向相反
	//使用 height - y 来修正.
	line(p1[0], SCREEN_HEIGHT - p1[1], p2[0], SCREEN_HEIGHT - p2[1]);
}

class Object3d
{
protected:
	//避免大量拷贝带来开销, 不允许直接创建实例, 只能通过create方法创建指针.
	Object3d() : m_matrix(Mat4::makeIdentity()) {}
public:
	~Object3d() {}

	CREATE_FUNC(Object3d)

	virtual void render(const Mat4& mvp, const Vec4f& viewPort)
	{
		m_winCoords.resize(m_vecPositions.size());
		auto winIter = m_winCoords.begin();
		memset(m_winCoords.data(), 0xff, m_winCoords.size() * sizeof(m_winCoords[0]));

		const Mat4& m = mvp * m_matrix;

		for (auto& pos : m_vecPositions)
		{
			Mat4::projectM4f(pos, m, viewPort, *winIter++);
		}

		for(int i = 1; i < m_vecIndices.size(); i += 2)
		{
			auto& p0 = m_winCoords[m_vecIndices[i - 1]];
			auto& p1 = m_winCoords[m_vecIndices[i]];

			if ((unsigned&)p0[0] != 0xffffffff && (unsigned&)p1[0] != 0xffffffff)
			{
				line(p0, p1);
			}
		}

	}

	virtual int objType() { return -1; }

	inline vector<Vec3f>& positions() { return m_vecPositions; }
	inline const vector<Vec3f>& positions() const { return m_vecPositions; }

	inline vector<int>& indices() { return m_vecIndices; }
	inline const vector<int>& indices() const { return m_vecIndices; }

	inline Mat4& matrix() { return m_matrix; }
	inline const Mat4& matrix() const { return m_matrix; }

protected:
	vector<Vec3f> m_vecPositions;
	vector<int> m_vecIndices;
	mutable vector<Vec2f> m_winCoords; //A cache for rendering.
	Mat4 m_matrix;
};

//使用xOz平面进行漫游， y方向朝上.
class Scene : public SceneInterface
{
public:

	Scene(float w, float h) : SceneInterface(w, h)
	{
	}

	~Scene()
	{
		for (auto* p : m_vecObjects)
		{
			delete p;
		}

		m_vecObjects.clear();
	}

	void render(const Vec4f& viewPort)
	{
		const Mat4& mvp = m_projectionMatrix * m_modelViewMatrix;

		for (auto* v : m_vecObjects)
		{
			v->render(mvp, viewPort);
		}
	}

	void addObject(Object3d* obj)
	{
		m_vecObjects.push_back(obj);
	}

private:
	vector<Object3d*> m_vecObjects;
};

//////////////////////////////////////////////////////////////////////////

//Special deal.
class Mesh : public Object3d
{
public:
	static inline Mesh* create(int w, int h)
	{
		assert(w > 0 && h > 0); //宽高必须大于0
		
		Mesh* m = new Mesh;
		m->m_width = w + 1;
		m->m_height = h + 1;
		auto& positions = m->positions();		
		int total = m->m_width * m->m_height;

		positions.reserve(total);
				
		for (int i = 0; i <= h; ++i)
		{
			for (int j = 0; j <= w; ++j)
			{
				positions.push_back(Vec3f(j / (float)w - 0.5f, i / (float)h - 0.5f, 0.0f));
			}
		}

		return m;
	}	

	int objType() { return 0; } //0 for mesh.

	virtual void render(const Mat4& mvp, const Vec4f& viewPort)
	{
		setcolor(0);
		m_winCoords.resize(m_vecPositions.size());
		auto winIter = m_winCoords.begin();
		memset(m_winCoords.data(), 0xff, m_winCoords.size() * sizeof(m_winCoords[0]));

		const Mat4& m = mvp * m_matrix;

		for (auto& pos : m_vecPositions)
		{
			Mat4::projectM4f(pos, m, viewPort, *winIter++);
		}

		const int w = m_width - 1;
		const int h = m_height - 1;

		for (int i = 0; i != h; ++i)
		{
			const int k = i + 1;
			const int thisLine = i * m_width;
			const int nextLine = k * m_width;

			for (int j = 0; j != w; ++j)
			{
				const int thisLineIndex = thisLine + j;
				const int nextLineIndex = nextLine + j;
				const auto& v1 = m_winCoords[thisLineIndex];
				const auto& v2 = m_winCoords[thisLineIndex + 1];
				const auto& v3 = m_winCoords[nextLineIndex + 1];
				const auto& v4 = m_winCoords[nextLineIndex];

#define TRANS_VEC2I_TYPE(v) Vec2i(v[0], v[1])

				//逆时针方向绘制
				const Vec2i pnts[4] = {
					TRANS_VEC2I_TYPE(v1),
					TRANS_VEC2I_TYPE(v2),
					TRANS_VEC2I_TYPE(v3),
					TRANS_VEC2I_TYPE(v4)
				};

				//Clip for perspective projection.
				if ((unsigned&)v1[0] != 0xffffffff && (unsigned&)v2[0] != 0xffffffff
					&& (unsigned&)v3[0] != 0xffffffff && (unsigned&)v4[0] != 0xffffffff)
				{
//					fillpoly(4, &pnts[0][0]); //填充矩形区域
//					drawpoly(4, &pnts[0][0]);

					line(pnts[0], pnts[1]);
					line(pnts[1], pnts[2]);
					line(pnts[2], pnts[3]);
					line(pnts[3], pnts[0]);
				}
			}
		}
	}

protected:

	int m_width, m_height;
};

class Ball : public Object3d
{
	Ball(int lats, int height) : m_lats(lats), m_height(height), m_rot(0.0f) {}
public:
	static inline Ball* create(float radius, int lats, int height, const Vec3f& transform)
	{
		auto* ball = new Ball(lats, height);
		assert(radius > 0.0f);
		auto& pos = ball->positions();
		auto& indices = ball->indices();
		pos.reserve(lats * height);
		indices.reserve(pos.size() * 2);

		for (int i = 0; i <= lats; ++i)
		{
			for (int j = 0; j <= height; ++j)
			{
				float theta = i * M_PI / lats;
				float phi = j * 2.0f * M_PI / height;
				float sinTheta = sinf(theta);
				float sinPhi = sinf(phi);
				float cosTheta = cosf(theta);
				float cosPhi = cosf(phi);

				float x = cosPhi * sinTheta;
				float y = cosTheta;
				float z = sinPhi * sinTheta;

				pos.push_back(Vec3f(x, y, z) * radius + transform);
			}
		}

		for (int i = 0; i != lats; ++i)
		{
			int thisLine = i * (height + 1);
			for (int j = 0; j != height; ++j)
			{
				int first = thisLine + j;
				int second = first + height + 1;

				indices.push_back(first);
				indices.push_back(first + 1);

				indices.push_back(first + 1);
				indices.push_back(second + 1);

				indices.push_back(second + 1);
				indices.push_back(second);

				indices.push_back(second);
				indices.push_back(first);
			}
		}

		return ball;
	}

	int objType() { return 1; }

	void render(const Mat4& mvp, const Vec4f& viewPort)
	{
		setcolor(0x333366);
		m_matrix = Mat4::makeTranslation(0.0f, 0.0f, 350.0f) * Mat4::makeXRotation(M_PI * 0.5f) * Mat4::makeYRotation(m_rot += 0.02f);
		Object3d::render(mvp, viewPort);
	}

private:
	int m_lats, m_height;
	float m_rot;
};

Mesh* getMesh(int w, int h, const Vec3f& rot, const Vec3f& scaling, const Vec3f& transform)
{
	Mesh* m = Mesh::create(w, h);
 	auto& matrix = m->matrix();

	matrix = Mat4::makeTranslation(transform[0], transform[1], transform[2]) *
		Mat4::makeXRotation(rot[0]) * Mat4::makeYRotation(rot[1]) * Mat4::makeZRotation(rot[2]) *
			Mat4::makeScale(scaling[0], scaling[1], scaling[2]);

	return m;
}

void initScene(Scene& s)
{
	//地面
	Mesh* m = getMesh(16, 16, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(2000.0f, 2000.0f, 2000.0f), Vec3f(0.0f, 0.0f, 0.0f));
	s.addObject(m);

	//前面墙
	m = getMesh(16, 4, Vec3f((float)M_PI / 2.0f, 0.0f, 0.0f), Vec3f(2000.0f, 500.0f, 2000.0f), Vec3f(0.0f, -1000.0f, 250.0f));
	s.addObject(m);

	//后面墙
	m = getMesh(16, 4, Vec3f((float)M_PI / 2.0f, 0.0f, 0.0f), Vec3f(2000.0f, 500.0f, 2000.0f), Vec3f(0.0f, 1000.0f, 250.0f));
	s.addObject(m);

	//左侧墙
	m = getMesh(4, 16, Vec3f(0.0f, (float)M_PI / 2.0f, 0.0f), Vec3f(500.0f, 2000.0f, 2000.0f), Vec3f(1000.0f, 0.0f, 250.0f));
	s.addObject(m);

	//右侧墙
	m = getMesh(4, 16, Vec3f(0.0f, (float)M_PI / 2.0f, 0.0f), Vec3f(500.0f, 2000.0f, 2000.0f), Vec3f(-1000.0f, 0.0f, 250.0f));
	s.addObject(m);

	//天花板
// 	m = getMesh(16, 16, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(2000.0f, 2000.0f, 2000.0f), Vec3f(0.0f, 0.0f, 500.0f));
// 	s.addObject(m);

	Ball* ball = Ball::create(200.0f, 10, 10, Vec3f(0.0f, 0.0f, 0.0f));
	s.addObject(ball);
}

int main()
{
	initgraph(SCREEN_WIDTH, SCREEN_HEIGHT, INIT_RENDERMANUAL);
	setcaption("Soft Scene Roaming by wysaid.");
 	setbkmode(TRANSPARENT);
	setfillcolor(0xaaaaaa);

	Scene scene(SCREEN_WIDTH, SCREEN_HEIGHT);

	initScene(scene);

	const Vec4f viewPort(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
	const float motion = 5.0f;
	const float standupHeight = 120.0f; //地板高度为0， 人站立眼睛高度120
	float jumpSpeed = 0.0f; //跳起速度
	float gravityAccSpeed = -0.3f; //重力加速度
	char textBuffer[1024];

	scene.setEyeXY(Vec2f(0.0f, 500.0f));
	scene.setEyeZ(standupHeight); // 站立观察高度
	scene.setLookdirXY(Vec2f(0.0f, -1000.0f));
	scene.updatePerspective();
	scene.updateView();

	for (; is_run(); delay_fps(60))
	{
		bar(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		setcolor(0);
		
		if (keystate('A') || keystate(key_left))
			scene.goLeft(motion);
		else if (keystate('D') || keystate(key_right))
			scene.goRight(motion);
		
		if (keystate('W') || keystate(key_up))
			scene.goForward(motion);
		else if (keystate('S') || keystate(key_down))
			scene.goBack(motion);

		auto& eye = scene.eyeXY();
		if (eye[0] < -900.0f) eye[0] = -900.0f;
		if (eye[0] > 900.0f) eye[0] = 900.0f;
		if (eye[1] < -900.0f) eye[1] = -900.0f;
		if (eye[1] > 900.0f) eye[1] = 900.0f;

		static bool isJumping = false;

		if (isJumping)
		{
			scene.setEyeZ(scene.eyeZ() + jumpSpeed);
			jumpSpeed += gravityAccSpeed;
			if (scene.eyeZ() < standupHeight)
			{
				scene.setEyeZ(standupHeight);
				isJumping = false;
			}
		}

		while (kbhit())
		{
			int key = getch();

			if (!isJumping && key == key_space)
			{
				isJumping = true;
				jumpSpeed = 12.0f;
			}
		}

		while (mousemsg())
		{
			static bool isMouseDown = false;
			static Vec2f lastMousePos;
			mouse_msg msg = getmouse();

			switch (msg.msg)
			{
			case mouse_msg_down:
					lastMousePos[0] = msg.x;
					lastMousePos[1] = msg.y;
					isMouseDown = true;
				break;
			case mouse_msg_up:
					isMouseDown = false;	
				break;
			case mouse_msg_move:
				if (isMouseDown)
				{
					Vec2f curr(msg.x, msg.y);
					Vec2f delta = curr - lastMousePos;
					scene.turn(delta[0] / 600.0f);
					scene.lookUp(delta[1] / 600.0f);
					lastMousePos = curr;
				}
				break;
			case mouse_msg_wheel:
				scene.lookIn(msg.wheel / 2000.0f);
				scene.updatePerspective();
				break;
			default:
				break;
			}

		}

		scene.updateView();
		scene.render(viewPort);

		static unsigned int frameCount = 0;
		if(frameCount % 6 == 0)
			sprintf(textBuffer, "使用方向键控制视点移动, 鼠标按住不放拖动视角\n空格键跳跃, 鼠标滚轮调节视景区\neye: %g, %g\nheight: %g\nlook dir: %g, %g", eye[0], eye[1], scene.eyeZ(), scene.lookDirXY()[0], scene.lookDirXY()[1]);

		++frameCount;
		setcolor(0x773333);
		outtextrect(20, 20, 500, 200, textBuffer);
	}

	closegraph();
	return 0;
}