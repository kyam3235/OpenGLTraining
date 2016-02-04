/*
* Example of a Windows OpenGL program.
* The OpenGL code is the same as that used in
* the X Window System sample
*/
#include <windows.h> 
//#include <GL/gl.h> 
//#include <GL/glu.h> 
#include <GL/glut.h>

/* Windows globals, defines, and prototypes */
CHAR szAppName[] = "Win OpenGL";
HWND  ghWnd;
HDC   ghDC;
HGLRC ghRC;

#define SWAPBUFFERS SwapBuffers(ghDC) 
#define BLACK_INDEX     0
#define RED_INDEX       13 
#define GREEN_INDEX     14 
#define BLUE_INDEX      16 
#define WIDTH           300 
#define HEIGHT          200 

LONG WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL bSetupPixelFormat(HDC);

/* OpenGL globals, defines, and prototypes */
GLfloat latitude, longitude, latinc, longinc;
GLdouble radius;

#define GLOBE    1 
#define CYLINDER 2 
#define CONE     3 

GLvoid resize(GLsizei, GLsizei);
GLvoid initializeGL(GLsizei, GLsizei);
GLvoid drawScene(GLvoid);
void polarView(GLdouble, GLdouble, GLdouble, GLdouble);

//static const int order = 3;
//static const int n = 4;
//static const int knotn = 4 + 3; //n + order
//static const int knotn = 4 + 2; //n + order
//GLfloat ctrlPoint[4][4] = {
//	{-0.5, 0., 0., 1.},
//	{-0.25, 0.5, 0., 1.},
//	{0.25, -0.5, 0., 1.},
//	{0.5, 0., 0., 1.}
//};


static const int order = 2;	//線の次数(2～)
//static const int n = 5;		//線を描画する時のプロット数
//static const int knotn = 5 + 2; //n + order →　knotベクトルの要素数
static const int stride = 4;
//GLfloat knot[7] = { 0., 1., 2., 3., 4., 5., 6. };	//次数が高いほど曲線の表現力が高い
//GLfloat ctrlPoint[5][4] = {
//	{ -1.0, 0., 0., 1. },
//	{ -0.25, 0.5, 0., 1. },
//	{ 0.25, -0.5, 0., 1. },
//	{ 0.5, 0., 0., 1. },
//	{ 1.0, 0.3, 0., 1. }
//};
static const int n = 50000;
static int knotn = 50000 + 2;
GLfloat knot[50002];
GLfloat ctrlPoint[50002][4];

static GLUnurbsObj *nurbs;
void nurbsError(GLenum);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG        msg;
	WNDCLASS   wndclass;

	/* Register the frame class */
	wndclass.style = 0;
	wndclass.lpfnWndProc = (WNDPROC)MainWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hInstance, szAppName);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndclass.lpszMenuName = szAppName;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return FALSE;

	/* Create the frame */
	ghWnd = CreateWindow(szAppName,
		"Generic OpenGL Sample",
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		WIDTH,
		HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	/* make sure window was created */
	if (!ghWnd)
		return FALSE;

	/* show and update main window */
	ShowWindow(ghWnd, nCmdShow);

	UpdateWindow(ghWnd);

	/* animation loop */
	while (1) {
		/*
		*  Process all pending messages
		*/

		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE)
		{
			if (GetMessage(&msg, NULL, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				return TRUE;
			}
		}
		drawScene();
	}
}

