```C++
#include <iostream>
#include <functional>

using namespace std;

class A{
public:
    A(){}

    int test(int n){
        cout<<n<<endl;
        return n;
    }

    int operator()(int n){
        cout<<n<<endl;
        return n;
    }
};

int test(int n){
    cout<<n<<endl;
    return n;
}

int bind_test(int a, int b, int c){
    cout<<a<<b<<c<<endl;
}

int main(){
    // nomoral function
    cout<<"nomoral function: ";
    function<int(int)> f1 = test;
    f1(12);

    // anonymity function
    cout<<"anonymity function: ";
    function<int(int)> f2 = [](int n)->int{
        cout<<n<<endl;
        return n;
    };
    f2(34);

    // class function
    cout<<"class function: ";
    function<int(A*, int)> f3 = &A::test;
    A a;
    f3(&a, 56);

    // functor
    cout<<"functor: ";
    function<int(int)> f4 = a;
    a(78);

    // bind
    cout<<"bind1: ";
    auto foo1 = bind(bind_test, 1, 2, 3);
    foo1();

    cout<<"bind2: ";
    auto foo2 = bind(bind_test, 1, std::placeholders::_1, 3);
    foo2(2);

    cout<<"bind3: ";
    auto foo3 = bind(bind_test, std::placeholders::_2, std::placeholders::_1, 3);
    foo3(2, 1);

    return 0;
}
```

打印输出：

```C++
```

