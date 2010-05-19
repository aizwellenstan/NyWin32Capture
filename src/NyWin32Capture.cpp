#include "NyWin32Capture.h"
#include <dshow.h>
#include <stdio.h>
#include <conio.h>
/**
�R���p�C���ɂ���
���̃R�[�h���R���p�C�����邽�߂ɂ́APSDK 7.0,PSDK6.1���K�v�ł��B
Microsoft��Web�T�C�g����_�E�����[�h���Ă��������B�_�E�����[�h������A
PSDK7.0��directshow/baseclasses�f�B���N�g������A.h��.c�t�@�C����
extlib/BaseClassesWin32�f�B���N�g���փR�s�[���Ă��������B


qedit.h�ɂ���
Windows PSDK 7.0�ɂ�qedit.h�������̂ŁAPSDK 6.1����R�s�[���Ă��������B
�����PSDK 6.1��qedit.h�ɂ̓o�O������A���݂��Ȃ�dxtrans.h���Q�Ƃ��Ă���̂ŁA
���̕ӂ̖�����������K�v������܂��B

��̓I�ȕ��@�́A�ȉ�URL���Q�l�ɂȂ�܂��B
http://social.msdn.microsoft.com/forums/en-US/windowssdk/thread/ed097d2c-3d68-4f48-8448-277eaaf68252/


*/
#include <qedit.h>
#include <math.h>
#include <exception>
#include <vector>
#include <streams.h>

using namespace std;

namespace NyWin32Capture
{
	/*	�ȈՃX�}�[�g�|�C���^
	*/
	template <class T> class AutoReleaseComPtr
	{
	private:
		AutoReleaseComPtr(const AutoReleaseComPtr& );
		AutoReleaseComPtr& operator=(const AutoReleaseComPtr&);
	public:
		T* ptr;
		AutoReleaseComPtr<T>()
		{
			this->ptr=NULL;
		}
		AutoReleaseComPtr<T>(T* i_ptr)
		{
			this->ptr=i_ptr;
		}
		virtual ~AutoReleaseComPtr<T>()
		{
			if(this->ptr!=NULL)
			{
				this->ptr->Release();
				this->ptr=NULL;
			}
		}
		T* operator->()
		{
			return this->ptr;
		}
		operator T*() const
		{
			return this->ptr;
		}
		void detach(T** o_ptr)
		{
			if(o_ptr!=NULL){
				*o_ptr=this->ptr;
			}else{
				this->ptr->Release();
			}
			this->ptr=NULL;
		}
	};


