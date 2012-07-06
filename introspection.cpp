// GameEngineGems2 �ŏЉ��Ă����Aclass �̃����o�ϐ��ɓ���I�ɃA�N�Z�X�����i��p�ӂ��āAserializer �� tool �Ƃ̘A���̎����Ȃǂ�֗��ɂ���A�Ƃ����A�C�f�A�̊ȒP�Ȏ�����B
// GEG2 �̃\�[�X�� https://github.com/jwatte/C---Introspection-and-Properties
// 
// ���� INTROSPECTION() �̃}�N�����A�N�Z�X��i������ (MEMBER() �̌��� "," �����Ȃ��g���b�N)
// struct TestData
// {
//     int hoge;
//     float hage;
//     std::string hige;
// 
//     INTROSPECTION(
//         TestData,
//         MEMBER(hoge)
//         MEMBER(hage)
//         MEMBER(hige)
//     )
// };
// 
// ���̂悤�ɂ��ă����o�ϐ�������ł��� 
// int main()
// {
//     TestData t = {0, 1.0f, "test"};
//     const MemberInfoCollection &members = t.GetMemberInfo();
//     for(size_t i=0; i<members.size(); ++i) {
//         printf("%s: ", members[i]->GetName());
//         members[i]->PrintValue(&t);
//         printf("\n");
//     }
// }
// 
// �o�͌��ʁF
// hoge: 0
// hage: 1.000000
// hige: "test"


#include <cstdio>
#include <cstdlib>
#include <string>

#define INTROSPECTION(type, members) \
    typedef type self_t; \
    static const MemberInfoCollection& GetMemberInfo() { \
        static IMemberInfo *data[] = { \
            members \
        }; \
        static MemberInfoCollection collection(data, _countof(data));\
        return collection;\
    }
#define MEMBER(name) \
    CreateMemberInfo(&self_t::name, #name),


struct IMemberInfo
{
    virtual ~IMemberInfo() {}
    virtual const char* GetName() const=0;
    virtual void PrintValue(const void *obj) const=0;
    // serializer �Ȃǂ���������ۂ� GetValue(), SetValue(), GetSize() �Ȃǂ������ɒǉ�
};

struct MemberInfoCollection
{
    IMemberInfo **data;
    size_t num;
    MemberInfoCollection(IMemberInfo **d, size_t n) : data(d), num(n) {}
    IMemberInfo* operator[](int i) const { return data[i]; }
    size_t size() const { return num; }
};

template<class T> void TPrintValue(const T &v);
template<> void TPrintValue<int>(const int &v) { printf("%d", v); }
template<> void TPrintValue<float>(const float &v) { printf("%f", v); }
template<> void TPrintValue<std::string>(const std::string &v) { printf("\"%s\"", v.c_str()); }

template<class T, class MemT>
struct MemberInfo : public IMemberInfo
{
    MemT T::*data;
    const char *name;
    MemberInfo(MemT T::*d, const char *n) : data(d), name(n) {}
    virtual const char* GetName() const { return name; }
    virtual void PrintValue(const void *obj) const { TPrintValue(reinterpret_cast<const T*>(obj)->*data); }
};
template<class T, class MemT>
IMemberInfo* CreateMemberInfo(MemT T::*data, const char *name)
{
    return new MemberInfo<T,MemT>(data, name);
}


struct TestData
{
    int hoge;
    float hage;
    std::string hige;

    INTROSPECTION(
        TestData,
        MEMBER(hoge)
        MEMBER(hage)
        MEMBER(hige)
    )
};

int main()
{
    TestData t = {0, 1.0f, "test"};
    const MemberInfoCollection &members = t.GetMemberInfo();
    for(size_t i=0; i<members.size(); ++i) {
        printf("%s: ", members[i]->GetName());
        members[i]->PrintValue(&t);
        printf("\n");
    }
}
