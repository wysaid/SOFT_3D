/*
*  Author: wysaid
*    Mail: admin@wysaid.org
*    Blog: http://blog.wysaid.org
*/

#define SHOW_CONSOLE
#include "cgeScene.h"
#include "cgeMat.h"
#include "graphics.h"
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

	virtual void render(const Mat4& mvp, const Vec4f& viewPort) const
	{

	}

	inline vector<Vec3f>& positions() { return m_vecPositions; }
	inline const vector<Vec3f>& positions() const { return m_vecPositions; }

	inline vector<int> indices() { return m_vecIndices; }
	inline const vector<int> indices() const { return m_vecIndices; }

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

	void render(const Vec4f& viewPort) const
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
				positions.push_back(Vec3f(j / (float)w - 0.5f, i / (float)h - 0.5f, 0));
			}
		}

		return m;
	}	

	virtual void render(const Mat4& mvp, const Vec4f& viewPort) const
	{
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

				//逆时针方向绘制
				const auto& v1 = m_winCoords[thisLineIndex];
				const auto& v2 = m_winCoords[thisLineIndex + 1];
				const auto& v3 = m_winCoords[nextLineIndex + 1];
				const auto& v4 = m_winCoords[nextLineIndex];

				//Clip for perspective projection.
				if ((int&)v1[0] != 0xffffffff && (int&)v4[1] != 0xffffffff)
				{
					line(v1, v2);
					line(v2, v3);
					line(v3, v4);
					line(v4, v1);
				}
			}
		}
	}

protected:

	int m_width, m_height;
};

Mesh* getMesh(int w, int h, const Vec3f& rot, const Vec3f& scaling)
{
	Mesh* m = Mesh::create(w, h);
 	auto& matrix = m->matrix();

	matrix = Mat4::makeScale(scaling[0], scaling[1], scaling[2]);
	matrix.rotateX(rot[0]);
	matrix.rotateY(rot[1]);
	matrix.rotateZ(rot[2]);

	return m;
}

int main()
{
	initgraph(SCREEN_WIDTH, SCREEN_HEIGHT, INIT_RENDERMANUAL);
	setcaption("Soft Scene Roaming by wysaid.");
 	setbkmode(TRANSPARENT);
	setfillcolor(DARKGRAY);

	Scene scene(SCREEN_WIDTH, SCREEN_HEIGHT);

	const Vec4f viewPort(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
	Mesh* m = getMesh(15, 15, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(2000.0f, 2000.0f, 2000.0f));

	const float motion = 5.0f;
	const float standupHeight = 120.0f;

	scene.addObject(m);

	scene.setEyeXY(Vec2f(0.0f, 1000.0f));
	scene.setEyeZ(standupHeight); // 站立观察高度
	scene.setLookdirXY(Vec2f(0.0f, -1000.0f));
	scene.updatePerspective();
	scene.updateView();

	for (; is_run(); delay_fps(60))
	{
		cleardevice();
		
		if (keystate('A') || keystate(key_left))
			scene.goLeft(motion);
		else if (keystate('D') || keystate(key_right))
			scene.goRight(motion);
		
		if (keystate('W') || keystate(key_up))
			scene.goForward(motion);
		else if (keystate('S') || keystate(key_down))
			scene.goBack(motion);

		while (kbhit())
		{
			static bool isJumping = false;

			if (isJumping)
			{
				scene.setEyeZ(scene.eyeZ() - 10.0f);
				if (scene.eyeZ() < standupHeight)
				{
					scene.setEyeZ(standupHeight);
					isJumping = false;
				}
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
	}

	closegraph();
	return 0;
}