	/*ICaptureGraphBuilder2����IAMStreamConfig�C���^�t�F�C�X���擾����B
	*/
	static bool mGetIAMStreamConfig(ICaptureGraphBuilder2* i_capbuilder,IBaseFilter* i_basefilter,const GUID& i_pin_category,IAMStreamConfig** o_config)
	{
		HRESULT hr;
		hr = i_capbuilder->FindInterface(&i_pin_category,0,i_basefilter,IID_IAMStreamConfig, (void **)o_config);
		if(FAILED(hr)){
			return false;
		}
		return true;
	}
	/*	Pin�̃J�e�S�����擾����B
	*/
	static bool mGetPinCategory(IPin* pPin,GUID& Category)
	{
		IKsPropertySet *pKs;
		HRESULT hr = pPin->QueryInterface(IID_IKsPropertySet, (void **)&pKs);
		if (SUCCEEDED(hr))
		{
			DWORD cbReturned;
			hr = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &Category, sizeof(GUID), &cbReturned);
			pKs->Release();
			return true;
		}
		return false;
	}

	/**/
	static IPin* mFindPinByDirection(IBaseFilter *pFilter, PIN_DIRECTION PinDir,int i_index)
	{
		BOOL       bFound = FALSE;
		IEnumPins  *pEnum;
		IPin       *pPin;
		int index=i_index;

		HRESULT hr = pFilter->EnumPins(&pEnum);
		if (FAILED(hr))
		{
			return NULL;
		}
		while(pEnum->Next(1, &pPin, 0) == S_OK)
		{
			PIN_DIRECTION PinDirThis;
			if(pPin->QueryDirection(&PinDirThis)!=S_OK){
				continue;
			}
			if(PinDir != PinDirThis)
			{
				pPin->Release();
				continue;
			}
			if(index>0)
			{
				index--;
				pPin->Release();
				continue;
			}
			bFound=true;
		}
		pEnum->Release();
		return (bFound ? pPin : NULL);  
	}


	static IPin* mFindPinByCategory(IBaseFilter* i_filter,const GUID& Category)
	{
		HRESULT hr;
		IEnumPins *pEnum = 0;
		if (SUCCEEDED(i_filter->EnumPins(&pEnum)))
		{
			IPin *pPin = 0;
			while (hr = pEnum->Next(1, &pPin, 0), hr == S_OK)
			{
				PIN_DIRECTION ThisPinDir;
				hr = pPin->QueryDirection(&ThisPinDir);
				if (FAILED(hr))
				{
					pPin->Release();
					pEnum->Release();
					return NULL; // �\�����Ȃ����Ƃ����������B
				}
				GUID pin_category;
				if (!mGetPinCategory(pPin, pin_category))
				{
					pEnum->Release();
					return NULL;
				}
				if(Category==pin_category)
				{
					pEnum->Release();
					return pPin;
				}
				pPin->Release();
			}
			pEnum->Release();
		}
		// ��v����s�����Ȃ��B
		return NULL;
	}

	/*	�O���t�̎��S�Ẵt�B���^�s����ؒf����B
	*/
	static bool mDisconnectAll(IGraphBuilder* i_graph)
	{
		HRESULT hr = S_OK;
		if (i_graph==NULL)
		{
			return false;
		}

		IEnumFilters* enum_filter = NULL;
		hr = i_graph->EnumFilters(&enum_filter);
		if(!SUCCEEDED(hr))
		{
			return false;
		}
		IBaseFilter* filter = NULL;
		while(S_OK == enum_filter->Next(1, &filter, NULL))
		{
			IEnumPins* enum_pins = NULL;
			hr = filter->EnumPins(&enum_pins);
			if(!SUCCEEDED(hr))
			{
				continue;
			}
			IPin* pin = NULL;
			while (S_OK == enum_pins->Next(1, &pin, NULL))
			{
				IPin* pin2 = NULL;
				if (S_OK == pin->ConnectedTo(&pin2))
				{
					// pins are connected, to disconnect filters, both pins must be disconnected
					hr = i_graph->Disconnect(pin);
					hr = i_graph->Disconnect(pin2);
					pin2->Release();
				}
				pin->Release();
			}
			filter->Release();
			enum_pins->Release();
		}
		enum_filter->Release();
		return true;
	}



}

//NyWin32CaptureException
//
namespace NyWin32Capture
{
	NyWin32CaptureException::NyWin32CaptureException():exception()
	{
	}
	NyWin32CaptureException::NyWin32CaptureException(exception e):exception(e)
	{
	}
	NyWin32CaptureException::NyWin32CaptureException(const char* m)
	{
		perror(m);
	}
}

