// NyWin32CaptureTestDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once


// CNyWin32CaptureTestDlg �_�C�A���O
class CNyWin32CaptureTestDlg : public CDialog
{
// �R���X�g���N�V����
public:
	CNyWin32CaptureTestDlg(CWnd* pParent = NULL);	// �W���R���X�g���N�^

// �_�C�A���O �f�[�^
	enum { IDD = IDD_NYWIN32CAPTURETEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �T�|�[�g


// ����
protected:
	HICON m_hIcon;

	// �������ꂽ�A���b�Z�[�W���蓖�Ċ֐�
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSwitch();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
};