#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <SOIL.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#else
#include <GL/glut.h>
#endif

#define VERBOSE 0
#define RESOLUTION 2000.0

#define FPS24 41.67
#define FPS30 33.33
#define FPS60 16.67
#define FPS90 11.11
float FR = FPS60;


using namespace std;

class Vertex{
public:
	double x;
	double y;
	Vertex(){};
	Vertex(double xx, double yy){
		x=xx;
		y=yy;
	}

	static Vertex delta(Vertex a, Vertex b){
		return Vertex(b.x-a.x, b.y-a.y);
	}

	static double dot(Vertex a, Vertex b){
		return (a.x*b.x)+(a.y*b.y);
	}

	void scale(double s){
		x *= s;
		y *= s;
	}

	void minus(Vertex v){
		x-=v.x;
		y-=v.y;
	}

	void plus(Vertex v){
		x+=v.x;
		y+=v.y;
	}
};
vector<Vertex> vertices;
vector<int> stickyVertexIndices;

class VertexIndexPair{
public:
	int uno;
	int dos;
	VertexIndexPair(int a, int b){
		uno = a;
		dos = b;
	}
};
vector<VertexIndexPair> fixDistanceVertexIndices;
vector<VertexIndexPair> paintIndices;

int width;
int height;

// Mouse variables-- could be done better
int mouseMove=0;
int followMouse = 0;
int lastx=0, lasty=0;


// Transformations
int xTrans=0;
int yTrans=0;
int xAngle=0;
int yAngle=0;
int xCentroid=0;
int yCentroid=0;
float scale = 1;

// Candy

int faceSize = 50;
int stick = 0;

// Highlighting
int inNeighborhood = 0;
Vertex nearestNeighbor(0,0);

int efEnable = 0;

void myInit();
void display();
void reshape(int x, int y);
void keyboard(unsigned char c, int x, int y);
void mouse(int b, int s, int x, int y);
void motion(int x, int y);
void passiveMotion(int x, int y);
void showHelp();
void idle();
void setCentroid();

// Helper Functions
int factorial(int i);
long int nChoosek(int a, int b);
double power(double b, int e);
int xMin();
int xMax();


class LagrangeCurve{
public:
	vector<Vertex> lVertices;

	void newPoint(){
		int k = vertices.size()-1;
		int xmin = xMin();
		int xmax = xMax();
		lVertices.clear();
		float step = (xmax-xmin)/RESOLUTION;
		int lastPoint = 0;
		for(float i=xmin; i<=xmax+1; i+=step){
			if(xmin==xmax) break;
			if(i>xmax){
				lastPoint=1;
				i=xmax;
			}
			Vertex vert(i,0);
			for (int j = 0; j<=k; j++){ // sigma
				float coeff=1;
				for(int m=0; m<=k; m++){ // pi
					if (m==j) continue;
					coeff*=((i - vertices[m].x) / (vertices[j].x-vertices[m].x));
				}
				vert.y += vertices[j].y*coeff;
			}
			lVertices.push_back(vert);
			if(lastPoint) break;
		}
	}

	void clear(){
		lVertices.clear();
	}
};
LagrangeCurve lCurve;

class Physics{
public:	
	vector<Vertex> pVertices;
	vector<Vertex> cVertices;
	vector<Vertex> nVertices;
	
	vector<Vertex> forces;

	int accuracy = 10;
	int projectile = 2;

	Physics(){
		Vertex grav(0,-10); ///////////////////////////////////////////////////// gravity
		forces.push_back(grav);
	}

	void sync(){
		pVertices.clear();
		nVertices.clear();
		cVertices.clear();
		
		cVertices = vertices;
		pVertices = vertices;
		// enableProjectile(); /////////////////////////////////////////////////////////////// Projectile
	}

	void enableProjectile(){
		for(int i=0; i<vertices.size(); i++){
			pVertices[i].x -= projectile;
			pVertices[i].y += projectile;
		}
	}

	void nextStep(){
		varlet();
		constraint();
	}

	void varlet(){
		nVertices.clear();

		for (int i=0; i<vertices.size(); i++){
			Vertex next;
			Vertex curr = cVertices[i];
			Vertex prev = pVertices[i];

			next.x = 2*curr.x - prev.x;
			next.y = 2*curr.y - prev.y;

			for(int j=0; j<forces.size(); j++){
				next.x -= 0.00005*(forces[j].x*(FR*FR));
				next.y -= 0.00005*(forces[j].y*(FR*FR));
			}
			nVertices.push_back(next);
		}

		pVertices = cVertices;
		cVertices = nVertices;
	}