//VideoFormat
//
namespace NyWin32Capture
{
	VideoFormat::VideoFormat(AM_MEDIA_TYPE* i_pmt,const VIDEO_STREAM_CONFIG_CAPS& i_scc)
	{
		this->_scc=(VIDEO_STREAM_CONFIG_CAPS*)malloc(sizeof(VIDEO_STREAM_CONFIG_CAPS));
		this->_pmt=i_pmt;
		*(this->_scc)=i_scc;
	}
	VideoFormat::~VideoFormat()
	{
		DeleteMediaType(this->_pmt);
		free(this->_scc);
	}
	int VideoFormat::getWidth()const
	{
		return reinterpret_cast<VIDEOINFOHEADER*>(this->_pmt->pbFormat)->bmiHeader.biWidth;
	}
	int VideoFormat::getHeight()const
	{
		return reinterpret_cast<VIDEOINFOHEADER*>(this->_pmt->pbFormat)->bmiHeader.biHeight;
	}
	double VideoFormat::getRate()const
	{
		return 10000000.0/((double)reinterpret_cast<VIDEOINFOHEADER*>(this->_pmt->pbFormat)->AvgTimePerFrame);
	}
	const GUID& VideoFormat::getFormat()const
	{
		return this->_pmt->subtype;
	}
	const AM_MEDIA_TYPE* VideoFormat::getMediaType()const
	{
		return reinterpret_cast<AM_MEDIA_TYPE*>(this->_pmt);
	}
	const VIDEOINFOHEADER* VideoFormat::getVideoInfoHeader()const
	{
		return reinterpret_cast<VIDEOINFOHEADER*>(this->_pmt->pbFormat);
	}
	const BITMAPINFOHEADER* VideoFormat::getBitmapInfoHeader()const
	{
		return &(reinterpret_cast<VIDEOINFOHEADER*>(this->_pmt->pbFormat)->bmiHeader);
	}
}

//VideoFormatList
//
namespace NyWin32Capture
{
	VideoFormatList::VideoFormatList()
	{
		this->_list=new std::vector<VideoFormat*>();
	}


	void VideoFormatList::clear()
	{
		for(int i=this->_list->size()-1;i>=0;i--){
			delete (*this->_list)[i];
		}
		this->_list->clear();
	}

	void VideoFormatList::update(IAMStreamConfig* i_config)
	{
		this->clear();
		//�t�H�[�}�b�g�̐����擾
		int iCount;
		int iSize;
		VIDEO_STREAM_CONFIG_CAPS scc;
		HRESULT hr;
		hr =i_config->GetNumberOfCapabilities( &iCount, &iSize );
		if(FAILED(hr) ||sizeof(scc) != iSize){
			throw NyWin32CaptureException();
		}
		for(int i=0;i<iCount;i++){
			AM_MEDIA_TYPE *pmt;
			hr = i_config->GetStreamCaps(i, &pmt,reinterpret_cast<BYTE*>(&scc));
			if(hr != S_OK)
			{
				DeleteMediaType(pmt);
				continue;
			}
			if (pmt->formattype != FORMAT_VideoInfo)
			{
				DeleteMediaType(pmt);
				continue;
			}
			VideoFormat* fmt=new VideoFormat(pmt,scc);
			//�ǉ�
			this->_list->push_back(fmt);
		}
	}
	VideoFormatList::~VideoFormatList()
	{
		this->clear();
		delete this->_list;
	}
	/**
	�t�H�[�}�b�g�L�[�ɏ]���āA�t�H�[�}�b�g����������B
	*/
	const VideoFormat* VideoFormatList::getFormat(int i_width,int i_height,const GUID& i_format)const
	{
		//��v���邻����ۂ��̂�T���B
		int l=(int)this->_list->size();
		for(int i=0;i<l;i++){
			const VideoFormat* f=((*this->_list)[i]);
			const VIDEOINFOHEADER* v=f->getVideoInfoHeader();
			if(i_width!=v->bmiHeader.biWidth)
			{
				continue;
			}
			if(i_height!=v->bmiHeader.biHeight)
			{
				continue;
			}
			if(i_format==f->getFormat())
			{
				continue;
			}
			return f;
		}
		return NULL;
	}
	const VideoFormat* VideoFormatList::getFormat(int i_index)const
	{
		return (*this->_list)[i_index];
	}
	int VideoFormatList::getNumberOfFormat()const
	{
		return (int)this->_list->size();
	}
}


