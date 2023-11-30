#pragma once

enum XiYouSdkNonError
{
    XiYouSdkNonError_kOK = 200,
    XiYouSdkNonError_kError = 201,
    XiYouSdkNonError_kCategoryTypeNotExists = 202,
    XiYouSdkNonError_kTimeout = 203,
    XiYouSdkNonError_kGameNotExists = 1400100002, // ��Ϸ�����ڣ�һ���ǲ�������
    XiYouSdkNonError_kChannelNotExists = 1400100003, // ���������ڣ�һ���ǲ�������
    XiYouSdkNonError_kUserVerifiedFail = 1400100005, // �û���֤ʧ�ܣ�һ�������sdk������ȥ��������������֤ʧ��
    XiYouSdkNonError_kUserNotExists = 1400100006, // �û������ڣ�һ����uid��������
    XiYouSdkNonError_kTokenIsNullValue = 1400100008, // tokenΪnull
    XiYouSdkNonError_kTokenIsNonError = 1400100009, // token����
    XiYouSdkNonError_kMoneyValueError = 1400100010, // ������
    XiYouSdkNonError_kCreateOrderFail = 1400100011, // ��������ʧ��
    XiYouSdkNonError_kDataSignError = 1400100012, // ǩ������
    XiYouSdkNonError_kTokenUseTimeout = 1400100013, // token��ʱ
    XiYouSdkNonError_kInvalidParameter = 1400100014, // ��������
    XiYouSdkNonError_kCurrentPacketError = 1400100015, // ��ǰ����Ч
    XiYouSdkNonError_kFetchQrCodeHongBaoError = 1400100016, // ��ȡ��ά����ʧ��
    XiYouSdkNonError_kRoleIsNull = 1400100017, // �û�ΪNULL
    XiYouSdkNonError_kRepeatOrderError = 1400100022, //  �ظ�����
};