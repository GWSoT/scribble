#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>


// �����o�֐��|�C���^�̓L���X�g����ł��Ȃ��悤�Ȃ̂ŁAunion �ŋ����ɒl���擾���܂�
// ���Ȃ݂� C++ �̋K�i�͊֐��|�C���^�� void* �ւ̃L���X�g�͓���s��ƋK�肵�Ă��܂�
template<class T>
inline void* mfp_to_voidp(T v)
{
    union {
        T mfp;
        void *vp;
    } c;
    c.mfp = v;
    return c.vp;
}


template<class T>
inline void** get_vtable(T _this)
{
    return ((void***)_this)[0];
}

template<class T>
inline void set_vtable(T _this, void **vtable)
{
    ((void***)_this)[0] = vtable;
}



void **g_ID3D11DeviceContext_default_vtable;
void *g_ID3D11DeviceContext_hooked_vtable[115];

class DummyDeviceContext
{
public:

    // x86 �� x64 �ňȉ��̂悤�ɕ��򂵂Ȃ��ƃN���b�V������B�v����
#if defined(_WIN64)
    void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation ) {
        ID3D11DeviceContext *_this = (ID3D11DeviceContext*)this;
#elif defined(_WIN32)
    void DrawIndexedInstanced(ID3D11DeviceContext *_this, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation ) {
#endif

        OutputDebugStringA("DummyDeviceContext::DrawIndexedInstanced()\n");

        // �ꎞ�I�� vtable �����ɖ߂��Ė{���̓����������
        set_vtable(_this, g_ID3D11DeviceContext_default_vtable);
        _this->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        set_vtable(_this, g_ID3D11DeviceContext_hooked_vtable);
    }
};



void SetHook(ID3D11DeviceContext *pDeviceContext)
{
    size_t vtable_size = sizeof(g_ID3D11DeviceContext_hooked_vtable);
    void **vtable = get_vtable(pDeviceContext);

    // ���� vtable ��ۑ�
    g_ID3D11DeviceContext_default_vtable = vtable;

    // hook �֐����d���� vtable ���쐬
    memcpy(g_ID3D11DeviceContext_hooked_vtable, g_ID3D11DeviceContext_default_vtable, vtable_size);
    g_ID3D11DeviceContext_hooked_vtable[20] = mfp_to_voidp(&DummyDeviceContext::DrawIndexedInstanced);

    // vtable ������ւ���
    set_vtable(pDeviceContext, g_ID3D11DeviceContext_hooked_vtable);
}