namespace NyWin32Capture
{
	CaptureDevice::CaptureDevice(IMoniker* i_moniker)
	{
		//HRESULT�`�F�b�N���ĂȂ��Ƃ��낪����B
		HRESULT hr;
		hr=i_moniker-> BindToObject( 0, 0, IID_IBaseFilter, (void**)&this->ds_res.sorce.filter);
		this->_moniker =i_moniker;
		{	//name�̎擾
			IPropertyBag* bag;
			hr=this->_moniker->BindToStorage(NULL,NULL,IID_IPropertyBag,(void**)&bag);
			if(!SUCCEEDED(hr)){
				throw NyWin32CaptureException();
			}
			VARIANT varName;
			varName.vt = VT_BSTR;
			hr=bag->Read(L"FriendlyName",&varName,NULL);
			this->_allocated_res.name=new WCHAR[wcslen(varName.bstrVal)+1];
			wcscpy(this->_allocated_res.name,varName.bstrVal);
			VariantClear(&varName);
			bag->Release();
		}
		this->_status  =ST_CLOSED;
	}
	CaptureDevice::~CaptureDevice()
	{
		switch(this->_status)
		{
		case ST_IDLE:
			this->closeDevice();
			break;
		case ST_RUN:
			this->stopCapture();
			this->closeDevice();
			break;
		default:
			break;
		}
		delete [] this->_allocated_res.name;
		this->_moniker->Release();
		this->ds_res.sorce.filter->Release();
	}

	void CaptureDevice::startCapture()
	{
		//��ԃ`�F�b�N
		if(this->_status!=ST_IDLE){
			throw NyWin32CaptureException();
		}
		//HRESULT�`�F�b�N���ĂȂ��Ƃ��낪����B
		HRESULT hr;


		//�L���v�`���O���t��ڑ�
		hr=this->ds_res.cap_builder->RenderStream(&this->ds_res.sorce.pin_category, &MEDIATYPE_Video,this->ds_res.sorce.filter, NULL,this->ds_res.render.filter);

		AM_MEDIA_TYPE amt;
		//����̃L���v�`�������擾
		hr=this->ds_res.render.grab->GetConnectedMediaType(&amt);
		VIDEOINFOHEADER *pVideoHeader = (VIDEOINFOHEADER*)amt.pbFormat;
		CopyMemory(&this->_capture_format,&(pVideoHeader->bmiHeader),sizeof(BITMAPINFOHEADER));

		//�����_���̊J�n
		hr=this->ds_res.render.grab->SetBufferSamples(TRUE);	// �O���u�J�n
		hr=this->ds_res.graph_builder.mc->Run();               // �����_�����O�J�n

		this->_status=ST_RUN;
		return;
	}

	const BITMAPINFOHEADER& CaptureDevice::getImageFormat()const
	{
		if(this->_status!=ST_RUN){
			throw NyWin32CaptureException();
		}
		return this->_capture_format;
	}
	void CaptureDevice::stopCapture()
	{
		//��ԃ`�F�b�N
		if(this->_status!=ST_RUN){
			throw NyWin32CaptureException();
		}
		//HRESULT�`�F�b�N���ĂȂ��Ƃ��낪����B
		HRESULT hr;

		//��~����
		hr=this->ds_res.graph_builder.mc->Stop();
		hr=this->ds_res.render.grab->SetBufferSamples(FALSE);

		//�t�B���^���̑Spin��ؒf
		mDisconnectAll(this->ds_res.graph_builder.graph);

		this->_status=ST_IDLE;
	}


	/*	�L���v�`���f�o�C�X���I�[�v�����܂��B
	*/
	void CaptureDevice::openDevice()
	{
		//��ԃ`�F�b�N
		if(this->_status!=ST_CLOSED)
		{
			throw NyWin32CaptureException();
		}
		
		HRESULT hr;

		// �O���o�t�B���^�����
		AutoReleaseComPtr<IBaseFilter> pF;
		hr=CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,IID_IBaseFilter, (LPVOID *)&(pF.ptr));
		if(!SUCCEEDED(hr)){
			throw NyWin32CaptureException();
		}

		AutoReleaseComPtr<ISampleGrabber> pGrab;
		hr=pF -> QueryInterface( IID_ISampleGrabber, (void **)&(pGrab.ptr));
		if(!SUCCEEDED(hr)){
			throw NyWin32CaptureException();
		}

