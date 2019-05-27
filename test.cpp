#include <iostream>
#include <cstdio>

using namespace std;

struct Node {
	int a;
	int b;
	char *c;
	Node *d[1];
};
int main()
{
	Node n;
	cout << "node size:" << sizeof(Node) << ", " << sizeof(int) << ", " << sizeof(char *) << ", " << sizeof(n) << endl;
	return 0;
}

