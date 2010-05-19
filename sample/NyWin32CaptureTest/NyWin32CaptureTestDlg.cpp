// NyWin32CaptureTestDlg.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NyWin32Capture.h"
#include "NyWin32CaptureTest.h"
#include "NyWin32CaptureTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace NyWin32Capture;


class AppCtrl
{
public:
	CWnd* _wnd;
	bool is_start;
	CaptureDeviceList* devlist;
public:
	AppCtrl(CWnd* i_app_window)
	{
		CoInitialize(NULL);					// COM�̏�����		
		this->_wnd=i_app_window;
		this->is_start=false;
		CButton* bn=(CButton*)_wnd->GetDlgItem(ID_SWITCH);
		bn->SetWindowText(_T("start capture"));
		//�J�����L���v�`���f�o�C�X�̃��X�g�����B
		devlist=new CaptureDeviceList();
		SetupCamera1();

	}
	virtual ~AppCtrl()
	{
		//�J�����L���v�`���f�o�C�X�̃��X�g���J��
		delete devlist;

		CoUninitialize();
	}
	//	�L���v�`���f�o�C�X���I�[�v�����āA�P�Ԗڂ̃J������QVGA�ŃL���v�`���ł���悤��
	//	�ݒ肷��B
	//
	bool SetupCamera1()
	{
		//�L���v�`���f�o�C�X�̃��X�g��ǂ�
		int nod=this->devlist->getNumberOfDevice();
		if(nod<1){
			return "This computer has not Capture device.";
		}
		//0�Ԗڂ̃J������������B
		CaptureDevice* d=devlist->getDevice(0);
		d->openDevice();
		//�t�H�[�}�b�g���X�g�𓾂�B
		VideoFormatList lt;
		d->getVideoFormatList(lt);
		int nof=lt.getNumberOfFormat();
		if(nof<1){
			return "The device has not Video format.";
		}
		//�C���[�W�̃t�H�[�}�b�g��ݒ�B���̃^�C�v��uuids.h�ɏ����Ă���B
		const VideoFormat* vf=lt.getFormat(320,240,MEDIASUBTYPE_RGB24);
		if(vf==NULL){
			return "The device has not 320,240,MEDIASUBTYPE_RGB24,30.0 format.";
		}
		//�C���[�W�t�H�[�}�b�g��ݒ�
		d->setVideoFormat(*vf,30.0);
		return NULL;
	}

	void OnPushSwitch()
	{/*
		CButton* bn=(CButton*)this->_wnd->GetDlgItem(ID_SWITCH);
		bn->SetWindowText(this->is_start?_T("stop capture"):_T("start capture"));
		this->is_start=(!this->is_start);
	*/	
	}
};

AppCtrl* appctrl;




// CNyWin32CaptureTestDlg �_�C�A���O




CNyWin32CaptureTestDlg::CNyWin32CaptureTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNyWin32CaptureTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CNyWin32CaptureTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNyWin32CaptureTestDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_SWITCH, &CNyWin32CaptureTestDlg::OnBnClickedSwitch)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CNyWin32CaptureTestDlg ���b�Z�[�W �n���h��

BOOL CNyWin32CaptureTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���̃_�C�A���O�̃A�C�R����ݒ肵�܂��B�A�v���P�[�V�����̃��C�� �E�B���h�E���_�C�A���O�łȂ��ꍇ�A
	//  Framework �́A���̐ݒ�������I�ɍs���܂��B
	SetIcon(m_hIcon, TRUE);			// �傫���A�C�R���̐ݒ�
	SetIcon(m_hIcon, FALSE);		// �������A�C�R���̐ݒ�

	// TODO: �������������ɒǉ����܂��B
	appctrl=new AppCtrl(this);

	return TRUE;  // �t�H�[�J�X���R���g���[���ɐݒ肵���ꍇ�������ATRUE ��Ԃ��܂��B
}

// �_�C�A���O�ɍŏ����{�^����ǉ�����ꍇ�A�A�C�R����`�悷�邽�߂�
//  ���̃R�[�h���K�v�ł��B�h�L�������g/�r���[ ���f�����g�� MFC �A�v���P�[�V�����̏ꍇ�A
//  ����́AFramework �ɂ���Ď����I�ɐݒ肳��܂��B

void CNyWin32CaptureTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �`��̃f�o�C�X �R���e�L�X�g

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// �N���C�A���g�̎l�p�`�̈���̒���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �A�C�R���̕`��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ���[�U�[���ŏ��������E�B���h�E���h���b�O���Ă���Ƃ��ɕ\������J�[�\�����擾���邽�߂ɁA
//  �V�X�e�������̊֐����Ăяo���܂��B
HCURSOR CNyWin32CaptureTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




void CNyWin32CaptureTestDlg::OnBnClickedSwitch()
{
	//�J�n�E��~�X�C�b�`��������
//	appctrl->OnPushSwitch();
}

void CNyWin32CaptureTestDlg::OnClose()
{
	// TODO: �����Ƀ��b�Z�[�W �n���h�� �R�[�h��ǉ����邩�A����̏������Ăяo���܂��B

	CDialog::OnClose();
}

void CNyWin32CaptureTestDlg::OnDestroy()
{
	delete appctrl;
	CDialog::OnDestroy();
	// TODO: �����Ƀ��b�Z�[�W �n���h�� �R�[�h��ǉ����܂��B
}