		//�L���v�`���O���t�����  
		AutoReleaseComPtr<IGraphBuilder> pGraph;
		hr=CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC,IID_IGraphBuilder, (void **) &(pGraph.ptr));
		if(!SUCCEEDED(hr)){
			throw NyWin32CaptureException();
		}

		AutoReleaseComPtr<IMediaControl> pMC;
		hr=pGraph -> QueryInterface( IID_IMediaControl, (LPVOID *) &(pMC.ptr));
		if(!SUCCEEDED(hr)){
			throw NyWin32CaptureException();
		}

		//CaptureGraphBuilder���쐬
		AutoReleaseComPtr<ICaptureGraphBuilder2> pCapture;
		hr=CoCreateInstance( CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,IID_ICaptureGraphBuilder2, (void **) &(pCapture.ptr));
		if(!SUCCEEDED(hr)){
			throw NyWin32CaptureException();
		}

		// �L���v�`���t�B���^���t�B���^�O���t�ɒǉ�
		hr=pGraph -> AddFilter(this->ds_res.sorce.filter, L"CaptureFilter");
		if(!SUCCEEDED(hr)){
			throw NyWin32CaptureException();
		}

		//�T���v���O���o�̒ǉ�
		hr=pGraph -> AddFilter(pF, L"SampleGrabber");
		if(!SUCCEEDED(hr)){
			throw NyWin32CaptureException();
		}

		//�t�B���^�O���t���L���v�`���O���t�ɑg�ݍ���
		hr=pCapture -> SetFiltergraph(pGraph);
		if(!SUCCEEDED(hr)){
			throw NyWin32CaptureException();
		}

		//Config�𓾂�(STILL��PREVIEW�̏�)
		AutoReleaseComPtr<IAMStreamConfig> config;
		if(mGetIAMStreamConfig(pCapture,this->ds_res.sorce.filter,PIN_CATEGORY_STILL,&config.ptr)){
			this->ds_res.sorce.pin_category=PIN_CATEGORY_STILL;
		}else{
			if(mGetIAMStreamConfig(pCapture,this->ds_res.sorce.filter,PIN_CATEGORY_PREVIEW,&config.ptr)){
				this->ds_res.sorce.pin_category=PIN_CATEGORY_PREVIEW;
			}else{
				throw NyWin32CaptureException();
			}
		}

		//�����J���|�C���^����؂藣��
		pCapture.detach(&this->ds_res.cap_builder);
		pF.detach(&this->ds_res.render.filter);
		pGrab.detach(&this->ds_res.render.grab);
		config.detach(&this->ds_res.sorce.config);
		pGraph.detach(&this->ds_res.graph_builder.graph);
		pMC.detach(&this->ds_res.graph_builder.mc);
		this->_status=ST_IDLE;
		return;
	}
	void CaptureDevice::closeDevice()
	{
		//��ԃ`�F�b�N
		if(this->_status!=ST_IDLE){
			throw NyWin32CaptureException();
		}
		//�C���^�t�F�C�X�̃����[�X
		this->ds_res.sorce.config->Release();
		this->ds_res.graph_builder.graph->Release();
		this->ds_res.graph_builder.mc->Release();
		this->ds_res.render.grab->Release();
		this->ds_res.render.filter->Release();
		this->ds_res.cap_builder->Release();
		this->_status=ST_CLOSED;
	}


	void CaptureDevice::getVideoFormatList(VideoFormatList& o_list)const
	{
		if(this->_status!=ST_IDLE){
			throw NyWin32CaptureException();
		}
		o_list.update(this->ds_res.sorce.config);
	}
	//i_buf�ɂ́Athis->_capture_format.biSizeImage���傫�ȃT�C�Y�̃o�b�t�@��^���邱�ƁB
	bool CaptureDevice::captureImage(void* i_buf,long i_buf_size)
	{
		if(this->_status!=ST_RUN){
			throw NyWin32CaptureException();
		}

		HRESULT hr;
		long n=i_buf_size==0?this->_capture_format.biSizeImage:i_buf_size;

		hr = this->ds_res.render.grab -> GetCurrentBuffer(&n,(long *)i_buf);
		return SUCCEEDED(hr);
	}
	bool CaptureDevice::setVideoFormat(int i_width,int i_height,GUID i_format,double i_rate)
	{
		if(this->_status!=ST_IDLE){
			throw NyWin32CaptureException();
		}
		VideoFormatList list;
		this->getVideoFormatList(list);
		const VideoFormat* vf=list.getFormat(i_width,i_height,i_format);
		return this->setVideoFormat(*vf,i_rate);
	}
	bool CaptureDevice::setVideoFormat(const VideoFormat& i_format,double i_rate)
	{
		if(this->_status!=ST_IDLE){
			throw NyWin32CaptureException();
		}
		//�T���v���O���o�̎󂯓���t�H�[�}�b�g�ݒ�
		HRESULT hr;
		hr=this->ds_res.render.grab->SetMediaType(i_format.getMediaType());
		if(!SUCCEEDED(hr)){
			return false;
		}

		//StreamConfig�̐ݒ�
		AM_MEDIA_TYPE* nmt=CreateMediaType(i_format.getMediaType());
		reinterpret_cast<VIDEOINFOHEADER*>(nmt->pbFormat)->AvgTimePerFrame=(REFERENCE_TIME)(10000000.0/i_rate);
		hr=this->ds_res.sorce.config->SetFormat(nmt);
		if(!SUCCEEDED(hr)){
			return false;
		}		
		DeleteMediaType(nmt);
		return true;
	}
	const WCHAR* CaptureDevice::getName()const
	{
		if(this->_status==ST_CLOSED){
			throw NyWin32CaptureException();
		}
		return this->_allocated_res.name;
	}
}

