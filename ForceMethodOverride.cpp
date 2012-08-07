#include <cstdio>
#include <algorithm>


/// vtable �̎擾/�ݒ�
/// ���Ԃ񂱂�� C++ �I�Ɉ�@���Ǝv���܂����A���Ȃ��Ƃ� VisualC++ �ł͋@�\���܂��B
template<class T> inline void** get_vtable(T _this) { return ((void***)_this)[0]; }
template<class T> inline void   set_vtable(T _this, void **vtable) { ((void***)_this)[0] = vtable; }

/// �����o�֐��͕��ʂ̃L���X�g�������Ȃ��̂ŁAunion �ő�p
template<class D, class S> inline D force_cast(S v) { union {S s; D d;} u={v}; return u.d; }

class IHoge
{
public:
    virtual ~IHoge() {}
    virtual void Test()=0;

protected:
    void *vtable[2];
};

class Hoge : public IHoge
{
public:
    Hoge(int i)
    {
        void **old = get_vtable(this);
        std::copy(old, old+2, vtable);
        set_vtable(this, vtable);

        switch(i) {
        case 1: vtable[1] = force_cast<void*>(&Hoge::Test1); break;
        case 2: vtable[1] = force_cast<void*>(&Hoge::Test2); break;
        }
    }

    virtual void Test() { printf("Hoge::Test()\n"); }
    void Test1() { printf("Hoge::Test1()\n"); }
    void Test2() { printf("Hoge::Test2()\n"); }

private:
};

int main()
{
    Hoge h0(0), h1(1), h2(2);
    IHoge *i; // h0.Test(); ���� vtable ��Ȃ��R�[�h�ɂȂ��ăI�[�o�[���C�h����Ȃ��̂ŁA���̃|�C���^�o�R�ŌĂԕK�v������
    i=&h0; i->Test();
    i=&h1; i->Test();
    i=&h2; i->Test();
}

// result:
// Hoge::Test()
// Hoge::Test1()
// Hoge::Test2()

