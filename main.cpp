#include <memory>
#include <iostream>
#include <vector>

#include "Query.h"

class A
{
public:
    typedef std::shared_ptr<A> ptr;

public:
    A() : mA(0), mFlag(false) { }
    A(int a) : mA(a), mFlag(false) { }

public:
    bool IsFlag() const { return mFlag; }
    void SetFlag(bool flag) { mFlag = flag; }

    bool CondTrue() const { return true; }
    bool CondFalse() const { return false; }

    void Increment() { ++mA; }
    int Get() const { return mA; }
    void Set(int a) { mA = a; }

    bool Test(int val) const { return val == mA; }

    void Print() { std::cout << mA << std::endl; }

private:
    int mA;
    bool mFlag;
};

int main()
{
    std::vector<A::ptr> myList(10);
    int ndx = 0;
    for (auto& a : myList)
        a = std::make_shared<A>(5);

    myList[2]->SetFlag(true);
    myList[6]->SetFlag(true);

    std::cout << Query::For(myList).
                 Where(&A::IsFlag).
                //Where(Less(&A::Get, 5)).
                // Apply(&A::Get).
                // Apply(plus_const(8)).
                Max()
             << std::endl;



    return 0;
}