namespace NyWin32Capture
{


	void CaptureDeviceList::createDeviceList()
	{
		//���X�g�̍\�z
		HRESULT hr;
		// ---- �L���v�`���t�B���^�̏��� ----
		// �L���v�`���f�o�C�X��T��
		ULONG cFetched;
		// �f�o�C�X�񋓎q���쐬
		ICreateDevEnum* pDevEnum = NULL;
		CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,IID_ICreateDevEnum, (void ** ) &pDevEnum);

		// �r�f�I�L���v�`���f�o�C�X�񋓎q���쐬
		if(pDevEnum == NULL){
			// do nothing
		}else{
			IEnumMoniker * pClassEnum = NULL;
			hr=pDevEnum -> CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
			if(hr!=S_OK){
				// do nothing
			}else{
				IMoniker * pMoniker = NULL;
				for(;;)
				{
					hr=pClassEnum -> Next(1, &pMoniker, &cFetched);
					if(hr!=S_OK){
						break;
					}
					CaptureDevice* cd=new CaptureDevice(pMoniker);
					this->_list->push_back(cd);
				}
			}
		}
		pDevEnum->Release();

	}
	void CaptureDeviceList::releaseDeviceList()
	{
		for(unsigned int i=0;i<this->_list->size();i++){
			delete (*this->_list)[i];
		}
		(*this->_list).clear();
	}


	CaptureDeviceList::CaptureDeviceList()
	{
		this->_list=new vector<CaptureDevice*>;
		this->createDeviceList();
	}
	CaptureDeviceList::~CaptureDeviceList()
	{
		this->releaseDeviceList();
		delete this->_list;
	}
	//	i_index�Ԗڂ̃L���v�`���f�o�C�X���擾���܂��B
	CaptureDevice* CaptureDeviceList::getDevice(int i_index)const
	{
		return (*this->_list)[i_index];
	}
	//	�f�o�C�X�̐����擾���܂��B
	int CaptureDeviceList::getNumberOfDevice()const
	{
		return this->_list->size();
	}
};

