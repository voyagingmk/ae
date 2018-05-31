#include "any.h"

using namespace std;

using namespace wynet;

int main(int argc, char **argv)
{
    Any i0 = 9;

    Any n;
    assert(n.is_null());

    string s1 = "foo";

    n = s1;
    assert(n.not_null());
    assert(n.is<string>());
    assert(!n.is<int>());

    int i1 = 10;
    n = i1;
    assert(n.not_null());
    assert(!n.is<string>());
    assert(n.is<int>());

    Any a1 = s1;

    assert(a1.not_null());
    assert(a1.is<string>());
    assert(!a1.is<int>());

    Any a2(a1);

    assert(a2.not_null());
    assert(a2.is<string>());
    assert(!a2.is<int>());

    string s2 = a2;

    assert(s1 == s2);

    printf("all test ok\n");
}