/* main window procedure */
LONG WINAPI MainWndProc(
	HWND    hWnd,
	UINT    uMsg,
	WPARAM  wParam,
	LPARAM  lParam)
{
	LONG    lRet = 1;
	PAINTSTRUCT    ps;
	RECT rect;

	switch (uMsg) {

	case WM_CREATE:
		ghDC = GetDC(hWnd);
		if (!bSetupPixelFormat(ghDC))
			PostQuitMessage(0);

		ghRC = wglCreateContext(ghDC);
		wglMakeCurrent(ghDC, ghRC);
		GetClientRect(hWnd, &rect);
		initializeGL(rect.right, rect.bottom);
		break;

	case WM_PAINT:
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_SIZE:
		GetClientRect(hWnd, &rect);
		resize(rect.right, rect.bottom);
		break;

	case WM_CLOSE:
		if (ghRC)
			wglDeleteContext(ghRC);
		if (ghDC)
			ReleaseDC(hWnd, ghDC);
		ghRC = 0;
		ghDC = 0;

		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		if (ghRC)
			wglDeleteContext(ghRC);
		if (ghDC)
			ReleaseDC(hWnd, ghDC);

		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_LEFT:
			longinc += 0.5F;
			break;
		case VK_RIGHT:
			longinc -= 0.5F;
			break;
		case VK_UP:
			latinc += 0.5F;
			break;
		case VK_DOWN:
			latinc -= 0.5F;
			break;
		}

	default:
		lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	return lRet;
}

BOOL bSetupPixelFormat(HDC hdc)
{
	PIXELFORMATDESCRIPTOR pfd, *ppfd;
	int pixelformat;

	ppfd = &pfd;

	ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
	ppfd->nVersion = 1;
	ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER;
	ppfd->dwLayerMask = PFD_MAIN_PLANE;
	ppfd->iPixelType = PFD_TYPE_COLORINDEX;
	ppfd->cColorBits = 8;
	ppfd->cDepthBits = 16;
	ppfd->cAccumBits = 0;
	ppfd->cStencilBits = 0;

	pixelformat = ChoosePixelFormat(hdc, ppfd);

	if ((pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0)
	{
		MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
		return FALSE;
	}

	if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE)
	{
		MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
		return FALSE;
	}

	return TRUE;
}

/* OpenGL code */

GLvoid resize(GLsizei width, GLsizei height)
{
	GLfloat aspect;

	glViewport(0, 0, width, height);

	aspect = (GLfloat)width / height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluPerspective(45.0, aspect, 3.0, 7.0);
	gluPerspective(0.0, aspect, 3.0, 7.0);
	glMatrixMode(GL_MODELVIEW);
}

GLvoid createObjects()
{
	GLUquadricObj *quadObj;

	nurbs = gluNewNurbsRenderer();
	gluNurbsProperty(nurbs, GLU_SAMPLING_METHOD, GLU_PATH_LENGTH);
	gluNurbsProperty(nurbs, GLU_SAMPLING_TOLERANCE, 10.);
	gluNurbsProperty(nurbs, GLU_DISPLAY_MODE, GLU_FILL);
	gluNurbsProperty(nurbs, GLU_AUTO_LOAD_MATRIX, GL_TRUE);
	//gluNurbsProperty(nurbs, GLU_ERROR, (void *)nurbsError);

	for (int i = 0; i < 50002; i++){
		knot[i] = i;
	}

	for (int i = 0; i < 50002; i++){
		ctrlPoint[i][0] = (i - 25001.0) / 25001.0;
		ctrlPoint[i][1] = (rand() - (RAND_MAX / 2.)) / (RAND_MAX / 2.);
		ctrlPoint[i][2] = 0.;
		ctrlPoint[i][3] = 1.;
	}
}

GLvoid initializeGL(GLsizei width, GLsizei height)
{
	GLfloat     maxObjectSize, aspect;
	GLdouble    near_plane, far_plane;

	glClearIndex((GLfloat)BLACK_INDEX);
	glClearDepth(1.0);

	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	aspect = (GLfloat)width / height;
	//gluPerspective(45.0, aspect, 3.0, 7.0);
	gluPerspective(0.0, aspect, 3.0, 7.0);
	glMatrixMode(GL_MODELVIEW);

	createObjects();
}

GLvoid drawScene(GLvoid)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	gluBeginCurve(nurbs);
	gluNurbsCurve(nurbs, knotn, knot, stride, &ctrlPoint[0][0], order, GL_MAP1_VERTEX_4);
	gluEndCurve(nurbs);

	glPopMatrix();

	SWAPBUFFERS;
}

void nurbsError(GLenum error)
{
	exit(0);
}