	void constraint(){
		for (int j=0; j<accuracy; j++){
			
			//C
			boundEm(); //////////////////////////////////////////////////////// bound

			//C
			if (followMouse){
				cVertices[1].x=lastx;
				cVertices[1].y=lasty;
			}
			for (int i : stickyVertexIndices){
				sticky(i);
			}

			//C
			// resetDistance(); //////////////////////////////////////////////////// length const
			for(VertexIndexPair a: fixDistanceVertexIndices){
				resetDistance(a.uno, a.dos);
				// limitDistance(a.uno, a.dos,2);
			}
			// resetDistance(0,2);
			// limitDistance(2,4,100); ///////////////////////////////////////////// ragdoll
			// limitDistance(0,2,10); ///////////////////////////////////////////// ragdoll
			// limitDistance(4,6,10); ///////////////////////////////////////////// ragdoll
			
		}
	}

	void sticky(int i){
		cVertices[i] = vertices[i];
	}

	void boundEm(){
			for(int i=0; i<vertices.size(); i++){
				cVertices[i].x = (cVertices[i].x<50) ? 50 : cVertices[i].x;
				cVertices[i].x = (cVertices[i].x>450) ? 450 : cVertices[i].x;
				cVertices[i].y = (cVertices[i].y<50) ? 50 : cVertices[i].y;
				cVertices[i].y = (cVertices[i].y>450) ? 450 : cVertices[i].y;
			}
	}

	void resetDistance(int a, int b){
		
		if (a >= vertices.size() || b >= vertices.size()){
			cout << "resetDistance" << endl;
			exit(-1);
		}

		Vertex d = Vertex::delta(cVertices[a], cVertices[b]);
		Vertex r = Vertex::delta(vertices[a], vertices[b]);
		double dp = Vertex::dot(d, d);
		double rl = Vertex::dot(r, r);
		double sc = ((rl)/(dp+rl))-0.5;
		d.scale(sc);
		cVertices[a].minus(d);
		cVertices[b].plus(d);
	}

	void resetDistance(){
		for (int i=0; i<vertices.size()-1; i++){
			resetDistance(i,i+1);
		}
	}

	void limitDistance(int a, int b, int dist){
		
		if (a >= vertices.size() || b >= vertices.size()){
			cout << "limitDistance" << endl;
			exit(-1);
		}

		Vertex d = Vertex::delta(cVertices[a], cVertices[b]);
		Vertex r = Vertex::delta(vertices[a], vertices[b]);
		double dp = Vertex::dot(d, d);
		double rl = Vertex::dot(r, r);
		
		if(abs(dp-rl)>(dist*dist)){
			double sc = ((rl)/(dp+rl))-0.5;
			d.scale(sc/dist);
			cVertices[a].minus(d);
			cVertices[b].plus(d);
		}
	}

};

Physics phy;

enum DrawTypes{
	simpleJoin=0,
	physics,
	lagrange,
	none
};

DrawTypes drawingType;

int main(int argc, char* argv[]) {

	glutInit(&argc, argv);
	myInit();
	showHelp();
	glutMainLoop();
	return 0;
}


void timer(int a){
	display();
}

