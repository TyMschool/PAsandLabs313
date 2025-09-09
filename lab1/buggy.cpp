#include <iostream>
#include <cstring> // for memcpy

struct Point {
    int x, y;

    Point () : x(), y() {};
    Point (int _x, int _y) : x(_x), y(_y) {};
};

class Shape {
    int vertices;
    Point** points;

public:
    Shape (int _vertices) {
        vertices = _vertices;
        points = new Point*[vertices+1];
    }

    ~Shape () { //garbage collect
        for (int i = 0; i < vertices; i++) {
            delete points[i];
        }
        delete[] points;    
    }

    void addPoints (Point* pts) {
        // fixed bounds for the two for loops. Dont want it to go out of bouds.
        for (int i = 0; i < vertices; i++) {
            points[i] = new Point(pts[i % vertices].x, pts[i % vertices].y); // added this for points below. wasnt filled
            memcpy(points[i], &pts[i%vertices], sizeof(Point));
        }
    }

    double area () { //return type had no reason to be a pointer.
        int sum = 0;
        for (int i = 0; i < vertices; i++) {
            // used -> and (*). to access. using modulo to "wrap" around after + 1
            int lhs = points[i]->x * points[(i+1)%vertices]->y;
            int rhs = (*points[(i+1)%vertices]).x * (*points[i]).y; 
            sum += (lhs - rhs);
        }
        return std::abs(sum)/2.0; 
    }
};

int main () {
    Point tri1;
    tri1.x = 0;
    tri1.y = 0;

    Point tri2(1,2);

    Point tri3 = {2,0};

    // adding points to tri
    Point triPts[3] = {tri1, tri2, tri3};
    Shape* tri = new Shape(3);
    tri->addPoints(triPts); //tri is a pointer. use ->


    Point quad1(0,0);
    Point quad2(0,2);
    Point quad3(2,2);
    Point quad4(2,0);

    // adding points to quad
    Point quadPts[4] = {quad1, quad2, quad3, quad4};
    Shape* quad = new Shape(4);
    quad->addPoints(quadPts);

    double triArea = tri->area();
    double quadArea = quad->area();
    std::cout << "area of tri is " << triArea << "." << std::endl <<"Area of quad is " << quadArea;

    delete tri;
    delete quad;
    return 0;
}
