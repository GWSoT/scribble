
// �Ⴆ�΁A������̊֐�������Ή�����֐����Ăԋ@�\���l����B
// ���񕶎���Ō�������͖̂��ʂȂ̂ŁA1 �x���������񂩂� ID �𓾂āA
// ���Ƃ͂��� ID ����֐��|�C���^�z��̗v�f�𓾂�Ȃ肷��悤�Ȏ����ɂȂ�Ǝv����B
//
// �����ă��[�U�[���ł� return_value = CallFunctionByName(DoSomething, arg1)
// �̂悤�ȏ��������ł���悤�ɂ��āA��L�̂悤�ȓ����������ӎ������ɍςނ悤�ɂ������B
// ������������邽�߁A�}�N���̒��� static �ϐ���p�ӂ��� ID ���L�����������B


// ���ꂾ�� CallFunctionByID �̖߂�l��Ԃ��Ȃ��B
// �]���̓}�N���̈����Ɏ󂯎��ϐ����w�肷�邵���Ȃ��������c
//#define CallFunctionByName(funcname, ...)\
//    {\
//        static const int id_##funcname = GetFunctionID(#funcname);\
//        CallFunctionByID(id_##funcname, __VA_ARGS__);\
//    }


// lambda ���g���ΕԂ���I
// ���������� lambda �� inline �W�J����Ȃ��悤�ŁA�ϐ��w������������͗�����
//#define CallFunctionByName(funcname, ...)\
//    ([&]()->variant{\
//        static const int id_##funcname = GetFunctionID(#funcname);\
//        return CallFunctionByID(id_##funcname, __VA_ARGS__);\
//    })()