GLuint TEX;
void loadTex(string fTex){
	cout << "Loading " << fTex << endl;
	// Load/Push Textures
	int width, height;
	unsigned char* image;
	width=0;
	height=0;
	image = SOIL_load_image(fTex.c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
	if (image == NULL){
		cout << "Failed to Load Fur." << endl;
		exit(-1);
	}
	glGenTextures(1, &TEX);
	glBindTexture(GL_TEXTURE_2D, TEX);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	stick = 1;

}

void loadSkeleton(string fname){
	ifstream ske(fname);
	if (ske.is_open()){

		// parse the file; load vertices and constraints
		string word;
		while(ske >> word){
			// cout << word << endl;
			if (word[0]=='#'){ // a comment
				getline(ske, word);
			}
			else{ // not a comment
				if (word == "v"){
					double x, y;
					ske >> x >> y;
					// cout << x << "," << y << endl;
					// double s = 1.5, mx = 150, my = 35;
					// double s = 1.5, mx = 150, my = 100; // cloth 100x100
					double s = 3, mx = 250, my = 100; // cloth 50x50
					// double s = 20, mx = 250, my = 150; // sticky

					Vertex v((s*x)+mx,(s*y)+my);
					vertices.push_back(v);
				}

				if (word == "c"){ // constraints
					ske >> word;
					
					if (word == "1"){ // sticky
						int vInd;
						ske >> vInd;
						stickyVertexIndices.push_back(vInd);
					}

					if (word == "2"){ // fixed dist
						int v1, v2;
						ske >> v1 >> v2;
						VertexIndexPair a(v1,v2);
						fixDistanceVertexIndices.push_back(a);
					}
				}

				if (word == "p"){
					cout << "here" << endl;
					int v1, v2;
					ske >> v1 >> v2;
					VertexIndexPair a(v1,v2);
					paintIndices.push_back(a);
				}
			}
		}
		ske.close();
	}
	else{
		cout << "ERROR: Can't open the Skeleton File.\n" << endl;
		exit(-1);
	}
}

void myInit(){
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_MULTISAMPLE);
	glutInitWindowSize(500,500);
	glutInitWindowPosition(750,100);
	glutCreateWindow("Dance for me!");
	

	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(passiveMotion);
	
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	glClearColor(0.1,0.1,0.1,1);
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(0.0, 0.0, 0.5,\
		0.0,0.0,0.0,\
		0.0,1.0,0.0);

	drawingType = none;

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glDisable (GL_DEPTH_TEST);

	loadSkeleton("skeletons/cloth50x50");


	// loadSkeleton("skeletons/stick");
	// loadTex("textures/kim.png");
}

void reshape(int x, int y){
	if(VERBOSE) cout << "))reshape" << endl;
	width = x;
	height = y;
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,width, 0,height, -width,width);
	glMatrixMode(GL_MODELVIEW);
	glutPostRedisplay();
}

void display(){
	glClear(GL_COLOR_BUFFER_BIT);

	glColor3f(1,1,1);
	glLineWidth(2.5);


	if (drawingType == simpleJoin){
		glBegin(GL_LINE_STRIP);
		for (int i=0; i<vertices.size(); i++){
			glVertex3f(vertices[i].x, height-vertices[i].y, 0);
		}
		glEnd();
	}
	else if (drawingType == lagrange){
		glBegin(GL_LINE_STRIP);
		for (int i=0; i<lCurve.lVertices.size(); i++){
			glVertex3f(lCurve.lVertices[i].x, height-lCurve.lVertices[i].y, 0);
		}
		glEnd();
	}
	else if (drawingType == physics){
		phy.nextStep();

		// glBegin(GL_LINE_STRIP);
		// for (int i=0; i<phy.cVertices.size(); i++){
		// 	glVertex3f(phy.cVertices[i].x, height-phy.cVertices[i].y, 0);
		// }
		// glEnd();

		for (int i=0; i<paintIndices.size(); i++){
			glBegin(GL_LINE_STRIP);
				glVertex3f(phy.cVertices[paintIndices[i].uno].x, height - phy.cVertices[paintIndices[i].uno].y, 0);
				glVertex3f(phy.cVertices[paintIndices[i].dos].x, height - phy.cVertices[paintIndices[i].dos].y, 0);
			glEnd();
		}
		
		glColor3f(0.1,0.5,0.4);
		glPointSize(10.0);
		glBegin(GL_POINTS);
		int count = phy.cVertices.size();
		for (int i=0; i<count; i++){
			glColor3f(0.1,0.8,((float)i)/count);
			glVertex3f(phy.cVertices[i].x, height-phy.cVertices[i].y, 0);
		}
		glEnd();

		if(stick){
			glColor3f(1,1,1);
			glEnable ( GL_BLEND );
			glBegin(GL_QUADS);
				glTexCoord2f(0,0);
				glVertex3f(phy.cVertices[0].x-faceSize, height - phy.cVertices[0].y+faceSize, 0);

				glTexCoord2f(0,1);
				glVertex3f(phy.cVertices[0].x-faceSize, height - phy.cVertices[0].y-faceSize, 0);

				glTexCoord2f(1,1);
				glVertex3f(phy.cVertices[0].x+faceSize, height - phy.cVertices[0].y-faceSize, 0);

				glTexCoord2f(1,0);
				glVertex3f(phy.cVertices[0].x+faceSize, height - phy.cVertices[0].y+faceSize, 0);
			glEnd();
			glDisable ( GL_BLEND );
		}
		
		glutTimerFunc(FR, timer, 0);
	}
	else if (drawingType == none){
		//Highlight Selectable Vertex
		if(inNeighborhood){
			glPointSize(15.0);
			glColor3f(0,0.5,0.4);
			glBegin(GL_POINTS);
			glVertex3f(nearestNeighbor.x,height-nearestNeighbor.y,0);
			glEnd();
		}

		glColor3f(1,1,1);
		// glBegin(GL_LINE_STRIP);
		// for (int i=0; i<vertices.size(); i++){
		// 	// glVertex3f(vertices[i].x, height-vertices[i].y, 0);
		// }
		// glEnd();
		for (int i=0; i<paintIndices.size(); i++){
			glBegin(GL_LINE_STRIP);
				glVertex3f(vertices[paintIndices[i].uno].x, height - vertices[paintIndices[i].uno].y, 0);
				glVertex3f(vertices[paintIndices[i].dos].x, height - vertices[paintIndices[i].dos].y, 0);
			glEnd();
		}
		glPointSize(4.0);
		glColor3f(1,0,0);
		glBegin(GL_POINTS);
		for (int i=0; i<vertices.size(); i++){
			glVertex3f(vertices[i].x, height-vertices[i].y, 0);
		}
		glEnd();

		if(stick){
			glColor3f(1,1,1);
			glEnable ( GL_BLEND );
			glBegin(GL_QUADS);
				glTexCoord2f(0,0);
				glVertex3f(vertices[0].x+faceSize, height - vertices[0].y+faceSize, 0);

				glTexCoord2f(0,1);
				glVertex3f(vertices[0].x+faceSize, height - vertices[0].y-faceSize, 0);

				glTexCoord2f(1,1);
				glVertex3f(vertices[0].x-faceSize, height - vertices[0].y-faceSize, 0);

				glTexCoord2f(1,0);
				glVertex3f(vertices[0].x-faceSize, height - vertices[0].y+faceSize, 0);
			glEnd();
			glDisable ( GL_BLEND );
		}
	}

	glPopMatrix();

	// AXES
	glPushMatrix();
	glLoadIdentity();
	glLineWidth(5);
	float cDepth = 0.5;
	glBegin(GL_LINE_STRIP);// X axis
		glColor3f(cDepth,0,0);
		glVertex3f(0,0,0);
		glVertex3f(width,0,0);
	glEnd();
	glBegin(GL_LINE_STRIP);// Y axis
		glColor3f(0,cDepth,0);
		glVertex3f(0,0,0);
		glVertex3f(0,height,0);
	glEnd();
	if(efEnable){
		glLineWidth(500);
		glBegin(GL_LINE_STRIP);
			glColor3f(0,1,1);
			glVertex3f(width,0,0);
			glVertex3f(width,height,0);
		glEnd();
		glLineWidth(5);
	}
	glPopMatrix();


	// Bounding Box
	glLineWidth(1);
	glColor3f(0.5,0.5,0.5);
	glBegin(GL_LINE_LOOP);// X axis
		glVertex3f(50,50,0);
		glVertex3f(450,50,0);
		glVertex3f(450,450,0);
		glVertex3f(50,450,0);
	glEnd();

	glutSwapBuffers();
}


void keyboard(unsigned char c, int x, int y){
	if (c == 27 || c == 81 || c == 113) { // ESC or Q
		cout << "\nExit" << endl;
		exit(0);
	}
	else if (c == 67 || c == 99){ // C
		vertices.clear();
		lCurve.clear();
		xTrans = 0;
		yTrans = 0;
		xAngle = 0;
		yAngle = 0;
		scale=1;
		followMouse = 0;
		drawingType = none;
		cout << "C: Screen Cleared.." << endl;
	}
	else if (c == 76 || c == 108){ // L
		cout << "L: Drawing lagrange" << endl;
		drawingType = lagrange;
	}
	else if (c == 83 || c == 115){ // S
		cout << "S: Connecting dots" << endl;
		drawingType = simpleJoin;
	}
	else if (c == 78 || c == 110){ // N
		cout << "N: Showing points only" << endl;
		drawingType = none;
	}
	else if (c == 82 || c == 114){ // R
		cout << "R: Resetting Transformations" << endl;
		xTrans = 0;
		yTrans = 0;
		xAngle = 0;
		yAngle = 0;
		scale=1;
		followMouse = 0;
	}
	else if ( c == 43 || c == 61 ){ // + or =
		cout << "+: Zooming in.." << endl;
		scale*=1.25;
	}
	else if ( c == 45 || c == 95){ // - or _
		cout << "-: Zooming out.." << endl;
		scale*=0.75;
	}

	else if ( c == 'p' || c == 'P'){
		cout << "Running Physics Engine!" << endl;
		drawingType = physics;
		phy.sync();
	}

	else if( c == 'g'){
		cout << "change in G" << endl;
		phy.forces[0].y*=-1;
	}
	else if( c == 'G'){
		cout << "switching G" << endl;
		static int gf = 1;
		if(gf) phy.forces[0].y=0; // g -10 to 0
		else phy.forces[0].y=-10; // g 0 to -10
		gf = !gf;
	}

	else if( c == 'e' || c == 'e'){
		cout << "switch EMF" << endl;
		if(phy.forces.size()!=2){
			Vertex ef(-50,0);
			phy.forces.push_back(ef);
			efEnable = 1;
		} else {
			phy.forces.pop_back();
			efEnable = 0;
		}
	}

	
	glutPostRedisplay();
}

void addDot(int x, int y){
	x-=xTrans;
	y+=yTrans;
	vertices.push_back(Vertex(x,y));
	lCurve.newPoint();
	setCentroid();
	glutPostRedisplay();
}

void removeDot(Vertex){
	for (int i=0; i<vertices.size(); i++){
		if (vertices[i].x==nearestNeighbor.x && vertices[i].y==nearestNeighbor.y){
			vertices.erase(vertices.begin()+i);
		}
	}
	lCurve.newPoint();
	
	inNeighborhood=0;
	setCentroid();
	glutPostRedisplay();
}

void mouse(int b, int s, int x, int y){
	
	if (b == GLUT_LEFT_BUTTON){
		lastx = x;
		lasty = y;
		if(!mouseMove && s == GLUT_UP) {
			if (!inNeighborhood) addDot(x,y);
			else removeDot(nearestNeighbor);
		}
		else if(mouseMove && s == GLUT_UP) mouseMove=0;
	}

	if (b == GLUT_RIGHT_BUTTON && s == GLUT_DOWN){
		lastx = x;
		lasty = y;
		followMouse = !followMouse;
	}
}

void motion(int x, int y){
	xTrans += (x - lastx);
	yTrans -= (y - lasty);
	lastx=x;
	lasty=y;
	mouseMove = 1;
	glutPostRedisplay();
}

void passiveMotion(int x, int y){
	if(followMouse){
		xAngle += (x - lastx);
		yAngle -= (y - lasty);
		lastx = x;
		lasty = y;
		glutPostRedisplay();
	}
	int thresh = 15;
	for (Vertex v : vertices){
		if(power(v.x - x, 2)+power(v.y - y,2) <= power(thresh,2)){
			inNeighborhood = 1;
			nearestNeighbor = v;
			glutPostRedisplay();
			break;
		}
		else if(power(v.x - x, 2)+power(v.y - y,2) <= power(5*thresh,2)){
			inNeighborhood = 0;
			glutPostRedisplay();
		}
	}
}


void setCentroid(){
	int sumx=0;
	int sumy=0;
	for (Vertex v : vertices){
		sumx+=v.x;
		sumy+=v.y;
	}
	int n = vertices.size();
	if(n==0) return;
	xCentroid = sumx/n;
	yCentroid = height-(sumy/n);

}

void showHelp(){
	cout << "Q or ESC to exit" << endl;
	cout << "C to clear" << endl;
	cout << "S for connecting dots (default)" << endl;
	cout << "L for lagrange" << endl;
	cout << "N for drawing only points" << endl;
	cout << "Click n Drag for translation" << endl;
	cout << "Right Click and move for rotation" << endl;
	cout << "R for resetting translations and rotations" << endl;
	cout << "*****" << endl;
}

// Helper Functions
int factorial(int i){
	if (i<=1) return 1;
	return i*factorial(i-1);
}

long int nChoosek(int n, int k){
	int min = ((n-k)<k)?(n-k):k;
	int max = ((n-k)>k)?(n-k):k;
	long int result = 1, temp_k = min;
	for(int temp = n; temp>max; temp--){
		result *= temp;
		if((!(result%temp_k))&&(temp_k>1))
		{
			result = result / temp_k;
			temp_k--;
		}
	}
	while(temp_k>1){
		result = result/temp_k--;
	}
	return result;
}

double power(double b, int e){
	if (e==0) return 1;
	if (e==1) return b;
	return b*power(b,e-1);
}

int xMin(){
	int min = vertices[0].x;
	for (Vertex v : vertices){
		if(v.x<min) min = v.x;
	}
	return min;
}

int xMax(){
	int max = vertices[0].x;
	for (Vertex v : vertices){
		if(v.x>max) max = v.x;
	}
	return max;
}